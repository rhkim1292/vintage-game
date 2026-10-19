// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperHeldItemController.h"
#include "InventoryGrid.h"

FIntPoint UFlipperHeldItemController::GetHeldFootprint() const
{
	if (!IsHolding())
	{
		return FIntPoint::ZeroValue;
	}

	return UInventoryGrid::GetFootprint(HeldItem.Definition, IsHeldRotated());
}

void UFlipperHeldItemController::Init(float InDragThresholdPixels)
{
	bHolding = false;
	HeldItem = {};
	OriginGrid = nullptr;
	OriginPlacement = {};
	GrabOffset = FIntPoint::ZeroValue;
	bHeldRotated = false;
	DragThresholdPixels = InDragThresholdPixels;
	PressScreenPos = FVector2D::ZeroVector;
	bDragged = false;
}

void UFlipperHeldItemController::PointerDown(UInventoryGrid* Grid, FIntPoint Cell, FVector2D ScreenPos)
{
	if (!Grid)
	{
		if (!IsHolding())
		{
			return;
		}

		SnapBackHeldItem();
		return;
	}

	if (!IsHolding())
	{
		const FFlipperPlacedItem* PlacedItemPtr = Grid->FindItemAt(Cell);

		if (!PlacedItemPtr)
		{
			return;
		}

		HeldItem = PlacedItemPtr->Item;
		bHolding = true;
		OriginGrid = Grid;
		OriginPlacement = PlacedItemPtr->Placement;
		GrabOffset = FIntPoint(Cell.X - PlacedItemPtr->Placement.Origin.X, Cell.Y - PlacedItemPtr->Placement.Origin.Y);
		bHeldRotated = PlacedItemPtr->Placement.bRotated;
		Grid->Remove(HeldItem);
		PressScreenPos = ScreenPos;
		bDragged = false;
		OnChanged.Broadcast();
		return;
	}

	DropHeldItem(Grid, Cell);
}

void UFlipperHeldItemController::PointerMove(FVector2D ScreenPos)
{
	if (FVector2D::Distance(ScreenPos, PressScreenPos) > DragThresholdPixels && IsHolding())
	{
		bDragged = true;
	}
}

void UFlipperHeldItemController::PointerUp(UInventoryGrid* Grid, FIntPoint Cell, FVector2D ScreenPos)
{
	if (IsHolding() && bDragged)
	{
		DropHeldItem(Grid, Cell);
	}
}

void UFlipperHeldItemController::Rotate()
{
	if (!IsHolding())
	{
		return;
	}
	FIntPoint HeldItemFootprint = GetHeldFootprint();
	bHeldRotated = !bHeldRotated;
	GrabOffset = FIntPoint(HeldItemFootprint.Y - 1 - GrabOffset.Y, GrabOffset.X);
	OnChanged.Broadcast();
}

void UFlipperHeldItemController::Cancel()
{
	SnapBackHeldItem();
}

bool UFlipperHeldItemController::GetGhost(const UInventoryGrid* Grid, FIntPoint HoveredCell, FIntPoint& OutOrigin, bool& bOutValid) const
{
	if (!IsHolding() || !Grid)
	{
		return false;
	}

	const bool bApproved = Grid == OriginGrid || !TransferApprover || TransferApprover(OriginGrid, Grid, HeldItem);
	OutOrigin = HoveredCell - GrabOffset;
	bOutValid = bApproved && Grid->CanPlace(HeldItem, OutOrigin, bHeldRotated);
	return true;
}

bool UFlipperHeldItemController::QuickTransfer(UInventoryGrid* From, FIntPoint Cell, UInventoryGrid* To)
{
	if (IsHolding())
	{
		return false;
	}

	if (!From || !To)
	{
		return false;
	}

	const FFlipperPlacedItem* FromGridPlacedItem = From->FindItemAt(Cell);

	if (!FromGridPlacedItem)
	{
		return false;
	}

	FFlipperItemInstance FromGridItem = FromGridPlacedItem->Item;

	if (TransferApprover && !TransferApprover(From, To, FromGridItem))
	{
		return false;
	}

	FIntPoint FitOrigin;
	bool	  bFitRotated;
	bool	  bFoundFit = To->FindFirstFit(FromGridItem, FitOrigin, bFitRotated);

	if (!bFoundFit)
	{
		return false;
	}

	if (From != To && TransferCommitted)
	{
		TransferCommitted(From, To, FromGridItem);
	}

	ensureMsgf(To->TryPlace(FromGridItem, FitOrigin, bFitRotated), TEXT("Placement failed: the held item could not be transferred at found fit location."));
	From->Remove(FromGridItem);
	return true;
}

void UFlipperHeldItemController::SnapBackHeldItem()
{
	if (!OriginGrid)
	{
		return;
	}

	ensureMsgf(OriginGrid->TryPlace(HeldItem, OriginPlacement.Origin, OriginPlacement.bRotated), TEXT("Snap-back failed: the held item could not be put back where it came from, so it is now in no grid."));
	Init(DragThresholdPixels);
	OnChanged.Broadcast();
}

void UFlipperHeldItemController::DropHeldItem(UInventoryGrid* Grid, FIntPoint Cell)
{
	if (!Grid)
	{
		if (!IsHolding())
		{
			return;
		}

		SnapBackHeldItem();
		return;
	}

	FIntPoint DropOrigin = FIntPoint(Cell.X - GrabOffset.X, Cell.Y - GrabOffset.Y);
	if (!Grid->CanPlace(HeldItem, DropOrigin, bHeldRotated))
	{
		SnapBackHeldItem();
		return;
	}

	if (Grid != OriginGrid && TransferApprover && !TransferApprover(OriginGrid, Grid, HeldItem))
	{
		SnapBackHeldItem();
		return;
	}

	if (Grid != OriginGrid && TransferCommitted)
	{
		TransferCommitted(OriginGrid, Grid, HeldItem);
	}

	ensureMsgf(Grid->TryPlace(HeldItem, DropOrigin, bHeldRotated), TEXT("Placement failed: the held item could not be dropped at the target cell."));
	Init(DragThresholdPixels);
	OnChanged.Broadcast();
}
