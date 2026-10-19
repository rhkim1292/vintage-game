// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

/**
 *  Tiny drawing helpers shared by the Flipper widgets (grid, held-item overlay, tooltip), which all paint
 *  themselves from NativePaint instead of using child widgets. Positions are in the widget's local space.
 */
namespace FlipperPaint
{
	inline void Box(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geo, FVector2D Pos, FVector2D Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Elements, Layer, Geo.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
	}

	/** A hollow rectangle: four thin boxes. */
	inline void Outline(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geo, FVector2D Pos, FVector2D Size, float Thickness, const FLinearColor& Color)
	{
		Box(Elements, Layer, Geo, Pos, FVector2D(Size.X, Thickness), Color);
		Box(Elements, Layer, Geo, FVector2D(Pos.X, Pos.Y + Size.Y - Thickness), FVector2D(Size.X, Thickness), Color);
		Box(Elements, Layer, Geo, Pos, FVector2D(Thickness, Size.Y), Color);
		Box(Elements, Layer, Geo, FVector2D(Pos.X + Size.X - Thickness, Pos.Y), FVector2D(Thickness, Size.Y), Color);
	}

	inline void Label(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geo, FVector2D Pos, const FString& Text, const FLinearColor& Color, int32 FontSize = 12)
	{
		FSlateDrawElement::MakeText(Elements, Layer, Geo.ToPaintGeometry(FVector2f(400.f, 24.f), FSlateLayoutTransform(FVector2f(Pos))), Text, FCoreStyle::GetDefaultFontStyle("Regular", FontSize), ESlateDrawEffect::None, Color);
	}
} // namespace FlipperPaint
