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

// ---------------------------------------------------------------------------------------------------------------
// Selling (Docs/FlipperSpec.md, Feature 5): the buyer's offer and the daily mood.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingOfferKnownStateTest, "Flipper.Pricing.OfferKnownState", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingOfferKnownStateTest::RunTest(const FString& Parameters)
{
	FPricingFixture F;

	// Tested and working: the Value.
	TestEqual(TEXT("Tested working Common 100 is offered 100"), FlipperPricing::GetOffer(F.MakeItem(100, EFlipperRarity::Common, true), F.Config, 1.f), 100);
	TestEqual(TEXT("Tested working Uncommon 100 (Value 200) is offered 200"), FlipperPricing::GetOffer(F.MakeItem(100, EFlipperRarity::Uncommon, true), F.Config, 1.f), 200);

	// Tested and dead: scrap.
	FFlipperItemInstance Dead = F.MakeItem(100, EFlipperRarity::Common, true);
	Dead.bWorks = false;
	TestEqual(TEXT("Tested dead Common 100 is offered scrap, 10"), FlipperPricing::GetOffer(Dead, F.Config, 1.f), 10);
	Dead.Rarity = EFlipperRarity::Rare;
	TestEqual(TEXT("Tested dead Rare 100 (Value 400) is offered scrap, 40"), FlipperPricing::GetOffer(Dead, F.Config, 1.f), 40);

	// Untested: half the Value, whether or not it works (the buyer does not open it).
	FFlipperItemInstance Untested = F.MakeItem(100, EFlipperRarity::Common, false);
	TestEqual(TEXT("Untested working Common 100 is offered 50"), FlipperPricing::GetOffer(Untested, F.Config, 1.f), 50);
	Untested.bWorks = false;
	TestEqual(TEXT("Untested dead Common 100 is offered the same 50"), FlipperPricing::GetOffer(Untested, F.Config, 1.f), 50);
	TestEqual(TEXT("Untested Rare 100 (Value 400) is offered 200"), FlipperPricing::GetOffer(F.MakeItem(100, EFlipperRarity::Rare, false), F.Config, 1.f), 200);

	// The untested fraction comes from the config.
	F.Config.UntestedOfferFraction = 0.4f;
	TestEqual(TEXT("Untested at 40 percent is offered 40"), FlipperPricing::GetOffer(F.MakeItem(100, EFlipperRarity::Common, false), F.Config, 1.f), 40);

	// What the player paid or what the seller asked does not matter.
	FFlipperItemInstance Bought = F.MakeItem(100, EFlipperRarity::Common, true);
	Bought.PaidPrice = 500;
	Bought.AskPrice = 500;
	TestEqual(TEXT("the offer ignores PaidPrice and AskPrice"), FlipperPricing::GetOffer(Bought, F.Config, 1.f), 100);

	TestEqual(TEXT("an item with no Definition is offered 0"), FlipperPricing::GetOffer(FFlipperItemInstance(), F.Config, 1.f), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingOfferMoodTest, "Flipper.Pricing.OfferMood", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingOfferMoodTest::RunTest(const FString& Parameters)
{
	FPricingFixture F;

	const FFlipperItemInstance Working = F.MakeItem(100, EFlipperRarity::Common, true);
	TestEqual(TEXT("a Slow day (x0.8) offers 80"), FlipperPricing::GetOffer(Working, F.Config, 0.8f), 80);
	TestEqual(TEXT("a Hot day (x1.25) offers 125"), FlipperPricing::GetOffer(Working, F.Config, 1.25f), 125);

	// The mood scales every kind of offer, scrap and Untested included.
	FFlipperItemInstance Dead = F.MakeItem(100, EFlipperRarity::Common, true);
	Dead.bWorks = false;
	TestEqual(TEXT("scrap 10 on a Slow day is 8"), FlipperPricing::GetOffer(Dead, F.Config, 0.8f), 8);
	TestEqual(TEXT("Untested 50 on a Slow day is 40"), FlipperPricing::GetOffer(F.MakeItem(100, EFlipperRarity::Common, false), F.Config, 0.8f), 40);
	TestEqual(TEXT("Untested Common 80 on a Hot day: 80 x 0.5 x 1.25 is 50"), FlipperPricing::GetOffer(F.MakeItem(80, EFlipperRarity::Common, false), F.Config, 1.25f), 50);

	// Rounded to the nearest dollar.
	TestEqual(TEXT("35 x 1.25 is 43.75, which rounds to 44"), FlipperPricing::GetOffer(F.MakeItem(35, EFlipperRarity::Common, true), F.Config, 1.25f), 44);

	// Never below $1 for a real item, even when scrap on a Slow day rounds to 0.
	FFlipperItemInstance Cheap = F.MakeItem(1, EFlipperRarity::Common, true);
	Cheap.bWorks = false;
	TestEqual(TEXT("a $1 dead item on a Slow day is still offered $1"), FlipperPricing::GetOffer(Cheap, F.Config, 0.8f), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperPricingMoodTest, "Flipper.Pricing.Mood", FLIPPER_PRICING_TEST_FLAGS)
bool FFlipperPricingMoodTest::RunTest(const FString& Parameters)
{
	FPricingFixture F;

	TestEqual(TEXT("Slow is x0.8"), FlipperPricing::GetMoodMultiplier(EFlipperBuyerMood::Slow, F.Config), 0.8f);
	TestEqual(TEXT("Normal is x1"), FlipperPricing::GetMoodMultiplier(EFlipperBuyerMood::Normal, F.Config), 1.f);
	TestEqual(TEXT("Hot is x1.25"), FlipperPricing::GetMoodMultiplier(EFlipperBuyerMood::Hot, F.Config), 1.25f);

	// 25 / 50 / 25 by default.
	FRandomStream Random(42);
	int32		  Counts[3] = { 0, 0, 0 };
	const int32	  Rolls = 4000;
	for (int32 i = 0; i < Rolls; ++i)
	{
		++Counts[static_cast<int32>(FlipperPricing::RollMood(F.Config, Random))];
	}
	TestTrue(FString::Printf(TEXT("about 25 percent Slow (got %d of %d)"), Counts[0], Rolls), Counts[0] > Rolls * 0.22 && Counts[0] < Rolls * 0.28);
	TestTrue(FString::Printf(TEXT("about 50 percent Normal (got %d of %d)"), Counts[1], Rolls), Counts[1] > Rolls * 0.47 && Counts[1] < Rolls * 0.53);
	TestTrue(FString::Printf(TEXT("about 25 percent Hot (got %d of %d)"), Counts[2], Rolls), Counts[2] > Rolls * 0.22 && Counts[2] < Rolls * 0.28);

	// The weights come from the config.
	F.Config.SlowMoodWeight = 0.f;
	F.Config.NormalMoodWeight = 0.f;
	F.Config.HotMoodWeight = 1.f;
	bool bAlwaysHot = true;
	for (int32 i = 0; i < 100; ++i)
	{
		bAlwaysHot &= FlipperPricing::RollMood(F.Config, Random) == EFlipperBuyerMood::Hot;
	}
	TestTrue(TEXT("all the weight on Hot always rolls Hot"), bAlwaysHot);

	// Same seed, same mood; exactly one number is drawn per roll.
	FFlipperPricingConfig Default;
	FRandomStream		  A(7);
	FRandomStream		  B(7);
	TestTrue(TEXT("a fixed seed gives the same mood"), FlipperPricing::RollMood(Default, A) == FlipperPricing::RollMood(Default, B));
	FRandomStream Used(7);
	FRandomStream Reference(7);
	FlipperPricing::RollMood(Default, Used);
	Reference.GetFraction();
	TestEqual(TEXT("exactly one number is drawn per roll"), Used.GetCurrentSeed(), Reference.GetCurrentSeed());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
