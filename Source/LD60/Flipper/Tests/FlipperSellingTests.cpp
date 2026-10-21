// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for selling (Docs/FlipperSpec.md, Feature 5): a sale on the buyer's counter, the
// buyer's offer reaching the wallet, the daily mood, and the whole buy, test and sell loop of a day.
// Run them with:  Automation RunTests Flipper.Selling

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "FlipperDayLoop.h"
	#include "FlipperHeldItemController.h"
	#include "FlipperTestHelpers.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperItemGenerator.h"
	#include "FlipperPricing.h"
	#include "FlipperTradeRule.h"
	#include "FlipperWallet.h"
	#include "InventoryGrid.h"

using namespace FlipperTestHelpers;

namespace FlipperSellingTests
{
	/**
	 *  A trunk and a seller table (both 6x4), a wallet with 100 cash, a controller and a trade rule bound to it, with
	 *  selling already open (the day loop does that in the Sell phase). Items have a base value of 100, so a Common one
	 *  that is Tested and working is offered 100.
	 */
	struct FSellFixture
	{
		TStrongObjectPtr<UInventoryGrid>				 Table;
		TStrongObjectPtr<UInventoryGrid>				 Trunk;
		TStrongObjectPtr<UFlipperWallet>				 Wallet;
		TStrongObjectPtr<UFlipperHeldItemController>	 Controller;
		TStrongObjectPtr<UFlipperTradeRule>				 Rule;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;
		int32											 NextItemId = 1;

		FSellFixture()
		{
			Table.Reset(NewObject<UInventoryGrid>());
			Table->Init(6, 4);
			Trunk.Reset(NewObject<UInventoryGrid>());
			Trunk->Init(6, 4);
			Wallet.Reset(NewObject<UFlipperWallet>());
			Wallet->Init(100);
			Controller.Reset(NewObject<UFlipperHeldItemController>());
			Controller->Init(5.f);
			Rule.Reset(NewObject<UFlipperTradeRule>());
			Rule->Init(Table.Get(), Trunk.Get(), Wallet.Get());
			Rule->Bind(Controller.Get());
			Rule->SetOpen(false);
			Rule->SetSellOpen(true);
		}

		FFlipperItemInstance MakeItem(bool bTested, bool bWorks, int32 BaseValue = 100)
		{
			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = 1;
			Definition->Height = 1;
			Definition->BaseValue = BaseValue;
			Definitions.Emplace(Definition);

			FFlipperItemInstance Item;
			Item.Id = NextItemId++;
			Item.Definition = Definition;
			Item.bTested = bTested;
			Item.bWorks = bWorks;
			return Item;
		}

		/** Puts a 1x1 item into Grid at Origin. */
		FFlipperItemInstance PlaceItem(UInventoryGrid* Grid, FIntPoint Origin, bool bTested, bool bWorks, int32 BaseValue = 100)
		{
			FFlipperItemInstance Item = MakeItem(bTested, bWorks, BaseValue);
			const bool			 bPlaced = Grid->TryPlace(Item, Origin, false);
			ensureMsgf(bPlaced, TEXT("Test setup: PlaceItem could not place the item"));
			return Item;
		}

		void Drag(UInventoryGrid* FromGrid, FIntPoint FromCell, UInventoryGrid* ToGrid, FIntPoint ToCell)
		{
			Controller->PointerDown(FromGrid, CellCentre(FromCell), FVector2D(0.0, 0.0));
			Controller->PointerMove(FVector2D(50.0, 0.0));
			Controller->PointerUp(ToGrid, CellCentre(ToCell), FVector2D(50.0, 0.0));
		}
	};

	/** The day loop's fixture: a trunk (6x4), a table (8x5), 1000 cash, a generator of 1x1 items worth 10, a loop of 3 days. */
	struct FLoopFixture
	{
		TStrongObjectPtr<UInventoryGrid>				 Trunk;
		TStrongObjectPtr<UInventoryGrid>				 Table;
		TStrongObjectPtr<UFlipperWallet>				 Wallet;
		TStrongObjectPtr<UFlipperHeldItemController>	 Controller;
		TStrongObjectPtr<UFlipperTradeRule>				 Rule;
		TStrongObjectPtr<UFlipperItemGenerator>			 Generator;
		TStrongObjectPtr<UFlipperDayLoop>				 Loop;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;

