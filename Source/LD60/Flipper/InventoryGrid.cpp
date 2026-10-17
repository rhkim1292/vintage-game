// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryGrid.h"

FIntPoint UInventoryGrid::GetFootprint(const UFlipperItemDefinition* Definition, bool bRotated)
{
	// If the item definition is nullptr, return a (0,0) footprint, otherwise return dimensions
	// of item from definition. If bRotated is true, width and height are swapped.
	if (!Definition)
	{
		return FIntPoint::ZeroValue;
	}

	return bRotated ? FIntPoint(Definition->Height, Definition->Width) : FIntPoint(Definition->Width, Definition->Height);
}

void UInventoryGrid::Init(int32 InWidth, int32 InHeight)
{
	// Store the size and empty the grid.
	GridWidth = InWidth;
	GridHeight = InHeight;
	ItemGrid.Init(INDEX_NONE, GridWidth*GridHeight);
}

bool UInventoryGrid::CanPlace(const FFlipperItemInstance& Item, FIntPoint Origin, bool bRotated) const
{
	// You can't place an item with a nullptr Definition. Return false.
	if (!Item.Definition)
	{
		return false;
	}

	TArray<int32> CoveredCellsInItemGrid = GetCoveredCellsByDefinedItem(Item.Definition, Origin, bRotated);

	if (CoveredCellsInItemGrid.IsEmpty())
	{
		return false;
	}

	for (int32 i = 0; i < CoveredCellsInItemGrid.Num(); i++)
	{
		if (ItemGrid[CoveredCellsInItemGrid[i]] != INDEX_NONE)
		{
			return false;
		}
	}

	// Return true if none of the guards above return false. This means the item can be placed.
	return true;
}

bool UInventoryGrid::TryPlace(const FFlipperItemInstance& Item, FIntPoint Origin, bool bRotated)
{
	if (!CanPlace(Item, Origin, bRotated))
	{
		return false;
	}

	if (PlacedItems.ContainsByPredicate([&](const FFlipperPlacedItem& PlacedItem) {
		return PlacedItem.Item.Id == Item.Id;
	}))
	{
		return false;
	}

	FFlipperPlacement ItemPlacement = {
		Origin,
		bRotated
	};

	FFlipperPlacedItem PlacedItem = {
		Item,
		ItemPlacement
	};

	PlacedItems.Add(PlacedItem);

	TArray<int32> CoveredCellsInItemGrid = GetCoveredCellsByDefinedItem(Item.Definition, Origin, bRotated);

	for (int32 i = 0; i < CoveredCellsInItemGrid.Num(); i++)
	{
		ItemGrid[CoveredCellsInItemGrid[i]] = Item.Id;
	}

	OnChanged.Broadcast();

	return true;
}

bool UInventoryGrid::Remove(const FFlipperItemInstance& Item)
{
	int32 IndexOfItem = PlacedItems.IndexOfByPredicate([&](const FFlipperPlacedItem& PlacedItem) {
		return PlacedItem.Item.Id == Item.Id;});
	if (IndexOfItem == INDEX_NONE)
	{
		return false;
	}

	FIntPoint PlacedItemOrigin = PlacedItems[IndexOfItem].Placement.Origin;
	bool bPlacedItemRotated = PlacedItems[IndexOfItem].Placement.bRotated;

	TArray<int32> CoveredCellsInItemGrid = GetCoveredCellsByDefinedItem(Item.Definition, PlacedItemOrigin, bPlacedItemRotated);

	for (int32 i = 0; i < CoveredCellsInItemGrid.Num(); i++)
	{
		ItemGrid[CoveredCellsInItemGrid[i]] = INDEX_NONE;
	}

	PlacedItems.RemoveAt(IndexOfItem, 1);

	OnChanged.Broadcast();

	return true;
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

int32 UInventoryGrid::ConvertPointToItemGridCell(int32 X, int32 Y) const
{
	if (X >= GridWidth || Y >= GridHeight || X < 0 || Y < 0)
	{
		return -2;
	}

	return X + (GridWidth * Y);
}

TArray<int32> UInventoryGrid::GetCoveredCellsByDefinedItem(UFlipperItemDefinition *Definition, FIntPoint Origin, bool bRotated) const
{
	// This is the result array that will be returned by this function
	TArray<int32> result;

	// X and Y endpoints for the item will change based on whether bRotated was set or not. The change
	// is a swap of Height and Width in the endpoint calculation.
	FIntPoint ItemFootprint = GetFootprint(Definition, bRotated);
	int32 ItemXEndPoint = Origin.X + ItemFootprint.X - 1;
	int32 ItemYEndPoint = Origin.Y + ItemFootprint.Y - 1;

	// Guard to check if the item is outside bounds of grid, if so return empty array.
	if (Origin.X < 0 || Origin.Y < 0 || ItemXEndPoint >= GridWidth || ItemYEndPoint >= GridHeight)
	{
		return {};
	}

    for (int32 i = Origin.X; i <= ItemXEndPoint; i++)
	{
		for (int32 j = Origin.Y; j <= ItemYEndPoint; j++)
		{
			result.Add(ConvertPointToItemGridCell(i, j));
		}
	}

	return result;
}
