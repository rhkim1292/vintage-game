// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperDayHud.h"
#include "FlipperDayLoop.h"
#include "FlipperPaint.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	const TCHAR* GetPhaseName(EFlipperPhase Phase)
	{
		switch (Phase)
		{
		case EFlipperPhase::Market:
			return TEXT("Market");
		case EFlipperPhase::Garage:
			return TEXT("Garage");
		case EFlipperPhase::Sell:
			return TEXT("Sell");
		default:
			return TEXT("Done");
		}
	}

	FLinearColor GetPhaseColor(EFlipperPhase Phase)
	{
		switch (Phase)
		{
		case EFlipperPhase::Market:
			return FLinearColor(1.f, 0.85f, 0.3f, 1.f);
		case EFlipperPhase::Garage:
			return FLinearColor(0.4f, 0.75f, 1.f, 1.f);
		case EFlipperPhase::Sell:
			return FLinearColor(0.3f, 0.9f, 0.4f, 1.f);
		default:
			return FLinearColor(0.8f, 0.8f, 0.85f, 1.f);
		}
	}

	FString GetEndButtonLabel(EFlipperPhase Phase)
	{
		switch (Phase)
		{
		case EFlipperPhase::Market:
			return TEXT("End Market");
		case EFlipperPhase::Garage:
			return TEXT("Done");
		default:
			return TEXT("End Sell");
		}
	}

	const TCHAR* GetMoodName(EFlipperBuyerMood Mood)
	{
		switch (Mood)
		{
		case EFlipperBuyerMood::Slow:
			return TEXT("Slow");
		case EFlipperBuyerMood::Hot:
			return TEXT("Hot");
		default:
			return TEXT("Normal");
		}
	}

	FLinearColor GetMoodColor(EFlipperBuyerMood Mood)
	{
		switch (Mood)
		{
		case EFlipperBuyerMood::Slow:
			return FLinearColor(0.6f, 0.6f, 0.65f, 1.f);
		case EFlipperBuyerMood::Hot:
			return FLinearColor(0.3f, 0.9f, 0.4f, 1.f);
		default:
			return FLinearColor::White;
		}
	}

	bool IsInside(FVector2D Point, FVector2D Pos, FVector2D Size)
	{
		return Point.X >= Pos.X && Point.X < Pos.X + Size.X && Point.Y >= Pos.Y && Point.Y < Pos.Y + Size.Y;
	}
} // namespace

// ---------------------------------------------------------------------------------------------------------------

void UFlipperDayHud::Setup(UFlipperDayLoop* InDayLoop)
{
	DayLoop = InDayLoop;
}

void UFlipperDayHud::NativeConstruct()
{
	Super::NativeConstruct();
	// The End phase button handles clicks itself, so this widget must be hit-testable (the default is not).
	SetVisibility(ESlateVisibility::Visible);
}

