// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DPICustomScalingRule.h"
#include "FlipperDpiScalingRule.generated.h"

/**
 *  Scales the whole UI so the Flipper screen block (AFlipperTestPlayerController::GetLayoutSize) fits the viewport with a
 *  margin on every side, whichever of the height or the width is the tighter. Chosen in DefaultEngine.ini
 *  ([/Script/Engine.UserInterfaceSettings], UIScaleRule=Custom). Slate applies the scale itself, so hit testing,
 *  the cursor maths and the centred widgets all keep working at any window size.
 */
UCLASS()
class UFlipperDpiScalingRule : public UDPICustomScalingRule
{
	GENERATED_BODY()

public:
	/** Share of the viewport's height and width left empty at each edge. */
	static constexpr float MarginFraction = 0.05f;

	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const override;
};
