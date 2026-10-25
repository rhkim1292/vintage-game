// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperItemTooltip.h"
#include "FlipperHeldItemController.h"
#include "FlipperPaint.h"
#include "FlipperTradeRule.h"
#include "Framework/Application/SlateApplication.h"
#include "InventoryGrid.h"
#include "InventoryGridWidget.h"

void UFlipperItemTooltip::Setup(UFlipperHeldItemController* InController, UFlipperTradeRule* InTradeRule)
{
	Controller = InController;
	TradeRule = InTradeRule;
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
	const FVector2D				  Pos = FlipperPaint::CursorLocal(Args, AllottedGeometry) + FVector2D(16.f, 16.f);
	// While selling is open, a trunk item also shows what the buyer would pay for it today (Docs/FlipperSpec.md, Feature 5).
	// (The counter never holds anything, because a sold item is used up, so any item under the cursor is a trunk item.)
	const bool bShowOffer = TradeRule && TradeRule->IsSellOpen();

	// One line each for: name, footprint, rarity and condition, status, every trait (Docs/FlipperSpec.md, Feature 7), the
	// price (its row is always reserved) and, while selling, the offer.
	TArray<const UFlipperTraitDefinition*> Traits;
	for (const TObjectPtr<UFlipperTraitDefinition>& Trait : Placed->Item.Traits)
	{
		if (Trait)
		{
			Traits.Add(Trait);
		}
	}
	constexpr float LineHeight = 22.f;
	const int32		LineCount = 5 + Traits.Num() + (bShowOffer ? 1 : 0);
	const FVector2D Size(190.f, LineCount * LineHeight + 12.f);

	const FLinearColor RarityColor = FlipperRarity::GetColor(Placed->Item.Rarity);
	const FLinearColor Dim(0.7f, 0.7f, 0.75f, 1.f);

	// Status line (Docs/FlipperSpec.md, Feature 2): Untested hides the works/dead flag; a Tested item shows it.
	FString		 Status = TEXT("Untested");
	FLinearColor StatusColor = Dim;
	if (Placed->Item.bTested)
	{
		Status = Placed->Item.bWorks ? TEXT("Tested: works") : TEXT("DEAD");
		StatusColor = Placed->Item.bWorks ? FLinearColor(0.3f, 0.9f, 0.4f, 1.f) : FLinearColor(1.f, 0.25f, 0.25f, 1.f);
	}

	FString Footprint = FString::Printf(TEXT("%d x %d"), Definition->Width, Definition->Height);
	if (Definition->bIsLot)
	{
		Footprint += FString::Printf(TEXT("   Lot of %d"), Definition->LotCount);
	}

	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, Pos, Size, FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, 1.f, RarityColor);

	float Y = 6.f;
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), Definition->DisplayName.ToString(), RarityColor);
	Y += LineHeight;
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), Footprint, Dim);
	Y += LineHeight;
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), FlipperRarity::GetName(Placed->Item.Rarity), RarityColor);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(96.f, Y), FlipperCondition::GetName(Placed->Item.Condition), FlipperCondition::GetColor(Placed->Item.Condition));
	Y += LineHeight;
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), Status, StatusColor);
	Y += LineHeight;

	for (const UFlipperTraitDefinition* Trait : Traits)
	{
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), Trait->DisplayName.ToString(), Trait->Color);
		Y += LineHeight;
	}

	// Price line (Docs/FlipperSpec.md, Feature 3): what the seller asks, or what the player paid once bought.
	if (Placed->Item.PaidPrice > 0)
	{
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), FString::Printf(TEXT("Paid $%d"), Placed->Item.PaidPrice), Dim);
	}
	else if (Placed->Item.AskPrice > 0)
	{
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), FString::Printf(TEXT("Ask $%d"), Placed->Item.AskPrice), FLinearColor(1.f, 0.85f, 0.3f, 1.f));
	}
	Y += LineHeight;

	if (bShowOffer)
	{
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(8.f, Y), FString::Printf(TEXT("Offer $%d"), TradeRule->GetOffer(Placed->Item)), FLinearColor(0.3f, 0.9f, 0.4f, 1.f));
	}

	return LayerId + 2;
}
