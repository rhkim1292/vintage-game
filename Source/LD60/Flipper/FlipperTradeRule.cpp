// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperTradeRule.h"
#include "FlipperHeldItemController.h"
#include "FlipperWallet.h"
#include "InventoryGrid.h"

void UFlipperTradeRule::Init(UInventoryGrid* InTable, UInventoryGrid* InTrunk, UFlipperWallet* InWallet)
{
	Table = InTable;
	Trunk = InTrunk;
	Wallet = InWallet;
}

void UFlipperTradeRule::Bind(UFlipperHeldItemController* Controller)
{
	if (!Controller)
	{
		return;
	}

	TWeakObjectPtr<UFlipperTradeRule> WeakThis(this);
	Controller->TransferApprover = [WeakThis](const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance& Item) {
		return !WeakThis.IsValid() || WeakThis->CanTransfer(From, To, Item);
	};
	Controller->TransferCommitted = [WeakThis](const UInventoryGrid* From, const UInventoryGrid* To, FFlipperItemInstance& Item) {
		if (WeakThis.IsValid())
		{
			WeakThis->CommitTransfer(From, To, Item);
		}
	};
}

bool UFlipperTradeRule::CanTransfer(const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance& Item) const
{
	// Table to Trunk needs Wallet->CanAfford(Item.AskPrice); Trunk to Table needs Item.bBoughtThisPhase;
	// every other pair is allowed.
	if (!bOpen && From != To)
	{
		return false;
	}

	if (From == Table && To == Trunk)
	{
		return Wallet && Wallet->CanAfford(Item.AskPrice);
	}

	if (From == Trunk && To == Table)
	{
		return Item.bBoughtThisPhase;
	}

	return true;
}

void UFlipperTradeRule::CommitTransfer(const UInventoryGrid* From, const UInventoryGrid* To, FFlipperItemInstance& Item)
{
	// Table to Trunk: Wallet->TrySpend(AskPrice), then PaidPrice = AskPrice and bBoughtThisPhase = true.
	// Trunk to Table: Wallet->Add(PaidPrice), then PaidPrice = 0 and bBoughtThisPhase = false. Other pairs: nothing.
	if (From == Table && To == Trunk)
	{
		if (ensureMsgf(Wallet && Wallet->TrySpend(Item.AskPrice), TEXT("CommitTransfer: the buy was approved but the wallet could not pay.")))
		{
			Item.PaidPrice = Item.AskPrice;
			Item.bBoughtThisPhase = true;
		}
	}
	else if (From == Trunk && To == Table)
	{
		if (Wallet)
		{
			Wallet->Add(Item.PaidPrice);
		}
		Item.PaidPrice = 0;
		Item.bBoughtThisPhase = false;
	}
}
