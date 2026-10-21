// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace FlipperTestHelpers
{
	/**
	 *  The pointer position, in fractional cell units, at the middle of Cell: what a click or hover over the middle of
	 *  that cell reports to the held-item controller (see UFlipperHeldItemController::GetDropOrigin).
	 */
	inline FVector2D CellCentre(int32 X, int32 Y)
	{
		return FVector2D(X + 0.5, Y + 0.5);
	}

	inline FVector2D CellCentre(FIntPoint Cell)
	{
		return CellCentre(Cell.X, Cell.Y);
	}
} // namespace FlipperTestHelpers
