// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for UFlipperHeldItemController (Docs/FlipperSpec.md, Feature 1,
// locked decisions 3 and 4, plus the rotation, invalid-drop and Shift-click routine choices).
// Run them with:  Automation RunTests Flipper.HeldItem

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "FlipperHeldItemController.h"
	#include "InventoryGrid.h"
	#include "FlipperItemDefinition.h"

namespace FlipperHeldItemTests
{
	/** Two 6x4 grids (A and B), a controller with a 5 px drag threshold, and a factory for items. */
	struct FHeldFixture
	{
		TStrongObjectPtr<UInventoryGrid>				 A;
		TStrongObjectPtr<UInventoryGrid>				 B;
		TStrongObjectPtr<UFlipperHeldItemController>	 Controller;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;
		int32											 NextItemId = 1;

		FHeldFixture()
		{
			A.Reset(NewObject<UInventoryGrid>());
			A->Init(6, 4);
			B.Reset(NewObject<UInventoryGrid>());
			B->Init(6, 4);
			Controller.Reset(NewObject<UFlipperHeldItemController>());
			Controller->Init(5.f);
		}

		/** Makes an item (unique Id) and places it in Grid. The placement must be valid. */
		FFlipperItemInstance PlaceItem(UInventoryGrid* Grid, int32 Width, int32 Height, FIntPoint Origin, bool bRotated = false)
		{
			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = Width;
			Definition->Height = Height;
			Definitions.Emplace(Definition);

			FFlipperItemInstance Item;
			Item.Id = NextItemId++;
			Item.Definition = Definition;

			const bool bPlaced = Grid->TryPlace(Item, Origin, bRotated);
			ensureMsgf(bPlaced, TEXT("Test setup: PlaceItem could not place the item"));
			return Item;
		}

		/** Id of the item covering Cell in Grid, or INDEX_NONE. */
		static int32 IdAt(const UInventoryGrid* Grid, FIntPoint Cell)
		{
			const FFlipperPlacedItem* Placed = Grid->FindItemAt(Cell);
			return Placed ? Placed->Item.Id : INDEX_NONE;
		}

