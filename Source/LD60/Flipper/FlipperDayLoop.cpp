// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperDayLoop.h"
#include "FlipperHeldItemController.h"
#include "FlipperItemGenerator.h"
#include "FlipperTradeRule.h"
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
		RollSellerTable();
		break;
	case EFlipperPhase::Garage:
		TestChargesLeft = Config.TestChargesPerDay;
		break;
	case EFlipperPhase::Sell:
		TimeLeft = Config.SellSeconds;
		break;
	default:
		break;
	}

	// Buying and refunds only happen in the Market.
	if (TradeRule)
	{
		TradeRule->SetOpen(NewPhase == EFlipperPhase::Market);
	}

	OnPhaseChanged.Broadcast(Phase, Day);
}

void UFlipperDayLoop::RollSellerTable()
{
	if (!Table || !Generator)
	{
		return;
	}

	const TArray<FFlipperPlacedItem> Leftovers = Table->GetPlacedItems();
	for (const FFlipperPlacedItem& Placed : Leftovers)
	{
		Table->Remove(Placed.Item);
	}

	FRandomStream DayRandom(Seed + Day);
	Generator->FillSellerTable(Table, DayRandom);
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
