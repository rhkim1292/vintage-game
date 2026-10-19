// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for UFlipperTradeRule: the buy and refund rules plugged into the held-item
// controller (Docs/FlipperSpec.md, Feature 3). Run them with:  Automation RunTests Flipper.TradeRule

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "FlipperHeldItemController.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperTradeRule.h"
	#include "FlipperWallet.h"
	#include "InventoryGrid.h"

namespace FlipperTradeRuleTests
{
	/** A seller table and a trunk (both 6x4), a wallet with 100 cash, a controller, and the trade rule bound to it. */
	struct FTradeFixture
	{
		TStrongObjectPtr<UInventoryGrid>				 Table;
		TStrongObjectPtr<UInventoryGrid>				 Trunk;
		TStrongObjectPtr<UFlipperWallet>				 Wallet;
		TStrongObjectPtr<UFlipperHeldItemController>	 Controller;
		TStrongObjectPtr<UFlipperTradeRule>				 Rule;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;
		int32											 NextItemId = 1;

		FTradeFixture()
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
		}

		/** A 1x1 item with the given ask price, placed in Grid at Origin. For a trunk item, pass the paid price and bought flag. */
		FFlipperItemInstance PlaceItem(UInventoryGrid* Grid, FIntPoint Origin, int32 AskPrice, int32 PaidPrice = 0, bool bBoughtThisPhase = false)
		{
			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = 1;
			Definition->Height = 1;
			Definitions.Emplace(Definition);

			FFlipperItemInstance Item;
			Item.Id = NextItemId++;
			Item.Definition = Definition;
			Item.AskPrice = AskPrice;
			Item.PaidPrice = PaidPrice;
			Item.bBoughtThisPhase = bBoughtThisPhase;
			const bool bPlaced = Grid->TryPlace(Item, Origin, false);
			ensureMsgf(bPlaced, TEXT("Test setup: PlaceItem could not place the item"));
			return Item;
		}

		/** A press-drag-release from one cell to another. */
		void Drag(UInventoryGrid* FromGrid, FIntPoint FromCell, UInventoryGrid* ToGrid, FIntPoint ToCell)
		{
			Controller->PointerDown(FromGrid, FromCell, FVector2D(0.0, 0.0));
			Controller->PointerMove(FVector2D(50.0, 0.0));
			Controller->PointerUp(ToGrid, ToCell, FVector2D(50.0, 0.0));
		}

