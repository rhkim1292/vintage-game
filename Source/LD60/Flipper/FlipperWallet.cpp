// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperWallet.h"

void UFlipperWallet::Init(int32 StartingCash)
{
	Cash = FMath::Max(0, StartingCash);
}

bool UFlipperWallet::TrySpend(int32 Amount)
{
	if (!CanAfford(Amount))
	{
		return false;
	}
	Cash -= Amount;
	OnChanged.Broadcast(Cash);
	return true;
}

void UFlipperWallet::Add(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	Cash += Amount;
	OnChanged.Broadcast(Cash);
}
