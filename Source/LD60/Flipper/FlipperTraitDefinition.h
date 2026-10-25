// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FlipperTraitDefinition.generated.h"

/**
 *  Static description of one trait (e.g. "Boxed"): a named extra property that an item may roll, up to two per item
 *  (Docs/FlipperItemDataFull-design.md). A trait changes an item's price and carries a Gameplay Tag so that
 *  collector requests can match it later; it has no other effect.
 */
UCLASS(BlueprintType)
class UFlipperTraitDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Name shown in the tooltip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trait")
	FText DisplayName;

	/** Multiplies the item's Value. Above 1 is a good trait (Boxed x1.4), below 1 a bad one (Missing parts x0.6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trait", meta = (ClampMin = 0))
	float ValueMultiplier = 1.f;

	/** What collector requests match on, e.g. Item.Trait.Boxed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trait", meta = (Categories = "Item.Trait"))
	FGameplayTag Tag;

	/** Colour of the trait's line in the tooltip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trait")
	FLinearColor Color = FLinearColor::White;

	/** How likely this trait is to be picked, relative to the other traits. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = 0))
	float RollWeight = 1.f;
};
