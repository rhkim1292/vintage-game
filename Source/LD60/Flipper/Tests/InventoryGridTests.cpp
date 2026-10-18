// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for UInventoryGrid (Docs/FlipperSpec.md, Feature 1).
// Run them with:  Automation RunTests Flipper.InventoryGrid

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "InventoryGrid.h"
	#include "FlipperItemDefinition.h"

namespace FlipperGridTests
{
	/** A grid plus a factory for items of any size. Keeps the UObjects alive for the length of one test. */
	struct FGridFixture
	{
		TStrongObjectPtr<UInventoryGrid>				 Grid;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>> Definitions;
		int32											 NextItemId = 1;

		FGridFixture(int32 Width, int32 Height)
		{
			Grid.Reset(NewObject<UInventoryGrid>());
			Grid->Init(Width, Height);
		}

		/** A new item with a unique Id and a Width x Height footprint. */
		FFlipperItemInstance MakeItem(int32 Width, int32 Height)
		{
			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = Width;
			Definition->Height = Height;
			Definitions.Emplace(Definition);

			FFlipperItemInstance Item;
			Item.Id = NextItemId++;
			Item.Definition = Definition;
			return Item;
		}

		/** Id of the item covering Cell, or INDEX_NONE for an empty or out-of-bounds cell. */
		int32 IdAt(FIntPoint Cell) const
		{
			const FFlipperPlacedItem* Placed = Grid->FindItemAt(Cell);
			return Placed ? Placed->Item.Id : INDEX_NONE;
		}
	};
} // namespace FlipperGridTests

	#define FLIPPER_GRID_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperGridTests;

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridFootprintTest, "Flipper.InventoryGrid.Footprint", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridFootprintTest::RunTest(const FString& Parameters)
{
	FGridFixture			   F(4, 4);
	const FFlipperItemInstance Wide = F.MakeItem(3, 1);
	const FFlipperItemInstance Square = F.MakeItem(2, 2);

	TestTrue(TEXT("3x1 unrotated is (3,1)"), UInventoryGrid::GetFootprint(Wide.Definition, false) == FIntPoint(3, 1));
	TestTrue(TEXT("3x1 rotated is (1,3)"), UInventoryGrid::GetFootprint(Wide.Definition, true) == FIntPoint(1, 3));
	TestTrue(TEXT("2x2 unrotated is (2,2)"), UInventoryGrid::GetFootprint(Square.Definition, false) == FIntPoint(2, 2));
	TestTrue(TEXT("2x2 rotated is (2,2)"), UInventoryGrid::GetFootprint(Square.Definition, true) == FIntPoint(2, 2));
	TestTrue(TEXT("null definition is (0,0)"), UInventoryGrid::GetFootprint(nullptr, false) == FIntPoint(0, 0));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridBoundsTest, "Flipper.InventoryGrid.Bounds", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridBoundsTest::RunTest(const FString& Parameters)
{
	FGridFixture			   F(5, 4);
	const FFlipperItemInstance Item = F.MakeItem(3, 2);

	// Unrotated 3x2 in a 5x4 grid: origin X may be 0..2 and origin Y may be 0..2.
	TestTrue(TEXT("top-left corner fits"), F.Grid->CanPlace(Item, FIntPoint(0, 0), false));
	TestTrue(TEXT("bottom-right corner fits exactly"), F.Grid->CanPlace(Item, FIntPoint(2, 2), false));
	TestFalse(TEXT("one column too far right"), F.Grid->CanPlace(Item, FIntPoint(3, 0), false));
	TestFalse(TEXT("one row too far down"), F.Grid->CanPlace(Item, FIntPoint(0, 3), false));
	TestFalse(TEXT("negative X"), F.Grid->CanPlace(Item, FIntPoint(-1, 0), false));
	TestFalse(TEXT("negative Y"), F.Grid->CanPlace(Item, FIntPoint(0, -1), false));
	TestFalse(TEXT("origin entirely outside"), F.Grid->CanPlace(Item, FIntPoint(5, 0), false));

	// Rotated, the same item is 2 wide and 3 tall: origin X may be 0..3 and origin Y may be 0..1.
	TestTrue(TEXT("rotated fits at the bottom-right"), F.Grid->CanPlace(Item, FIntPoint(3, 1), true));
	TestFalse(TEXT("rotated sticks out the bottom"), F.Grid->CanPlace(Item, FIntPoint(3, 2), true));
	TestFalse(TEXT("rotated sticks out the right"), F.Grid->CanPlace(Item, FIntPoint(4, 0), true));

	// An item bigger than the grid fits nowhere, in either orientation.
	const FFlipperItemInstance Huge = F.MakeItem(6, 1);
	TestFalse(TEXT("6x1 is wider than the grid"), F.Grid->CanPlace(Huge, FIntPoint(0, 0), false));
	TestFalse(TEXT("6x1 rotated is taller than the grid"), F.Grid->CanPlace(Huge, FIntPoint(0, 0), true));

	TestEqual(TEXT("CanPlace never changes the grid"), F.Grid->GetPlacedItems().Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridOverlapTest, "Flipper.InventoryGrid.Overlap", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridOverlapTest::RunTest(const FString& Parameters)
{
	FGridFixture			   F(5, 4);
	const FFlipperItemInstance A = F.MakeItem(2, 2);
	const FFlipperItemInstance B = F.MakeItem(2, 2);

	TestTrue(TEXT("A placed at (1,1)"), F.Grid->TryPlace(A, FIntPoint(1, 1), false));

	// A covers (1,1) (2,1) (1,2) (2,2).
	TestFalse(TEXT("same spot overlaps"), F.Grid->CanPlace(B, FIntPoint(1, 1), false));
	TestFalse(TEXT("sharing one corner cell overlaps"), F.Grid->CanPlace(B, FIntPoint(2, 2), false));
	TestFalse(TEXT("overlapping from the top-left overlaps"), F.Grid->CanPlace(B, FIntPoint(0, 0), false));
	TestTrue(TEXT("touching on the right edge is fine"), F.Grid->CanPlace(B, FIntPoint(3, 1), false));
	TestTrue(TEXT("touching on the right edge, one row lower, is fine"), F.Grid->CanPlace(B, FIntPoint(3, 2), false));

	// A refused TryPlace must leave everything as it was.
	TestFalse(TEXT("TryPlace refuses an overlap"), F.Grid->TryPlace(B, FIntPoint(2, 2), false));
	TestEqual(TEXT("still exactly one item"), F.Grid->GetPlacedItems().Num(), 1);
	TestEqual(TEXT("A still owns (2,2)"), F.IdAt(FIntPoint(2, 2)), A.Id);
	TestEqual(TEXT("(3,3) stayed empty"), F.IdAt(FIntPoint(3, 3)), static_cast<int32>(INDEX_NONE));

	// The same item cannot be placed twice, even on free cells.
	TestFalse(TEXT("TryPlace refuses an Id that is already in the grid"), F.Grid->TryPlace(A, FIntPoint(3, 2), false));
	TestEqual(TEXT("still exactly one item after the duplicate"), F.Grid->GetPlacedItems().Num(), 1);
	TestEqual(TEXT("A did not move"), F.IdAt(FIntPoint(1, 1)), A.Id);

	TestTrue(TEXT("B fits beside A"), F.Grid->TryPlace(B, FIntPoint(3, 2), false));
	TestEqual(TEXT("two items now"), F.Grid->GetPlacedItems().Num(), 2);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridRotatedFootprintTest, "Flipper.InventoryGrid.RotatedFootprint", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridRotatedFootprintTest::RunTest(const FString& Parameters)
{
	FGridFixture			   F(5, 5);
	const FFlipperItemInstance Bar = F.MakeItem(3, 1);
	const FFlipperItemInstance Dot = F.MakeItem(1, 1);

	// Rotated, the 3x1 bar covers (0,0) (0,1) (0,2): a column, not a row.
	TestTrue(TEXT("bar placed rotated at (0,0)"), F.Grid->TryPlace(Bar, FIntPoint(0, 0), true));
	TestEqual(TEXT("(0,2) belongs to the bar"), F.IdAt(FIntPoint(0, 2)), Bar.Id);
	TestEqual(TEXT("(1,0) is empty"), F.IdAt(FIntPoint(1, 0)), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("(2,0) is empty"), F.IdAt(FIntPoint(2, 0)), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("(0,3) is empty"), F.IdAt(FIntPoint(0, 3)), static_cast<int32>(INDEX_NONE));

	TestTrue(TEXT("a dot fits where the unrotated bar would have been"), F.Grid->CanPlace(Dot, FIntPoint(1, 0), false));
	TestFalse(TEXT("a dot cannot go on the bar's third cell"), F.Grid->CanPlace(Dot, FIntPoint(0, 2), false));
	TestTrue(TEXT("a dot fits just below the bar"), F.Grid->CanPlace(Dot, FIntPoint(0, 3), false));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridRemoveAndReplaceTest, "Flipper.InventoryGrid.RemoveAndReplace", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridRemoveAndReplaceTest::RunTest(const FString& Parameters)
{
	FGridFixture			   F(4, 4);
	const FFlipperItemInstance A = F.MakeItem(2, 2);
	const FFlipperItemInstance Stranger = F.MakeItem(1, 1);

	TestTrue(TEXT("A placed"), F.Grid->TryPlace(A, FIntPoint(0, 0), false));
	TestFalse(TEXT("removing an item that was never placed fails"), F.Grid->Remove(Stranger));
	TestEqual(TEXT("a failed Remove changes nothing"), F.Grid->GetPlacedItems().Num(), 1);

	TestTrue(TEXT("removing A succeeds"), F.Grid->Remove(A));
	TestEqual(TEXT("grid is empty again"), F.Grid->GetPlacedItems().Num(), 0);
	TestEqual(TEXT("its cells are free"), F.IdAt(FIntPoint(1, 1)), static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("removing A twice fails"), F.Grid->Remove(A));

	// Freed cells are usable straight away, by anything.
	const FFlipperItemInstance Big = F.MakeItem(4, 4);
	TestTrue(TEXT("a full-grid item fits now"), F.Grid->CanPlace(Big, FIntPoint(0, 0), false));

	// The same item can be placed again, somewhere else and turned.
	TestTrue(TEXT("A re-placed rotated elsewhere"), F.Grid->TryPlace(A, FIntPoint(2, 2), true));
	TestEqual(TEXT("A owns its new cell"), F.IdAt(FIntPoint(3, 3)), A.Id);
	TestEqual(TEXT("A left its old cell"), F.IdAt(FIntPoint(0, 0)), static_cast<int32>(INDEX_NONE));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridFindItemAtTest, "Flipper.InventoryGrid.FindItemAt", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridFindItemAtTest::RunTest(const FString& Parameters)
{
	FGridFixture			   F(6, 4);
	const FFlipperItemInstance Block = F.MakeItem(3, 2);
	const FFlipperItemInstance Bar = F.MakeItem(2, 1);

	TestTrue(TEXT("block placed at (1,1)"), F.Grid->TryPlace(Block, FIntPoint(1, 1), false));
	TestTrue(TEXT("bar placed rotated at (5,2)"), F.Grid->TryPlace(Bar, FIntPoint(5, 2), true));

	// Every cell of the 3x2 block, from top-left to bottom-right.
	for (int32 Y = 1; Y <= 2; ++Y)
	{
		for (int32 X = 1; X <= 3; ++X)
		{
			TestEqual(*FString::Printf(TEXT("block owns (%d,%d)"), X, Y), F.IdAt(FIntPoint(X, Y)), Block.Id);
		}
	}

	// The cells just outside it.
	TestEqual(TEXT("left of the block"), F.IdAt(FIntPoint(0, 1)), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("right of the block"), F.IdAt(FIntPoint(4, 1)), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("above the block"), F.IdAt(FIntPoint(1, 0)), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("below the block"), F.IdAt(FIntPoint(1, 3)), static_cast<int32>(INDEX_NONE));

	// The rotated 2x1 bar is 1 wide and 2 tall.
	TestEqual(TEXT("rotated bar owns its top cell"), F.IdAt(FIntPoint(5, 2)), Bar.Id);
	TestEqual(TEXT("rotated bar owns its bottom cell"), F.IdAt(FIntPoint(5, 3)), Bar.Id);
	TestEqual(TEXT("left of the rotated bar is empty"), F.IdAt(FIntPoint(4, 2)), static_cast<int32>(INDEX_NONE));

	// Outside the grid is simply "nothing there", never a crash.
	TestNull(TEXT("negative X"), F.Grid->FindItemAt(FIntPoint(-1, 0)));
	TestNull(TEXT("X past the right edge"), F.Grid->FindItemAt(FIntPoint(6, 0)));
	TestNull(TEXT("Y past the bottom edge"), F.Grid->FindItemAt(FIntPoint(0, 4)));

	// The result also carries the placement.
	const FFlipperPlacedItem* Found = F.Grid->FindItemAt(FIntPoint(5, 3));
	if (TestNotNull(TEXT("bar found by its second cell"), Found))
	{
		TestTrue(TEXT("placement origin is the top-left cell"), Found->Placement.Origin == FIntPoint(5, 2));
		TestTrue(TEXT("placement remembers the rotation"), Found->Placement.bRotated);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridOnChangedTest, "Flipper.InventoryGrid.OnChanged", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridOnChangedTest::RunTest(const FString& Parameters)
{
	FGridFixture			   F(3, 3);
	const FFlipperItemInstance A = F.MakeItem(2, 2);
	const FFlipperItemInstance B = F.MakeItem(2, 2);

	int32 Broadcasts = 0;
	F.Grid->OnChanged.AddLambda([&Broadcasts]() { ++Broadcasts; });

	F.Grid->CanPlace(A, FIntPoint(0, 0), false);
	TestEqual(TEXT("CanPlace does not broadcast"), Broadcasts, 0);

	F.Grid->TryPlace(A, FIntPoint(0, 0), false);
	TestEqual(TEXT("a successful TryPlace broadcasts once"), Broadcasts, 1);

	F.Grid->TryPlace(B, FIntPoint(1, 1), false);
	TestEqual(TEXT("a refused TryPlace does not broadcast"), Broadcasts, 1);

	F.Grid->Remove(B);
	TestEqual(TEXT("a failed Remove does not broadcast"), Broadcasts, 1);

	F.Grid->Remove(A);
	TestEqual(TEXT("a successful Remove broadcasts once"), Broadcasts, 2);

	FIntPoint Origin;
	bool	  bRotated = false;
	F.Grid->FindFirstFit(A, Origin, bRotated);
	TestEqual(TEXT("FindFirstFit does not broadcast"), Broadcasts, 2);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridFirstFitUnrotatedTest, "Flipper.InventoryGrid.FindFirstFit.Unrotated", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridFirstFitUnrotatedTest::RunTest(const FString& Parameters)
{
	FGridFixture F(4, 3);
	FIntPoint	 Origin(-1, -1);
	bool		 bRotated = true;

	// An empty grid: the very first cell, not rotated (unrotated is preferred when both work).
	const FFlipperItemInstance Pair = F.MakeItem(2, 1);
	TestTrue(TEXT("fits in an empty grid"), F.Grid->FindFirstFit(Pair, Origin, bRotated));
	TestTrue(TEXT("empty grid: top-left"), Origin == FIntPoint(0, 0));
	TestFalse(TEXT("empty grid: unrotated"), bRotated);
	TestEqual(TEXT("FindFirstFit does not place anything"), F.Grid->GetPlacedItems().Num(), 0);

	// Row by row, not column by column: with (0,0) and (1,0) taken, a 1x1 goes to (2,0), not (0,1).
	TestTrue(TEXT("first blocker"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(0, 0), false));
	TestTrue(TEXT("second blocker"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(1, 0), false));
	const FFlipperItemInstance Dot = F.MakeItem(1, 1);
	TestTrue(TEXT("a dot still fits"), F.Grid->FindFirstFit(Dot, Origin, bRotated));
	TestTrue(TEXT("scans along the row first"), Origin == FIntPoint(2, 0));
	TestFalse(TEXT("dot is unrotated"), bRotated);

	// A 2x2 needs a free 2x2 block: origins (0,0) and (1,0) touch the blockers, so the first free block is at (2,0).
	const FFlipperItemInstance Square = F.MakeItem(2, 2);
	TestTrue(TEXT("a 2x2 fits"), F.Grid->FindFirstFit(Square, Origin, bRotated));
	TestTrue(TEXT("2x2 lands at the first free 2x2 block"), Origin == FIntPoint(2, 0));
	TestFalse(TEXT("2x2 is unrotated"), bRotated);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridFirstFitRotatedTest, "Flipper.InventoryGrid.FindFirstFit.Rotated", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridFirstFitRotatedTest::RunTest(const FString& Parameters)
{
	FIntPoint Origin(-1, -1);
	bool	  bRotated = false;

	// A 4x1 item in a 3-wide, 4-tall grid only fits standing up.
	{
		FGridFixture			   F(3, 4);
		const FFlipperItemInstance Long = F.MakeItem(4, 1);
		TestTrue(TEXT("fits when turned"), F.Grid->FindFirstFit(Long, Origin, bRotated));
		TestTrue(TEXT("top-left"), Origin == FIntPoint(0, 0));
		TestTrue(TEXT("rotated"), bRotated);

		TestTrue(TEXT("blocker at (0,0)"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(0, 0), false));
		TestTrue(TEXT("still fits turned"), F.Grid->FindFirstFit(Long, Origin, bRotated));
		TestTrue(TEXT("next column"), Origin == FIntPoint(1, 0));
		TestTrue(TEXT("still rotated"), bRotated);
	}

	// Each cell is tried unrotated, then rotated, before moving to the next cell.
	// Grid 3x2 with a dot at (1,0): a 2x1 does not fit at (0,0) lying down, but does standing up.
	{
		FGridFixture F(3, 2);
		TestTrue(TEXT("blocker at (1,0)"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(1, 0), false));
		const FFlipperItemInstance Pair = F.MakeItem(2, 1);
		TestTrue(TEXT("pair fits"), F.Grid->FindFirstFit(Pair, Origin, bRotated));
		TestTrue(TEXT("(0,0) comes before (0,1), even though only rotated works there"), Origin == FIntPoint(0, 0));
		TestTrue(TEXT("rotated at (0,0)"), bRotated);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridFirstFitNoneTest, "Flipper.InventoryGrid.FindFirstFit.None", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridFirstFitNoneTest::RunTest(const FString& Parameters)
{
	FIntPoint Origin;
	bool	  bRotated = false;

	// A completely full grid.
	{
		FGridFixture F(2, 2);
		TestTrue(TEXT("filled with one 2x2"), F.Grid->TryPlace(F.MakeItem(2, 2), FIntPoint(0, 0), false));
		TestFalse(TEXT("a dot has nowhere to go"), F.Grid->FindFirstFit(F.MakeItem(1, 1), Origin, bRotated));
	}

	// Too big for the grid in either orientation.
	{
		FGridFixture F(4, 4);
		TestFalse(TEXT("5x5 never fits"), F.Grid->FindFirstFit(F.MakeItem(5, 5), Origin, bRotated));
		TestFalse(TEXT("1x5 never fits"), F.Grid->FindFirstFit(F.MakeItem(1, 5), Origin, bRotated));
	}

	// Free cells exist but none of them touch: a plus-shaped set of blockers leaves four isolated corners
	// and an isolated centre, so a 2x1 fits nowhere, lying down or standing up.
	{
		FGridFixture F(3, 3);
		TestTrue(TEXT("top blocker"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(1, 0), false));
		TestTrue(TEXT("left blocker"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(0, 1), false));
		TestTrue(TEXT("right blocker"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(2, 1), false));
		TestTrue(TEXT("bottom blocker"), F.Grid->TryPlace(F.MakeItem(1, 1), FIntPoint(1, 2), false));
		TestFalse(TEXT("a 2x1 does not fit between the blockers"), F.Grid->FindFirstFit(F.MakeItem(2, 1), Origin, bRotated));
		TestTrue(TEXT("but a 1x1 still does"), F.Grid->FindFirstFit(F.MakeItem(1, 1), Origin, bRotated));
		TestTrue(TEXT("at the first free cell, the top-left corner"), Origin == FIntPoint(0, 0));
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperGridCellIndexTest, "Flipper.InventoryGrid.CellIndex", FLIPPER_GRID_TEST_FLAGS)
bool FFlipperGridCellIndexTest::RunTest(const FString& Parameters)
{
	// Row-major: a row is Width cells long, so (X, Y) is X + Width * Y.
	{
		FGridFixture F(5, 4);
		TestEqual(TEXT("top-left is 0"), F.Grid->ConvertPointToItemGridCell(0, 0), 0);
		TestEqual(TEXT("end of the first row is Width-1"), F.Grid->ConvertPointToItemGridCell(4, 0), 4);
		TestEqual(TEXT("start of the second row is Width"), F.Grid->ConvertPointToItemGridCell(0, 1), 5);
		TestEqual(TEXT("an inner cell"), F.Grid->ConvertPointToItemGridCell(2, 2), 12);
		TestEqual(TEXT("bottom-right is Width*Height-1"), F.Grid->ConvertPointToItemGridCell(4, 3), 19);
	}

	// Every cell gets its own index, counting up row by row with no gaps and no repeats.
	{
		FGridFixture F(5, 4);
		int32		 Expected = 0;
		bool		 bAllMatch = true;
		for (int32 Y = 0; Y < 4; ++Y)
		{
			for (int32 X = 0; X < 5; ++X)
			{
				bAllMatch &= (F.Grid->ConvertPointToItemGridCell(X, Y) == Expected);
				++Expected;
			}
		}
		TestTrue(TEXT("cells map to 0..19 in reading order"), bAllMatch);
	}

	// A grid that is taller than it is wide would expose a Width/Height mix-up.
	{
		FGridFixture F(3, 7);
		TestEqual(TEXT("tall grid: (2,0)"), F.Grid->ConvertPointToItemGridCell(2, 0), 2);
		TestEqual(TEXT("tall grid: (0,6)"), F.Grid->ConvertPointToItemGridCell(0, 6), 18);
		TestEqual(TEXT("tall grid: bottom-right"), F.Grid->ConvertPointToItemGridCell(2, 6), 20);
	}

	// Cells outside the grid must not give a usable array index, and must not look like "empty cell" either:
	// the grid stores INDEX_NONE in empty cells, so returning INDEX_NONE would be easy to mistake for a cell.
	{
		FGridFixture	F(5, 4);
		const FIntPoint OutsideCells[] = {
			FIntPoint(-1, 0),
			FIntPoint(0, -1),
			FIntPoint(-1, -1),
			FIntPoint(5, 0),
			FIntPoint(0, 4),
			FIntPoint(5, 4),
			FIntPoint(4, 4),
			FIntPoint(5, 3),
		};
		for (const FIntPoint& Cell : OutsideCells)
		{
			const int32 Index = F.Grid->ConvertPointToItemGridCell(Cell.X, Cell.Y);
			TestTrue(*FString::Printf(TEXT("(%d,%d) is not a valid index"), Cell.X, Cell.Y), Index < 0 || Index >= 5 * 4);
			TestNotEqual(*FString::Printf(TEXT("(%d,%d) is not confusable with an empty cell"), Cell.X, Cell.Y), Index, static_cast<int32>(INDEX_NONE));
		}

		// X = Width must be rejected, not wrapped onto the next row (this is the easy one to miss).
		TestTrue(TEXT("(5,0) does not wrap to (0,1)"), F.Grid->ConvertPointToItemGridCell(5, 0) != F.Grid->ConvertPointToItemGridCell(0, 1));
	}

	// Init again with a different size and the mapping follows the new width.
	{
		FGridFixture F(5, 4);
		F.Grid->Init(2, 6);
		TestEqual(TEXT("after re-Init: (1,1) is 3"), F.Grid->ConvertPointToItemGridCell(1, 1), 3);
		TestEqual(TEXT("after re-Init: (1,5) is 11"), F.Grid->ConvertPointToItemGridCell(1, 5), 11);
		TestTrue(TEXT("after re-Init: (2,0) is now outside"), F.Grid->ConvertPointToItemGridCell(2, 0) < 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
