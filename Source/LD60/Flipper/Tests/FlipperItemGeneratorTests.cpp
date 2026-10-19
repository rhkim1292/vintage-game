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
			Generator.Reset(NewObject<UFlipperItemGenerator>());
			Generator->Init(Raw, Config);
			return Generator.Get();
		}
	};

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

	// Default config: 60% of items are Untested, and 65% of those work.
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
	TestTrue(FString::Printf(TEXT("about 65%% of Untested items work (got %.3f)"), WorkingShare), WorkingShare >= 0.62f && WorkingShare <= 0.68f);
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
		F.Config.UntestedWorksChance = Case.WorksChance;
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
		bSame = L.Item.Id == R.Item.Id && L.Item.Definition == R.Item.Definition && L.Item.Rarity == R.Item.Rarity && L.Item.bTested == R.Item.bTested && L.Item.bWorks == R.Item.bWorks && L.Placement.Origin == R.Placement.Origin && L.Placement.bRotated == R.Placement.bRotated;
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
