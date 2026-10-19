// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperTypes.h"
#include "FlipperTradeRule.generated.h"

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
	 *  May Item move from From to To? Does not change anything.
	 *    - Table to Trunk: true only if the wallet can afford Item.AskPrice.
	 *    - Trunk to Table: true only if Item.bBoughtThisPhase.
	 *    - Any other pair: true.
	 */
	bool CanTransfer(const UInventoryGrid* From, const UInventoryGrid* To, const FFlipperItemInstance& Item) const;

	/**
	 *  The move is happening: do the money and update the item that is about to be placed.
	 *    - Table to Trunk: spend AskPrice, then set Item.PaidPrice = AskPrice and Item.bBoughtThisPhase = true.
	 *    - Trunk to Table: add Item.PaidPrice, then set Item.PaidPrice = 0 and Item.bBoughtThisPhase = false.
	 *    - Any other pair: nothing.
	 *  Only called after CanTransfer said yes.
	 */
	void CommitTransfer(const UInventoryGrid* From, const UInventoryGrid* To, FFlipperItemInstance& Item);

private:
	UPROPERTY()
	TObjectPtr<UInventoryGrid> Table;

	UPROPERTY()
	TObjectPtr<UInventoryGrid> Trunk;

	UPROPERTY()
	TObjectPtr<UFlipperWallet> Wallet;
};
