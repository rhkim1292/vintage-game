// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FlipperTestPlayerController.generated.h"

class UInventoryGrid;
class UInventoryGridWidget;
class UFlipperHeldItemController;
class UFlipperHeldItemOverlay;
class UFlipperItemTooltip;
class UFlipperItemDefinition;
class UFlipperItemGenerator;
class UFlipperWallet;
class UFlipperTradeRule;
class UFlipperCashWidget;
class UFlipperDayLoop;
class UFlipperDayHud;
class UFlipperPhaseBanner;
class UFlipperEndScreen;
enum class EFlipperPhase : uint8;

/**
 *  Phase 2.2 test screen: builds a 10x6 trunk and an 8x5 seller table, fills them with hard-coded test items,
 *  shows both with the held-item overlay and tooltip, and routes the keys that are not over a grid
 *  (R / right-click rotate, Escape cancel, a left click over empty space snaps a held item back).
 *  Throwaway debug scaffolding: the real game screen replaces it.
 */
UCLASS()
class AFlipperTestPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	static constexpr float CellSize = 64.f;

	/** The trunk is small on purpose: limited space makes the player choose what to keep. */
	static constexpr int32 TrunkColumns = 4;
	static constexpr int32 TrunkRows = 3;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> Trunk;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> SellerTable;

	UPROPERTY()
	TObjectPtr<UFlipperHeldItemController> HeldItemController;

	UPROPERTY()
	TArray<TObjectPtr<UFlipperItemDefinition>> TestDefinitions;

	UPROPERTY()
	TObjectPtr<UFlipperItemGenerator> Generator;

	UPROPERTY()
	TObjectPtr<UFlipperWallet> Wallet;

	UPROPERTY()
	TObjectPtr<UFlipperTradeRule> TradeRule;

	UPROPERTY()
	TObjectPtr<UFlipperCashWidget> CashWidget;

	UPROPERTY()
	TObjectPtr<UFlipperDayLoop> DayLoop;

	UPROPERTY()
	TObjectPtr<UFlipperDayHud> DayHud;

	UPROPERTY()
	TObjectPtr<UFlipperPhaseBanner> PhaseBanner;

	UPROPERTY()
	TObjectPtr<UFlipperEndScreen> EndScreen;

	UPROPERTY()
	TObjectPtr<UInventoryGridWidget> TrunkWidget;

	UPROPERTY()
	TObjectPtr<UInventoryGridWidget> SellerTableWidget;

	UPROPERTY()
	TObjectPtr<UFlipperHeldItemOverlay> Overlay;

	UPROPERTY()
	TObjectPtr<UFlipperItemTooltip> Tooltip;

	UFlipperItemDefinition* MakeDefinition(const TCHAR* Name, int32 Width, int32 Height);
	void					FillTestItems();

	/** Loads the item definition data assets from /Game/Flipper/Items and sets up the generator. False if none are found. The day loop rolls the seller table. */
	bool FillRolledItems();

	/** The seller's table has nothing to offer in the Garage (no buying, and the day's leftovers are about to be thrown away), so its grid is hidden then. */
	void OnPhaseChanged(EFlipperPhase NewPhase, int32 Day);

	void OnRotatePressed();
	void OnCancelPressed();
	void OnLeftPressed();
};
