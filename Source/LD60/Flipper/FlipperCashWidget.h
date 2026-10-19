// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlipperCashWidget.generated.h"

class UFlipperWallet;

/**
 *  The cash HUD (Docs/FlipperSpec.md, Feature 3): a small click-through panel in the top-left corner showing the
 *  wallet's whole-dollar cash. Paints itself from NativePaint like the other Flipper widgets and reads the wallet
 *  every frame, so it is always current.
 */
UCLASS()
class UFlipperCashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UFlipperWallet* InWallet);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY()
	TObjectPtr<UFlipperWallet> Wallet;
};
