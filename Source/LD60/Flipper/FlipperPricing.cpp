// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperPricing.h"
#include "FlipperItemDefinition.h"

int32 FlipperPricing::GetValue(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config)
{
	if (!Item.Definition)
	{
		return 0;
	}

	return FMath::RoundToInt32(Item.Definition->BaseValue * GetRarityMultiplier(Item.Rarity, Config));
}

int32 FlipperPricing::GetScrapValue(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config)
{
	return FMath::RoundToInt32(GetValue(Item, Config) * Config.ScrapFraction);
}

int32 FlipperPricing::GetAskPrice(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config, FRandomStream& Random)
{
	if (!Item.Definition)
	{
		return 0;
	}

	// Pick the fraction first, then multiply once. The variation is always drawn, so every ask takes exactly one number from Random.
	const float AskFraction = Item.bTested ? Config.TestedAskFraction : Config.UntestedAskFraction;
	const float Variation = Random.FRandRange(1.f - Config.AskVariation, 1.f + Config.AskVariation);

	return FMath::Max(1, FMath::RoundToInt32(GetValue(Item, Config) * AskFraction * Variation));
}
