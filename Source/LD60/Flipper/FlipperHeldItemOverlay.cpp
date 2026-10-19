// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperHeldItemOverlay.h"
#include "FlipperHeldItemController.h"
#include "FlipperPaint.h"
#include "Framework/Application/SlateApplication.h"

void UFlipperHeldItemOverlay::Setup(UFlipperHeldItemController* InController, float InCellSize)
{
	Controller = InController;
	CellSize = InCellSize;
}

void UFlipperHeldItemOverlay::NativeConstruct()
{
	Super::NativeConstruct();
	// Never take clicks: the grid widgets underneath (or the player controller, over empty space) must get them.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UFlipperHeldItemOverlay::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!Controller || !Controller->IsHolding())
	{
		return LayerId;
	}

	const FVector2D CursorLocal = AllottedGeometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
	const FIntPoint Footprint = Controller->GetHeldFootprint();
	const FIntPoint Grab = Controller->GetGrabOffset();

	// Centre the grabbed cell on the cursor.
	const FVector2D Pos = CursorLocal - FVector2D((Grab.X + 0.5f) * CellSize, (Grab.Y + 0.5f) * CellSize);
	const FVector2D Size = FVector2D(Footprint.X * CellSize, Footprint.Y * CellSize);
	const uint8		Hue = static_cast<uint8>(GetTypeHash(Controller->GetHeldItem().Definition->GetFName()) % 256);

	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, Pos, Size, FLinearColor::MakeFromHSV8(Hue, 140, 220) * FLinearColor(1.f, 1.f, 1.f, 0.8f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, 2.f, FLinearColor::White);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(4.f, 2.f), Controller->GetHeldItem().Definition->DisplayName.ToString(), FLinearColor::Black);

	return LayerId + 2;
}
