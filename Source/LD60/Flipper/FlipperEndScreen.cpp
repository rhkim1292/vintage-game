// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperEndScreen.h"
#include "FlipperPaint.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	bool IsInsideRect(FVector2D Point, FVector2D Pos, FVector2D Size)
	{
		return Point.X >= Pos.X && Point.X < Pos.X + Size.X && Point.Y >= Pos.Y && Point.Y < Pos.Y + Size.Y;
	}
} // namespace

void UFlipperEndScreen::Setup(UFlipperDayLoop* InDayLoop)
{
	DayLoop = InDayLoop;
	if (DayLoop)
	{
		DayLoop->OnPhaseChanged.AddUObject(this, &UFlipperEndScreen::OnPhaseChanged);
	}
	Refresh();
}

void UFlipperEndScreen::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UFlipperEndScreen::OnPhaseChanged(EFlipperPhase NewPhase, int32 Day)
{
	Refresh();
}

void UFlipperEndScreen::Refresh()
{
	// Visible (not click-through) so the dimmed layer swallows clicks meant for the grids behind it.
	const bool bFinished = DayLoop && DayLoop->GetDay() > 0 && DayLoop->GetPhase() == EFlipperPhase::Finished;
	SetVisibility(bFinished ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

int32 UFlipperEndScreen::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!DayLoop || DayLoop->GetPhase() != EFlipperPhase::Finished)
	{
		return LayerId;
	}

	const bool		   bWon = DayLoop->GetResult() == EFlipperRunResult::Won;
	const FLinearColor Accent = bWon ? FLinearColor(0.3f, 0.9f, 0.4f, 1.f) : FLinearColor(1.f, 0.3f, 0.3f, 1.f);
	const FLinearColor Dim(0.7f, 0.7f, 0.75f, 1.f);

	const FVector2D ScreenSize = FVector2D(AllottedGeometry.GetLocalSize());
	const FVector2D Pos = GetPanelPos(ScreenSize);
	const FVector2D Size(PanelWidth, PanelHeight);

	FlipperPaint::Box(OutDrawElements, LayerId, AllottedGeometry, FVector2D::ZeroVector, ScreenSize, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	FlipperPaint::Box(OutDrawElements, LayerId + 1, AllottedGeometry, Pos, Size, FLinearColor(0.02f, 0.02f, 0.03f, 0.97f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 2, AllottedGeometry, Pos, Size, 2.f, Accent);

	// Roughly centre the headline: a 36 pt glyph is about 20 px wide on average.
	const FString Headline = bWon ? TEXT("You made rent!") : TEXT("Evicted!");
	FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, Pos + FVector2D((PanelWidth - Headline.Len() * 20.f) * 0.5f, 28.f), Headline, Accent, 36);

	FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, Pos + FVector2D(150.f, 108.f), FString::Printf(TEXT("Final cash:  $%d"), DayLoop->GetCash()), FLinearColor::White, 18);
	FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, Pos + FVector2D(150.f, 140.f), FString::Printf(TEXT("Rent:  $%d"), DayLoop->GetRent()), Dim, 18);
	FlipperPaint::Label(OutDrawElements, LayerId + 2, AllottedGeometry, Pos + FVector2D(150.f, 172.f), FString::Printf(TEXT("Days played:  %d"), DayLoop->GetDay()), Dim, 18);

	const FVector2D LocalCursor = AllottedGeometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
	const FVector2D ButtonPos = GetButtonPos(ScreenSize);
	const bool		bHover = IsInsideRect(LocalCursor, ButtonPos, GetButtonSize());
	FlipperPaint::Box(OutDrawElements, LayerId + 2, AllottedGeometry, ButtonPos, GetButtonSize(), bHover ? FLinearColor(0.25f, 0.25f, 0.3f, 1.f) : FLinearColor(0.12f, 0.12f, 0.15f, 1.f));
	FlipperPaint::Outline(OutDrawElements, LayerId + 3, AllottedGeometry, ButtonPos, GetButtonSize(), 1.f, Accent);
	FlipperPaint::Label(OutDrawElements, LayerId + 3, AllottedGeometry, ButtonPos + FVector2D(52.f, 11.f), TEXT("Play Again"), FLinearColor::White, 16);

	return LayerId + 4;
}

FReply UFlipperEndScreen::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	if (IsInsideRect(Local, GetButtonPos(FVector2D(InGeometry.GetLocalSize())), GetButtonSize()))
	{
		// Reloading the level builds a new controller, which rolls a new seed: a fresh run.
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
	return FReply::Handled();
}
