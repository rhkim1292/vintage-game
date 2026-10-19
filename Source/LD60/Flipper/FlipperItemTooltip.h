// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlipperItemTooltip.generated.h"

class UFlipperHeldItemController;

/**
 *  Full-screen, click-through widget that draws a small card next to the cursor describing the item under it
 *  (found through whichever UInventoryGridWidget is under the cursor). Hidden while an item is held.
 *  Phase 3 adds condition, traits and value to the card.
 */
UCLASS()
class UFlipperItemTooltip : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UFlipperHeldItemController* InController);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY()
	TObjectPtr<UFlipperHeldItemController> Controller;
};