		static const FFlipperPlacedItem* At(const UInventoryGrid* Grid, FIntPoint Cell) { return Grid->FindItemAt(Cell); }
	};
} // namespace FlipperTradeRuleTests

	#define FLIPPER_TRADE_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperTradeRuleTests;

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeBuyTest, "Flipper.TradeRule.Buy", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeBuyTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	F.PlaceItem(F.Table.Get(), FIntPoint(0, 0), 30);

	F.Drag(F.Table.Get(), FIntPoint(0, 0), F.Trunk.Get(), FIntPoint(2, 2));

	const FFlipperPlacedItem* Bought = F.At(F.Trunk.Get(), FIntPoint(2, 2));
	if (!TestNotNull(TEXT("the item is in the trunk"), Bought))
	{
		return true;
	}
	TestNull(TEXT("and no longer on the table"), F.At(F.Table.Get(), FIntPoint(0, 0)));
	TestEqual(TEXT("buying took the ask price"), F.Wallet->GetCash(), 70);
	TestEqual(TEXT("the item remembers what was paid"), Bought->Item.PaidPrice, 30);
	TestTrue(TEXT("and that it was bought this phase"), Bought->Item.bBoughtThisPhase);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeBuyExactCashTest, "Flipper.TradeRule.BuyWithExactCash", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeBuyExactCashTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	F.PlaceItem(F.Table.Get(), FIntPoint(0, 0), 100);

	F.Drag(F.Table.Get(), FIntPoint(0, 0), F.Trunk.Get(), FIntPoint(0, 0));

	TestNotNull(TEXT("an item costing exactly the cash can be bought"), F.At(F.Trunk.Get(), FIntPoint(0, 0)));
	TestEqual(TEXT("leaving 0 cash"), F.Wallet->GetCash(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeRefusedWhenShortTest, "Flipper.TradeRule.RefusedWhenShort", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeRefusedWhenShortTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	F.PlaceItem(F.Table.Get(), FIntPoint(0, 0), 101);

	// The ghost over the trunk is red while the item is too expensive.
	F.Controller->PointerDown(F.Table.Get(), FIntPoint(0, 0), FVector2D(0.0, 0.0));
	FIntPoint Origin;
	bool	  bValid = true;
	TestTrue(TEXT("a ghost is shown"), F.Controller->GetGhost(F.Trunk.Get(), FIntPoint(1, 1), Origin, bValid));
	TestFalse(TEXT("but it is invalid: not enough cash"), bValid);
	F.Controller->PointerMove(FVector2D(50.0, 0.0));
	F.Controller->PointerUp(F.Trunk.Get(), FIntPoint(1, 1), FVector2D(50.0, 0.0));

	TestNull(TEXT("nothing arrived in the trunk"), F.At(F.Trunk.Get(), FIntPoint(1, 1)));
	TestNotNull(TEXT("the item snapped back to the table"), F.At(F.Table.Get(), FIntPoint(0, 0)));
	TestEqual(TEXT("and no money moved"), F.Wallet->GetCash(), 100);
	TestFalse(TEXT("the controller is not still holding it"), F.Controller->IsHolding());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeRefundTest, "Flipper.TradeRule.Refund", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeRefundTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	F.PlaceItem(F.Table.Get(), FIntPoint(0, 0), 30);

	F.Drag(F.Table.Get(), FIntPoint(0, 0), F.Trunk.Get(), FIntPoint(0, 0)); // buy
	TestEqual(TEXT("after buying, cash is 70"), F.Wallet->GetCash(), 70);

	F.Drag(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get(), FIntPoint(3, 3)); // refund in the same phase

	const FFlipperPlacedItem* Returned = F.At(F.Table.Get(), FIntPoint(3, 3));
	if (!TestNotNull(TEXT("the item is back on the table"), Returned))
	{
		return true;
	}
	TestNull(TEXT("and out of the trunk"), F.At(F.Trunk.Get(), FIntPoint(0, 0)));
	TestEqual(TEXT("the refund gives the money back"), F.Wallet->GetCash(), 100);
	TestEqual(TEXT("the paid price is cleared"), Returned->Item.PaidPrice, 0);
	TestFalse(TEXT("and it is no longer marked as bought"), Returned->Item.bBoughtThisPhase);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeRefundPaidPriceTest, "Flipper.TradeRule.RefundIsWhatWasPaid", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeRefundPaidPriceTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	// Asked 40 on the table but 25 was paid: the refund is the paid price.
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), 40, 25, true);

	F.Drag(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get(), FIntPoint(0, 0));

	TestNotNull(TEXT("the item went back to the table"), F.At(F.Table.Get(), FIntPoint(0, 0)));
	TestEqual(TEXT("the refund is exactly what was paid"), F.Wallet->GetCash(), 125);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeOldItemTest, "Flipper.TradeRule.OldItemCannotGoBack", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeOldItemTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	// In the trunk from an earlier day: not bought this phase.
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), 40, 25, false);

	// The ghost over the table is red.
	F.Controller->PointerDown(F.Trunk.Get(), FIntPoint(0, 0), FVector2D(0.0, 0.0));
	FIntPoint Origin;
	bool	  bValid = true;
	TestTrue(TEXT("a ghost is shown"), F.Controller->GetGhost(F.Table.Get(), FIntPoint(2, 2), Origin, bValid));
	TestFalse(TEXT("but it is invalid: no refund for an old item"), bValid);
	F.Controller->PointerMove(FVector2D(50.0, 0.0));
	F.Controller->PointerUp(F.Table.Get(), FIntPoint(2, 2), FVector2D(50.0, 0.0));

	TestNotNull(TEXT("the item snapped back to the trunk"), F.At(F.Trunk.Get(), FIntPoint(0, 0)));
	TestNull(TEXT("and is not on the table"), F.At(F.Table.Get(), FIntPoint(2, 2)));
	TestEqual(TEXT("no money moved"), F.Wallet->GetCash(), 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeFreeMoveTest, "Flipper.TradeRule.MoveInsideOneGridIsFree", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeFreeMoveTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	F.PlaceItem(F.Trunk.Get(), FIntPoint(0, 0), 40, 25, true);
	F.PlaceItem(F.Table.Get(), FIntPoint(0, 0), 60);

	F.Drag(F.Trunk.Get(), FIntPoint(0, 0), F.Trunk.Get(), FIntPoint(4, 3));
	F.Drag(F.Table.Get(), FIntPoint(0, 0), F.Table.Get(), FIntPoint(5, 3));

	const FFlipperPlacedItem* InTrunk = F.At(F.Trunk.Get(), FIntPoint(4, 3));
	TestNotNull(TEXT("the trunk item moved inside the trunk"), InTrunk);
	TestNotNull(TEXT("the table item moved inside the table"), F.At(F.Table.Get(), FIntPoint(5, 3)));
	TestEqual(TEXT("no money moved"), F.Wallet->GetCash(), 100);
	if (InTrunk)
	{
		TestEqual(TEXT("the item keeps what was paid"), InTrunk->Item.PaidPrice, 25);
		TestTrue(TEXT("and stays refundable"), InTrunk->Item.bBoughtThisPhase);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperTradeQuickTransferTest, "Flipper.TradeRule.QuickTransfer", FLIPPER_TRADE_TEST_FLAGS)
bool FFlipperTradeQuickTransferTest::RunTest(const FString& Parameters)
{
	FTradeFixture F;
	F.PlaceItem(F.Table.Get(), FIntPoint(0, 0), 30);
	F.PlaceItem(F.Table.Get(), FIntPoint(1, 0), 90);

	TestTrue(TEXT("Shift-click buys an affordable item"), F.Controller->QuickTransfer(F.Table.Get(), FIntPoint(0, 0), F.Trunk.Get()));
	TestEqual(TEXT("and charges for it"), F.Wallet->GetCash(), 70);
	const FFlipperPlacedItem* Bought = F.At(F.Trunk.Get(), FIntPoint(0, 0));
	if (TestNotNull(TEXT("the item is in the trunk"), Bought))
	{
		TestEqual(TEXT("with the paid price"), Bought->Item.PaidPrice, 30);
		TestTrue(TEXT("and the bought flag"), Bought->Item.bBoughtThisPhase);
	}

	TestFalse(TEXT("Shift-click on an item that costs more than the cash is refused"), F.Controller->QuickTransfer(F.Table.Get(), FIntPoint(1, 0), F.Trunk.Get()));
	TestEqual(TEXT("no money moved"), F.Wallet->GetCash(), 70);
	TestNotNull(TEXT("the expensive item is still on the table"), F.At(F.Table.Get(), FIntPoint(1, 0)));

	TestTrue(TEXT("Shift-click refunds an item bought this phase"), F.Controller->QuickTransfer(F.Trunk.Get(), FIntPoint(0, 0), F.Table.Get()));
	TestEqual(TEXT("and gives the money back"), F.Wallet->GetCash(), 100);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
