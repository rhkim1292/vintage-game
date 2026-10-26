// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperRandom.h"

int32 FlipperRandom::RollWeightedIndex(const TArray<float>& Weights, FRandomStream& Random)
{
	float TotalWeight = 0.f;
	for (const float Weight : Weights)
	{
		TotalWeight += FMath::Max(Weight, 0.f);
	}

	// Same running-total walk as RollDefinition: each index owns the stretch [previous total, new total).
	const float Roll = Random.GetFraction() * TotalWeight;
	float		RunningTotal = 0.f;
	int32		LastUsable = INDEX_NONE;
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		if (Weights[Index] <= 0.f)
		{
			continue;
		}
		RunningTotal += Weights[Index];
		LastUsable = Index;
		if (Roll < RunningTotal)
		{
			return Index;
		}
	}

	// Only reachable through float rounding (or with no weight at all): give the leftover to the last index that has weight.
	return LastUsable;
}
