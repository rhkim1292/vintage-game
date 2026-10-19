// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlipperDayHud.generated.h"

class UFlipperDayLoop;

/**
 *  The day-loop panel (Docs/FlipperSpec.md, Feature 4): the day number, the phase, the timer (red in the last seconds),
 *  the test charges left in the Garage, and the End phase button. Paints itself from NativePaint like the other
 *  Flipper widgets. It is small and Visible (it has a button), so size and place it with SetDesiredSizeInViewport and
 *  SetPositionInViewport; a full-screen one would swallow the clicks meant for the grids.
 */
UCLASS()
class UFlipperDayHud : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr float PanelWidth = 512.f;
	static constexpr float PanelHeight = 54.f;

	void Setup(UFlipperDayLoop* InDayLoop);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY()
	TObjectPtr<UFlipperDayLoop> DayLoop;

	/** The End phase button, in the panel's local space. */
	static FVector2D GetButtonPos() { return FVector2D(PanelWidth - 120.f, 10.f); }
	static FVector2D GetButtonSize() { return FVector2D(110.f, 34.f); }
};

/**
 *  The big "Day 2: Market" banner shown for a moment after every phase change. Full-screen and click-through.
 */
UCLASS()
class UFlipperPhaseBanner : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UFlipperDayLoop* InDayLoop);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY()
	TObjectPtr<UFlipperDayLoop> DayLoop;
};
