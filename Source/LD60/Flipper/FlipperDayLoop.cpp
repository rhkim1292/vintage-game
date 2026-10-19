// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperDayLoop.h"
#include "FlipperHeldItemController.h"
#include "FlipperItemGenerator.h"
#include "FlipperTradeRule.h"
#include "FlipperWallet.h"
#include "InventoryGrid.h"

namespace
{
	/** Swaps the item in Grid (matched by Id) for NewItem at the same place. Used to change an item's flags in place. */
	void ReplaceInPlace(UInventoryGrid* Grid, const FFlipperPlacedItem& Old, const FFlipperItemInstance& NewItem)
	{
		const FFlipperPlacement Placement = Old.Placement;
		Grid->Remove(Old.Item);
		ensureMsgf(Grid->TryPlace(NewItem, Placement.Origin, Placement.bRotated), TEXT("ReplaceInPlace: the changed item no longer fits where it was."));
	}
} // namespace

void UFlipperDayLoop::Init(const FFlipperDayConfig& InConfig, UInventoryGrid* InTrunk, UInventoryGrid* InTable, UFlipperItemGenerator* InGenerator, UFlipperHeldItemController* InHeldItemController, UFlipperTradeRule* InTradeRule, int32 InSeed)
{
	Config = InConfig;
	Trunk = InTrunk;
	Table = InTable;
	Generator = InGenerator;
	HeldItemController = InHeldItemController;
	TradeRule = InTradeRule;
	Seed = InSeed;
	Phase = EFlipperPhase::Market;
	Day = 0;
	TimeLeft = 0.f;
	TestChargesLeft = 0;
	BannerTimeLeft = 0.f;
	Mood = EFlipperBuyerMood::Normal;
	Result = EFlipperRunResult::InProgress;
}

int32 UFlipperDayLoop::GetCash() const
{
	return Wallet ? Wallet->GetCash() : 0;
}

void UFlipperDayLoop::Bind(UFlipperHeldItemController* Controller)
{
	if (!Controller)
	{
		return;
	}

	TWeakObjectPtr<UFlipperDayLoop> WeakThis(this);
	Controller->PickupIntercept = [WeakThis](UInventoryGrid* Grid, FIntPoint Cell) {
		return WeakThis.IsValid() && WeakThis->TryTestAt(Grid, Cell);
	};
}

void UFlipperDayLoop::StartRun()
{
	Day = 1;
	EnterPhase(EFlipperPhase::Market);
}

void UFlipperDayLoop::Tick(float DeltaSeconds)
{
	if (Day <= 0)
	{
		return;
	}

	BannerTimeLeft = FMath::Max(0.f, BannerTimeLeft - DeltaSeconds);

	if (IsTimed())
	{
		TimeLeft -= DeltaSeconds;
		if (TimeLeft <= 0.f)
		{
			EndPhase();
		}
	}
}

void UFlipperDayLoop::EndPhase()
{
	if (Day <= 0 || Phase == EFlipperPhase::Finished)
	{
		return;
	}

	// Whatever the player is holding goes back where it came from before anything else changes.
	if (HeldItemController)
	{
		HeldItemController->Cancel();
	}

	switch (Phase)
	{
	case EFlipperPhase::Market:
		ClearBoughtFlags();
		EnterPhase(EFlipperPhase::Garage);
		break;
	case EFlipperPhase::Garage:
		EnterPhase(EFlipperPhase::Sell);
		break;
	case EFlipperPhase::Sell:
		if (Day >= Config.DayCount)
		{
			EnterPhase(EFlipperPhase::Finished);
		}
		else
		{
			++Day;
			EnterPhase(EFlipperPhase::Market);
		}
		break;
	default:
		break;
	}
}

bool UFlipperDayLoop::TryTestAt(const UInventoryGrid* Grid, FIntPoint Cell)
{
	if (Phase != EFlipperPhase::Garage || TestChargesLeft <= 0 || !Trunk || Grid != Trunk)
	{
		return false;
	}

	const FFlipperPlacedItem* Placed = Trunk->FindItemAt(Cell);
	if (!Placed || Placed->Item.bTested)
	{
		return false;
	}

	FFlipperItemInstance Tested = Placed->Item;
	Tested.bTested = true;
	ReplaceInPlace(Trunk, *Placed, Tested);
	--TestChargesLeft;
	return true;
}

void UFlipperDayLoop::EnterPhase(EFlipperPhase NewPhase)
{
	Phase = NewPhase;
	BannerTimeLeft = Config.BannerSeconds;
	TestChargesLeft = 0;
	TimeLeft = 0.f;

	switch (NewPhase)
	{
	case EFlipperPhase::Market:
		TimeLeft = Config.MarketSeconds;
		RollMood();
		RollSellerTable();
		break;
	case EFlipperPhase::Garage:
		TestChargesLeft = Config.TestChargesPerDay;
		break;
	case EFlipperPhase::Sell:
		TimeLeft = Config.SellSeconds;
		// The seller has packed up: the table becomes the buyer's counter.
		ClearSellerTable();
		break;
	case EFlipperPhase::Finished:
		// The one rent check of the run. Cash only: whatever is left in the trunk is worth nothing here.
		if (Wallet)
		{
			Result = IsRentMet(Wallet->GetCash(), Config.RentAmount) ? EFlipperRunResult::Won : EFlipperRunResult::Lost;
		}
		break;
	default:
		break;
	}

	// Buying and refunds only happen in the Market, selling only in the Sell phase.
	if (TradeRule)
	{
		TradeRule->SetOpen(NewPhase == EFlipperPhase::Market);
		TradeRule->SetSellOpen(NewPhase == EFlipperPhase::Sell);
	}

	OnPhaseChanged.Broadcast(Phase, Day);
}

void UFlipperDayLoop::ClearSellerTable()
{
	if (!Table)
	{
		return;
	}

	const TArray<FFlipperPlacedItem> Leftovers = Table->GetPlacedItems();
	for (const FFlipperPlacedItem& Placed : Leftovers)
	{
		Table->Remove(Placed.Item);
	}
}

void UFlipperDayLoop::RollSellerTable()
{
	if (!Table || !Generator)
	{
		return;
	}

	ClearSellerTable();

	FRandomStream DayRandom(Seed + Day);
	Generator->FillSellerTable(Table, DayRandom);
}

void UFlipperDayLoop::RollMood()
{
	// Its own stream, offset from the seller table's, so the two rolls never disturb each other.
	constexpr int32 MoodSeedOffset = 7919;
	FRandomStream	MoodRandom(Seed + Day + MoodSeedOffset);
	Mood = FlipperPricing::RollMood(PricingConfig, MoodRandom);

	if (TradeRule)
	{
		TradeRule->SetPricingConfig(PricingConfig);
		TradeRule->SetOfferMultiplier(GetMoodMultiplier());
	}
}

void UFlipperDayLoop::ClearBoughtFlags()
{
	if (!Trunk)
	{
		return;
	}

	const TArray<FFlipperPlacedItem> Snapshot = Trunk->GetPlacedItems();
	for (const FFlipperPlacedItem& Placed : Snapshot)
	{
		if (Placed.Item.bBoughtThisPhase)
		{
			FFlipperItemInstance Kept = Placed.Item;
			Kept.bBoughtThisPhase = false;
			ReplaceInPlace(Trunk, Placed, Kept);
		}
	}
}