		/** A press-drag-release: down on FromCell, move well past the threshold, up over ToCell. Returns whether the press picked something up. */
		bool Drag(UInventoryGrid* FromGrid, FIntPoint FromCell, UInventoryGrid* ToGrid, FIntPoint ToCell)
		{
			Controller->PointerDown(FromGrid, FromCell, FVector2D(0.0, 0.0));
			const bool bPickedUp = Controller->IsHolding();
			Controller->PointerMove(FVector2D(50.0, 0.0));
			Controller->PointerUp(ToGrid, ToCell, FVector2D(50.0, 0.0));
			return bPickedUp;
		}
	};
} // namespace FlipperHeldItemTests

	#define FLIPPER_HELD_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperHeldItemTests;

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldPickUpTest, "Flipper.HeldItem.PickUp", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldPickUpTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.A.Get(), 2, 1, FIntPoint(3, 2));

	// Pressing an empty cell picks nothing up.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	TestFalse(TEXT("empty cell: nothing held"), F.Controller->IsHolding());

	// Pressing the right-hand cell of the 2x1 picks the whole item up.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(4, 2), FVector2D(10.0, 10.0));
	TestTrue(TEXT("holding after pressing an item"), F.Controller->IsHolding());
	TestEqual(TEXT("held the right item"), F.Controller->GetHeldItem().Id, Item.Id);
	TestTrue(TEXT("remembers the origin grid"), F.Controller->GetOriginGrid() == F.A.Get());
	TestTrue(TEXT("remembers the origin cell"), F.Controller->GetOriginPlacement().Origin == FIntPoint(3, 2));
	TestFalse(TEXT("remembers it was unrotated"), F.Controller->GetOriginPlacement().bRotated);
	TestTrue(TEXT("grab offset is the pressed cell minus the origin"), F.Controller->GetGrabOffset() == FIntPoint(1, 0));
	TestFalse(TEXT("not rotated"), F.Controller->IsHeldRotated());
	TestTrue(TEXT("footprint is 2x1"), F.Controller->GetHeldFootprint() == FIntPoint(2, 1));

	// While held, the item is in no grid.
	TestEqual(TEXT("left its cells empty (left half)"), F.IdAt(F.A.Get(), FIntPoint(3, 2)), (int32)INDEX_NONE);
	TestEqual(TEXT("left its cells empty (right half)"), F.IdAt(F.A.Get(), FIntPoint(4, 2)), (int32)INDEX_NONE);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldClickStyleTest, "Flipper.HeldItem.ClickStyle", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldClickStyleTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.A.Get(), 2, 1, FIntPoint(0, 0));

	// Click once: press and release in the same spot. The item stays held.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	F.Controller->PointerUp(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	TestTrue(TEXT("still held after a click"), F.Controller->IsHolding());

	// Click again over (3,1): the grab offset is (0,0), so the item lands with its origin on (3,1).
	F.Controller->PointerDown(F.A.Get(), FIntPoint(3, 1), FVector2D(200.0, 80.0));
	TestFalse(TEXT("placed by the second click"), F.Controller->IsHolding());
	TestEqual(TEXT("left cell of the placed item"), F.IdAt(F.A.Get(), FIntPoint(3, 1)), Item.Id);
	TestEqual(TEXT("right cell of the placed item"), F.IdAt(F.A.Get(), FIntPoint(4, 1)), Item.Id);
	TestEqual(TEXT("old cell is empty"), F.IdAt(F.A.Get(), FIntPoint(0, 0)), (int32)INDEX_NONE);

	// The mouse-up that ends the second click must not pick anything up or move anything.
	F.Controller->PointerUp(F.A.Get(), FIntPoint(3, 1), FVector2D(200.0, 80.0));
	TestFalse(TEXT("release after placing holds nothing"), F.Controller->IsHolding());
	TestEqual(TEXT("item stayed where it was placed"), F.IdAt(F.A.Get(), FIntPoint(3, 1)), Item.Id);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldDragStyleTest, "Flipper.HeldItem.DragStyle", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldDragStyleTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.A.Get(), 2, 1, FIntPoint(0, 0));

	// Grab the right-hand cell, drag, release over (4,3): the origin is (4,3) - grab offset (1,0) = (3,3).
	F.Drag(F.A.Get(), FIntPoint(1, 0), F.A.Get(), FIntPoint(4, 3));

	TestFalse(TEXT("dropped on release"), F.Controller->IsHolding());
	TestEqual(TEXT("left cell of the dropped item"), F.IdAt(F.A.Get(), FIntPoint(3, 3)), Item.Id);
	TestEqual(TEXT("right cell of the dropped item"), F.IdAt(F.A.Get(), FIntPoint(4, 3)), Item.Id);
	TestEqual(TEXT("old cell is empty"), F.IdAt(F.A.Get(), FIntPoint(0, 0)), (int32)INDEX_NONE);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldDragThresholdTest, "Flipper.HeldItem.DragThreshold", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldDragThresholdTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.A.Get(), 1, 1, FIntPoint(2, 2));

	// A 3 px wobble is under the 5 px threshold, so releasing is still a click: the item stays held.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(2, 2), FVector2D(10.0, 10.0));
	F.Controller->PointerMove(FVector2D(13.0, 10.0));
	F.Controller->PointerUp(F.A.Get(), FIntPoint(0, 0), FVector2D(13.0, 10.0));
	TestTrue(TEXT("3 px is a click, not a drag"), F.Controller->IsHolding());

	F.Controller->Cancel();
	TestFalse(TEXT("cancel ends the hold"), F.Controller->IsHolding());

	// 6 px is over the threshold, so releasing over a valid cell drops the item there.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(2, 2), FVector2D(10.0, 10.0));
	F.Controller->PointerMove(FVector2D(16.0, 10.0));
	F.Controller->PointerUp(F.A.Get(), FIntPoint(0, 0), FVector2D(16.0, 10.0));
	TestFalse(TEXT("6 px is a drag: dropped on release"), F.Controller->IsHolding());
	TestEqual(TEXT("dropped at the released cell"), F.IdAt(F.A.Get(), FIntPoint(0, 0)), Item.Id);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldInvalidDropTest, "Flipper.HeldItem.InvalidDropSnapsBack", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldInvalidDropTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Wide = F.PlaceItem(F.A.Get(), 2, 1, FIntPoint(0, 0));
	const FFlipperItemInstance Blocker = F.PlaceItem(F.A.Get(), 1, 1, FIntPoint(4, 0));

	// Drop onto another item: refused, the wide item goes back to where it started.
	TestTrue(TEXT("overlap: the press picked the item up"), F.Drag(F.A.Get(), FIntPoint(0, 0), F.A.Get(), FIntPoint(4, 0)));
	TestFalse(TEXT("overlap: nothing held afterwards"), F.Controller->IsHolding());
	TestEqual(TEXT("overlap: back at its left cell"), F.IdAt(F.A.Get(), FIntPoint(0, 0)), Wide.Id);
	TestEqual(TEXT("overlap: back at its right cell"), F.IdAt(F.A.Get(), FIntPoint(1, 0)), Wide.Id);
	TestEqual(TEXT("overlap: the other item is untouched"), F.IdAt(F.A.Get(), FIntPoint(4, 0)), Blocker.Id);

	// Drop half off the edge of the 6-wide grid: refused, snaps back.
	TestTrue(TEXT("off the edge: the press picked the item up"), F.Drag(F.A.Get(), FIntPoint(0, 0), F.A.Get(), FIntPoint(5, 2)));
	TestFalse(TEXT("off the edge: nothing held afterwards"), F.Controller->IsHolding());
	TestEqual(TEXT("off the edge: back at its origin"), F.IdAt(F.A.Get(), FIntPoint(0, 0)), Wide.Id);
	TestEqual(TEXT("off the edge: nothing left at the target"), F.IdAt(F.A.Get(), FIntPoint(5, 2)), (int32)INDEX_NONE);

	// Click style: the second click lands on an occupied cell. Same rule: snap back.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	F.Controller->PointerUp(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	TestTrue(TEXT("click style overlap: held after the first click"), F.Controller->IsHolding());
	F.Controller->PointerDown(F.A.Get(), FIntPoint(4, 0), FVector2D(90.0, 10.0));
	TestFalse(TEXT("click style overlap: nothing held afterwards"), F.Controller->IsHolding());
	TestEqual(TEXT("click style overlap: back at its origin"), F.IdAt(F.A.Get(), FIntPoint(0, 0)), Wide.Id);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldDropOutsideTest, "Flipper.HeldItem.DropOverNoGrid", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldDropOutsideTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.A.Get(), 1, 2, FIntPoint(1, 1));

	// Released over nothing (a null grid): snap back.
	TestTrue(TEXT("drag release over no grid: the press picked the item up"), F.Drag(F.A.Get(), FIntPoint(1, 1), nullptr, FIntPoint(0, 0)));
	TestFalse(TEXT("drag release over no grid: nothing held"), F.Controller->IsHolding());
	TestEqual(TEXT("drag release over no grid: back at its origin"), F.IdAt(F.A.Get(), FIntPoint(1, 1)), Item.Id);

	// Second click over nothing: snap back too.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(1, 2), FVector2D(10.0, 10.0));
	F.Controller->PointerUp(F.A.Get(), FIntPoint(1, 2), FVector2D(10.0, 10.0));
	TestTrue(TEXT("click over no grid: held after the first click"), F.Controller->IsHolding());
	F.Controller->PointerDown(nullptr, FIntPoint(0, 0), FVector2D(300.0, 10.0));
	TestFalse(TEXT("click over no grid: nothing held"), F.Controller->IsHolding());
	TestEqual(TEXT("click over no grid: back at its origin"), F.IdAt(F.A.Get(), FIntPoint(1, 1)), Item.Id);
	TestEqual(TEXT("click over no grid: back at its origin (second cell)"), F.IdAt(F.A.Get(), FIntPoint(1, 2)), Item.Id);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldRotateTest, "Flipper.HeldItem.Rotate", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldRotateTest::RunTest(const FString& Parameters)
{
	FHeldFixture F;
	F.PlaceItem(F.A.Get(), 3, 2, FIntPoint(0, 0));

	// Rotating with nothing held does nothing.
	F.Controller->Rotate();
	TestFalse(TEXT("rotate with nothing held: still nothing held"), F.Controller->IsHolding());

	// Grab the 3x2's top-right cell: offset (2,0).
	F.Controller->PointerDown(F.A.Get(), FIntPoint(2, 0), FVector2D(10.0, 10.0));
	TestTrue(TEXT("grab offset (2,0)"), F.Controller->GetGrabOffset() == FIntPoint(2, 0));

	// A quarter turn clockwise: (x, y) in W x H becomes (H-1-y, x) in H x W, i.e. (2,0) in 3x2 becomes (1,2) in 2x3.
	F.Controller->Rotate();
	TestTrue(TEXT("now rotated"), F.Controller->IsHeldRotated());
	TestTrue(TEXT("footprint is 2x3"), F.Controller->GetHeldFootprint() == FIntPoint(2, 3));
	TestTrue(TEXT("grab offset turned to (1,2)"), F.Controller->GetGrabOffset() == FIntPoint(1, 2));

	// A second quarter turn is a half turn overall: back to the 3x2 footprint, offset (W-1-x, H-1-y) = (0,1).
	F.Controller->Rotate();
	TestFalse(TEXT("rotated back"), F.Controller->IsHeldRotated());
	TestTrue(TEXT("footprint is 3x2 again"), F.Controller->GetHeldFootprint() == FIntPoint(3, 2));
	TestTrue(TEXT("grab offset turned to (0,1)"), F.Controller->GetGrabOffset() == FIntPoint(0, 1));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldGhostTest, "Flipper.HeldItem.Ghost", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldGhostTest::RunTest(const FString& Parameters)
{
	FHeldFixture F;
	F.PlaceItem(F.A.Get(), 3, 2, FIntPoint(0, 0));
	F.PlaceItem(F.A.Get(), 1, 1, FIntPoint(5, 3));

	FIntPoint Origin;
	bool	  bValid = false;

	TestFalse(TEXT("no ghost while nothing is held"), F.Controller->GetGhost(F.A.Get(), FIntPoint(2, 2), Origin, bValid));

	// Hold the 3x2 by its top-right cell (offset (2,0)).
	F.Controller->PointerDown(F.A.Get(), FIntPoint(2, 0), FVector2D(10.0, 10.0));

	TestFalse(TEXT("no ghost over no grid"), F.Controller->GetGhost(nullptr, FIntPoint(2, 2), Origin, bValid));

	// Cursor over (4,2): origin = (4,2) - (2,0) = (2,2). Covers x 2..4, y 2..3, all free: green.
	TestTrue(TEXT("ghost exists"), F.Controller->GetGhost(F.A.Get(), FIntPoint(4, 2), Origin, bValid));
	TestTrue(TEXT("ghost origin is the hovered cell minus the grab offset"), Origin == FIntPoint(2, 2));
	TestTrue(TEXT("free cells: green"), bValid);

	// Cursor over (5,3): origin (3,3) would run off the bottom and right edges: red. The origin is still reported.
	TestTrue(TEXT("ghost exists off the edge"), F.Controller->GetGhost(F.A.Get(), FIntPoint(5, 3), Origin, bValid));
	TestTrue(TEXT("origin still reported when red"), Origin == FIntPoint(3, 3));
	TestFalse(TEXT("hanging off the edge: red"), bValid);

	// Cursor over (5,1): origin (3,1) covers x 3..5, y 1..2, all free (the 1x1 sits at (5,3)): green.
	// Cursor over (5,2): origin (3,2) covers y 2..3 and includes (5,3), which is taken: red.
	F.Controller->GetGhost(F.A.Get(), FIntPoint(5, 1), Origin, bValid);
	TestTrue(TEXT("just clear of the other item: green"), bValid);
	F.Controller->GetGhost(F.A.Get(), FIntPoint(5, 2), Origin, bValid);
	TestFalse(TEXT("overlapping the other item: red"), bValid);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldRotateGhostTest, "Flipper.HeldItem.RotateKeepsCellUnderCursor", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldRotateGhostTest::RunTest(const FString& Parameters)
{
	FHeldFixture F;
	F.PlaceItem(F.A.Get(), 3, 2, FIntPoint(0, 0));

	// Hold by the top-right cell and hover (4,2): the ghost origin is (2,2).
	F.Controller->PointerDown(F.A.Get(), FIntPoint(2, 0), FVector2D(10.0, 10.0));
	FIntPoint Origin;
	bool	  bValid = false;
	F.Controller->GetGhost(F.A.Get(), FIntPoint(4, 2), Origin, bValid);
	TestTrue(TEXT("before rotating"), Origin == FIntPoint(2, 2));

	// After rotating, the footprint is 2x3 and the grab offset (1,2), so with the cursor still on (4,2) the origin is (3,0).
	// That 2x3 covers x 3..4, y 0..2, which still includes (4,2): the cell you grabbed stays under the cursor.
	F.Controller->Rotate();
	F.Controller->GetGhost(F.A.Get(), FIntPoint(4, 2), Origin, bValid);
	TestTrue(TEXT("after rotating"), Origin == FIntPoint(3, 0));
	TestTrue(TEXT("rotated ghost is valid"), bValid);

	// Dropping now places the item rotated.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(4, 2), FVector2D(10.0, 10.0));
	TestFalse(TEXT("dropped"), F.Controller->IsHolding());
	TestEqual(TEXT("top of the rotated item"), F.IdAt(F.A.Get(), FIntPoint(3, 0)), 1);
	TestEqual(TEXT("bottom-right of the rotated item"), F.IdAt(F.A.Get(), FIntPoint(4, 2)), 1);
	const FFlipperPlacedItem* Placed = F.A->FindItemAt(FIntPoint(4, 2));
	TestTrue(TEXT("placed rotated"), Placed && Placed->Placement.bRotated);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldCancelTest, "Flipper.HeldItem.Cancel", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldCancelTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.A.Get(), 2, 1, FIntPoint(3, 2));

	// Cancel with nothing held does nothing.
	F.Controller->Cancel();
	TestEqual(TEXT("cancel with nothing held changes nothing"), F.IdAt(F.A.Get(), FIntPoint(3, 2)), Item.Id);

	// Pick up, rotate, cancel: back in its original, unrotated placement.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(3, 2), FVector2D(10.0, 10.0));
	TestTrue(TEXT("picked up before cancelling"), F.Controller->IsHolding());
	F.Controller->Rotate();
	TestTrue(TEXT("rotated before cancelling"), F.Controller->IsHeldRotated());
	F.Controller->Cancel();
	TestFalse(TEXT("cancelled: nothing held"), F.Controller->IsHolding());
	TestEqual(TEXT("back at its left cell"), F.IdAt(F.A.Get(), FIntPoint(3, 2)), Item.Id);
	TestEqual(TEXT("back at its right cell (unrotated)"), F.IdAt(F.A.Get(), FIntPoint(4, 2)), Item.Id);
	TestEqual(TEXT("not left standing on its rotated cell"), F.IdAt(F.A.Get(), FIntPoint(3, 3)), (int32)INDEX_NONE);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldRotatedPlacedItemTest, "Flipper.HeldItem.PickUpRotatedItem", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldRotatedPlacedItemTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.A.Get(), 3, 1, FIntPoint(1, 0), /*bRotated=*/true);

	// The 3x1 sits turned: a 1x3 column over (1,0), (1,1), (1,2). Grab its bottom cell.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(1, 2), FVector2D(10.0, 10.0));
	TestTrue(TEXT("holding"), F.Controller->IsHolding());
	TestTrue(TEXT("still rotated"), F.Controller->IsHeldRotated());
	TestTrue(TEXT("origin placement was rotated"), F.Controller->GetOriginPlacement().bRotated);
	TestTrue(TEXT("footprint is the rotated 1x3"), F.Controller->GetHeldFootprint() == FIntPoint(1, 3));
	TestTrue(TEXT("grab offset is in the rotated footprint"), F.Controller->GetGrabOffset() == FIntPoint(0, 2));

	// Snap back restores the rotated placement.
	F.Controller->Cancel();
	TestEqual(TEXT("top of the column"), F.IdAt(F.A.Get(), FIntPoint(1, 0)), Item.Id);
	TestEqual(TEXT("middle of the column"), F.IdAt(F.A.Get(), FIntPoint(1, 1)), Item.Id);
	TestEqual(TEXT("bottom of the column"), F.IdAt(F.A.Get(), FIntPoint(1, 2)), Item.Id);
	TestEqual(TEXT("not left lying on its side"), F.IdAt(F.A.Get(), FIntPoint(2, 0)), (int32)INDEX_NONE);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldCrossGridTest, "Flipper.HeldItem.CrossGridMove", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldCrossGridTest::RunTest(const FString& Parameters)
{
	FHeldFixture			   F;
	const FFlipperItemInstance Item = F.PlaceItem(F.B.Get(), 1, 1, FIntPoint(0, 0));

	// With no approver bound, a move from grid B to grid A is allowed.
	F.Drag(F.B.Get(), FIntPoint(0, 0), F.A.Get(), FIntPoint(2, 2));
	TestEqual(TEXT("moved into A"), F.IdAt(F.A.Get(), FIntPoint(2, 2)), Item.Id);
	TestEqual(TEXT("gone from B"), F.IdAt(F.B.Get(), FIntPoint(0, 0)), (int32)INDEX_NONE);

	// An approver that refuses: the item goes back to B (not A), and the approver was asked about exactly that move.
	int32				   Calls = 0;
	const UInventoryGrid*  AskedFrom = nullptr;
	const UInventoryGrid*  AskedTo = nullptr;
	int32				   AskedId = INDEX_NONE;
	F.Controller->TransferApprover = [&](const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance& Moved)
	{
		++Calls;
		AskedFrom = From;
		AskedTo = To;
		AskedId = Moved.Id;
		return false;
	};

	F.Drag(F.A.Get(), FIntPoint(2, 2), F.B.Get(), FIntPoint(4, 1));
	TestEqual(TEXT("approver was asked once"), Calls, 1);
	TestTrue(TEXT("approver saw the origin grid"), AskedFrom == F.A.Get());
	TestTrue(TEXT("approver saw the destination grid"), AskedTo == F.B.Get());
	TestEqual(TEXT("approver saw the item"), AskedId, Item.Id);
	TestEqual(TEXT("refused: item stays in A"), F.IdAt(F.A.Get(), FIntPoint(2, 2)), Item.Id);
	TestEqual(TEXT("refused: item not in B"), F.IdAt(F.B.Get(), FIntPoint(4, 1)), (int32)INDEX_NONE);

	// A move inside one grid is never put to the approver, even one that refuses everything.
	F.Drag(F.A.Get(), FIntPoint(2, 2), F.A.Get(), FIntPoint(0, 3));
	TestEqual(TEXT("same-grid move did not consult the approver"), Calls, 1);
	TestEqual(TEXT("same-grid move succeeded"), F.IdAt(F.A.Get(), FIntPoint(0, 3)), Item.Id);

	// The ghost shows red when the approver would refuse the cross-grid drop.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(0, 3), FVector2D(10.0, 10.0));
	FIntPoint Origin;
	bool	  bValid = true;
	F.Controller->GetGhost(F.B.Get(), FIntPoint(1, 1), Origin, bValid);
	TestFalse(TEXT("approver refuses: ghost is red over a free cell in the other grid"), bValid);
	F.Controller->GetGhost(F.A.Get(), FIntPoint(1, 1), Origin, bValid);
	TestTrue(TEXT("same grid: ghost is green"), bValid);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldQuickTransferTest, "Flipper.HeldItem.QuickTransfer", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldQuickTransferTest::RunTest(const FString& Parameters)
{
	FHeldFixture F;
	F.PlaceItem(F.A.Get(), 1, 1, FIntPoint(0, 0));
	const FFlipperItemInstance Item = F.PlaceItem(F.B.Get(), 2, 1, FIntPoint(3, 3));

	// A has one item at (0,0), so the first spot a 2x1 fits is (1,0).
	TestTrue(TEXT("quick transfer succeeds"), F.Controller->QuickTransfer(F.B.Get(), FIntPoint(4, 3), F.A.Get()));
	TestEqual(TEXT("landed at the first fit (left cell)"), F.IdAt(F.A.Get(), FIntPoint(1, 0)), Item.Id);
	TestEqual(TEXT("landed at the first fit (right cell)"), F.IdAt(F.A.Get(), FIntPoint(2, 0)), Item.Id);
	TestEqual(TEXT("gone from the source"), F.IdAt(F.B.Get(), FIntPoint(3, 3)), (int32)INDEX_NONE);
	TestFalse(TEXT("quick transfer never leaves anything held"), F.Controller->IsHolding());

	// No item at the cell: false.
	TestFalse(TEXT("empty cell"), F.Controller->QuickTransfer(F.B.Get(), FIntPoint(0, 0), F.A.Get()));

	// An approver that refuses: false, and the item stays where it was.
	F.Controller->TransferApprover = [](const UInventoryGrid*, const UInventoryGrid*, const FFlipperItemInstance&) { return false; };
	TestFalse(TEXT("approver refuses"), F.Controller->QuickTransfer(F.A.Get(), FIntPoint(1, 0), F.B.Get()));
	TestEqual(TEXT("refused: item stays put"), F.IdAt(F.A.Get(), FIntPoint(1, 0)), Item.Id);
	F.Controller->TransferApprover = nullptr;

	// While an item is held, quick transfer is refused (B is empty, so the 2x1 would fit there).
	F.Controller->PointerDown(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	TestTrue(TEXT("holding the 1x1"), F.Controller->IsHolding());
	TestFalse(TEXT("refused while holding"), F.Controller->QuickTransfer(F.A.Get(), FIntPoint(1, 0), F.B.Get()));
	TestEqual(TEXT("refused while holding: item stays put"), F.IdAt(F.A.Get(), FIntPoint(1, 0)), Item.Id);
	F.Controller->Cancel();

	// Nothing fits: fill B completely with one 6x4 item, then try to send the 2x1 there.
	F.PlaceItem(F.B.Get(), 6, 4, FIntPoint(0, 0));
	TestFalse(TEXT("destination full"), F.Controller->QuickTransfer(F.A.Get(), FIntPoint(1, 0), F.B.Get()));
	TestEqual(TEXT("full: item stays put"), F.IdAt(F.A.Get(), FIntPoint(1, 0)), Item.Id);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperHeldOnChangedTest, "Flipper.HeldItem.OnChanged", FLIPPER_HELD_TEST_FLAGS)
bool FFlipperHeldOnChangedTest::RunTest(const FString& Parameters)
{
	FHeldFixture F;
	F.PlaceItem(F.A.Get(), 2, 1, FIntPoint(0, 0));

	int32 Count = 0;
	F.Controller->OnChanged.AddLambda([&Count]() { ++Count; });

	// Pressing an empty cell changes nothing, so it fires nothing.
	F.Controller->PointerDown(F.A.Get(), FIntPoint(5, 3), FVector2D(10.0, 10.0));
	TestEqual(TEXT("empty press: no broadcast"), Count, 0);

	int32 Before = Count;
	F.Controller->PointerDown(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	TestTrue(TEXT("pick-up broadcasts"), Count > Before);

	Before = Count;
	F.Controller->Rotate();
	TestTrue(TEXT("rotate broadcasts"), Count > Before);

	Before = Count;
	F.Controller->Cancel();
	TestTrue(TEXT("cancel broadcasts"), Count > Before);

	F.Controller->PointerDown(F.A.Get(), FIntPoint(0, 0), FVector2D(10.0, 10.0));
	Before = Count;
	F.Controller->PointerMove(FVector2D(60.0, 10.0));
	F.Controller->PointerUp(F.A.Get(), FIntPoint(2, 2), FVector2D(60.0, 10.0));
	TestTrue(TEXT("drop broadcasts"), Count > Before);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