int32 UFlipperDayHud::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!DayLoop || DayLoop->GetDay() <= 0)
	{
		return LayerId;
	}

	const EFlipperPhase Phase = DayLoop->GetPhase();
	const FLinearColor	PhaseColor = GetPhaseColor(Phase);
	const FLinearColor	Dim(0.7f, 0.7f, 0.75f, 1.f);

	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, FVector2D::ZeroVector, FVector2D(PanelWidth, PanelHeight), FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D::ZeroVector, FVector2D(PanelWidth, PanelHeight), 1.f, PhaseColor);

	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(10.f, 6.f), FString::Printf(TEXT("Day %d / %d"), DayLoop->GetDay(), DayLoop->GetConfig().DayCount), Dim);
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(100.f, 6.f), GetPhaseName(Phase), PhaseColor);

	if (DayLoop->IsTimed())
	{
		const int32 Seconds = FMath::CeilToInt32(DayLoop->GetTimeLeft());
		const FLinearColor TimerColor = DayLoop->IsWarning() ? FLinearColor(1.f, 0.25f, 0.25f, 1.f) : FLinearColor::White;
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(180.f, 6.f), FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60), TimerColor);
	}

	// The buyer's mood is known all day, so the player can plan the purchases and the tests around it.
	if (Phase != EFlipperPhase::Finished)
	{
		const EFlipperBuyerMood Mood = DayLoop->GetMood();
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(250.f, 6.f), FString::Printf(TEXT("Buyer: %s x%s"), GetMoodName(Mood), *FString::SanitizeFloat(DayLoop->GetMoodMultiplier())), GetMoodColor(Mood));
	}

	// The goal is on screen for the whole run. It turns green once the cash covers it and, on the last day, red while it does not.
	{
		const bool		   bMet = UFlipperDayLoop::IsRentMet(DayLoop->GetCash(), DayLoop->GetRent());
		const bool		   bLastDay = DayLoop->GetDay() >= DayLoop->GetConfig().DayCount;
		const FLinearColor RentColor = bMet ? FLinearColor(0.3f, 0.9f, 0.4f, 1.f) : (bLastDay ? FLinearColor(1.f, 0.25f, 0.25f, 1.f) : FLinearColor(1.f, 0.85f, 0.3f, 1.f));
		FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(390.f, 6.f), FString::Printf(TEXT("Rent $%d"), DayLoop->GetRent()), RentColor);
	}

	FString Hint;
	switch (Phase)
	{
	case EFlipperPhase::Market:
		Hint = TEXT("Drag or Shift-click items from the table to your trunk");
		break;
	case EFlipperPhase::Garage:
		Hint = FString::Printf(TEXT("Click an Untested item to test it   (tests left: %d)"), DayLoop->GetTestChargesLeft());
		break;
	case EFlipperPhase::Sell:
		Hint = TEXT("Drag or Shift-click items onto this counter to sell them");
		break;
	default:
		Hint = TEXT("The run is over");
		break;
	}
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(10.f, 30.f), Hint, Dim);

	if (Phase != EFlipperPhase::Finished)
	{
		const FVector2D LocalCursor = FlipperPaint::CursorLocal(Args, AllottedGeometry);
		const bool		bHover = IsInside(LocalCursor, GetButtonPos(), GetButtonSize());
		FlipperPaint::Box(OutDrawElements, LayerId + 1, AllottedGeometry, GetButtonPos(), GetButtonSize(), bHover ? FLinearColor(0.25f, 0.25f, 0.3f, 1.f) : FLinearColor(0.12f, 0.12f, 0.15f, 1.f));
		FlipperPaint::Outline(OutDrawElements, LayerId + 2, AllottedGeometry, GetButtonPos(), GetButtonSize(), 1.f, PhaseColor);
		FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, GetButtonPos() + FVector2D(14.f, 8.f), GetEndButtonLabel(Phase), FLinearColor::White);
	}

	return LayerId + 3;
}

FReply UFlipperDayHud::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!DayLoop || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || DayLoop->GetPhase() == EFlipperPhase::Finished)
	{
		return FReply::Unhandled();
	}

	const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	if (!IsInside(Local, GetButtonPos(), GetButtonSize()))
	{
		return FReply::Unhandled();
	}

	DayLoop->EndPhase();
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------

void UFlipperPhaseBanner::Setup(UFlipperDayLoop* InDayLoop)
{
	DayLoop = InDayLoop;
}

void UFlipperPhaseBanner::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UFlipperPhaseBanner::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	// The end screen takes over once the run is Finished.
	if (!DayLoop || DayLoop->GetDay() <= 0 || DayLoop->GetBannerTimeLeft() <= 0.f || DayLoop->GetPhase() == EFlipperPhase::Finished)
	{
		return LayerId;
	}

	const EFlipperPhase Phase = DayLoop->GetPhase();
	const FString		Text = FString::Printf(TEXT("Day %d: %s"), DayLoop->GetDay(), GetPhaseName(Phase));

	// Fade out over the last third of a second.
	const float Alpha = FMath::Clamp(DayLoop->GetBannerTimeLeft() / 0.33f, 0.f, 1.f);
	const FVector2D ScreenSize = FVector2D(AllottedGeometry.GetLocalSize());
	const FVector2D Size(440.f, 90.f);
	const FVector2D Pos((ScreenSize.X - Size.X) * 0.5f, ScreenSize.Y * 0.3f);

	FLinearColor PhaseColor = GetPhaseColor(Phase);
	PhaseColor.A = Alpha;
	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, Pos, Size, FLinearColor(0.02f, 0.02f, 0.03f, 0.85f * Alpha));
	FlipperPaint::Outline(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, 2.f, PhaseColor);
	// Roughly centre the text: a 36 pt glyph is about 20 px wide on average.
	const float TextWidth = Text.Len() * 20.f;
	FlipperPaint::Label(OutDrawElements, LayerId + 1, AllottedGeometry, Pos + FVector2D((Size.X - TextWidth) * 0.5f, 22.f), Text, PhaseColor, 36);
	return LayerId + 2;
}
