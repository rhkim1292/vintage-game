// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for UFlipperItemGenerator (Docs/FlipperSpec.md, Feature 2).
// Run them with:  Automation RunTests Flipper.ItemGenerator
//
// The odds tests roll thousands of items with a fixed seed, so they give the same answer on every run. Their
// ranges are wide enough (at least 3 standard deviations) that a correct implementation passes for any seed.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperItemGenerator.h"
	#include "InventoryGrid.h"

namespace FlipperGeneratorTests
{
	/** A generator plus a factory for definitions. Change Config, add definitions, then call Build(). */
	struct FGeneratorFixture
	{
		TStrongObjectPtr<UFlipperItemGenerator>			 Generator;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;
		FFlipperGenerationConfig						 Config;

		TArray<TStrongObjectPtr<UFlipperTraitDefinition>> Traits;

		UFlipperTraitDefinition* AddTrait(const TCHAR* Name, float ValueMultiplier = 1.f, float RollWeight = 1.f)
		{
			UFlipperTraitDefinition* Trait = NewObject<UFlipperTraitDefinition>();
			Trait->DisplayName = FText::FromString(Name);
			Trait->ValueMultiplier = ValueMultiplier;
			Trait->RollWeight = RollWeight;
			Traits.Emplace(Trait);
			return Trait;
		}

		UFlipperItemDefinition* AddDefinition(const TCHAR* Name, int32 Width, int32 Height, float SpawnWeight = 1.f)
		{
			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->DisplayName = FText::FromString(Name);
			Definition->Width = Width;
			Definition->Height = Height;
			Definition->SpawnWeight = SpawnWeight;
			Definitions.Emplace(Definition);
			return Definition;
		}

		/** Creates the generator from the current Definitions and Config. Call again after changing either. */
		UFlipperItemGenerator* Build()
		{
			TArray<UFlipperItemDefinition*> Raw;
			for (const TStrongObjectPtr<UFlipperItemDefinition>& Definition : Definitions)
			{
				Raw.Add(Definition.Get());
			}
			TArray<UFlipperTraitDefinition*> RawTraits;
			for (const TStrongObjectPtr<UFlipperTraitDefinition>& Trait : Traits)
			{
				RawTraits.Add(Trait.Get());
			}
			Generator.Reset(NewObject<UFlipperItemGenerator>());
			Generator->Init(Raw, Config, FFlipperPricingConfig(), RawTraits);
			return Generator.Get();
		}
	};

	/** Gives every condition the same works chance. */
	void SetWorksChances(FFlipperGenerationConfig& Config, float WorksChance)
	{
		Config.PoorWorksChance = WorksChance;
		Config.FairWorksChance = WorksChance;
		Config.GoodWorksChance = WorksChance;
		Config.MintWorksChance = WorksChance;
	}

	/** Makes Condition the only one that can be rolled. */
	void OnlyCondition(FFlipperGenerationConfig& Config, EFlipperCondition Condition)
	{
		Config.PoorWeight = Condition == EFlipperCondition::Poor ? 1.f : 0.f;
		Config.FairWeight = Condition == EFlipperCondition::Fair ? 1.f : 0.f;
		Config.GoodWeight = Condition == EFlipperCondition::Good ? 1.f : 0.f;
		Config.MintWeight = Condition == EFlipperCondition::Mint ? 1.f : 0.f;
	}

