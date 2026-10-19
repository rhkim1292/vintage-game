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
	const FVector2D				  Size(190.f, 122.f);
	const FLinearColor			  RarityColor = FlipperRarity::GetColor(Placed->Item.Rarity);
	const FLinearColor			  Dim(0.7f, 0.7f, 0.75f, 1.f);

	// Status line (Docs/FlipperSpec.md, Feature 2): Untested hides the works/dead flag; a Tested item shows it.
	FString		 Status = TEXT("Untested");
	FLinearColor StatusColor = Dim;
	if (Placed->Item.bTested)
	{
		Status = Placed->Item.bWorks ? TEXT("Tested: works") : TEXT("DEAD");
		StatusColor = Placed->Item.bWorks ? FLinearColor(0.3f, 0.9f, 0.4f, 1.f) : FLinearColor(1.f, 0.25f, 0.25f, 1.f);
	}

	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, Pos, Size, FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, 1.f, RarityColor);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 6.f), Definition->DisplayName.ToString(), RarityColor);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 28.f), FString::Printf(TEXT("%d x %d"), Definition->Width, Definition->Height), Dim);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 50.f), FlipperRarity::GetName(Placed->Item.Rarity), RarityColor);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 72.f), Status, StatusColor);

	// Price line (Docs/FlipperSpec.md, Feature 3): what the seller asks, or what the player paid once bought.
	if (Placed->Item.PaidPrice > 0)
	{
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 94.f), FString::Printf(TEXT("Paid $%d"), Placed->Item.PaidPrice), Dim);
	}
	else if (Placed->Item.AskPrice > 0)
	{
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, 94.f), FString::Printf(TEXT("Ask $%d"), Placed->Item.AskPrice), FLinearColor(1.f, 0.85f, 0.3f, 1.f));
	}

	return LayerId + 2;
}
