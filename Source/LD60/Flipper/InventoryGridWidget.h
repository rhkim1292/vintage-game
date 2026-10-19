// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryGridWidget.generated.h"

class UInventoryGrid;
class UFlipperHeldItemController;

/**
 *  Draws one UInventoryGrid (cells, items, the drop ghost) and turns mouse input over it into calls on the shared
 *  UFlipperHeldItemController. Paints itself in NativePaint; there are no child widgets or Blueprint assets.
 *  Add it to the viewport with SetDesiredSizeInViewport(GetPixelSize()) so it is exactly the size of the grid.
 *  See Docs/FlipperSpec.md, Feature 1.
 */
UCLASS()
class UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** QuickTransferTarget is the grid Shift-click sends items to (the other grid); may be null. */
	void Setup(UInventoryGrid* InGrid, UFlipperHeldItemController* InController, UInventoryGrid* InQuickTransferTarget, float InCellSize);

	UInventoryGrid* GetGrid() const { return Grid; }

	/** Size in pixels of the whole grid at the current cell size. */
	FVector2D GetPixelSize() const;

	/**
	 *  TODO (yours): AbsolutePos is a Slate absolute (desktop) position, e.g. FSlateApplication::Get().GetCursorPos()
	 *  or FPointerEvent::GetScreenSpacePosition(). Turn it into the grid cell under it and return true, or return false
	 *  when it is outside the grid. Hint: GetCachedGeometry().AbsoluteToLocal(...) gives widget-local pixels; CellSize
	 *  is the side of one cell in those pixels. Until this is written no click, ghost or tooltip works.
	 */
	bool ScreenToCell(FVector2D AbsolutePos, FIntPoint& OutCell) const;

	/** The grid widget (of all those currently on screen) under AbsolutePos, with the cell under it. Null if none. */
	static UInventoryGridWidget* FindWidgetAt(FVector2D AbsolutePos, FIntPoint& OutCell);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/**
	 *  TODO (yours): the colour of the drop ghost. The spec says green-ish when the drop is valid and red when it is not
	 *  (the controller already folds the transfer approver into bValid).
	 */
	FLinearColor GetGhostColor(bool bValid) const;

	/** Called when a Shift-click transfer was refused. TODO (yours, optional): shake the item. */
	void OnQuickTransferRefused(FIntPoint Cell);

private:
	UPROPERTY()
	TObjectPtr<UInventoryGrid> Grid;

	UPROPERTY()
	TObjectPtr<UFlipperHeldItemController> Controller;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> QuickTransferTarget;

	float CellSize = 64.f;

	/** Every grid widget currently constructed, so a drag released over another grid (or over none) can be resolved. */
	static TArray<TWeakObjectPtr<UInventoryGridWidget>> Registry;
};
