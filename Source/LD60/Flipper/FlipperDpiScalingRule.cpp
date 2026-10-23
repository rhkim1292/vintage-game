// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperDpiScalingRule.h"
#include "FlipperTestPlayerController.h"

float UFlipperDpiScalingRule::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	const FVector2D Block = AFlipperTestPlayerController::GetLayoutSize();
	const float		Usable = 1.f - 2.f * MarginFraction;
	const float		ScaleToFitHeight = Size.Y * Usable / Block.Y;
	const float		ScaleToFitWidth = Size.X * Usable / Block.X;
	return FMath::Max(FMath::Min(ScaleToFitHeight, ScaleToFitWidth), 0.01f);
}
