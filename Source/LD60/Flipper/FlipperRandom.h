// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Small helpers for rolling from a caller's FRandomStream, shared by the generators. */
namespace FlipperRandom
{
	/**
	 *  Picks an index into Weights by relative weight, taking exactly one number from Random. A weight of 0 (or below)
	 *  never wins. Returns INDEX_NONE when no weight is above 0 (the number is still taken).
	 */
	int32 RollWeightedIndex(const TArray<float>& Weights, FRandomStream& Random);
} // namespace FlipperRandom
