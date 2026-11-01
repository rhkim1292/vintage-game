// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperTradeRule.h"
#include "FlipperCollectorBoard.h"
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
	Controller->TransferConsumes = [WeakThis](const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance&) {
		return WeakThis.IsValid() && WeakThis->IsSale(From, To);
	};

	// The buyer cards: the controller asks, pays and picks through the rule, like the grid moves above.
	Controller->BuyerApprover = [WeakThis](int32 Buyer, const FFlipperItemInstance& Item) {
		return WeakThis.IsValid() && WeakThis->CanSellTo(Buyer, Item);
	};
	Controller->BuyerCommitted = [WeakThis](int32 Buyer, FFlipperItemInstance& Item) {
		if (WeakThis.IsValid())
		{
			WeakThis->CommitSale(Buyer, Item);
		}
	};
	Controller->BuyerChooser = [WeakThis](const FFlipperItemInstance& Item) {
		return WeakThis.IsValid() ? WeakThis->FindBestBuyer(Item) : FlipperBuyer::PawnShop;
	};
}

int32 UFlipperTradeRule::GetOffer(const FFlipperItemInstance& Item) const
{
	return FlipperPricing::GetOffer(Item, PricingConfig, OfferMultiplier);
}

bool UFlipperTradeRule::IsSale(const UInventoryGrid* From, const UInventoryGrid* To) const
{
	return bSellOpen && From == Trunk && To == Table && From != To;
}

bool UFlipperTradeRule::CanTransfer(const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance& Item) const
{
	// Table to Trunk needs Wallet->CanAfford(Item.AskPrice); Trunk to Table needs Item.bBoughtThisPhase;
	// every other pair is allowed.
	if (IsSale(From, To))
	{
		return true;
	}

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
	if (IsSale(From, To))
	{
		if (Wallet)
		{
			Wallet->Add(GetOffer(Item));
		}
	}
	else if (From == Table && To == Trunk)
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

bool UFlipperTradeRule::CanSellTo(int32 Buyer, const FFlipperItemInstance& Item) const
{
	// TODO: see the contract in the header. Selling closed -> false; the pawn shop -> true; a collector -> on the board,
	// not sold, and FlipperRequests::Matches; anything else -> false.
	if (!IsSellOpen())
	{
		return false;
	}

	if (Buyer == FlipperBuyer::PawnShop)
	{
		return true;
	}

	if (!Board)
	{
		return false;
	}

	const FFlipperCollector* Collector = Board->Find(Buyer);

	if (Collector && !Collector->bSold && FlipperRequests::Matches(Item, Collector->Request))
	{
		return true;
	}

	return false;
}

int32 UFlipperTradeRule::GetPayout(int32 Buyer, const FFlipperItemInstance& Item) const
{
	// Mood and premium (or pawn fraction) reach GetOffer as ONE multiplier, so the offer is rounded once.
	if (Buyer == FlipperBuyer::PawnShop)
	{
		return FlipperPricing::GetOffer(Item, PricingConfig, OfferMultiplier * PricingConfig.PawnShopFraction);
	}

	// Find is null for no board, a negative index and an index past the last card: all of them pay nothing.
	const FFlipperCollector* Collector = Board ? Board->Find(Buyer) : nullptr;
	if (!Collector)
	{
		return 0;
	}

	return FlipperRequests::GetCollectorOffer(Item, Collector->Request, PricingConfig, RequestConfig, OfferMultiplier);
}

int32 UFlipperTradeRule::FindBestBuyer(const FFlipperItemInstance& Item) const
{
	// TODO: the best-paying collector that CanSellTo says yes to (ties: the lower index), else the pawn shop.
	return FlipperBuyer::PawnShop;
}

void UFlipperTradeRule::CommitSale(int32 Buyer, FFlipperItemInstance& Item)
{
	// TODO: Wallet->Add(GetPayout(Buyer, Item)); and for a collector, Board->MarkSold(Buyer).
}
