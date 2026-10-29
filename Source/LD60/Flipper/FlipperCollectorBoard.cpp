// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperCollectorBoard.h"

void UFlipperCollectorBoard::Roll(const TArray<FFlipperItemInstance>& TableItems, const TArray<UFlipperCollectorDefinition*>& CollectorDefinitions, const FFlipperRequestConfig& Config, FRandomStream& Random)
{
	Collectors = FlipperRequests::RollBoard(TableItems, CollectorDefinitions, Config, Random);
}

void UFlipperCollectorBoard::MarkSold(int32 Index)
{
	if (Collectors.IsValidIndex(Index))
	{
		Collectors[Index].bSold = true;
	}
}
