// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryGridWidget.h"
#include "FlipperHeldItemController.h"
#include "FlipperPaint.h"
#include "Framework/Application/SlateApplication.h"
#include "InventoryGrid.h"

TArray<TWeakObjectPtr<UInventoryGridWidget>> UInventoryGridWidget::Registry;

void UInventoryGridWidget::Setup(UInventoryGrid* InGrid, UFlipperHeldItemController* InController, UInventoryGrid* InQuickTransferTarget, float InCellSize)
{
	Grid = InGrid;
	Controller = InController;
	QuickTransferTarget = InQuickTransferTarget;
	CellSize = InCellSize;
}

FVector2D UInventoryGridWidget::GetPixelSize() const
{
	return Grid ? FVector2D(Grid->GetWidth() * CellSize, Grid->GetHeight() * CellSize) : FVector2D::ZeroVector;
}

bool UInventoryGridWidget::ScreenToCellPos(FVector2D AbsolutePos, FVector2D& OutCellPos) const
{
	// A hidden grid keeps its last cached geometry, so without this check it would still answer for the spot it used to cover.
	if (!Grid || !IsVisible())
	{
		return false;
	}

	const FGeometry IGWFGeometry = GetCachedGeometry();
	const FVector2D CellPos = IGWFGeometry.AbsoluteToLocal(AbsolutePos) / CellSize;

	if (CellPos.X < 0.0 || CellPos.X >= Grid->GetWidth() || CellPos.Y < 0.0 || CellPos.Y >= Grid->GetHeight())
	{
		return false;
	}

	OutCellPos = CellPos;
	return true;
}

bool UInventoryGridWidget::ScreenToCell(FVector2D AbsolutePos, FIntPoint& OutCell) const
{
	FVector2D CellPos;
	if (!ScreenToCellPos(AbsolutePos, CellPos))
	{
		return false;
	}

	OutCell = FIntPoint(FMath::FloorToInt32(CellPos.X), FMath::FloorToInt32(CellPos.Y));
	return true;
}

UInventoryGridWidget* UInventoryGridWidget::FindWidgetAt(FVector2D AbsolutePos, FIntPoint& OutCell)
{
	for (const TWeakObjectPtr<UInventoryGridWidget>& Candidate : Registry)
	{
		if (Candidate.IsValid() && Candidate->ScreenToCell(AbsolutePos, OutCell))
		{
			return Candidate.Get();
		}
	}
	return nullptr;
}

FLinearColor UInventoryGridWidget::GetGhostColor(bool bValid) const
{
	// TODO: green-ish when valid, red when not. For now one neutral colour so the ghost is at least visible.
	if (bValid)
	{
		return FLinearColor(0.f, 255.f, 0.f, 0.35f);
	}
	else
	{
		return FLinearColor(255.f, 0.f, 0.f, 0.35f);
	}
}

void UInventoryGridWidget::OnQuickTransferRefused(FIntPoint Cell)
{
	// TODO (optional): shake the item at Cell. Logged for now.
	UE_LOG(LogTemp, Verbose, TEXT("Quick transfer refused at (%d, %d)"), Cell.X, Cell.Y);
}

void UInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// UUserWidget defaults to SelfHitTestInvisible, which lets clicks through to children only. This widget has no
	// children and handles the clicks itself, so it must be hit-testable.
	SetVisibility(ESlateVisibility::Visible);
	Registry.AddUnique(this);
}

void UInventoryGridWidget::NativeDestruct()
{
	Registry.Remove(this);
	Super::NativeDestruct();
}

