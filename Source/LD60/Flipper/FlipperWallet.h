// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperWallet.generated.h"

/** Broadcast after the cash changes, with the new amount. The cash HUD listens to this. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCashChanged, int32 /*NewCash*/);

/**
 *  The player's cash (Docs/FlipperSpec.md, Feature 3). Whole dollars, never below zero: the jam build has no debt,
 *  so a purchase the player cannot afford is simply refused. Pure logic, no widgets.
 */
UCLASS(BlueprintType)
class UFlipperWallet : public UObject
{
	GENERATED_BODY()

public:
	/** Sets the cash to StartingCash. Does not broadcast. */
	void Init(int32 StartingCash);

	int32 GetCash() const { return Cash; }

	/** True when the cash covers Amount (exactly enough counts). */
	bool CanAfford(int32 Amount) const { return Amount >= 0 && Cash >= Amount; }

	/** Takes Amount if the player can afford it and returns true; otherwise changes nothing and returns false. A negative Amount is refused. */
	bool TrySpend(int32 Amount);

	/** Adds Amount. A negative Amount is ignored (use TrySpend to take money). */
	void Add(int32 Amount);

	FOnCashChanged OnChanged;

private:
	int32 Cash = 0;
};
