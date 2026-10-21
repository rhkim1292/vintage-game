// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperTypes.h"
#include "InventoryGrid.h"
#include "FlipperHeldItemController.generated.h"

/** Broadcast whenever the held item changes in a way the held-item visual or a ghost must redraw for. */
DECLARE_MULTICAST_DELEGATE(FOnHeldItemChanged);

/**
 *  Decides whether an item may move from one grid to another (buy / refund / credit-limit rules live in
 *  whatever implements this later). Consulted only for moves between two different grids.
 *  Left unbound, every move is allowed.
 */
using FFlipperTransferApprover = TFunction<bool(const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance& Item)>;

/** Told that a move between two grids is about to happen (see UFlipperHeldItemController::TransferCommitted). */
using FFlipperTransferCommitted = TFunction<void(const UInventoryGrid* From, const UInventoryGrid* To, FFlipperItemInstance& Item)>;

/**
 *  The one held-item state machine behind both input styles (press-drag-release and click-to-pick-up /
 *  click-to-place). Shared by the trunk and seller-table widgets. Pure logic: no widgets, no mouse API.
 *  The widgets turn mouse events into the calls below (cell under the cursor + screen position).
 *  See Docs/FlipperSpec.md, Feature 1, locked decisions 3 and 4, and the routine choices on rotation,
 *  invalid drops and Shift-click.
 *
 *  While an item is held it is NOT in any grid: picking up removes it from its origin grid, and every way
 *  of ending the hold either places it somewhere or puts it back where it came from (snap-back).
 */
UCLASS(BlueprintType)
class UFlipperHeldItemController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 *  How far (in screen pixels) the mouse must move after a press before releasing counts as a drag-drop.
	 *  Below this, releasing keeps the item held (click style). Resets all held state.
	 */
	void Init(float InDragThresholdPixels);

	bool IsHolding() const { return bHolding; }

	/** Valid only while IsHolding(). */
	const FFlipperItemInstance& GetHeldItem() const { return HeldItem; }

	/** The grid the held item was picked up from (where a snap-back returns it). */
	UInventoryGrid* GetOriginGrid() const { return OriginGrid; }

	/** Where the held item sat in OriginGrid when picked up. */
	const FFlipperPlacement& GetOriginPlacement() const { return OriginPlacement; }

	bool IsHeldRotated() const { return bHeldRotated; }

	/** Footprint of the held item in its current rotation, or (0,0) when nothing is held. */
	FIntPoint GetHeldFootprint() const;

	/**
	 *  The origin the held item takes with the pointer at CellPos, which is the pointer in fractional cell units of the
	 *  grid under it (the local pointer divided by the cell size, so (2.5, 1.5) is the middle of cell (2,1)). The held
	 *  item is drawn centred on the pointer, so this is the footprint's top-left corner (CellPos minus half the
	 *  footprint) rounded to the nearest cell. The ghost and the drop both use it, so they always agree.
	 */
	FIntPoint GetDropOrigin(FVector2D CellPos) const;

	/** Unbound = every cross-grid move is allowed. */
	FFlipperTransferApprover TransferApprover;

	/**
	 *  Called once, with the item that is about to be placed, when a move between two different grids is going to
	 *  happen (a drop or a Shift-click that the approver allowed). The item may be changed here (a purchase stamps
	 *  what was paid on it). Unbound = nothing happens. Not called for a move inside one grid, or a snap-back.
	 */
	FFlipperTransferCommitted TransferCommitted;

	/**
	 *  Asked, for a move between two different grids that the approver allowed, whether the item is used up by the move
	 *  (a sale): then TransferCommitted still runs, but the item is removed from where it was and never placed in the
	 *  target grid, which it does not even need room in for a Shift-click. Unbound = items are always placed.
	 */
	FFlipperTransferApprover TransferConsumes;

	/**
	 *  Asked first when the pointer goes down on an item with nothing held. If it returns true the click was used up
	 *  (the Garage test) and the item is not picked up. Unbound = every click on an item picks it up.
	 */
	TFunction<bool(UInventoryGrid* Grid, FIntPoint Cell)> PickupIntercept;

	/** Fired after a pick-up, rotation, drop, snap-back or cancel. */
	FOnHeldItemChanged OnChanged;

	/**
	 *  Mouse button pressed with the pointer at CellPos (fractional cell units, see GetDropOrigin) of Grid (Grid may be
	 *  null for "not over any grid").
	 *  - Nothing held: if an item covers the cell under CellPos, pick it up (remove it from Grid, remember the origin
	 *    grid, its placement and its rotation) and remember ScreenPos as the press position. Pressing an empty cell
	 *    does nothing.
	 *  - Something held (click style, second click): try to drop it at GetDropOrigin(CellPos) on Grid.
	 *    A drop that is invalid or over no grid snaps the item back to its origin.
	 */
	void PointerDown(UInventoryGrid* Grid, FVector2D CellPos, FVector2D ScreenPos);

	/** Mouse moved to ScreenPos. Once it has been further than the drag threshold from the press, this press is a drag. */
	void PointerMove(FVector2D ScreenPos);

	/**
	 *  Mouse button released with the pointer at CellPos (fractional cell units) of Grid (Grid may be null).
	 *  - Holding and the press turned into a drag: try to drop at GetDropOrigin(CellPos) on Grid (snap back if invalid or null).
	 *  - Holding but never dragged: keep holding (click style; the next PointerDown places it).
	 *  - Not holding: nothing happens.
	 */
	void PointerUp(UInventoryGrid* Grid, FVector2D CellPos, FVector2D ScreenPos);

	/**
	 *  Toggle rotation of the held item (R or right-click). The item stays centred on the cursor, because the origin is
	 *  worked out from the new footprint every time. Nothing held: does nothing.
	 */
	void Rotate();

	/** Escape, or the phase timer ran out: snap the held item back to its origin grid and placement. Nothing held: does nothing. */
	void Cancel();

	/**
	 *  The ghost for the current hover: the origin the held item would take with the pointer at CellPos (fractional cell
	 *  units, GetDropOrigin), and whether dropping there is valid (it fits in Grid and the TransferApprover, if needed,
	 *  allows it). Returns false when nothing is held or Grid is null; the outputs are then unspecified.
	 */
	bool GetGhost(const UInventoryGrid* Grid, FVector2D CellPos, FIntPoint& OutOrigin, bool& bOutValid) const;

	/**
	 *  Shift-click: move the item covering Cell in From to the first fit in To (UInventoryGrid::FindFirstFit),
	 *  following the TransferApprover. Returns false and changes nothing if an item is currently held, there is no
	 *  item at Cell, nothing fits in To, or the approver refuses. The caller shakes the item on false.
	 */
	bool QuickTransfer(UInventoryGrid* From, FIntPoint Cell, UInventoryGrid* To);

private:
	// TODO: whatever private helpers you want (for example a TryDrop and a SnapBack that PointerDown, PointerUp and
	// Cancel all share, so there is exactly one drop path and one snap-back path).

	bool				 bHolding = false;
	FFlipperItemInstance HeldItem;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> OriginGrid;

	FFlipperPlacement OriginPlacement;
	bool			  bHeldRotated = false;

	float	  DragThresholdPixels = 5.f;
	FVector2D PressScreenPos = FVector2D::ZeroVector;
	bool	  bDragged = false;

	void SnapBackHeldItem();
	void DropHeldItem(UInventoryGrid* Grid, FVector2D CellPos);
};
