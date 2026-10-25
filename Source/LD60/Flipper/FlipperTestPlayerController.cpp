// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperTestPlayerController.h"
#include "Components/InputComponent.h"
#include "Engine/ObjectLibrary.h"
#include "FlipperItemGenerator.h"
#include "FlipperCashWidget.h"
#include "FlipperDayHud.h"
#include "FlipperDayLoop.h"
#include "FlipperEndScreen.h"
#include "FlipperTradeRule.h"
#include "FlipperWallet.h"
#include "Framework/Application/SlateApplication.h"
#include "FlipperHeldItemController.h"
#include "FlipperHeldItemOverlay.h"
#include "FlipperItemDefinition.h"
#include "FlipperItemTooltip.h"
#include "InventoryGrid.h"
#include "InventoryGridWidget.h"

void AFlipperTestPlayerController::BeginPlay()
{
	Super::BeginPlay();

	Trunk = NewObject<UInventoryGrid>(this);
	Trunk->Init(TrunkColumns, TrunkRows);
	SellerTable = NewObject<UInventoryGrid>(this);
	SellerTable->Init(SellerColumns, SellerRows);

	HeldItemController = NewObject<UFlipperHeldItemController>(this);
	HeldItemController->Init(5.f);

	if (!FillRolledItems())
	{
		FillTestItems();
	}

	// Buying and refunds (Docs/FlipperSpec.md, Feature 3): the player starts with the starting cash and an empty trunk.
	Wallet = NewObject<UFlipperWallet>(this);
	Wallet->Init(FFlipperPricingConfig().StartingCash);
	TradeRule = NewObject<UFlipperTradeRule>(this);
	TradeRule->Init(SellerTable, Trunk, Wallet);
	TradeRule->Bind(HeldItemController);

	TrunkWidget = CreateWidget<UInventoryGridWidget>(this);
	TrunkWidget->Setup(Trunk, HeldItemController, SellerTable, CellSize);
	TrunkWidget->AddToViewport(0);

	SellerTableWidget = CreateWidget<UInventoryGridWidget>(this);
	SellerTableWidget->Setup(SellerTable, HeldItemController, Trunk, CellSize);
	SellerTableWidget->AddToViewport(0);

	Overlay = CreateWidget<UFlipperHeldItemOverlay>(this);
	Overlay->Setup(HeldItemController, CellSize);
	Overlay->AddToViewport(10);

	Tooltip = CreateWidget<UFlipperItemTooltip>(this);
	Tooltip->Setup(HeldItemController, TradeRule);
	Tooltip->AddToViewport(11);

	CashWidget = CreateWidget<UFlipperCashWidget>(this);
	CashWidget->Setup(Wallet);
	CashWidget->AddToViewport(12);

	// The day loop (Docs/FlipperSpec.md, Feature 4): Market, Garage, Sell for each day. It rolls the seller table itself.
	DayLoop = NewObject<UFlipperDayLoop>(this);
	DayLoop->Init(FFlipperDayConfig(), Trunk, SellerTable, Generator, HeldItemController, TradeRule, FMath::Rand());
	DayLoop->Bind(HeldItemController);
	DayLoop->SetWallet(Wallet);
	DayLoop->OnPhaseChanged.AddUObject(this, &AFlipperTestPlayerController::OnPhaseChanged);

	DayHud = CreateWidget<UFlipperDayHud>(this);
	DayHud->Setup(DayLoop);
	DayHud->AddToViewport(12);

	PhaseBanner = CreateWidget<UFlipperPhaseBanner>(this);
	PhaseBanner->Setup(DayLoop);
	PhaseBanner->AddToViewport(13);

	// The win/lose screen (Feature 6), collapsed until the run is Finished.
	EndScreen = CreateWidget<UFlipperEndScreen>(this);
	EndScreen->Setup(DayLoop);
	EndScreen->AddToViewport(14);

	LayoutWidgets();

	DayLoop->StartRun();

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

FVector2D AFlipperTestPlayerController::GetLayoutSize()
{
	const float GridsTop = UFlipperDayHud::PanelHeight + RowGap;
	const float RightColumnX = TrunkColumns * CellSize + ColumnGap;
	return FVector2D(RightColumnX + FMath::Max(UFlipperDayHud::PanelWidth, SellerColumns * CellSize), GridsTop + FMath::Max(TrunkRows, SellerRows) * CellSize);
}

void AFlipperTestPlayerController::LayoutWidgets()
{
	const FVector2D TrunkSize = TrunkWidget->GetPixelSize();
	const FVector2D TableSize = SellerTableWidget->GetPixelSize();
	const FVector2D HudSize(UFlipperDayHud::PanelWidth, UFlipperDayHud::PanelHeight);
	const FVector2D CashSize(UFlipperCashWidget::PanelWidth, UFlipperCashWidget::PanelHeight);

	// The block, in its own space (origin at its top-left): the cash panel and the trunk on the left, the day HUD above
	// the seller table on the right. Its size is only used to find its centre.
	const float		GridsTop = HudSize.Y + RowGap;
	const float		RightColumnX = TrunkSize.X + ColumnGap;
	const FVector2D BlockSize = GetLayoutSize();

	// Every piece is anchored to the viewport's centre and offset from it, so the block stays centred whatever the window size.
	// SetPositionInViewport and SetDesiredSizeInViewport both reset the anchors to the top-left corner, so the anchors go last.
	auto Place = [&BlockSize](UUserWidget* Widget, FVector2D BlockPos, FVector2D Size)
	{
		Widget->SetPositionInViewport(BlockPos - BlockSize * 0.5, false);
		Widget->SetDesiredSizeInViewport(Size);
		Widget->SetAnchorsInViewport(FAnchors(0.5f, 0.5f));
	};

	Place(CashWidget, FVector2D(0.f, (HudSize.Y - CashSize.Y) * 0.5f), CashSize);
	Place(DayHud, FVector2D(RightColumnX, 0.f), HudSize);
	Place(TrunkWidget, FVector2D(0.f, GridsTop), TrunkSize);
	Place(SellerTableWidget, FVector2D(RightColumnX, GridsTop), TableSize);
}

void AFlipperTestPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (DayLoop)
	{
		DayLoop->Tick(DeltaTime);
	}
}

void AFlipperTestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFlipperTestPlayerController::OnRotatePressed);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFlipperTestPlayerController::OnRotatePressed);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AFlipperTestPlayerController::OnCancelPressed);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFlipperTestPlayerController::OnLeftPressed);
}

UFlipperItemDefinition* AFlipperTestPlayerController::MakeDefinition(const TCHAR* Name, int32 Width, int32 Height)
{
	UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>(this);
	Definition->DisplayName = FText::FromString(Name);
	Definition->Width = Width;
	Definition->Height = Height;
	TestDefinitions.Add(Definition);
	return Definition;
}

void AFlipperTestPlayerController::FillTestItems()
{
	UFlipperItemDefinition* Cartridge = MakeDefinition(TEXT("Cartridge"), 1, 1);
	UFlipperItemDefinition* Walkman = MakeDefinition(TEXT("Walkman"), 2, 1);
	UFlipperItemDefinition* GameBoy = MakeDefinition(TEXT("Game Boy"), 2, 3);
	UFlipperItemDefinition* Console = MakeDefinition(TEXT("Console"), 3, 2);

	int32 NextId = 1;
	auto  Stock = [&NextId](UInventoryGrid* Grid, UFlipperItemDefinition* Definition)
	{
		FFlipperItemInstance Item;
		Item.Id = NextId++;
		Item.Definition = Definition;
		FIntPoint Origin;
		bool	  bRotated = false;
		if (Grid->FindFirstFit(Item, Origin, bRotated))
		{
			Grid->TryPlace(Item, Origin, bRotated);
		}
	};

	for (UFlipperItemDefinition* Definition : { Console, GameBoy, GameBoy, Walkman, Walkman, Cartridge, Cartridge, Cartridge })
	{
		Stock(Trunk, Definition);
	}
	for (UFlipperItemDefinition* Definition : { Walkman, Cartridge, Console })
	{
		Stock(SellerTable, Definition);
	}
}

bool AFlipperTestPlayerController::FillRolledItems()
{
	UObjectLibrary* Library = UObjectLibrary::CreateLibrary(UFlipperItemDefinition::StaticClass(), false, GIsEditor);
	Library->LoadAssetsFromPath(TEXT("/Game/Flipper/Items"));
	TArray<UFlipperItemDefinition*> Definitions;
	Library->GetObjects<UFlipperItemDefinition>(Definitions);
	if (Definitions.IsEmpty())
	{
		return false;
	}

	// Traits are optional: with no assets under /Game/Flipper/Traits, no item gets one.
	UObjectLibrary* TraitLibrary = UObjectLibrary::CreateLibrary(UFlipperTraitDefinition::StaticClass(), false, GIsEditor);
	TraitLibrary->LoadAssetsFromPath(TEXT("/Game/Flipper/Traits"));
	TArray<UFlipperTraitDefinition*> Traits;
	TraitLibrary->GetObjects<UFlipperTraitDefinition>(Traits);

	Generator = NewObject<UFlipperItemGenerator>(this);
	Generator->Init(Definitions, FFlipperGenerationConfig(), FFlipperPricingConfig(), Traits);

	// The trunk starts empty (everything in it has to be bought) and the day loop rolls each day's seller table.
	return true;
}

void AFlipperTestPlayerController::OnPhaseChanged(EFlipperPhase NewPhase, int32 Day)
{
	if (SellerTableWidget)
	{
		// Collapsed, not just invisible: the grid widget then takes no clicks, hovers or drops either.
		SellerTableWidget->SetVisibility(NewPhase == EFlipperPhase::Garage ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void AFlipperTestPlayerController::OnRotatePressed()
{
	// A right-click over a grid is handled by the grid widget and never gets here.
	if (HeldItemController)
	{
		HeldItemController->Rotate();
	}
}

void AFlipperTestPlayerController::OnCancelPressed()
{
	if (HeldItemController)
	{
		HeldItemController->Cancel();
	}
}

void AFlipperTestPlayerController::OnLeftPressed()
{
	// A left click over a grid is handled by the grid widget. One that reaches here landed on empty space:
	// while holding, that is an invalid drop and snaps the item back.
	if (HeldItemController && HeldItemController->IsHolding())
	{
		HeldItemController->PointerDown(nullptr, FVector2D::ZeroVector,FSlateApplication::Get().GetCursorPos());
	}
}
