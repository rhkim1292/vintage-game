// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperTypes.h"
#include "FlipperPricing.h"
#include "FlipperDayLoop.generated.h"

class UInventoryGrid;
class UFlipperHeldItemController;
class UFlipperItemGenerator;
class UFlipperTradeRule;

/** The three phases of a day, in order, and Finished once the last day's Sell phase is over. */
UENUM(BlueprintType)
enum class EFlipperPhase : uint8
{
	Market,
	Garage,
	Sell,
	Finished
};

/** Every number of the day loop, kept in data so the balance pass can tune it (Docs/FlipperSpec.md, Feature 4). */
USTRUCT(BlueprintType)
struct FFlipperDayConfig
{
	GENERATED_BODY()

	/** How many days a run lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day", meta = (ClampMin = 1))
	int32 DayCount = 5;

	/** Length of the Market phase in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day", meta = (ClampMin = 1))
	float MarketSeconds = 60.f;

	/** Length of the Sell phase in seconds. The Garage has no clock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day", meta = (ClampMin = 1))
	float SellSeconds = 45.f;

	/** How many Untested items the player can test in the Garage each day. Unused charges are lost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day", meta = (ClampMin = 0))
	int32 TestChargesPerDay = 3;

	/** A timed phase counts as "running out" (the HUD turns red) when this many seconds or fewer are left. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day", meta = (ClampMin = 0))
	float WarningSeconds = 10.f;

	/** How long the phase banner stays on screen after each phase change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day", meta = (ClampMin = 0))
	float BannerSeconds = 1.f;
};

/** Fired after the phase changes (NewPhase, Day). A new day starts with a Market phase. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFlipperPhaseChanged, EFlipperPhase /*NewPhase*/, int32 /*Day*/);

/**
 *  The day loop (Docs/FlipperSpec.md, Feature 4): Market (timed), Garage (limited by test charges), Sell (timed), then
 *  the next day, for DayCount days. Pure logic: no widgets. The caller feeds it time with Tick.
 *
 *    - Entering Market empties the seller table and rolls a fresh one (a new FRandomStream per day), and the trade rule
 *      (if given) opens. The trunk is never touched, so unsold items carry over.
 *    - Leaving a phase first snaps back any held item. Leaving Market also clears bBoughtThisPhase on every trunk item
 *      (they can no longer be refunded) and closes the trade rule.
 *    - Entering Garage gives TestChargesPerDay charges. TryTestAt spends one to reveal an Untested trunk item.
 *    - Selling (Feature 5): entering Market rolls the day's buyer mood (its own FRandomStream, so the table roll is not
 *      disturbed) and hands its multiplier to the trade rule. Entering Sell clears the seller table (the seller has left)
 *      and opens selling on the trade rule; leaving Sell closes it.
 */
UCLASS(BlueprintType)
class UFlipperDayLoop : public UObject
{
	GENERATED_BODY()

public:
	/**
	 *  Sets everything up and does not start the run. TradeRule may be null (the loop then does not gate trading).
	 *  Seed makes the rolled seller tables repeatable: day N rolls with FRandomStream(Seed + N).
	 */
	void Init(const FFlipperDayConfig& InConfig, UInventoryGrid* InTrunk, UInventoryGrid* InTable, UFlipperItemGenerator* InGenerator, UFlipperHeldItemController* InHeldItemController, UFlipperTradeRule* InTradeRule, int32 InSeed);

	/** The moods are rolled and scaled with these numbers. Call before StartRun; the default is FFlipperPricingConfig(). */
	void SetPricingConfig(const FFlipperPricingConfig& InConfig) { PricingConfig = InConfig; }

	/** Hooks the Garage test click up to the controller (its PickupIntercept). */
	void Bind(UFlipperHeldItemController* Controller);

	/** Day 1, Market. Fires OnPhaseChanged. */
	void StartRun();

	/** Advances the clock of a timed phase (Market, Sell); when it reaches zero the phase ends. Also runs the banner. */
	void Tick(float DeltaSeconds);

	/** Ends the current phase now and starts the next one (the End phase button). Does nothing once Finished. */
	void EndPhase();

	/**
	 *  The Garage test. True, and one charge spent, only if: it is the Garage phase, a charge is left, Grid is the trunk,
	 *  and the item covering Cell is Untested. The item becomes Tested (bWorks is unchanged: the truth is revealed).
	 */
	bool TryTestAt(const UInventoryGrid* Grid, FIntPoint Cell);

	EFlipperPhase GetPhase() const { return Phase; }

	/** 1-based. 0 before StartRun. */
	int32 GetDay() const { return Day; }

	const FFlipperDayConfig& GetConfig() const { return Config; }

	/** Seconds left in a timed phase. 0 in the Garage and when Finished. */
	float GetTimeLeft() const { return TimeLeft; }

	/** True for Market and Sell. */
	bool IsTimed() const { return Phase == EFlipperPhase::Market || Phase == EFlipperPhase::Sell; }

	/** True when a timed phase has WarningSeconds or fewer left. */
	bool IsWarning() const { return IsTimed() && TimeLeft <= Config.WarningSeconds; }

	/** Test charges left today. Only above 0 in the Garage. */
	int32 GetTestChargesLeft() const { return TestChargesLeft; }

	/** Seconds the phase banner still has to show: BannerSeconds right after a phase change, counting down to 0. */
	float GetBannerTimeLeft() const { return BannerTimeLeft; }

	/** Today's buyer mood. Rolled when the day starts (Market), so it is known all day. Normal before StartRun. */
	EFlipperBuyerMood GetMood() const { return Mood; }

	/** The offer multiplier of today's mood. */
	float GetMoodMultiplier() const { return FlipperPricing::GetMoodMultiplier(Mood, PricingConfig); }

	FOnFlipperPhaseChanged OnPhaseChanged;

private:
	FFlipperDayConfig Config;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> Trunk;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> Table;

	UPROPERTY()
	TObjectPtr<UFlipperItemGenerator> Generator;

	UPROPERTY()
	TObjectPtr<UFlipperHeldItemController> HeldItemController;

	UPROPERTY()
	TObjectPtr<UFlipperTradeRule> TradeRule;

	int32		   Seed = 0;
	EFlipperPhase  Phase = EFlipperPhase::Market;
	int32		   Day = 0;
	float		   TimeLeft = 0.f;
	int32		   TestChargesLeft = 0;
	float		   BannerTimeLeft = 0.f;

	FFlipperPricingConfig PricingConfig;
	EFlipperBuyerMood	  Mood = EFlipperBuyerMood::Normal;

	void EnterPhase(EFlipperPhase NewPhase);
	void ClearSellerTable();
	void RollSellerTable();
	void RollMood();
	void ClearBoughtFlags();
};
