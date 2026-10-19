// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for UFlipperDayLoop: the Market, Garage and Sell phases of each day
// (Docs/FlipperSpec.md, Feature 4). Run them with:  Automation RunTests Flipper.DayLoop

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "FlipperDayLoop.h"
	#include "FlipperHeldItemController.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperItemGenerator.h"
	#include "FlipperTradeRule.h"
	#include "FlipperWallet.h"
	#include "InventoryGrid.h"

namespace FlipperDayLoopTests
{
	/** A trunk (6x4), a seller table (8x5), a generator of 1x1 items, a trade rule, a controller and a day loop (3 days). */
	struct FDayFixture
	{
		TStrongObjectPtr<UInventoryGrid>				 Trunk;
		TStrongObjectPtr<UInventoryGrid>				 Table;
		TStrongObjectPtr<UFlipperWallet>				 Wallet;
		TStrongObjectPtr<UFlipperHeldItemController>	 Controller;
		TStrongObjectPtr<UFlipperTradeRule>				 Rule;
		TStrongObjectPtr<UFlipperItemGenerator>			 Generator;
		TStrongObjectPtr<UFlipperDayLoop>				 Loop;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;
		int32											 NextItemId = 1000;

		explicit FDayFixture(int32 Seed = 7, int32 DayCount = 3)
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
			Config.DayCount = DayCount;
			Loop.Reset(NewObject<UFlipperDayLoop>());
			Loop->Init(Config, Trunk.Get(), Table.Get(), Generator.Get(), Controller.Get(), Rule.Get(), Seed);
			Loop->Bind(Controller.Get());
		}

		/** A 1x1 item put straight into the trunk at Origin. */
		FFlipperItemInstance PlaceInTrunk(FIntPoint Origin, bool bTested, bool bWorks, bool bBoughtThisPhase = false, int32 PaidPrice = 0)
		{
			FFlipperItemInstance Item;
			Item.Id = NextItemId++;
			Item.Definition = Definitions[0].Get();
			Item.bTested = bTested;
			Item.bWorks = bWorks;
			Item.bBoughtThisPhase = bBoughtThisPhase;
			Item.PaidPrice = PaidPrice;
			const bool bPlaced = Trunk->TryPlace(Item, Origin, false);
			ensureMsgf(bPlaced, TEXT("Test setup: PlaceInTrunk could not place the item"));
			return Item;
		}

		/** Ends phases until the Garage of the current day. */
		void GoToGarage()
		{
			while (Loop->GetPhase() != EFlipperPhase::Garage)
			{
				Loop->EndPhase();
			}
		}

		/** Ends the current day and starts the next one's Market. */
		void GoToNextDay()
		{
			const int32 Day = Loop->GetDay();
			while (Loop->GetDay() == Day && Loop->GetPhase() != EFlipperPhase::Finished)
			{
				Loop->EndPhase();
			}
		}

		TArray<int32> TableIds() const
		{
			TArray<int32> Ids;
			for (const FFlipperPlacedItem& Placed : Table->GetPlacedItems())
			{
				Ids.Add(Placed.Item.Id);
			}
			Ids.Sort();
			return Ids;
		}
	};
} // namespace FlipperDayLoopTests

	#define FLIPPER_DAY_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperDayLoopTests;

