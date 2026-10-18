// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperTypes.h"
#include "InventoryGrid.generated.h"

/** Broadcast after the grid's contents actually change (a successful TryPlace or Remove). */
DECLARE_MULTICAST_DELEGATE(FOnInventoryGridChanged);

/**
 *  A rectangular grid of cells holding rectangular items.
 *  Pure logic: no widgets, no money, no input. It only answers "does this fit here?" and remembers what is where.
 *  Coordinates: Cell (0,0) is the top-left, X grows to the right, Y grows downward.
 *  See Docs/FlipperSpec.md, Feature 1, for the rules behind this class.
 */
UCLASS(BlueprintType)
class UInventoryGrid : public UObject
{
	GENERATED_BODY()

public:
	/**
	 *  Footprint of an item in cells as (Width, Height). Swapped to (Height, Width) when bRotated.
	 *  Returns (0,0) for a null Definition.
	 */
	static FIntPoint GetFootprint(const UFlipperItemDefinition* Definition, bool bRotated);

	/** Sets the grid size and empties it. Safe to call again to reset. */
	void Init(int32 InWidth, int32 InHeight);

	int32 GetWidth() const { return GridWidth; }
	int32 GetHeight() const { return GridHeight; }

	/** Every item currently in the grid, in no guaranteed order. */
	const TArray<FFlipperPlacedItem>& GetPlacedItems() const { return PlacedItems; }

	/**
	 *  True if the whole footprint starting at Origin lies inside the grid and overlaps no item already placed.
	 *  Does not change the grid. Does not look at whether Item itself is already in the grid
	 *  (the held-item controller removes an item before testing where it can go next).
	 */
	bool CanPlace(const FFlipperItemInstance& Item, FIntPoint Origin, bool bRotated) const;

	/**
	 *  Places Item if CanPlace allows it. Returns false and changes nothing when it does not fit,
	 *  or when an item with the same Id is already in the grid. Broadcasts OnChanged on success only.
	 */
	bool TryPlace(const FFlipperItemInstance& Item, FIntPoint Origin, bool bRotated);

	/**
	 *  Removes the item with Item.Id. Returns false if no such item is in the grid.
	 *  Broadcasts OnChanged on success only.
	 */
	bool Remove(const FFlipperItemInstance& Item);

	/**
	 *  The item covering Cell, or nullptr for an empty or out-of-bounds cell.
	 *  The pointer is only valid until the grid next changes.
	 */
	const FFlipperPlacedItem* FindItemAt(FIntPoint Cell) const;

	/**
	 *  Finds where Item fits without placing it. Scans cells row by row from the top-left; at each cell tries
	 *  unrotated first, then rotated. Returns false (outputs unspecified) if it fits nowhere.
	 *  Does not change the grid.
	 */
	bool FindFirstFit(const FFlipperItemInstance& Item, FIntPoint& OutOrigin, bool& bOutRotated) const;

	/** Fired after a successful TryPlace or Remove. The grid widget redraws from this. */
	FOnInventoryGridChanged OnChanged;

private:
#if WITH_DEV_AUTOMATION_TESTS
	// Lets the CellIndex automation test call the private index helper directly.
	friend class FFlipperGridCellIndexTest;
#endif

	int32 GridWidth = 0;
	int32 GridHeight = 0;

	/**
	 *  Every placed item with its placement. This is the source of truth.
	 *  You may add your own derived data next to it (for example a per-cell lookup) if it makes the
	 *  logic simpler, but then keep the two in step inside Init, TryPlace and Remove.
	 */
	UPROPERTY()
	TArray<FFlipperPlacedItem> PlacedItems;

	TArray<int32> ItemGrid;

	/**
	 * Converts a point to an ItemGrid cell index. The index is represented by a single int32 value.
	 */
	int32 ConvertPointToItemGridCell(int32 X, int32 Y) const;

	TArray<int32> GetCoveredCellsByDefinedItem(UFlipperItemDefinition* Definition, FIntPoint Origin, bool bRotated) const;
};
