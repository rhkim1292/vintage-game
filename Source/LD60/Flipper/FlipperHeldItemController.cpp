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

FIntPoint UFlipperHeldItemController::GetDropOrigin(FVector2D CellPos) const
{
	// The picture is centred on the pointer, so its top-left corner is half the footprint up and left of it.
	// Rounding (not flooring) picks the nearest cell, which is what makes an even side flip once the pointer passes the middle of a cell.
	const FIntPoint Footprint = GetHeldFootprint();
	return FIntPoint(FMath::RoundToInt32(CellPos.X - Footprint.X * 0.5), FMath::RoundToInt32(CellPos.Y - Footprint.Y * 0.5));
}

void UFlipperHeldItemController::Init(float InDragThresholdPixels)
{
	bHolding = false;
	HeldItem = {};
	OriginGrid = nullptr;
	OriginPlacement = {};
	bHeldRotated = false;
	DragThresholdPixels = InDragThresholdPixels;
	PressScreenPos = FVector2D::ZeroVector;
	bDragged = false;
}

void UFlipperHeldItemController::PointerDown(UInventoryGrid* Grid, FVector2D CellPos, FVector2D ScreenPos)
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
		const FIntPoint			  Cell(FMath::FloorToInt32(CellPos.X), FMath::FloorToInt32(CellPos.Y));
		const FFlipperPlacedItem* PlacedItemPtr = Grid->FindItemAt(Cell);

		if (!PlacedItemPtr)
		{
			return;
		}

		if (PickupIntercept && PickupIntercept(Grid, Cell))
		{
			return;
		}

		HeldItem = PlacedItemPtr->Item;
		bHolding = true;
		OriginGrid = Grid;
		OriginPlacement = PlacedItemPtr->Placement;
		bHeldRotated = PlacedItemPtr->Placement.bRotated;
		Grid->Remove(HeldItem);
		PressScreenPos = ScreenPos;
		bDragged = false;
		OnChanged.Broadcast();
		return;
	}

	DropHeldItem(Grid, CellPos);
}

void UFlipperHeldItemController::PointerMove(FVector2D ScreenPos)
{
	if (FVector2D::Distance(ScreenPos, PressScreenPos) > DragThresholdPixels && IsHolding())
	{
		bDragged = true;
	}
}

void UFlipperHeldItemController::PointerUp(UInventoryGrid* Grid, FVector2D CellPos, FVector2D ScreenPos)
{
	if (IsHolding() && bDragged)
	{
		DropHeldItem(Grid, CellPos);
	}
}

void UFlipperHeldItemController::Rotate()
{
	if (!IsHolding())
	{
		return;
	}
	bHeldRotated = !bHeldRotated;
	OnChanged.Broadcast();
}

void UFlipperHeldItemController::Cancel()
{
	SnapBackHeldItem();
}

bool UFlipperHeldItemController::GetGhost(const UInventoryGrid* Grid, FVector2D CellPos, FIntPoint& OutOrigin, bool& bOutValid) const
{
	if (!IsHolding() || !Grid)
	{
		return false;
	}

	const bool bApproved = Grid == OriginGrid || !TransferApprover || TransferApprover(OriginGrid, Grid, HeldItem);
	OutOrigin = GetDropOrigin(CellPos);
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

	// A sale uses the item up, so it needs no room in the target grid.
	const bool bConsumed = From != To && TransferConsumes && TransferConsumes(From, To, FromGridItem);

	FIntPoint FitOrigin = FIntPoint::ZeroValue;
	bool	  bFitRotated = false;
	if (!bConsumed && !To->FindFirstFit(FromGridItem, FitOrigin, bFitRotated))
	{
		return false;
	}

	if (From != To && TransferCommitted)
	{
		TransferCommitted(From, To, FromGridItem);
	}

	if (!bConsumed)
	{
		ensureMsgf(To->TryPlace(FromGridItem, FitOrigin, bFitRotated), TEXT("Placement failed: the held item could not be transferred at found fit location."));
	}
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

void UFlipperHeldItemController::DropHeldItem(UInventoryGrid* Grid, FVector2D CellPos)
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

	const FIntPoint DropOrigin = GetDropOrigin(CellPos);
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

	// A sale uses the item up: it is paid for in TransferCommitted and never placed. It is in no grid already (held).
	const bool bConsumed = Grid != OriginGrid && TransferConsumes && TransferConsumes(OriginGrid, Grid, HeldItem);

	if (Grid != OriginGrid && TransferCommitted)
	{
		TransferCommitted(OriginGrid, Grid, HeldItem);
	}

	if (!bConsumed)
	{
		ensureMsgf(Grid->TryPlace(HeldItem, DropOrigin, bHeldRotated), TEXT("Placement failed: the held item could not be dropped at the target cell."));
	}
	Init(DragThresholdPixels);
	OnChanged.Broadcast();
}