// ---------------------------------------------------------------------------------------------------------------
// The phases and the clock

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayStartTest, "Flipper.DayLoop.StartRun", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayStartTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();

	TestEqual(TEXT("the run starts in the Market"), F.Loop->GetPhase(), EFlipperPhase::Market);
	TestEqual(TEXT("on day 1"), F.Loop->GetDay(), 1);
	TestEqual(TEXT("with a full Market clock"), F.Loop->GetTimeLeft(), 60.f);
	TestEqual(TEXT("and no test charges yet"), F.Loop->GetTestChargesLeft(), 0);
	TestEqual(TEXT("and a rolled seller table"), F.Table->GetPlacedItems().Num(), 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayMarketClockTest, "Flipper.DayLoop.MarketClockCountsDown", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayMarketClockTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();

	F.Loop->Tick(10.f);

	TestEqual(TEXT("10 seconds off the Market clock"), F.Loop->GetTimeLeft(), 50.f);
	TestEqual(TEXT("still the Market"), F.Loop->GetPhase(), EFlipperPhase::Market);
	TestFalse(TEXT("no warning yet"), F.Loop->IsWarning());

	F.Loop->Tick(41.f);
	TestTrue(TEXT("in the last 10 seconds the warning is on"), F.Loop->IsWarning());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayMarketTimeoutTest, "Flipper.DayLoop.MarketTimeoutStartsGarage", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayMarketTimeoutTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();

	F.Loop->Tick(60.f);

	TestEqual(TEXT("when the clock reaches zero the Garage starts"), F.Loop->GetPhase(), EFlipperPhase::Garage);
	TestEqual(TEXT("the Garage has no clock"), F.Loop->GetTimeLeft(), 0.f);
	TestFalse(TEXT("so it never warns"), F.Loop->IsWarning());
	TestEqual(TEXT("and gives 3 test charges"), F.Loop->GetTestChargesLeft(), 3);

	F.Loop->Tick(1000.f);
	TestEqual(TEXT("time passing does not end the Garage"), F.Loop->GetPhase(), EFlipperPhase::Garage);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayEndPhaseTest, "Flipper.DayLoop.EndPhaseMovesOn", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayEndPhaseTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();

	F.Loop->EndPhase();
	TestEqual(TEXT("ending the Market early starts the Garage"), F.Loop->GetPhase(), EFlipperPhase::Garage);

	F.Loop->EndPhase();
	TestEqual(TEXT("ending the Garage starts the Sell phase"), F.Loop->GetPhase(), EFlipperPhase::Sell);
	TestEqual(TEXT("with a full Sell clock"), F.Loop->GetTimeLeft(), 45.f);

	F.Loop->EndPhase();
	TestEqual(TEXT("ending the Sell phase starts the Market"), F.Loop->GetPhase(), EFlipperPhase::Market);
	TestEqual(TEXT("of the next day"), F.Loop->GetDay(), 2);
	TestEqual(TEXT("with a full Market clock again"), F.Loop->GetTimeLeft(), 60.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDaySellTimeoutTest, "Flipper.DayLoop.SellTimeoutStartsNextDay", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDaySellTimeoutTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.Loop->EndPhase();
	F.Loop->EndPhase();

	F.Loop->Tick(45.f);

	TestEqual(TEXT("the Sell clock running out starts the next day's Market"), F.Loop->GetPhase(), EFlipperPhase::Market);
	TestEqual(TEXT("on day 2"), F.Loop->GetDay(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayFinishTest, "Flipper.DayLoop.LastDayFinishesTheRun", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayFinishTest::RunTest(const FString& Parameters)
{
	FDayFixture F(7, 2);
	F.Loop->StartRun();

	F.GoToNextDay();
	TestEqual(TEXT("day 2 starts"), F.Loop->GetDay(), 2);
	F.GoToNextDay();

	TestEqual(TEXT("after the last day's Sell phase the run is Finished"), F.Loop->GetPhase(), EFlipperPhase::Finished);
	TestEqual(TEXT("still on the last day"), F.Loop->GetDay(), 2);

	F.Loop->EndPhase();
	F.Loop->Tick(500.f);
	TestEqual(TEXT("nothing moves a finished run"), F.Loop->GetPhase(), EFlipperPhase::Finished);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayBannerTest, "Flipper.DayLoop.BannerShowsAfterEveryPhaseChange", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayBannerTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	TestEqual(TEXT("the banner starts at its full time"), F.Loop->GetBannerTimeLeft(), 1.f);

	F.Loop->Tick(0.4f);
	TestTrue(TEXT("it counts down"), FMath::IsNearlyEqual(F.Loop->GetBannerTimeLeft(), 0.6f, 0.001f));

	F.Loop->Tick(5.f);
	TestEqual(TEXT("and stops at zero"), F.Loop->GetBannerTimeLeft(), 0.f);

	F.Loop->EndPhase();
	TestEqual(TEXT("a new phase brings it back"), F.Loop->GetBannerTimeLeft(), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayEventTest, "Flipper.DayLoop.PhaseChangeIsBroadcast", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayEventTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	TArray<EFlipperPhase> Seen;
	F.Loop->OnPhaseChanged.AddLambda([&Seen](EFlipperPhase Phase, int32 Day) { Seen.Add(Phase); });

	F.Loop->StartRun();
	F.Loop->EndPhase();
	F.Loop->EndPhase();

	if (TestEqual(TEXT("one event per phase"), Seen.Num(), 3))
	{
		TestEqual(TEXT("Market first"), Seen[0], EFlipperPhase::Market);
		TestEqual(TEXT("then Garage"), Seen[1], EFlipperPhase::Garage);
		TestEqual(TEXT("then Sell"), Seen[2], EFlipperPhase::Sell);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The seller table and the trunk across days

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayRerollTest, "Flipper.DayLoop.SellerTableIsRerolledEachDay", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayRerollTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	const TArray<int32> DayOne = F.TableIds();

	F.GoToNextDay();
	const TArray<int32> DayTwo = F.TableIds();

	TestEqual(TEXT("day 2 has a full table"), DayTwo.Num(), 8);
	bool bAnyReused = false;
	for (int32 Id : DayTwo)
	{
		bAnyReused |= DayOne.Contains(Id);
	}
	TestFalse(TEXT("none of day 1's items are left on it (new items, new Ids)"), bAnyReused);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDaySameSeedTest, "Flipper.DayLoop.SameSeedRollsTheSameTable", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDaySameSeedTest::RunTest(const FString& Parameters)
{
	FDayFixture A(42);
	FDayFixture B(42);
	A.Loop->StartRun();
	B.Loop->StartRun();

	const TArray<FFlipperPlacedItem>& ItemsA = A.Table->GetPlacedItems();
	const TArray<FFlipperPlacedItem>& ItemsB = B.Table->GetPlacedItems();
	if (!TestEqual(TEXT("same item count"), ItemsA.Num(), ItemsB.Num()))
	{
		return true;
	}
	for (int32 Index = 0; Index < ItemsA.Num(); ++Index)
	{
		TestEqual(TEXT("same rarity"), ItemsA[Index].Item.Rarity, ItemsB[Index].Item.Rarity);
		TestEqual(TEXT("same ask price"), ItemsA[Index].Item.AskPrice, ItemsB[Index].Item.AskPrice);
		TestEqual(TEXT("same place"), ItemsA[Index].Placement.Origin, ItemsB[Index].Placement.Origin);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTrunkCarriesOverTest, "Flipper.DayLoop.TrunkItemsCarryOver", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTrunkCarriesOverTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(2, 1), true, true);

	F.GoToNextDay();

	TestNotNull(TEXT("an unsold item is still in the trunk on the next day"), F.Trunk->FindItemAt(FIntPoint(2, 1)));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Leaving the Market

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayClearBoughtTest, "Flipper.DayLoop.EndOfMarketClearsBoughtFlags", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayClearBoughtTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(0, 0), false, true, true, 25);

	F.Loop->EndPhase();

	const FFlipperPlacedItem* Placed = F.Trunk->FindItemAt(FIntPoint(0, 0));
	if (TestNotNull(TEXT("the item is still in the trunk"), Placed))
	{
		TestFalse(TEXT("but it can no longer be refunded"), Placed->Item.bBoughtThisPhase);
		TestEqual(TEXT("it still remembers what was paid"), Placed->Item.PaidPrice, 25);
		TestFalse(TEXT("and clearing the flag did not reveal it"), Placed->Item.bTested);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayBoughtItemStuckTest, "Flipper.DayLoop.BoughtItemCannotGoBackAfterMarket", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayBoughtItemStuckTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(0, 0), true, true, true, 25);
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	TestEqual(TEXT("(setup: three days later the Market is open again)"), F.Loop->GetPhase(), EFlipperPhase::Market);

	const int32 CashBefore = F.Wallet->GetCash();
	F.Controller->PointerDown(F.Trunk.Get(), FIntPoint(0, 0), FVector2D(0.0, 0.0));
	F.Controller->PointerMove(FVector2D(50.0, 0.0));
	F.Controller->PointerUp(F.Table.Get(), FIntPoint(7, 4), FVector2D(50.0, 0.0));

	TestNotNull(TEXT("an item from an earlier day stays in the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
	TestEqual(TEXT("and no refund is paid"), F.Wallet->GetCash(), CashBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayEndSnapsBackTest, "Flipper.DayLoop.EndOfPhaseSnapsBackHeldItem", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayEndSnapsBackTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(1, 1), true, true, true, 10);
	F.Controller->PointerDown(F.Trunk.Get(), FIntPoint(1, 1), FVector2D(0.0, 0.0));
	TestTrue(TEXT("(setup: the item is held)"), F.Controller->IsHolding());

	F.Loop->Tick(60.f);

	TestFalse(TEXT("when the Market clock runs out nothing is held any more"), F.Controller->IsHolding());
	const FFlipperPlacedItem* Placed = F.Trunk->FindItemAt(FIntPoint(1, 1));
	if (TestNotNull(TEXT("the item is back where it was"), Placed))
	{
		TestFalse(TEXT("and it is no longer refundable"), Placed->Item.bBoughtThisPhase);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTradingTest, "Flipper.DayLoop.TradingOnlyInTheMarket", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTradingTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	TestTrue(TEXT("the Market is open for trade"), F.Rule->IsOpen());

	// Buy in the Market.
	const FFlipperPlacedItem* OnTable = F.Table->FindItemAt(F.Table->GetPlacedItems()[0].Placement.Origin);
	const FIntPoint			  TableCell = OnTable->Placement.Origin;
	F.Controller->PointerDown(F.Table.Get(), TableCell, FVector2D(0.0, 0.0));
	F.Controller->PointerMove(FVector2D(50.0, 0.0));
	F.Controller->PointerUp(F.Trunk.Get(), FIntPoint(0, 0), FVector2D(50.0, 0.0));
	TestNotNull(TEXT("buying works in the Market"), F.Trunk->FindItemAt(FIntPoint(0, 0)));

	F.Loop->EndPhase();
	TestFalse(TEXT("the Garage is closed for trade"), F.Rule->IsOpen());
	F.Loop->EndPhase();
	TestFalse(TEXT("so is the Sell phase"), F.Rule->IsOpen());

	// Shift-click a table item into the trunk during Sell: refused. (The Sell phase empties the table, so put one there.)
	FFlipperItemInstance Stray;
	Stray.Id = F.NextItemId++;
	Stray.Definition = F.Definitions[0].Get();
	F.Table->TryPlace(Stray, FIntPoint(0, 0), false);
	TestFalse(TEXT("a quick-transfer outside the Market is refused"), F.Controller->QuickTransfer(F.Table.Get(), FIntPoint(0, 0), F.Trunk.Get()));
	F.Table->Remove(Stray);

	F.Loop->EndPhase();
	TestTrue(TEXT("and the next day's Market opens it again"), F.Rule->IsOpen());
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The Garage test

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTestRevealsTest, "Flipper.DayLoop.GarageTestRevealsAnItem", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTestRevealsTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(0, 0), false, false);
	F.GoToGarage();

	const bool bTested = F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(0, 0));

	TestTrue(TEXT("testing an Untested trunk item works"), bTested);
	const FFlipperPlacedItem* Placed = F.Trunk->FindItemAt(FIntPoint(0, 0));
	if (TestNotNull(TEXT("the item is still there"), Placed))
	{
		TestTrue(TEXT("now Tested"), Placed->Item.bTested);
		TestFalse(TEXT("and the truth is revealed: it is dead"), Placed->Item.bWorks);
	}
	TestEqual(TEXT("it cost a charge"), F.Loop->GetTestChargesLeft(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTestChargesTest, "Flipper.DayLoop.GarageTestsAreLimited", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTestChargesTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	for (int32 X = 0; X < 4; ++X)
	{
		F.PlaceInTrunk(FIntPoint(X, 0), false, true);
	}
	F.GoToGarage();

	TestTrue(TEXT("test 1"), F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(0, 0)));
	TestTrue(TEXT("test 2"), F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(1, 0)));
	TestTrue(TEXT("test 3"), F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(2, 0)));
	TestFalse(TEXT("there is no fourth"), F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(3, 0)));

	TestFalse(TEXT("the fourth item is still Untested"), F.Trunk->FindItemAt(FIntPoint(3, 0))->Item.bTested);
	TestEqual(TEXT("no charges left"), F.Loop->GetTestChargesLeft(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTestRefillTest, "Flipper.DayLoop.ChargesComeBackEachDay", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTestRefillTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(0, 0), false, true);
	F.GoToGarage();
	F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(0, 0));
	TestEqual(TEXT("(setup: one charge spent)"), F.Loop->GetTestChargesLeft(), 2);

	F.Loop->EndPhase();
	TestEqual(TEXT("unused charges do not wait around in the Sell phase"), F.Loop->GetTestChargesLeft(), 0);
	F.Loop->EndPhase();
	F.GoToGarage();

	TestEqual(TEXT("the next day's Garage has a full set again, not 5"), F.Loop->GetTestChargesLeft(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTestWrongPlaceTest, "Flipper.DayLoop.GarageTestRefusals", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTestWrongPlaceTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(0, 0), false, true);

	TestFalse(TEXT("no testing in the Market"), F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(0, 0)));

	F.PlaceInTrunk(FIntPoint(1, 0), true, true);
	F.GoToGarage();
	TestFalse(TEXT("an already Tested item cannot be tested again"), F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(1, 0)));
	TestFalse(TEXT("an empty cell cannot be tested"), F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(5, 3)));
	TestFalse(TEXT("only trunk items can be tested, not the table's"), F.Loop->TryTestAt(F.Table.Get(), F.Table->GetPlacedItems()[0].Placement.Origin));
	TestEqual(TEXT("and none of that cost a charge"), F.Loop->GetTestChargesLeft(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTestClickTest, "Flipper.DayLoop.ClickingAnUntestedItemInTheGarageTestsIt", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTestClickTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	F.PlaceInTrunk(FIntPoint(0, 0), false, true);
	F.PlaceInTrunk(FIntPoint(1, 0), true, true);

	// In the Market a click just picks the item up.
	F.Controller->PointerDown(F.Trunk.Get(), FIntPoint(0, 0), FVector2D(0.0, 0.0));
	TestTrue(TEXT("in the Market a click picks the item up"), F.Controller->IsHolding());
	F.Controller->Cancel();

	F.GoToGarage();
	F.Controller->PointerDown(F.Trunk.Get(), FIntPoint(0, 0), FVector2D(0.0, 0.0));
	TestFalse(TEXT("in the Garage a click on an Untested item tests it instead of picking it up"), F.Controller->IsHolding());
	TestTrue(TEXT("and it is now Tested"), F.Trunk->FindItemAt(FIntPoint(0, 0))->Item.bTested);
	TestEqual(TEXT("for one charge"), F.Loop->GetTestChargesLeft(), 2);

	F.Controller->PointerDown(F.Trunk.Get(), FIntPoint(1, 0), FVector2D(0.0, 0.0));
	TestTrue(TEXT("a click on a Tested item still picks it up (to rearrange the trunk)"), F.Controller->IsHolding());
	F.Controller->Cancel();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperDayTestNoChargeClickTest, "Flipper.DayLoop.WithoutChargesAClickPicksUp", FLIPPER_DAY_TEST_FLAGS)
bool FFlipperDayTestNoChargeClickTest::RunTest(const FString& Parameters)
{
	FDayFixture F;
	F.Loop->StartRun();
	for (int32 X = 0; X < 4; ++X)
	{
		F.PlaceInTrunk(FIntPoint(X, 0), false, true);
	}
	F.GoToGarage();
	for (int32 X = 0; X < 3; ++X)
	{
		F.Loop->TryTestAt(F.Trunk.Get(), FIntPoint(X, 0));
	}

	F.Controller->PointerDown(F.Trunk.Get(), FIntPoint(3, 0), FVector2D(0.0, 0.0));

	TestTrue(TEXT("with no charges left a click on an Untested item picks it up like any other"), F.Controller->IsHolding());
	F.Controller->Cancel();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
