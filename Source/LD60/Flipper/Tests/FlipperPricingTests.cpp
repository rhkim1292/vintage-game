// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for the pricing formulas and the wallet (Docs/FlipperSpec.md, Feature 3).
// Run them with:  Automation RunTests Flipper.Pricing
//
// Most tests use a base value of 100 so the answers are round numbers (and never land exactly on .5, where
// rounding direction would matter).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperItemGenerator.h"
	#include "FlipperPricing.h"
	#include "FlipperWallet.h"

namespace FlipperPricingTests
{
	/** Keeps the definitions alive and makes items of a given base value, rarity and tested state. */
	struct FPricingFixture
	{
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;
		FFlipperPricingConfig							 Config;
		int32											 NextItemId = 1;

		UFlipperItemDefinition* MakeDefinition(int32 BaseValue)
		{
			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = 1;
			Definition->Height = 1;
			Definition->BaseValue = BaseValue;
			Definitions.Emplace(Definition);
			return Definition;
		}

		FFlipperItemInstance MakeItem(int32 BaseValue, EFlipperRarity Rarity = EFlipperRarity::Common, bool bTested = true)
		{
			FFlipperItemInstance Item;
			Item.Id = NextItemId++;
			Item.Definition = MakeDefinition(BaseValue);
			Item.Rarity = Rarity;
			Item.bTested = bTested;
			return Item;
		}
	};
} // namespace FlipperPricingTests

	#define FLIPPER_PRICING_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperPricingTests;

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingValueTest, "Flipper.Pricing.Value", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingValueTest::RunTest(const FString& Parameters)
{
	FPricingFixture F;

	// Defaults: Common x1, Uncommon x2, Rare x4.
	TestEqual(TEXT("Common is x1"), FlipperPricing::GetValue(F.MakeItem(100, EFlipperRarity::Common), F.Config), 100);
	TestEqual(TEXT("Uncommon is x2"), FlipperPricing::GetValue(F.MakeItem(100, EFlipperRarity::Uncommon), F.Config), 200);
	TestEqual(TEXT("Rare is x4"), FlipperPricing::GetValue(F.MakeItem(100, EFlipperRarity::Rare), F.Config), 400);

	// Value does not depend on the tested state or on whether it works.
	FFlipperItemInstance Untested = F.MakeItem(100, EFlipperRarity::Common, false);
	Untested.bWorks = false;
	TestEqual(TEXT("an untested dead item still has its Value"), FlipperPricing::GetValue(Untested, F.Config), 100);

	// The multipliers come from the config, and the result is rounded to the nearest dollar.
	F.Config.RareMultiplier = 2.5f;
	TestEqual(TEXT("Rare x2.5 on 10 is 25"), FlipperPricing::GetValue(F.MakeItem(10, EFlipperRarity::Rare), F.Config), 25);
	F.Config.UncommonMultiplier = 1.5f;
	TestEqual(TEXT("Uncommon x1.5 on 7 is 10.5, which rounds to 11"), FlipperPricing::GetValue(F.MakeItem(7, EFlipperRarity::Uncommon), F.Config), 11);
	F.Config.CommonMultiplier = 1.2f;
	TestEqual(TEXT("Common x1.2 on 6 is 7.2, which rounds to 7"), FlipperPricing::GetValue(F.MakeItem(6, EFlipperRarity::Common), F.Config), 7);

	// No definition, no value.
	TestEqual(TEXT("an item with no Definition is worth 0"), FlipperPricing::GetValue(FFlipperItemInstance(), F.Config), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingScrapTest, "Flipper.Pricing.Scrap", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingScrapTest::RunTest(const FString& Parameters)
{
	FPricingFixture F;

	// Default scrap is 10 percent of the (rounded) Value.
	TestEqual(TEXT("Common 100 scraps for 10"), FlipperPricing::GetScrapValue(F.MakeItem(100, EFlipperRarity::Common), F.Config), 10);
	TestEqual(TEXT("Rare 100 (Value 400) scraps for 40"), FlipperPricing::GetScrapValue(F.MakeItem(100, EFlipperRarity::Rare), F.Config), 40);
	TestEqual(TEXT("Common 6 (Value 6) scraps for 0.6, which rounds to 1"), FlipperPricing::GetScrapValue(F.MakeItem(6, EFlipperRarity::Common), F.Config), 1);

	F.Config.ScrapFraction = 0.f;
	TestEqual(TEXT("a scrap fraction of 0 makes a dead item worthless"), FlipperPricing::GetScrapValue(F.MakeItem(100), F.Config), 0);

	TestEqual(TEXT("an item with no Definition scraps for 0"), FlipperPricing::GetScrapValue(FFlipperItemInstance(), F.Config), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingAskFractionsTest, "Flipper.Pricing.AskFractions", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingAskFractionsTest::RunTest(const FString& Parameters)
{
	FPricingFixture F;
	F.Config.AskVariation = 0.f; // no variation: the ask is exactly the fraction of Value
	FRandomStream Random(1);

	TestEqual(TEXT("Tested Common 100 asks 75"), FlipperPricing::GetAskPrice(F.MakeItem(100, EFlipperRarity::Common, true), F.Config, Random), 75);
	TestEqual(TEXT("Untested Common 100 asks 50"), FlipperPricing::GetAskPrice(F.MakeItem(100, EFlipperRarity::Common, false), F.Config, Random), 50);
	TestEqual(TEXT("Tested Uncommon 100 (Value 200) asks 150"), FlipperPricing::GetAskPrice(F.MakeItem(100, EFlipperRarity::Uncommon, true), F.Config, Random), 150);
	TestEqual(TEXT("Untested Rare 100 (Value 400) asks 200"), FlipperPricing::GetAskPrice(F.MakeItem(100, EFlipperRarity::Rare, false), F.Config, Random), 200);
	TestEqual(TEXT("Tested Common 35 asks 26.25, which rounds to 26"), FlipperPricing::GetAskPrice(F.MakeItem(35, EFlipperRarity::Common, true), F.Config, Random), 26);

	// The seller does not know whether an Untested item works: a dead one asks the same.
	FFlipperItemInstance Dead = F.MakeItem(100, EFlipperRarity::Common, false);
	Dead.bWorks = false;
	TestEqual(TEXT("an Untested dead item asks the same as a working one"), FlipperPricing::GetAskPrice(Dead, F.Config, Random), 50);

	// The fractions come from the config.
	F.Config.TestedAskFraction = 0.8f;
	F.Config.UntestedAskFraction = 0.4f;
	TestEqual(TEXT("Tested at 80 percent asks 80"), FlipperPricing::GetAskPrice(F.MakeItem(100, EFlipperRarity::Common, true), F.Config, Random), 80);
	TestEqual(TEXT("Untested at 40 percent asks 40"), FlipperPricing::GetAskPrice(F.MakeItem(100, EFlipperRarity::Common, false), F.Config, Random), 40);

	TestEqual(TEXT("an item with no Definition asks 0"), FlipperPricing::GetAskPrice(FFlipperItemInstance(), F.Config, Random), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingAskVariationTest, "Flipper.Pricing.AskVariation", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingAskVariationTest::RunTest(const FString& Parameters)
{
	FPricingFixture F; // default variation: plus or minus 20 percent
	const FFlipperItemInstance Item = F.MakeItem(100, EFlipperRarity::Common, true); // base ask 75, so 60 to 90

	FRandomStream Random(99);
	int32		  Lowest = MAX_int32;
	int32		  Highest = 0;
	int64		  Sum = 0;
	const int32	  Rolls = 2000;
	for (int32 i = 0; i < Rolls; ++i)
	{
		const int32 Ask = FlipperPricing::GetAskPrice(Item, F.Config, Random);
		Lowest = FMath::Min(Lowest, Ask);
		Highest = FMath::Max(Highest, Ask);
		Sum += Ask;
	}

	TestTrue(FString::Printf(TEXT("never below 60 (lowest %d)"), Lowest), Lowest >= 60);
	TestTrue(FString::Printf(TEXT("never above 90 (highest %d)"), Highest), Highest <= 90);
	TestTrue(FString::Printf(TEXT("the low end is reached (lowest %d)"), Lowest), Lowest <= 62);
	TestTrue(FString::Printf(TEXT("the high end is reached (highest %d)"), Highest), Highest >= 88);
	const double Average = static_cast<double>(Sum) / Rolls;
	TestTrue(FString::Printf(TEXT("centred on 75 (average %.2f)"), Average), Average > 73.5 && Average < 76.5);

	// Same seed, same asks.
	FRandomStream A(7);
	FRandomStream B(7);
	TestEqual(TEXT("a fixed seed gives the same ask"), FlipperPricing::GetAskPrice(Item, F.Config, A), FlipperPricing::GetAskPrice(Item, F.Config, B));

	// Exactly one number is taken from the stream per ask.
	FRandomStream Used(7);
	FRandomStream Reference(7);
	FlipperPricing::GetAskPrice(Item, F.Config, Used);
	Reference.GetFraction();
	TestEqual(TEXT("exactly one number is drawn per ask"), Used.GetCurrentSeed(), Reference.GetCurrentSeed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingAskMinimumTest, "Flipper.Pricing.AskMinimum", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingAskMinimumTest::RunTest(const FString& Parameters)
{
	FPricingFixture F;
	FRandomStream	Random(5);

	// A $1 item asking 50 percent with variation is 0.4 to 0.6 dollars: it still asks at least $1.
	const FFlipperItemInstance Cheap = F.MakeItem(1, EFlipperRarity::Common, false);
	for (int32 i = 0; i < 200; ++i)
	{
		const int32 Ask = FlipperPricing::GetAskPrice(Cheap, F.Config, Random);
		if (Ask < 1)
		{
			AddError(FString::Printf(TEXT("a $1 item asked %d"), Ask));
			break;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingGeneratorSetsAskTest, "Flipper.Pricing.GeneratorSetsAsk", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingGeneratorSetsAskTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UFlipperItemDefinition> Definition(NewObject<UFlipperItemDefinition>());
	Definition->Width = 1;
	Definition->Height = 1;
	Definition->BaseValue = 100;

	TStrongObjectPtr<UFlipperItemGenerator> Generator(NewObject<UFlipperItemGenerator>());
	Generator->Init({ Definition.Get() }, FFlipperGenerationConfig());

	FRandomStream Random(11);
	for (int32 i = 0; i < 50; ++i)
	{
		const FFlipperItemInstance Item = Generator->RollItem(Random);
		TestTrue(FString::Printf(TEXT("rolled item %d has an ask price (got %d)"), Item.Id, Item.AskPrice), Item.AskPrice >= 1);
		TestEqual(TEXT("a rolled item has not been bought"), Item.PaidPrice, 0);
		TestFalse(TEXT("a rolled item is not marked as bought"), Item.bBoughtThisPhase);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperWalletTest, "Flipper.Pricing.Wallet", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperWalletTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UFlipperWallet> Wallet(NewObject<UFlipperWallet>());
	Wallet->Init(200);
	TestEqual(TEXT("starts with the starting cash"), Wallet->GetCash(), 200);
	TestTrue(TEXT("can afford less than the cash"), Wallet->CanAfford(150));
	TestTrue(TEXT("can afford exactly the cash"), Wallet->CanAfford(200));
	TestFalse(TEXT("cannot afford more than the cash"), Wallet->CanAfford(201));

	int32 LastBroadcast = -1;
	Wallet->OnChanged.AddLambda([&LastBroadcast](int32 NewCash) { LastBroadcast = NewCash; });

	TestTrue(TEXT("spending 50 works"), Wallet->TrySpend(50));
	TestEqual(TEXT("cash is 150"), Wallet->GetCash(), 150);
	TestEqual(TEXT("the change is broadcast"), LastBroadcast, 150);

	TestFalse(TEXT("spending more than the cash is refused"), Wallet->TrySpend(151));
	TestEqual(TEXT("a refused spend changes nothing"), Wallet->GetCash(), 150);

	TestTrue(TEXT("spending exactly all the cash works"), Wallet->TrySpend(150));
	TestEqual(TEXT("cash is 0, never negative"), Wallet->GetCash(), 0);

	Wallet->Add(30);
	TestEqual(TEXT("adding 30 gives 30"), Wallet->GetCash(), 30);
	TestEqual(TEXT("the add is broadcast"), LastBroadcast, 30);

	Wallet->Add(-10);
	TestEqual(TEXT("adding a negative amount is ignored"), Wallet->GetCash(), 30);
	TestFalse(TEXT("spending a negative amount is refused"), Wallet->TrySpend(-5));
	TestEqual(TEXT("and changes nothing"), Wallet->GetCash(), 30);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