	/** A grid of the given size. */
	TStrongObjectPtr<UInventoryGrid> MakeGrid(int32 Width, int32 Height)
	{
		TStrongObjectPtr<UInventoryGrid> Grid(NewObject<UInventoryGrid>());
		Grid->Init(Width, Height);
		return Grid;
	}
} // namespace FlipperGeneratorTests

	#define FLIPPER_GENERATOR_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperGeneratorTests;

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorRarityOddsTest, "Flipper.ItemGenerator.RarityOdds", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorRarityOddsTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Any"), 1, 1);
	UFlipperItemGenerator* Generator = F.Build();

	// Default weights are 70 / 25 / 5.
	FRandomStream Random(1234);
	int32		  Counts[3] = { 0, 0, 0 };
	const int32	  Rolls = 10000;
	for (int32 i = 0; i < Rolls; ++i)
	{
		++Counts[static_cast<int32>(Generator->RollRarity(Random))];
	}

	TestTrue(FString::Printf(TEXT("about 70%% Common (got %d of %d)"), Counts[0], Rolls), Counts[0] >= 6700 && Counts[0] <= 7300);
	TestTrue(FString::Printf(TEXT("about 25%% Uncommon (got %d of %d)"), Counts[1], Rolls), Counts[1] >= 2200 && Counts[1] <= 2800);
	TestTrue(FString::Printf(TEXT("about 5%% Rare (got %d of %d)"), Counts[2], Rolls), Counts[2] >= 350 && Counts[2] <= 650);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorRarityWeightsTest, "Flipper.ItemGenerator.RarityWeights", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorRarityWeightsTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Any"), 1, 1);

	struct FCase
	{
		const TCHAR*   Label;
		float		   Common, Uncommon, Rare;
		EFlipperRarity Expected;
	};
	const FCase Cases[] = {
		{ TEXT("only Common has weight"), 1.f, 0.f, 0.f, EFlipperRarity::Common },
		{ TEXT("only Uncommon has weight"), 0.f, 3.f, 0.f, EFlipperRarity::Uncommon },
		{ TEXT("only Rare has weight"), 0.f, 0.f, 0.5f, EFlipperRarity::Rare },
		{ TEXT("all weights 0 falls back to Common"), 0.f, 0.f, 0.f, EFlipperRarity::Common },
	};

	for (const FCase& Case : Cases)
	{
		F.Config.CommonWeight = Case.Common;
		F.Config.UncommonWeight = Case.Uncommon;
		F.Config.RareWeight = Case.Rare;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(7);
		bool		  bAllExpected = true;
		for (int32 i = 0; i < 200; ++i)
		{
			bAllExpected &= (Generator->RollRarity(Random) == Case.Expected);
		}
		TestTrue(Case.Label, bAllExpected);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorDefinitionWeightsTest, "Flipper.ItemGenerator.DefinitionWeights", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorDefinitionWeightsTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture		F;
	UFlipperItemDefinition* Likely = F.AddDefinition(TEXT("Likely"), 1, 1, 3.f);
	UFlipperItemDefinition* Unlikely = F.AddDefinition(TEXT("Unlikely"), 1, 1, 1.f);
	UFlipperItemDefinition* Never = F.AddDefinition(TEXT("Never"), 1, 1, 0.f);
	UFlipperItemGenerator*	Generator = F.Build();

	FRandomStream Random(99);
	int32		  LikelyCount = 0;
	int32		  UnlikelyCount = 0;
	int32		  NeverCount = 0;
	const int32	  Rolls = 10000;
	for (int32 i = 0; i < Rolls; ++i)
	{
		const UFlipperItemDefinition* Rolled = Generator->RollDefinition(Random);
		LikelyCount += (Rolled == Likely);
		UnlikelyCount += (Rolled == Unlikely);
		NeverCount += (Rolled == Never);
	}

	TestEqual(TEXT("a definition with weight 0 never comes up"), NeverCount, 0);
	TestEqual(TEXT("only the weighted definitions come up"), LikelyCount + UnlikelyCount, Rolls);
	TestTrue(FString::Printf(TEXT("weight 3 against weight 1 is about 75%% (got %d of %d)"), LikelyCount, Rolls), LikelyCount >= 7200 && LikelyCount <= 7800);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorNoDefinitionsTest, "Flipper.ItemGenerator.NoDefinitions", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorNoDefinitionsTest::RunTest(const FString& Parameters)
{
	FRandomStream Random(5);

	// No definitions at all.
	FGeneratorFixture Empty;
	UFlipperItemGenerator* EmptyGenerator = Empty.Build();
	TestNull(TEXT("RollDefinition with no definitions returns nullptr"), EmptyGenerator->RollDefinition(Random));
	const FFlipperItemInstance NoItem = EmptyGenerator->RollItem(Random);
	TestEqual(TEXT("RollItem with no definitions has Id INDEX_NONE"), NoItem.Id, static_cast<int32>(INDEX_NONE));
	TestNull(TEXT("RollItem with no definitions has a null Definition"), NoItem.Definition.Get());

	// Definitions that all have weight 0.
	FGeneratorFixture Zero;
	Zero.AddDefinition(TEXT("Zero"), 1, 1, 0.f);
	TestNull(TEXT("RollDefinition with only weight-0 definitions returns nullptr"), Zero.Build()->RollDefinition(Random));

	// Filling a table with nothing to roll places nothing and does not crash.
	TStrongObjectPtr<UInventoryGrid> Table = MakeGrid(8, 5);
	TestEqual(TEXT("FillSellerTable with no definitions places 0"), EmptyGenerator->FillSellerTable(Table.Get(), Random), 0);
	TestEqual(TEXT("and leaves the table empty"), Table->GetPlacedItems().Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorRollItemTest, "Flipper.ItemGenerator.RollItem", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorRollItemTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture		F;
	UFlipperItemDefinition* A = F.AddDefinition(TEXT("A"), 1, 1);
	UFlipperItemDefinition* B = F.AddDefinition(TEXT("B"), 2, 2);
	UFlipperItemGenerator*	Generator = F.Build();

	FRandomStream Random(42);
	bool		  bIdsCount = true;
	bool		  bDefinitionsKnown = true;
	for (int32 i = 1; i <= 50; ++i)
	{
		const FFlipperItemInstance Item = Generator->RollItem(Random);
		bIdsCount &= (Item.Id == i);
		bDefinitionsKnown &= (Item.Definition == A || Item.Definition == B);
	}
	TestTrue(TEXT("Ids are 1, 2, 3, ... in the order the items are rolled"), bIdsCount);
	TestTrue(TEXT("every item's Definition is one of the definitions given to Init"), bDefinitionsKnown);

	// Init restarts the counter.
	Generator->Init({ A, B }, F.Config);
	TestEqual(TEXT("Init restarts the Ids at 1"), Generator->RollItem(Random).Id, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorUntestedGambleTest, "Flipper.ItemGenerator.UntestedGamble", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorUntestedGambleTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Any"), 1, 1);
	UFlipperItemGenerator* Generator = F.Build();

	// Default config: 60% of items are Untested. The works chance depends on the condition (45 / 60 / 75 / 90% at
	// weights 20 / 35 / 30 / 15), which averages about 66%.
	FRandomStream Random(2024);
	const int32	  Rolls = 10000;
	int32		  Untested = 0;
	int32		  UntestedWorking = 0;
	bool		  bTestedAlwaysWorks = true;
	for (int32 i = 0; i < Rolls; ++i)
	{
		const FFlipperItemInstance Item = Generator->RollItem(Random);
		if (Item.bTested)
		{
			bTestedAlwaysWorks &= Item.bWorks;
		}
		else
		{
			++Untested;
			UntestedWorking += Item.bWorks;
		}
	}

	TestTrue(TEXT("a Tested item always works (a dead item is never labelled on the table)"), bTestedAlwaysWorks);
	TestTrue(FString::Printf(TEXT("about 60%% are Untested (got %d of %d)"), Untested, Rolls), Untested >= 5700 && Untested <= 6300);
	const float WorkingShare = Untested > 0 ? static_cast<float>(UntestedWorking) / Untested : 0.f;
	TestTrue(FString::Printf(TEXT("about 66%% of Untested items work (got %.3f)"), WorkingShare), WorkingShare >= 0.62f && WorkingShare <= 0.70f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorUntestedExtremesTest, "Flipper.ItemGenerator.UntestedExtremes", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorUntestedExtremesTest::RunTest(const FString& Parameters)
{
	struct FCase
	{
		const TCHAR* Label;
		float		 UntestedChance;
		float		 WorksChance;
		bool		 bExpectTested;
		bool		 bExpectWorks;
	};
	const FCase Cases[] = {
		{ TEXT("UntestedChance 0: every item is Tested and works"), 0.f, 0.f, true, true },
		{ TEXT("UntestedChance 1, works chance 0: every item is Untested and dead"), 1.f, 0.f, false, false },
		{ TEXT("UntestedChance 1, works chance 1: every item is Untested and works"), 1.f, 1.f, false, true },
	};

	for (const FCase& Case : Cases)
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		F.Config.UntestedChance = Case.UntestedChance;
		SetWorksChances(F.Config, Case.WorksChance);
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(11);
		bool		  bAllMatch = true;
		for (int32 i = 0; i < 200; ++i)
		{
			const FFlipperItemInstance Item = Generator->RollItem(Random);
			bAllMatch &= (Item.bTested == Case.bExpectTested && Item.bWorks == Case.bExpectWorks);
		}
		TestTrue(Case.Label, bAllMatch);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

// Condition, traits and lots (Docs/FlipperSpec.md, Feature 7).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorConditionOddsTest, "Flipper.ItemGenerator.ConditionOdds", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorConditionOddsTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Any"), 1, 1);
	UFlipperItemGenerator* Generator = F.Build();

	// Default weights are 20 / 35 / 30 / 15 for Poor / Fair / Good / Mint.
	FRandomStream Random(1234);
	int32		  Counts[4] = { 0, 0, 0, 0 };
	const int32	  Rolls = 10000;
	for (int32 i = 0; i < Rolls; ++i)
	{
		++Counts[static_cast<int32>(Generator->RollCondition(Random))];
	}

	TestTrue(FString::Printf(TEXT("about 20%% Poor (got %d of %d)"), Counts[0], Rolls), Counts[0] >= 1880 && Counts[0] <= 2120);
	TestTrue(FString::Printf(TEXT("about 35%% Fair (got %d of %d)"), Counts[1], Rolls), Counts[1] >= 3350 && Counts[1] <= 3650);
	TestTrue(FString::Printf(TEXT("about 30%% Good (got %d of %d)"), Counts[2], Rolls), Counts[2] >= 2850 && Counts[2] <= 3150);
	TestTrue(FString::Printf(TEXT("about 15%% Mint (got %d of %d)"), Counts[3], Rolls), Counts[3] >= 1380 && Counts[3] <= 1620);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorConditionWeightsTest, "Flipper.ItemGenerator.ConditionWeights", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorConditionWeightsTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Any"), 1, 1);

	struct FCase
	{
		const TCHAR*	  Label;
		float			  Poor, Fair, Good, Mint;
		EFlipperCondition Expected;
	};
	const FCase Cases[] = {
		{ TEXT("only Poor has weight"), 1.f, 0.f, 0.f, 0.f, EFlipperCondition::Poor },
		{ TEXT("only Fair has weight"), 0.f, 2.f, 0.f, 0.f, EFlipperCondition::Fair },
		{ TEXT("only Good has weight"), 0.f, 0.f, 0.5f, 0.f, EFlipperCondition::Good },
		{ TEXT("only Mint has weight"), 0.f, 0.f, 0.f, 3.f, EFlipperCondition::Mint },
		{ TEXT("all weights 0 falls back to Good"), 0.f, 0.f, 0.f, 0.f, EFlipperCondition::Good },
	};

	for (const FCase& Case : Cases)
	{
		F.Config.PoorWeight = Case.Poor;
		F.Config.FairWeight = Case.Fair;
		F.Config.GoodWeight = Case.Good;
		F.Config.MintWeight = Case.Mint;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(7);
		bool		  bAllExpected = true;
		for (int32 i = 0; i < 200; ++i)
		{
			bAllExpected &= (Generator->RollCondition(Random) == Case.Expected);
		}
		TestTrue(Case.Label, bAllExpected);
	}

	// Exactly one number is taken from the stream per roll, even when every weight is 0.
	for (const FCase& Case : { Cases[0], Cases[4] })
	{
		F.Config.PoorWeight = Case.Poor;
		F.Config.FairWeight = Case.Fair;
		F.Config.GoodWeight = Case.Good;
		F.Config.MintWeight = Case.Mint;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Used(7);
		FRandomStream Reference(7);
		Generator->RollCondition(Used);
		Reference.GetFraction();
		TestEqual(FString::Printf(TEXT("exactly one number is drawn per roll (%s)"), Case.Label), Used.GetCurrentSeed(), Reference.GetCurrentSeed());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorWorksByConditionTest, "Flipper.ItemGenerator.WorksByCondition", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorWorksByConditionTest::RunTest(const FString& Parameters)
{
	// Every item Untested, one condition at a time: its works chance is the config's number for that condition.
	const EFlipperCondition Conditions[] = { EFlipperCondition::Poor, EFlipperCondition::Fair, EFlipperCondition::Good, EFlipperCondition::Mint };
	for (const EFlipperCondition Condition : Conditions)
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		F.Config.UntestedChance = 1.f;
		OnlyCondition(F.Config, Condition);
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(31);
		const int32	  Rolls = 4000;
		int32		  Working = 0;
		bool		  bConditionSet = true;
		for (int32 i = 0; i < Rolls; ++i)
		{
			const FFlipperItemInstance Item = Generator->RollItem(Random);
			bConditionSet &= (Item.Condition == Condition);
			Working += Item.bWorks;
		}

		const float Expected = F.Config.GetWorksChance(Condition);
		const float Share = static_cast<float>(Working) / Rolls;
		TestTrue(FString::Printf(TEXT("%s: every rolled item has that condition"), FlipperCondition::GetName(Condition)), bConditionSet);
		TestTrue(FString::Printf(TEXT("%s: about %.0f%% of Untested items work (got %.3f)"), FlipperCondition::GetName(Condition), Expected * 100.f, Share), FMath::Abs(Share - Expected) <= 0.04f);
	}

	// The works chances are read from the config, one per condition.
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		F.Config.UntestedChance = 1.f;
		OnlyCondition(F.Config, EFlipperCondition::Mint);
		SetWorksChances(F.Config, 1.f);
		F.Config.MintWorksChance = 0.f;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(5);
		bool		  bAllDead = true;
		for (int32 i = 0; i < 200; ++i)
		{
			bAllDead &= !Generator->RollItem(Random).bWorks;
		}
		TestTrue(TEXT("a Mint works chance of 0 makes every Mint Untested item dead, whatever the others say"), bAllDead);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorTraitCountOddsTest, "Flipper.ItemGenerator.TraitCountOdds", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorTraitCountOddsTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Any"), 1, 1);
	for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E") })
	{
		F.AddTrait(Name);
	}
	UFlipperItemGenerator* Generator = F.Build();

	// Default weights are 55 / 35 / 10 for 0 / 1 / 2 traits.
	FRandomStream Random(77);
	int32		  Counts[3] = { 0, 0, 0 };
	const int32	  Rolls = 10000;
	for (int32 i = 0; i < Rolls; ++i)
	{
		const int32 Count = Generator->RollTraits(Random).Num();
		if (Count >= 0 && Count <= 2)
		{
			++Counts[Count];
		}
		else
		{
			AddError(FString::Printf(TEXT("an item got %d traits (the most allowed is 2)"), Count));
			return true;
		}
	}

	TestTrue(FString::Printf(TEXT("about 55%% have no trait (got %d of %d)"), Counts[0], Rolls), Counts[0] >= 5350 && Counts[0] <= 5650);
	TestTrue(FString::Printf(TEXT("about 35%% have one trait (got %d of %d)"), Counts[1], Rolls), Counts[1] >= 3350 && Counts[1] <= 3650);
	TestTrue(FString::Printf(TEXT("about 10%% have two traits (got %d of %d)"), Counts[2], Rolls), Counts[2] >= 910 && Counts[2] <= 1090);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorTraitLimitsTest, "Flipper.ItemGenerator.TraitLimits", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorTraitLimitsTest::RunTest(const FString& Parameters)
{
	struct FCase
	{
		const TCHAR* Label;
		float		 None, One, Two;
		int32		 TraitsGiven;
		int32		 ExpectedCount;
	};
	const FCase Cases[] = {
		{ TEXT("only the 0-trait weight: no traits"), 1.f, 0.f, 0.f, 5, 0 },
		{ TEXT("only the 1-trait weight: one trait"), 0.f, 1.f, 0.f, 5, 1 },
		{ TEXT("only the 2-trait weight: two traits"), 0.f, 0.f, 1.f, 5, 2 },
		{ TEXT("all three weights 0: no traits"), 0.f, 0.f, 0.f, 5, 0 },
		{ TEXT("a count of 2 with only one trait available gives that one"), 0.f, 0.f, 1.f, 1, 1 },
		{ TEXT("no traits given to Init: none, whatever the count"), 0.f, 0.f, 1.f, 0, 0 },
	};

	for (const FCase& Case : Cases)
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		for (int32 i = 0; i < Case.TraitsGiven; ++i)
		{
			F.AddTrait(*FString::Printf(TEXT("Trait%d"), i));
		}
		F.Config.NoTraitsWeight = Case.None;
		F.Config.OneTraitWeight = Case.One;
		F.Config.TwoTraitsWeight = Case.Two;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(13);
		bool		  bAllMatch = true;
		for (int32 i = 0; i < 200; ++i)
		{
			bAllMatch &= (Generator->RollTraits(Random).Num() == Case.ExpectedCount);
		}
		TestTrue(Case.Label, bAllMatch);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorTraitPicksTest, "Flipper.ItemGenerator.TraitPicks", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorTraitPicksTest::RunTest(const FString& Parameters)
{
	// Two traits on one item are always different, and always ones that were given to Init.
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		TArray<UFlipperTraitDefinition*> Given;
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E") })
		{
			Given.Add(F.AddTrait(Name));
		}
		F.Config.NoTraitsWeight = 0.f;
		F.Config.OneTraitWeight = 0.f;
		F.Config.TwoTraitsWeight = 1.f;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(3);
		bool		  bDistinct = true;
		bool		  bKnown = true;
		for (int32 i = 0; i < 500; ++i)
		{
			const TArray<TObjectPtr<UFlipperTraitDefinition>> Traits = Generator->RollTraits(Random);
			bDistinct &= (Traits.Num() == 2 && Traits[0] != Traits[1]);
			for (const TObjectPtr<UFlipperTraitDefinition>& Trait : Traits)
			{
				bKnown &= Given.Contains(Trait.Get());
			}
		}
		TestTrue(TEXT("two traits on one item are never the same trait"), bDistinct);
		TestTrue(TEXT("every trait comes from the ones given to Init"), bKnown);
	}

	// A trait with weight 0 never comes up, and weights are relative.
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		UFlipperTraitDefinition* Likely = F.AddTrait(TEXT("Likely"), 1.f, 3.f);
		UFlipperTraitDefinition* Unlikely = F.AddTrait(TEXT("Unlikely"), 1.f, 1.f);
		UFlipperTraitDefinition* Never = F.AddTrait(TEXT("Never"), 1.f, 0.f);
		F.Config.NoTraitsWeight = 0.f;
		F.Config.OneTraitWeight = 1.f;
		F.Config.TwoTraitsWeight = 0.f;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(99);
		int32		  LikelyCount = 0;
		int32		  UnlikelyCount = 0;
		int32		  NeverCount = 0;
		const int32	  Rolls = 10000;
		for (int32 i = 0; i < Rolls; ++i)
		{
			const TArray<TObjectPtr<UFlipperTraitDefinition>> Traits = Generator->RollTraits(Random);
			if (Traits.Num() == 1)
			{
				LikelyCount += (Traits[0] == Likely);
				UnlikelyCount += (Traits[0] == Unlikely);
				NeverCount += (Traits[0] == Never);
			}
		}
		TestEqual(TEXT("a trait with weight 0 never comes up"), NeverCount, 0);
		TestEqual(TEXT("only the weighted traits come up"), LikelyCount + UnlikelyCount, Rolls);
		TestTrue(FString::Printf(TEXT("weight 3 against weight 1 is about 75%% (got %d of %d)"), LikelyCount, Rolls), LikelyCount >= 7200 && LikelyCount <= 7800);
	}

	// One number for the count, then one per trait picked.
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C") })
		{
			F.AddTrait(Name);
		}
		UFlipperItemGenerator* Generator = F.Build();

		bool bDrawsMatch = true;
		for (int32 Seed = 1; Seed <= 60; ++Seed)
		{
			FRandomStream Used(Seed);
			FRandomStream Reference(Seed);
			const int32	  Count = Generator->RollTraits(Used).Num();
			for (int32 i = 0; i < 1 + Count; ++i)
			{
				Reference.GetFraction();
			}
			bDrawsMatch &= (Used.GetCurrentSeed() == Reference.GetCurrentSeed());
		}
		TestTrue(TEXT("RollTraits draws one number for the count plus one per trait it returns"), bDrawsMatch);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorRollItemExtrasTest, "Flipper.ItemGenerator.RollItemExtras", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorRollItemExtrasTest::RunTest(const FString& Parameters)
{
	// RollItem uses the condition and trait rolls: with only Mint and exactly two traits possible, every item is Mint with two.
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Any"), 1, 1);
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D") })
		{
			F.AddTrait(Name);
		}
		OnlyCondition(F.Config, EFlipperCondition::Mint);
		F.Config.NoTraitsWeight = 0.f;
		F.Config.OneTraitWeight = 0.f;
		F.Config.TwoTraitsWeight = 1.f;
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream Random(17);
		bool		  bAllMint = true;
		bool		  bTwoDistinctTraits = true;
		for (int32 i = 0; i < 100; ++i)
		{
			const FFlipperItemInstance Item = Generator->RollItem(Random);
			bAllMint &= (Item.Condition == EFlipperCondition::Mint);
			bTwoDistinctTraits &= (Item.Traits.Num() == 2 && Item.Traits[0] != Item.Traits[1]);
		}
		TestTrue(TEXT("every rolled item has the only condition that can come up"), bAllMint);
		TestTrue(TEXT("every rolled item has two different traits"), bTwoDistinctTraits);
	}

	// A lot rolls like any other item: rarity, condition, traits and the dead state all vary.
	{
		FGeneratorFixture		F;
		UFlipperItemDefinition* Lot = F.AddDefinition(TEXT("Box of Floppies"), 3, 2);
		Lot->bIsLot = true;
		Lot->LotCount = 10;
		F.AddTrait(TEXT("Boxed"), 1.4f);
		F.AddTrait(TEXT("Modded"), 0.7f);
		UFlipperItemGenerator* Generator = F.Build();

		FRandomStream  Random(23);
		TSet<int32>	   ConditionsSeen;
		bool		   bSawTrait = false;
		bool		   bSawDead = false;
		bool		   bSawUncommon = false;
		for (int32 i = 0; i < 2000; ++i)
		{
			const FFlipperItemInstance Item = Generator->RollItem(Random);
			ConditionsSeen.Add(static_cast<int32>(Item.Condition));
			bSawTrait |= !Item.Traits.IsEmpty();
			bSawDead |= (!Item.bTested && !Item.bWorks);
			bSawUncommon |= (Item.Rarity != EFlipperRarity::Common);
		}
		TestEqual(TEXT("a lot rolls all four conditions"), ConditionsSeen.Num(), 4);
		TestTrue(TEXT("a lot can have traits"), bSawTrait);
		TestTrue(TEXT("an Untested lot can be dead"), bSawDead);
		TestTrue(TEXT("a lot can be Uncommon or Rare"), bSawUncommon);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorFillCountTest, "Flipper.ItemGenerator.FillCount", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorFillCountTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Small"), 1, 1);
	F.AddDefinition(TEXT("Long"), 2, 1);
	F.AddDefinition(TEXT("Tall"), 1, 2);

	// Default: 8 items a day on the 8x5 table. These definitions are small, so all 8 always fit.
	UFlipperItemGenerator*			 Generator = F.Build();
	TStrongObjectPtr<UInventoryGrid> Table = MakeGrid(8, 5);
	FRandomStream					 Random(3);
	TestEqual(TEXT("FillSellerTable returns ItemsPerDay when everything fits"), Generator->FillSellerTable(Table.Get(), Random), 8);
	TestEqual(TEXT("and that many items are in the table"), Table->GetPlacedItems().Num(), 8);

	// ItemsPerDay is read from the config.
	F.Config.ItemsPerDay = 3;
	Generator = F.Build();
	Table = MakeGrid(8, 5);
	TestEqual(TEXT("FillSellerTable follows Config.ItemsPerDay"), Generator->FillSellerTable(Table.Get(), Random), 3);
	TestEqual(TEXT("and that many items are in the table (3)"), Table->GetPlacedItems().Num(), 3);

	// It adds to what is already there.
	TestEqual(TEXT("a second fill adds 3 more"), Generator->FillSellerTable(Table.Get(), Random), 3);
	TestEqual(TEXT("so the table now holds 6"), Table->GetPlacedItems().Num(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorFillCrowdedTest, "Flipper.ItemGenerator.FillCrowded", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorFillCrowdedTest::RunTest(const FString& Parameters)
{
	// A 2x2 table can only hold four 1x1 items. The other four rolls do not fit, and are skipped.
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Small"), 1, 1);
		UFlipperItemGenerator*			 Generator = F.Build();
		TStrongObjectPtr<UInventoryGrid> Table = MakeGrid(2, 2);
		FRandomStream					 Random(8);
		TestEqual(TEXT("a full table stops at what fits"), Generator->FillSellerTable(Table.Get(), Random), 4);
		TestEqual(TEXT("and holds exactly 4 items"), Table->GetPlacedItems().Num(), 4);
	}

	// An item that fits nowhere is skipped, not forced in.
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Huge"), 4, 3);
		UFlipperItemGenerator*			 Generator = F.Build();
		TStrongObjectPtr<UInventoryGrid> Table = MakeGrid(3, 3);
		FRandomStream					 Random(8);
		TestEqual(TEXT("items that fit nowhere are skipped"), Generator->FillSellerTable(Table.Get(), Random), 0);
		TestEqual(TEXT("and the table stays empty"), Table->GetPlacedItems().Num(), 0);
	}

	// A big item that does not fit does not stop smaller ones from being placed (the re-roll gets another chance).
	{
		FGeneratorFixture F;
		F.AddDefinition(TEXT("Small"), 1, 1);
		F.AddDefinition(TEXT("Huge"), 4, 3);
		F.Config.ItemsPerDay = 6;
		UFlipperItemGenerator*			 Generator = F.Build();
		TStrongObjectPtr<UInventoryGrid> Table = MakeGrid(3, 3);
		FRandomStream					 Random(21);
		const int32						 Placed = Generator->FillSellerTable(Table.Get(), Random);
		TestTrue(FString::Printf(TEXT("some small items are placed even though Huge never fits (got %d)"), Placed), Placed >= 1);
		TestEqual(TEXT("the return value matches what is in the table"), Table->GetPlacedItems().Num(), Placed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorFillDeterministicTest, "Flipper.ItemGenerator.FillDeterministic", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorFillDeterministicTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("A"), 1, 1);
	F.AddDefinition(TEXT("B"), 2, 1);
	F.AddDefinition(TEXT("C"), 2, 2);
	F.AddDefinition(TEXT("D"), 3, 2);

	// Two generators with the same definitions and the same seed must produce the same table.
	auto FillWithSeed = [&F](int32 Seed) -> TArray<FFlipperPlacedItem>
	{
		UFlipperItemGenerator*			 Generator = F.Build();
		TStrongObjectPtr<UInventoryGrid> Table = MakeGrid(8, 5);
		FRandomStream					 Random(Seed);
		Generator->FillSellerTable(Table.Get(), Random);
		TArray<FFlipperPlacedItem> Items = Table->GetPlacedItems();
		Items.Sort([](const FFlipperPlacedItem& L, const FFlipperPlacedItem& R) { return L.Item.Id < R.Item.Id; });
		return Items;
	};

	const TArray<FFlipperPlacedItem> First = FillWithSeed(555);
	const TArray<FFlipperPlacedItem> Second = FillWithSeed(555);

	bool bSame = First.Num() == Second.Num() && First.Num() > 0;
	for (int32 i = 0; bSame && i < First.Num(); ++i)
	{
		const FFlipperPlacedItem& L = First[i];
		const FFlipperPlacedItem& R = Second[i];
		bSame = L.Item.Id == R.Item.Id && L.Item.Definition == R.Item.Definition && L.Item.Rarity == R.Item.Rarity && L.Item.Condition == R.Item.Condition && L.Item.Traits == R.Item.Traits && L.Item.bTested == R.Item.bTested && L.Item.bWorks == R.Item.bWorks && L.Placement.Origin == R.Placement.Origin && L.Placement.bRotated == R.Placement.bRotated;
	}
	TestTrue(TEXT("the same seed gives the same table"), bSame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGeneratorUniqueIdsTest, "Flipper.ItemGenerator.UniqueIds", FLIPPER_GENERATOR_TEST_FLAGS)
bool FFlipperGeneratorUniqueIdsTest::RunTest(const FString& Parameters)
{
	FGeneratorFixture F;
	F.AddDefinition(TEXT("Small"), 1, 1);
	UFlipperItemGenerator* Generator = F.Build();

	// One generator hands out the Ids for the whole run, so two days' tables never share an Id.
	FRandomStream					 Random(1);
	TStrongObjectPtr<UInventoryGrid> DayOne = MakeGrid(8, 5);
	TStrongObjectPtr<UInventoryGrid> DayTwo = MakeGrid(8, 5);
	Generator->FillSellerTable(DayOne.Get(), Random);
	Generator->FillSellerTable(DayTwo.Get(), Random);

	TSet<int32> Ids;
	bool		bDuplicate = false;
	for (const UInventoryGrid* Table : { DayOne.Get(), DayTwo.Get() })
	{
		for (const FFlipperPlacedItem& Placed : Table->GetPlacedItems())
		{
			bDuplicate |= Ids.Contains(Placed.Item.Id);
			Ids.Add(Placed.Item.Id);
		}
	}
	TestFalse(TEXT("no Id appears twice across two fills"), bDuplicate);
	TestEqual(TEXT("both fills placed 8 items"), Ids.Num(), 16);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