		explicit FLoopFixture(int32 Seed)
		{
			Trunk.Reset(NewObject<UInventoryGrid>());
			Trunk->Init(6, 4);
			Table.Reset(NewObject<UInventoryGrid>());
			Table->Init(8, 5);
			Wallet.Reset(NewObject<UFlipperWallet>());
			Wallet->Init(1000);
			Controller.Reset(NewObject<UFlipperHeldItemController>());
			Controller->Init(5.f);
			Rule.Reset(NewObject<UFlipperTradeRule>());
			Rule->Init(Table.Get(), Trunk.Get(), Wallet.Get());
			Rule->Bind(Controller.Get());

			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = 1;
			Definition->Height = 1;
			Definition->BaseValue = 10;
			Definition->SpawnWeight = 1.f;
			Definitions.Emplace(Definition);

			Generator.Reset(NewObject<UFlipperItemGenerator>());
			Generator->Init({ Definition }, FFlipperGenerationConfig());

			FFlipperDayConfig Config;
			Config.DayCount = 3;
			Loop.Reset(NewObject<UFlipperDayLoop>());
			Loop->Init(Config, Trunk.Get(), Table.Get(), Generator.Get(), Controller.Get(), Rule.Get(), Seed);
			Loop->Bind(Controller.Get());
		}

		void Drag(UInventoryGrid* FromGrid, FIntPoint FromCell, UInventoryGrid* ToGrid, FIntPoint ToCell)
		{
			Controller->PointerDown(FromGrid, CellCentre(FromCell), FVector2D(0.0, 0.0));
			Controller->PointerMove(FVector2D(50.0, 0.0));
			Controller->PointerUp(ToGrid, CellCentre(ToCell), FVector2D(50.0, 0.0));
		}
	};
} // namespace FlipperSellingTests

	#define FLIPPER_SELLING_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperSellingTests;

