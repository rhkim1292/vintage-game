// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlipperDayLoop.h"
#include "FlipperEndScreen.generated.h"

/**
 *  The win/lose screen (Docs/FlipperSpec.md, Feature 6): shown once the day loop is Finished. A dimmed full-screen layer
 *  (so the grids behind it stop taking clicks) with a centred panel: "You made rent!" or "Evicted!", the final cash,
 *  the rent, the days played, and a Play Again button that reloads the level for a fresh run on a new random seed.
 *  Collapsed until the run is Finished. Paints itself from NativePaint like the other Flipper widgets.
 */
UCLASS()
class UFlipperEndScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UFlipperDayLoop* InDayLoop);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	static constexpr float PanelWidth = 480.f;
	static constexpr float PanelHeight = 300.f;

	UPROPERTY()
	TObjectPtr<UFlipperDayLoop> DayLoop;

	/** Shows the screen when the run is Finished and hides it otherwise. */
	void Refresh();
	void OnPhaseChanged(EFlipperPhase NewPhase, int32 Day);

	/** The panel and its button in the widget's local space, which is the whole screen. */
	static FVector2D GetPanelPos(FVector2D ScreenSize) { return (ScreenSize - FVector2D(PanelWidth, PanelHeight)) * 0.5f; }
	static FVector2D GetButtonPos(FVector2D ScreenSize) { return GetPanelPos(ScreenSize) + FVector2D((PanelWidth - 200.f) * 0.5f, PanelHeight - 70.f); }
	static FVector2D GetButtonSize() { return FVector2D(200.f, 44.f); }
};
