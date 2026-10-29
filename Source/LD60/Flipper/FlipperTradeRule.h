// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperTypes.h"
#include "FlipperPricing.h"
#include "FlipperRequests.h"
#include "FlipperTradeRule.generated.h"

class UFlipperCollectorBoard;
class UInventoryGrid;
class UFlipperWallet;
class UFlipperHeldItemController;

/**
 *  The buy and refund rules (Docs/FlipperSpec.md, Feature 3), plugged into the held-item controller:
 *
 *    - Seller table to trunk is a BUY: it needs cash for the item's AskPrice, and completing the move charges it.
 *    - Trunk to seller table is a REFUND: allowed only for an item with bBoughtThisPhase, and completing the move
 *      gives back exactly PaidPrice.
 *    - Any other pair of grids: allowed, and costs nothing.
 *    - While selling is open (the Sell phase, Feature 5), trunk to seller table is a SALE instead: always allowed, it pays
 *      the buyer's offer and the item is used up (the controller's TransferConsumes). Buying and refunds are closed then.
 *
 *  The controller asks CanTransfer before it lets a move happen (and for the red / green ghost), and calls
 *  CommitTransfer once, just before it places the item, so CommitTransfer is the only place money changes hands.
 */
UCLASS(BlueprintType)
class UFlipperTradeRule : public UObject
{
	GENERATED_BODY()

public:
	void Init(UInventoryGrid* InTable, UInventoryGrid* InTrunk, UFlipperWallet* InWallet);

	/** Hooks CanTransfer and CommitTransfer up to the controller's TransferApprover and TransferCommitted. */
	void Bind(UFlipperHeldItemController* Controller);

	/**
	 *  The day loop closes trading outside the Market phase: while closed, CanTransfer refuses every move between two
	 *  different grids. Open by default.
	 */
	void SetOpen(bool bInOpen) { bOpen = bInOpen; }
	bool IsOpen() const { return bOpen; }

	/**
	 *  The day loop opens selling in the Sell phase. While open, a move from the trunk to the seller table (the buyer's
	 *  counter) is a sale. Closed by default.
	 */
	void SetSellOpen(bool bInSellOpen) { bSellOpen = bInSellOpen; }
	bool IsSellOpen() const { return bSellOpen; }

	/** Today's buyer mood multiplier (the day loop sets it when a day starts). 1 by default. */
	void  SetOfferMultiplier(float InMultiplier) { OfferMultiplier = InMultiplier; }
	float GetOfferMultiplier() const { return OfferMultiplier; }

	/** The numbers the offers are made from. Defaults to FFlipperPricingConfig(). */
	void SetPricingConfig(const FFlipperPricingConfig& InConfig) { PricingConfig = InConfig; }

	/** What the buyer would pay for Item today: FlipperPricing::GetOffer with the pricing config and today's multiplier. */
	int32 GetOffer(const FFlipperItemInstance& Item) const;

	/** True when moving an item from From to To is a sale: selling is open, From is the trunk and To is the seller table. */
	bool IsSale(const UInventoryGrid* From, const UInventoryGrid* To) const;

	/**
	 *  May Item move from From to To? Does not change anything.
	 *    - A sale (IsSale): true.
	 *    - Trading closed (SetOpen(false)) and From is not To: false.
	 *    - Table to Trunk: true only if the wallet can afford Item.AskPrice.
	 *    - Trunk to Table: true only if Item.bBoughtThisPhase.
	 *    - Any other pair: true.
	 */
	bool CanTransfer(const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance& Item) const;

	/**
	 *  The move is happening: do the money and update the item that is about to be placed.
	 *    - Table to Trunk: spend AskPrice, then set Item.PaidPrice = AskPrice and Item.bBoughtThisPhase = true.
	 *    - Trunk to Table: add Item.PaidPrice, then set Item.PaidPrice = 0 and Item.bBoughtThisPhase = false.
	 *    - A sale (IsSale): add GetOffer(Item) to the wallet. (The item is used up, so there is nothing to stamp on it.)
	 *    - Any other pair: nothing.
	 *  Only called after CanTransfer said yes.
	 */
	void CommitTransfer(const UInventoryGrid* From, const UInventoryGrid* To, FFlipperItemInstance& Item);

	// -----------------------------------------------------------------------------------------------------------
	// Selling to a buyer card (Docs/FlipperCollectors-design.md). A buyer is named by an int32: a collector is its
	// index on the board, the pawn shop is FlipperBuyer::PawnShop (-1). These sit beside the grid-to-grid sale above,
	// which stays until the cards are in the game.

	/** The day's collectors. Null (the default) means an empty board: only the pawn shop buys. */
	void SetBoard(UFlipperCollectorBoard* InBoard) { Board = InBoard; }

	/** The collectors' premium numbers. Defaults to FFlipperRequestConfig(). */
	void SetRequestConfig(const FFlipperRequestConfig& InConfig) { RequestConfig = InConfig; }

	/**
	 *  May Item be sold to Buyer right now? Does not change anything.
	 *    - Selling must be open (IsSellOpen), or the answer is no for every buyer.
	 *    - FlipperBuyer::PawnShop: yes, any item, any number of times.
	 *    - A collector index: yes only if it is on the board, has not bought yet (not bSold), and FlipperRequests::Matches
	 *      says the item satisfies its request. (An Untested item never satisfies must-work; Matches already says so.)
	 *    - Any other index (not on the board): no.
	 */
	bool CanSellTo(int32 Buyer, const FFlipperItemInstance& Item) const;

	/**
	 *  What Buyer would pay for Item today, whether or not CanSellTo says yes (the UI shows the payout of the buyers that
	 *  accept it). Rounded once.
	 *    - FlipperBuyer::PawnShop: FlipperPricing::GetOffer with today's multiplier times PricingConfig.PawnShopFraction.
	 *    - A collector: FlipperRequests::GetCollectorOffer with today's multiplier (it applies the request's premium).
	 *    - Any other index: 0.
	 */
	int32 GetPayout(int32 Buyer, const FFlipperItemInstance& Item) const;

	/**
	 *  Where Shift-click sends Item: the collector that pays the MOST among those CanSellTo says yes to (a tie goes to the
	 *  lower index, the first card), or FlipperBuyer::PawnShop when no collector will take it.
	 */
	int32 FindBestBuyer(const FFlipperItemInstance& Item) const;

	/**
	 *  The sale is happening: pay GetPayout(Buyer, Item) into the wallet and, for a collector, close it on the board
	 *  (MarkSold). The pawn shop stays open. The item is used up, so there is nothing to stamp on it. Only called after
	 *  CanSellTo said yes.
	 */
	void CommitSale(int32 Buyer, FFlipperItemInstance& Item);

private:
	bool  bOpen = true;
	bool  bSellOpen = false;
	float OfferMultiplier = 1.f;

	FFlipperPricingConfig PricingConfig;
	FFlipperRequestConfig RequestConfig;

	UPROPERTY()
	TObjectPtr<UFlipperCollectorBoard> Board;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> Table;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> Trunk;

	UPROPERTY()
	TObjectPtr<UFlipperWallet> Wallet;
};
