// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlipperHeldItemOverlay.generated.h"

class UFlipperHeldItemController;

/**
 *  Full-screen, click-through widget that draws the held item at the cursor (the footprint's grabbed cell sits under
 *  the cursor). Add it to the viewport above the grid widgets.
 */
UCLASS()
class UFlipperHeldItemOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UFlipperHeldItemController* InController, float InCellSize);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY()
	TObjectPtr<UFlipperHeldItemController> Controller;

	float CellSize = 64.f;
};