// ---------------------------------------------------------------------------------------------------------------
// A sale on the counter

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellDragTest, "Flipper.Selling.DragToTheCounterSells", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellDragTest::RunTest(const FString& Parameters)
{
	FSellFixture F;
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), true, true);

	F.Drag(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get(), FIntPoint(2, 2));

	TestEqual(TEXT("the buyer paid the offer (100) on top of the 100 cash"), F.Wallet->GetCash(), 200);
	TestNull(TEXT("the item left the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
	TestNull(TEXT("and is not on the counter either: a sale is final and the item is used up"), F.Table->FindItemAt(FIntPoint(2, 2)));
	TestFalse(TEXT("nothing is held"), F.Controller->IsHolding());
	TestEqual(TEXT("the counter stays empty so it is free for the next sale"), F.Table->GetPlacedItems().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellShiftClickTest, "Flipper.Selling.ShiftClickSells", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellShiftClickTest::RunTest(const FString& Parameters)
{
	FSellFixture F;
	F.PlaceItem(F.Trunk.Get(), FIntPoint(1, 1), true, true);

	TestTrue(TEXT("the Shift-click sells"), F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(1, 1), F.Table.Get()));
	TestEqual(TEXT("and pays the offer"), F.Wallet->GetCash(), 200);
	TestNull(TEXT("the item left the trunk"), F.Trunk->FindItemAt(FIntPoint(1, 1)));
	TestEqual(TEXT("and nothing was placed on the counter"), F.Table->GetPlacedItems().Num(), 0);

	// The item is used up, so a Shift-click sells even when the counter has no free cell.
	for (int32 Y = 0; Y < 4; ++Y)
	{
		for (int32 X = 0; X < 6; ++X)
		{
			F.PlaceItem(F.Table.Get(), FIntPoint(X, Y), true, true);
		}
	}
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), true, true);
	TestTrue(TEXT("a Shift-click sells even when the counter is full"), F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get()));
	TestEqual(TEXT("and pays again"), F.Wallet->GetCash(), 300);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellOfferKindsTest, "Flipper.Selling.OfferDependsOnWhatIsKnown", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellOfferKindsTest::RunTest(const FString& Parameters)
{
	FSellFixture F;
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), true, false);  // Tested, dead: scrap, 10
	F.PlaceItem(F.Trunk.Get(), FIntPoint(1, 0), false, true);  // Untested, works: 50
	F.PlaceItem(F.Trunk.Get(), FIntPoint(2, 0), false, false); // Untested, dead: 50 as well

	F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get());
	TestEqual(TEXT("a Tested dead item sells for scrap"), F.Wallet->GetCash(), 110);
	F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(1, 0), F.Table.Get());
	TestEqual(TEXT("an Untested working item sells for half its Value"), F.Wallet->GetCash(), 160);
	F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(2, 0), F.Table.Get());
	TestEqual(TEXT("an Untested dead item sells for the same half"), F.Wallet->GetCash(), 210);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellMoodTest, "Flipper.Selling.MoodScalesTheOffer", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellMoodTest::RunTest(const FString& Parameters)
{
	FSellFixture F;
	F.Rule->SetOfferMultiplier(1.25f);
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), true, true);

	const FFlipperPlacedItem* Placed = F.Trunk->FindItemAt(FIntPoint(0, 0));
	TestEqual(TEXT("the tooltip's offer on a Hot day"), F.Rule->GetOffer(Placed->Item), 125);

	F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get());
	TestEqual(TEXT("and the wallet gets the same 125"), F.Wallet->GetCash(), 225);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellRulesTest, "Flipper.Selling.OnlySellsWhenSellingIsOpen", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellRulesTest::RunTest(const FString& Parameters)
{
	FSellFixture F;
	const FFlipperItemInstance InTrunk = F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), true, true);
	const FFlipperItemInstance OnTable = F.PlaceItem(F.Table.Get(), FIntPoint(0, 0), true, true);

	// While selling is open nothing can be bought or refunded.
	TestTrue(TEXT("trunk to counter is a sale"), F.Rule->CanTransfer(F.Trunk.Get(), F.Table.Get(), InTrunk));
	TestFalse(TEXT("counter to trunk is refused (no buying)"), F.Rule->CanTransfer(F.Table.Get(), F.Trunk.Get(), OnTable));

	// Closed: the trunk's contents cannot be sold or refunded.
	F.Rule->SetSellOpen(false);
	TestFalse(TEXT("with selling closed trunk to counter is refused"), F.Rule->CanTransfer(F.Trunk.Get(), F.Table.Get(), InTrunk));
	TestFalse(TEXT("selling is not a sale when closed"), F.Rule->IsSale(F.Trunk.Get(), F.Table.Get()));
	F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get());
	TestEqual(TEXT("so no money moved"), F.Wallet->GetCash(), 100);
	TestNotNull(TEXT("and the item is still in the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellGhostTest, "Flipper.Selling.BadDropKeepsTheItem", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellGhostTest::RunTest(const FString& Parameters)
{
	FSellFixture F;
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), true, true);

	F.Controller->PointerDown(F.Trunk.Get(), CellCentre(0, 0), FVector2D(0.0, 0.0));
	FIntPoint Origin;
	bool	  bValid = false;
	TestTrue(TEXT("a ghost is shown over the counter"), F.Controller->GetGhost(F.Table.Get(), CellCentre(3, 3), Origin, bValid));
	TestTrue(TEXT("and it is valid: the sale is allowed"), bValid);

	// Dropping outside the counter is the usual invalid drop: the item snaps back, nothing is sold.
	F.Controller->PointerMove(FVector2D(50.0, 0.0));
	F.Controller->PointerUp(nullptr, FVector2D::ZeroVector, FVector2D(50.0, 0.0));
	TestNotNull(TEXT("the item is back in the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
	TestEqual(TEXT("and no money moved"), F.Wallet->GetCash(), 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellMoveInsideTrunkTest, "Flipper.Selling.MovingInsideTheTrunkIsNotASale", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellMoveInsideTrunkTest::RunTest(const FString& Parameters)
{
	FSellFixture F;
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), true, true);

	F.Drag(F.Trunk.Get(), FIntPoint(0, 0), F.Trunk.Get(), FIntPoint(4, 2));

	TestNotNull(TEXT("the item moved inside the trunk"), F.Trunk->FindItemAt(FIntPoint(4, 2)));
	TestEqual(TEXT("and was not sold"), F.Wallet->GetCash(), 100);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The day loop and the mood

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellPhaseTest, "Flipper.Selling.TheSellPhaseTurnsTheTableIntoACounter", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellPhaseTest::RunTest(const FString& Parameters)
{
	FLoopFixture F(3);
	F.Loop->StartRun();
	TestFalse(TEXT("no selling in the Market"), F.Rule->IsSellOpen());
	TestTrue(TEXT("the seller's table is full"), F.Table->GetPlacedItems().Num() > 0);

	F.Loop->EndPhase();
	TestFalse(TEXT("no selling in the Garage"), F.Rule->IsSellOpen());
	TestTrue(TEXT("the table is still the seller's"), F.Table->GetPlacedItems().Num() > 0);

	F.Loop->EndPhase();
	TestEqual(TEXT("it is the Sell phase"), F.Loop->GetPhase(), EFlipperPhase::Sell);
	TestTrue(TEXT("selling is open"), F.Rule->IsSellOpen());
	TestFalse(TEXT("buying and refunds are closed"), F.Rule->IsOpen());
	TestEqual(TEXT("the seller packed up: the table is empty"), F.Table->GetPlacedItems().Num(), 0);

	F.Loop->EndPhase();
	TestEqual(TEXT("the next day starts in the Market"), F.Loop->GetPhase(), EFlipperPhase::Market);
	TestFalse(TEXT("selling is closed again"), F.Rule->IsSellOpen());
	TestTrue(TEXT("and the seller is back with a fresh table"), F.Table->GetPlacedItems().Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellFinishedTest, "Flipper.Selling.NoSellingAfterTheRun", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellFinishedTest::RunTest(const FString& Parameters)
{
	FLoopFixture F(3);
	F.Loop->StartRun();
	while (F.Loop->GetPhase() != EFlipperPhase::Finished)
	{
		F.Loop->EndPhase();
	}

	TestFalse(TEXT("selling is closed once the run is over"), F.Rule->IsSellOpen());
	TestFalse(TEXT("and so is buying"), F.Rule->IsOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellMoodRolledTest, "Flipper.Selling.MoodIsRolledOncePerDay", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellMoodRolledTest::RunTest(const FString& Parameters)
{
	FLoopFixture F(11);
	F.Loop->StartRun();

	const EFlipperBuyerMood Day1Mood = F.Loop->GetMood();
	TestEqual(TEXT("the rule uses the loop's multiplier from the start of the day"), F.Rule->GetOfferMultiplier(), F.Loop->GetMoodMultiplier());
	TestEqual(TEXT("the multiplier is the mood's"), F.Loop->GetMoodMultiplier(), FlipperPricing::GetMoodMultiplier(Day1Mood, FFlipperPricingConfig()));

	F.Loop->EndPhase();
	F.Loop->EndPhase();
	TestTrue(TEXT("the mood is the same all day"), F.Loop->GetMood() == Day1Mood);

	// The same seed gives the same moods on every day.
	FLoopFixture Twin(11);
	Twin.Loop->StartRun();
	TestTrue(TEXT("a fixed seed gives the same day 1 mood"), Twin.Loop->GetMood() == Day1Mood);
	F.Loop->EndPhase();
	Twin.Loop->EndPhase();
	Twin.Loop->EndPhase();
	Twin.Loop->EndPhase();
	TestTrue(TEXT("and the same day 2 mood"), Twin.Loop->GetMood() == F.Loop->GetMood());

	// Over many seeds every mood shows up on day 1.
	bool bSeen[3] = { false, false, false };
	for (int32 Seed = 0; Seed < 60; ++Seed)
	{
		FLoopFixture Other(Seed);
		Other.Loop->StartRun();
		bSeen[static_cast<int32>(Other.Loop->GetMood())] = true;
	}
	TestTrue(TEXT("Slow, Normal and Hot all get rolled across seeds"), bSeen[0] && bSeen[1] && bSeen[2]);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// A whole day: buy, test, sell

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperSellWholeDayTest, "Flipper.Selling.BuyTestAndSellInOneDay", FLIPPER_SELLING_TEST_FLAGS)
bool FFlipperSellWholeDayTest::RunTest(const FString& Parameters)
{
	for (int32 Seed = 1; Seed <= 25; ++Seed)
	{
		FLoopFixture F(Seed);
		F.Loop->StartRun();

		// Market: buy the first item on the table.
		const FFlipperPlacedItem Wanted = F.Table->GetPlacedItems()[0];
		const int32				 Ask = Wanted.Item.AskPrice;
		F.Drag(F.Table.Get(), Wanted.Placement.Origin, F.Trunk.Get(), FIntPoint(0, 0));
		if (!TestNotNull(TEXT("bought into the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0))))
		{
			return true;
		}
		TestEqual(TEXT("paid the ask"), F.Wallet->GetCash(), 1000 - Ask);

		// Garage: an Untested item is tested with one click.
		F.Loop->EndPhase();
		if (!F.Trunk->FindItemAt(FIntPoint(0, 0))->Item.bTested)
		{
			F.Controller->PointerDown(F.Trunk.Get(), CellCentre(0, 0), FVector2D(0.0, 0.0));
			TestTrue(TEXT("tested in the Garage"), F.Trunk->FindItemAt(FIntPoint(0, 0))->Item.bTested);
		}

		// Sell: drag it onto the counter.
		F.Loop->EndPhase();
		const FFlipperItemInstance Item = F.Trunk->FindItemAt(FIntPoint(0, 0))->Item;
		const int32				   Expected = FlipperPricing::GetOffer(Item, FFlipperPricingConfig(), F.Loop->GetMoodMultiplier());
		TestEqual(TEXT("the tooltip shows the offer the wallet will get"), F.Rule->GetOffer(Item), Expected);

		F.Drag(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get(), FIntPoint(3, 3));
		TestNull(TEXT("sold: the trunk is empty again"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
		TestEqual(TEXT("the wallet got the offer"), F.Wallet->GetCash(), 1000 - Ask + Expected);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
