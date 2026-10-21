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
	SellerTable->Init(8, 5);

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
	TrunkWidget->SetDesiredSizeInViewport(TrunkWidget->GetPixelSize());
	TrunkWidget->SetPositionInViewport(FVector2D(40.f, 80.f), false);

	SellerTableWidget = CreateWidget<UInventoryGridWidget>(this);
	SellerTableWidget->Setup(SellerTable, HeldItemController, Trunk, CellSize);
	SellerTableWidget->AddToViewport(0);
	SellerTableWidget->SetDesiredSizeInViewport(SellerTableWidget->GetPixelSize());
	SellerTableWidget->SetPositionInViewport(FVector2D(40.f + TrunkColumns * CellSize + 60.f, 80.f), false);

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
	DayHud->SetDesiredSizeInViewport(FVector2D(UFlipperDayHud::PanelWidth, UFlipperDayHud::PanelHeight));
	DayHud->SetPositionInViewport(FVector2D(40.f + TrunkColumns * CellSize + 60.f, 14.f), false);

	PhaseBanner = CreateWidget<UFlipperPhaseBanner>(this);
	PhaseBanner->Setup(DayLoop);
	PhaseBanner->AddToViewport(13);

	// The win/lose screen (Feature 6), collapsed until the run is Finished.
	EndScreen = CreateWidget<UFlipperEndScreen>(this);
	EndScreen->Setup(DayLoop);
	EndScreen->AddToViewport(14);

	DayLoop->StartRun();

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
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

	Generator = NewObject<UFlipperItemGenerator>(this);
	Generator->Init(Definitions, FFlipperGenerationConfig());

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