int32 UInventoryGridWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!Grid)
	{
		return LayerId;
	}

	// Cells
	for (int32 Y = 0; Y < Grid->GetHeight(); ++Y)
	{
		for (int32 X = 0; X < Grid->GetWidth(); ++X)
		{
			FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, FVector2D(X * CellSize + 1.f, Y * CellSize + 1.f), FVector2D(CellSize - 2.f, CellSize - 2.f), FLinearColor(0.08f, 0.08f, 0.1f, 0.85f));
		}
	}

	// Items (a held item is in no grid, so it is not drawn here; the overlay draws it at the cursor)
	for (const FFlipperPlacedItem& Placed : Grid->GetPlacedItems())
	{
		const FIntPoint Footprint = UInventoryGrid::GetFootprint(Placed.Item.Definition, Placed.Placement.bRotated);
		const FVector2D Pos = FVector2D(Placed.Placement.Origin.X * CellSize + 2.f, Placed.Placement.Origin.Y * CellSize + 2.f);
		const FVector2D Size = FVector2D(Footprint.X * CellSize - 4.f, Footprint.Y * CellSize - 4.f);
		const uint8		Hue = static_cast<uint8>(GetTypeHash(Placed.Item.Definition->GetFName()) % 256);
		FlipperPaint::Box(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, FLinearColor::MakeFromHSV8(Hue, 140, 190));
		FlipperPaint::Outline(OutDrawElements, LayerId + 2, AllottedGeometry, Pos, Size, 3.f, FlipperRarity::GetColor(Placed.Item.Rarity));
		FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, Pos + FVector2D(4.f, 2.f), Placed.Item.Definition->DisplayName.ToString(), FLinearColor::Black);
			// Condition badge (Docs/FlipperSpec.md, Feature 7): a coloured letter on a dark pill in the top-right corner,
			// readable whatever colour the item's box is.
			const FVector2D BadgePos = Pos + FVector2D(Size.X - 18.f, 2.f);
			FlipperPaint::Box(OutDrawElements, LayerId + 2, AllottedGeometry, BadgePos, FVector2D(15.f, 18.f), FLinearColor(0.02f, 0.02f, 0.03f, 0.75f));
			FlipperPaint::Label(OutDrawElements, LayerId + 3, AllottedGeometry, BadgePos + FVector2D(3.f, 0.f), FlipperCondition::GetBadge(Placed.Item.Condition), FlipperCondition::GetColor(Placed.Item.Condition));

			// A "?" marks an item whose works/dead flag is still hidden; a red "X" marks one that tested dead.
			const FVector2D MarkPos = Pos + FVector2D(Size.X - 16.f, Size.Y - 22.f);
			if (!Placed.Item.bTested)
			{
				FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, MarkPos, TEXT("?"), FLinearColor::White);
			}
			else if (!Placed.Item.bWorks)
			{
				FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, MarkPos, TEXT("X"), FLinearColor(1.f, 0.15f, 0.15f, 1.f));
			}
	}

	// Ghost: only on the grid the cursor is over, only while something is held
	if (Controller && Controller->IsHolding())
	{
		FVector2D HoveredCellPos;
		FIntPoint GhostOrigin;
		bool	  bGhostValid = false;
		if (ScreenToCellPos(FSlateApplication::Get().GetCursorPos(), HoveredCellPos) && Controller->GetGhost(Grid, HoveredCellPos, GhostOrigin, bGhostValid))
		{
			const FIntPoint Footprint = Controller->GetHeldFootprint();
			FlipperPaint::Box(OutDrawElements, LayerId + 3, AllottedGeometry, FVector2D(GhostOrigin.X * CellSize, GhostOrigin.Y * CellSize), FVector2D(Footprint.X * CellSize, Footprint.Y * CellSize), GetGhostColor(bGhostValid));
		}
	}

	return LayerId + 4;
}

FReply UInventoryGridWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!Grid || !Controller)
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		Controller->Rotate();
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	const FVector2D AbsolutePos = InMouseEvent.GetScreenSpacePosition();
	FVector2D		CellPos;
	if (!ScreenToCellPos(AbsolutePos, CellPos))
	{
		return FReply::Unhandled();
	}
	const FIntPoint Cell(FMath::FloorToInt32(CellPos.X), FMath::FloorToInt32(CellPos.Y));

	if (InMouseEvent.IsShiftDown() && !Controller->IsHolding())
	{
		if (QuickTransferTarget && !Controller->QuickTransfer(Grid, Cell, QuickTransferTarget))
		{
			OnQuickTransferRefused(Cell);
		}
		return FReply::Handled();
	}

	Controller->PointerDown(Grid, CellPos, AbsolutePos);

	// Capture only after a pick-up, so the release of a drag reaches us wherever the cursor ends up.
	FReply Reply = FReply::Handled();
	if (Controller->IsHolding())
	{
		Reply.CaptureMouse(TakeWidget());
	}
	return Reply;
}

FReply UInventoryGridWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!Controller || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	// The release may be over a different grid, or over none: resolve it through the registry.
	const FVector2D		  AbsolutePos = InMouseEvent.GetScreenSpacePosition();
	FIntPoint			  Cell = FIntPoint::ZeroValue;
	FVector2D			  CellPos = FVector2D::ZeroVector;
	UInventoryGridWidget* Target = FindWidgetAt(AbsolutePos, Cell);
	if (Target)
	{
		Target->ScreenToCellPos(AbsolutePos, CellPos);
	}
	Controller->PointerUp(Target ? Target->GetGrid() : nullptr, CellPos, AbsolutePos);

	return FReply::Handled().ReleaseMouseCapture();
}

FReply UInventoryGridWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Controller)
	{
		Controller->PointerMove(InMouseEvent.GetScreenSpacePosition());
	}
	return FReply::Unhandled();
}
