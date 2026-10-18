// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperHeldItemController.h"
#include "InventoryGrid.h"

FIntPoint UFlipperHeldItemController::GetHeldFootprint() const
{
	// TODO: (0,0) when nothing is held, otherwise the held item's footprint in its current rotation.
	return FIntPoint::ZeroValue;
}

void UFlipperHeldItemController::Init(float InDragThresholdPixels)
{
	// TODO: store the threshold and reset every piece of held state (nothing held, no origin grid, no drag).
}

void UFlipperHeldItemController::PointerDown(UInventoryGrid* Grid, FIntPoint Cell, FVector2D ScreenPos)
{
	// TODO: see the header. Not holding -> pick up whatever covers Cell. Holding -> try to drop at Cell.
}

void UFlipperHeldItemController::PointerMove(FVector2D ScreenPos)
{
	// TODO: once ScreenPos is further than the drag threshold from the press position, this press is a drag.
}

void UFlipperHeldItemController::PointerUp(UInventoryGrid* Grid, FIntPoint Cell, FVector2D ScreenPos)
{
	// TODO: holding and dragged -> try to drop at Cell. Otherwise leave the item held.
}

void UFlipperHeldItemController::Rotate()
{
	// TODO: toggle the rotation and turn the grab offset clockwise: (x, y) in W x H becomes (H-1-y, x).
}

void UFlipperHeldItemController::Cancel()
{
	// TODO: snap the held item back to its origin grid and placement.
}

bool UFlipperHeldItemController::GetGhost(const UInventoryGrid* Grid, FIntPoint HoveredCell, FIntPoint& OutOrigin, bool& bOutValid) const
{
	// TODO: origin = HoveredCell - GrabOffset; valid = fits in Grid and (for a different grid) the approver allows it.
	return false;
}

bool UFlipperHeldItemController::QuickTransfer(UInventoryGrid* From, FIntPoint Cell, UInventoryGrid* To)
{
	// TODO: the item at Cell in From -> first fit in To, honouring the approver. False (and no change) otherwise.
	return false;
}
