// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperCashWidget.h"
#include "FlipperPaint.h"
#include "FlipperWallet.h"

void UFlipperCashWidget::Setup(UFlipperWallet* InWallet)
{
	Wallet = InWallet;
}

void UFlipperCashWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UFlipperCashWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!Wallet)
	{
		return LayerId;
	}

	const FVector2D Pos(40.f, 24.f);
	const FVector2D Size(150.f, 34.f);
	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, Pos, Size, FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, 1.f, FLinearColor(0.3f, 0.9f, 0.4f, 1.f));
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D(10.f, 8.f), FString::Printf(TEXT("Cash  $%d"), Wallet->GetCash()), FLinearColor(0.3f, 0.9f, 0.4f, 1.f));
	return LayerId + 2;
}
