// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperPricing.h"
#include "FlipperItemDefinition.h"

float FlipperPricing::GetMoodMultiplier(EFlipperBuyerMood Mood, const FFlipperPricingConfig& Config)
{
	switch (Mood)
	{
	case EFlipperBuyerMood::Slow:
		return Config.SlowMoodMultiplier;
	case EFlipperBuyerMood::Hot:
		return Config.HotMoodMultiplier;
	default:
		return Config.NormalMoodMultiplier;
	}
}

EFlipperBuyerMood FlipperPricing::RollMood(const FFlipperPricingConfig& Config, FRandomStream& Random)
{
	const float Total = Config.SlowMoodWeight + Config.NormalMoodWeight + Config.HotMoodWeight;
	const float Pick = Random.GetFraction() * Total;
	if (Pick < Config.SlowMoodWeight)
	{
		return EFlipperBuyerMood::Slow;
	}
	if (Pick < Config.SlowMoodWeight + Config.NormalMoodWeight)
	{
		return EFlipperBuyerMood::Normal;
	}
	// Hot is the rest of the range; with every weight at 0 there is nothing to roll, so fall back to Normal.
	return Total > 0.f ? EFlipperBuyerMood::Hot : EFlipperBuyerMood::Normal;
}

int32 FlipperPricing::GetOffer(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config, float MoodMultiplier)
{
	if (!Item.Definition)
	{
		return 0;
	}

	// The buyer pays for what is known. An Untested item is offered a fixed share of its Value whether or not it works.
	float Base;
	if (!Item.bTested)
	{
		Base = GetValue(Item, Config) * Config.UntestedOfferFraction;
	}
	else if (Item.bWorks)
	{
		Base = GetValue(Item, Config);
	}
	else
	{
		Base = GetValue(Item, Config) * Config.ScrapFraction;
	}

	// Round once, at the end, so the mood does not add a second rounding error.
	return FMath::Max(1, FMath::RoundToInt32(Base * MoodMultiplier));
}

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
