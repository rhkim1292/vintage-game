// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperRequests.h"
#include "FlipperCollectorBoard.generated.h"

class UFlipperCollectorDefinition;

/** Names a buyer in the Sell phase: a collector is its index on the board (0, 1, 2), the pawn shop is this. */
namespace FlipperBuyer
{
	inline constexpr int32 PawnShop = -1;
} // namespace FlipperBuyer

/**
 *  The day's collectors (Docs/FlipperCollectors-design.md): who is here, what they want and whether they have bought.
 *  Pure state: no widgets, no pricing. The day loop rolls it when a day starts and clears it when the Sell phase ends;
 *  the trade rule reads it to price and close sales, and the board widget draws it.
 */
UCLASS(BlueprintType)
class UFlipperCollectorBoard : public UObject
{
	GENERATED_BODY()

public:
	/** Replaces the board with FlipperRequests::RollBoard(...). Takes exactly the draws that function documents. */
	void Roll(const TArray<FFlipperItemInstance>& TableItems, const TArray<UFlipperCollectorDefinition*>& CollectorDefinitions, const FFlipperRequestConfig& Config, FRandomStream& Random);

	/** Adds one collector to the end of the board (a hand-made board for tests and tools; the game uses Roll). */
	void Add(const FFlipperCollector& Collector) { Collectors.Add(Collector); }

	/** Empties the board: no collectors, nothing to sell to but the pawn shop. */
	void Clear() { Collectors.Reset(); }

	const TArray<FFlipperCollector>& GetCollectors() const { return Collectors; }

	int32 Num() const { return Collectors.Num(); }

	bool IsValidCollector(int32 Index) const { return Collectors.IsValidIndex(Index); }

	/** The collector at Index, or null if there is none (so the pawn shop, -1, and out-of-range indices give null). */
	const FFlipperCollector* Find(int32 Index) const { return Collectors.IsValidIndex(Index) ? &Collectors[Index] : nullptr; }

	/** Closes the collector at Index: it has bought its one item. Does nothing for an invalid index. */
	void MarkSold(int32 Index);

private:
	UPROPERTY()
	TArray<FFlipperCollector> Collectors;
};
