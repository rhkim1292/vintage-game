// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperItemTooltip.h"
#include "FlipperHeldItemController.h"
#include "FlipperPaint.h"
#include "Framework/Application/SlateApplication.h"
#include "InventoryGrid.h"
#include "InventoryGridWidget.h"

void UFlipperItemTooltip::Setup(UFlipperHeldItemController* InController)
{
	Controller = InController;
}

void UFlipperItemTooltip::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UFlipperItemTooltip::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!Controller || Controller->IsHolding())
	{
		return LayerId;
	}

	const FVector2D		  AbsoluteCursor = FSlateApplication::Get().GetCursorPos();
	FIntPoint			  Cell;
	UInventoryGridWidget* GridWidget = UInventoryGridWidget::FindWidgetAt(AbsoluteCursor, Cell);
	if (!GridWidget)
	{
		return LayerId;
	}

	const FFlipperPlacedItem* Placed = GridWidget->GetGrid()->FindItemAt(Cell);
	if (!Placed || !Placed->Item.Definition)
	{
		return LayerId;
	}

	const UFlipperItemDefinition* Definition = Placed->Item.Definition;
	const FVector2D				  Pos = AllottedGeometry.AbsoluteToLocal(AbsoluteCursor) + FVector2D(16.f, 16.f);
	const FVector2D				  Size(180.f, 56.f);

	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, Pos, Size, FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, 1.f, FLinearColor(0.7f, 0.7f, 0.75f, 1.f));
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 6.f), Definition->DisplayName.ToString(), FLinearColor::White);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 28.f), FString::Printf(TEXT("%d x %d"), Definition->Width, Definition->Height), FLinearColor(0.7f, 0.7f, 0.75f, 1.f));

	return LayerId + 2;
}
