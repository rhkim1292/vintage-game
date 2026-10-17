// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryGrid.h"

// All bodies below are yours to write (Phase 1.2). Run the Flipper.InventoryGrid.* automation tests
// to see which parts of the spec are still unmet. The stubs return "nothing works" values on purpose.

FIntPoint UInventoryGrid::GetFootprint(const UFlipperItemDefinition* Definition, bool bRotated)
{
	// TODO: (Width, Height), swapped when bRotated. Null Definition -> (0, 0).
	return FIntPoint::ZeroValue;
}

void UInventoryGrid::Init(int32 InWidth, int32 InHeight)
{
	// TODO: store the size and empty the grid.
}

bool UInventoryGrid::CanPlace(const FFlipperItemInstance& Item, FIntPoint Origin, bool bRotated) const
{
	// TODO: inside the grid on all four sides, and no overlap with a placed item.
	return false;
}

bool UInventoryGrid::TryPlace(const FFlipperItemInstance& Item, FIntPoint Origin, bool bRotated)
{
	// TODO: refuse a duplicate Id or a spot CanPlace rejects; otherwise add it and broadcast OnChanged.
	return false;
}

bool UInventoryGrid::Remove(const FFlipperItemInstance& Item)
{
	// TODO: remove the item with this Id; broadcast OnChanged only if something was removed.
	return false;
}

const FFlipperPlacedItem* UInventoryGrid::FindItemAt(FIntPoint Cell) const
{
	// TODO: the placed item whose footprint covers Cell, or nullptr.
	return nullptr;
}

bool UInventoryGrid::FindFirstFit(const FFlipperItemInstance& Item, FIntPoint& OutOrigin, bool& bOutRotated) const
{
	// TODO: row by row from the top-left; at each cell try unrotated, then rotated.
	return false;
}
