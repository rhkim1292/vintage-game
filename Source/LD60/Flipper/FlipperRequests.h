// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FlipperTypes.h"
#include "FlipperPricing.h"
#include "FlipperRequests.generated.h"

class UFlipperCollectorDefinition;

/**
 *  Every number the collectors are made from. Kept in data (not in code) so the balance pass can tune it without
 *  recompiling. Defaults are the first guesses from Docs/FlipperCollectors-design.md.
 */
USTRUCT(BlueprintType)
struct FFlipperRequestConfig
{
	GENERATED_BODY()

	/** How many collectors come each day (fewer if the seller table has fewer items to build requests from). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectors", meta = (ClampMin = 0))
	int32 CollectorsPerDay = 3;

	/** Relative chances of a request having 1, 2 or 3 clauses. The defaults give 40% / 40% / 20%. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectors", meta = (ClampMin = 0))
	float OneClauseWeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectors", meta = (ClampMin = 0))
	float TwoClausesWeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectors", meta = (ClampMin = 0))
	float ThreeClausesWeight = 20.f;

	/** A collector pays the plain offer times the premium of its request: the harder the request, the bigger the premium. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectors", meta = (ClampMin = 0))
	float OneClausePremium = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectors", meta = (ClampMin = 0))
	float TwoClausesPremium = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectors", meta = (ClampMin = 0))
	float ThreeClausesPremium = 1.75f;

	/** The premium for a request with this many clauses: 0 clauses pays no premium (1), 3 or more use the three-clause premium. */
	float GetPremium(int32 ClauseCount) const
	{
		if (ClauseCount <= 0)
		{
			return 1.f;
		}
		switch (ClauseCount)
		{
		case 1:
			return OneClausePremium;
		case 2:
			return TwoClausesPremium;
		default:
			return ThreeClausesPremium;
		}
	}
};

/**
 *  What one collector wants: a list of clauses an item must all satisfy (Docs/FlipperCollectors-design.md). The
 *  defaults mean "no clause": a request with every field at its default is met by any item.
 */
USTRUCT(BlueprintType)
struct FFlipperRequest
{
	GENERATED_BODY()

	/** Tags the item must carry: at most one category tag (Item.Category.*) and one trait tag (Item.Trait.*). Each is a clause. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	FGameplayTagContainer RequiredTags;

	/** The item must be Tested and working. An Untested item never satisfies this, even if it happens to work. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	bool bMustWork = false;

	/** The item's condition must be at least this. Poor means no clause (every condition is at least Poor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	EFlipperCondition MinCondition = EFlipperCondition::Poor;

	/** The item's rarity must be at least this. Common means no clause. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Request")
	EFlipperRarity MinRarity = EFlipperRarity::Common;

	/** How many clauses the request has: each required tag, must-work, a minimum condition above Poor, a minimum rarity above Common. */
	int32 GetClauseCount() const
	{
		return RequiredTags.Num() + (bMustWork ? 1 : 0) + (MinCondition != EFlipperCondition::Poor ? 1 : 0) + (MinRarity != EFlipperRarity::Common ? 1 : 0);
	}
};

/** One collector on the day's board: who, what they want, and whether they have already bought. */
USTRUCT(BlueprintType)
struct FFlipperCollector
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collector")
	TObjectPtr<UFlipperCollectorDefinition> Definition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collector")
	FFlipperRequest Request;

	/** True once the collector has bought their one item. A sold collector takes no more drops. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collector")
	bool bSold = false;
};

/**
 *  Collector requests (Docs/FlipperCollectors-design.md). Pure functions: no state, no widgets.
 */
namespace FlipperRequests
{
	/**
	 *  Does Item satisfy EVERY clause of Request?
	 *    - An item's tags are the Categories of its definition plus the Tag of each non-null trait it carries.
	 *      Request.RequiredTags must all be among them.
	 *    - bMustWork: the item must be Tested AND working (bTested && bWorks). An Untested item never qualifies.
	 *    - MinCondition: the item's condition must be at least that grade (Poor < Fair < Good < Mint).
	 *    - MinRarity: the item's rarity must be at least that tier (Common < Uncommon < Rare).
	 *  An item with no Definition satisfies nothing, not even an empty request. A request with no clauses is met by
	 *  every item that has a Definition.
	 */
	bool Matches(const FFlipperItemInstance& Item, const FFlipperRequest& Request);

	/**
	 *  Builds the request for one collector from Source, a real item on the day's table, so the request can always be
	 *  met by buying something that is there. It keeps a random subset of the source's properties as clauses.
	 *
	 *  The clauses Source can supply (the "pool"):
	 *    - Category:     one of the tags in its definition's Categories (a request never has two category tags).
	 *    - Trait:        the Tag of one of its traits, only if it has a trait with a valid tag.
	 *    - Must work:    always available.
	 *    - Min condition: its condition, only if above Poor.
	 *    - Min rarity:    its rarity, only if above Common.
	 *
	 *  How many clauses: 1, 2 or 3 by Config's weights (FlipperRandom::RollWeightedIndex), capped at the size of the
	 *  pool. Which ones: that many DIFFERENT clause kinds from the pool, at random. The request is then filled in
	 *  from Source, so Source satisfies it, except that a must-work clause also needs Source to be Tested and working.
	 *
	 *  A Source with no Definition has no categories, so only the other kinds are in its pool (never crash on it).
	 *
	 *  Always takes exactly 8 numbers from Random, whatever Source is (so a seed stays aligned): 1 for the clause
	 *  count, 1 for which category, 1 for which trait, and 5 (one per clause kind) for the order the kinds are tried
	 *  in. A number is taken even when it ends up unused (no trait, a pool smaller than the count, ...).
	 */
	FFlipperRequest RollRequest(const FFlipperItemInstance& Source, const FFlipperRequestConfig& Config, FRandomStream& Random);

	/**
	 *  What a collector pays for Item: FlipperPricing::GetOffer with the multiplier MoodMultiplier times the premium of
	 *  the request's clause count, so it is rounded once. Does NOT check Matches: the caller does.
	 */
	int32 GetCollectorOffer(const FFlipperItemInstance& Item, const FFlipperRequest& Request, const FFlipperPricingConfig& Pricing, const FFlipperRequestConfig& Config, float MoodMultiplier);

	/**
	 *  Rolls the day's board from the seller table's items. Builds min(CollectorsPerDay, number of table items with a
	 *  Definition) collectors. Each takes a DIFFERENT table item as its source (so no two collectors are built from the
	 *  same item), gets a request from RollRequest, and a definition from CollectorDefinitions: an unused one while
	 *  there are any, otherwise any (the pool refills). Null entries are ignored; with none, the Definition is null.
	 *  Takes exactly 10 numbers from Random per collector built: 1 to pick the source, 8 in RollRequest, 1 to pick the
	 *  definition. None are taken for collectors that are not built.
	 */
	TArray<FFlipperCollector> RollBoard(const TArray<FFlipperItemInstance>& TableItems, const TArray<UFlipperCollectorDefinition*>& CollectorDefinitions, const FFlipperRequestConfig& Config, FRandomStream& Random);
} // namespace FlipperRequests
