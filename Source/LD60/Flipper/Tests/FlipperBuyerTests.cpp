// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for selling to buyer cards (Docs/FlipperCollectors-design.md, scaffold stage 2):
// the day's board, the trade rule's per-buyer rules, the held-item controller's card drops, and the board's life in the
// day loop. They fail until the TODOs in FlipperTradeRule.cpp, FlipperHeldItemController.cpp and FlipperDayLoop.cpp are done.
// Run them with:  Automation RunTests Flipper.Buyers

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "GameplayTagContainer.h"
	#include "FlipperCollectorBoard.h"
	#include "FlipperCollectorDefinition.h"
	#include "FlipperDayLoop.h"
	#include "FlipperHeldItemController.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperItemGenerator.h"
	#include "FlipperPricing.h"
	#include "FlipperRequests.h"
	#include "FlipperTestHelpers.h"
	#include "FlipperTradeRule.h"
	#include "FlipperWallet.h"
	#include "InventoryGrid.h"

using namespace FlipperTestHelpers;

namespace FlipperBuyerTests
{
	static FGameplayTag Tag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}

	static FFlipperRequest MakeRequest(std::initializer_list<const TCHAR*> Tags, bool bMustWork = false, EFlipperCondition MinCondition = EFlipperCondition::Poor, EFlipperRarity MinRarity = EFlipperRarity::Common)
	{
		FFlipperRequest Request;
		for (const TCHAR* TagName : Tags)
		{
			Request.RequiredTags.AddTag(Tag(TagName));
		}
		Request.bMustWork = bMustWork;
		Request.MinCondition = MinCondition;
		Request.MinRarity = MinRarity;
		return Request;
	}

	static bool SameRequest(const FFlipperRequest& A, const FFlipperRequest& B)
	{
		return A.RequiredTags == B.RequiredTags && A.bMustWork == B.bMustWork && A.MinCondition == B.MinCondition && A.MinRarity == B.MinRarity;
	}

	/**
	 *  A trunk and a seller table (both 6x4), a wallet with 100 cash, a controller and a trade rule bound to it, selling
	 *  open, and an empty board shared by the rule. Items are 1x1, Common, Good, worth 100 (so the pawn shop pays 100, a
	 *  one-clause collector 125, a two-clause one 150, a three-clause one 175).
	 */
	struct FBuyerFixture
	{
		TStrongObjectPtr<UInventoryGrid>					  Table;
		TStrongObjectPtr<UInventoryGrid>					  Trunk;
		TStrongObjectPtr<UFlipperWallet>					  Wallet;
		TStrongObjectPtr<UFlipperHeldItemController>		  Controller;
		TStrongObjectPtr<UFlipperTradeRule>					  Rule;
		TStrongObjectPtr<UFlipperCollectorBoard>			  Board;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>>	  Definitions;
		TArray<TStrongObjectPtr<UFlipperCollectorDefinition>> Names;
		int32												  NextItemId = 1;

		FBuyerFixture()
		{
			Table.Reset(NewObject<UInventoryGrid>());
			Table->Init(6, 4);
			Trunk.Reset(NewObject<UInventoryGrid>());
			Trunk->Init(6, 4);
			Wallet.Reset(NewObject<UFlipperWallet>());
			Wallet->Init(100);
			Controller.Reset(NewObject<UFlipperHeldItemController>());
			Controller->Init(5.f);
			Board.Reset(NewObject<UFlipperCollectorBoard>());
			Rule.Reset(NewObject<UFlipperTradeRule>());
			Rule->Init(Table.Get(), Trunk.Get(), Wallet.Get());
			Rule->Bind(Controller.Get());
			Rule->SetBoard(Board.Get());
			Rule->SetOpen(false);
			Rule->SetSellOpen(true);
		}

		FFlipperItemInstance MakeItem(std::initializer_list<const TCHAR*> CategoryTags = {}, bool bTested = true, bool bWorks = true, int32 BaseValue = 100)
		{
			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = 1;
			Definition->Height = 1;
			Definition->BaseValue = BaseValue;
			for (const TCHAR* CategoryTag : CategoryTags)
			{
				Definition->Categories.AddTag(Tag(CategoryTag));
			}
			Definitions.Emplace(Definition);

			FFlipperItemInstance Item;
			Item.Id = NextItemId++;
			Item.Definition = Definition;
			Item.bTested = bTested;
			Item.bWorks = bWorks;
			return Item;
		}

		FFlipperItemInstance PlaceInTrunk(FIntPoint Origin, std::initializer_list<const TCHAR*> CategoryTags = {}, bool bTested = true, bool bWorks = true, int32 BaseValue = 100)
		{
			FFlipperItemInstance Item = MakeItem(CategoryTags, bTested, bWorks, BaseValue);
			const bool			 bPlaced = Trunk->TryPlace(Item, Origin, false);
			ensureMsgf(bPlaced, TEXT("Test setup: PlaceInTrunk could not place the item"));
			return Item;
		}

		/** Adds a collector with this request to the board and returns its index. */
		int32 AddCollector(const FFlipperRequest& Request)
		{
			UFlipperCollectorDefinition* Name = NewObject<UFlipperCollectorDefinition>();
			Name->DisplayName = FText::FromString(FString::Printf(TEXT("Collector %d"), Board->Num()));
			Names.Emplace(Name);

			FFlipperCollector Collector;
			Collector.Definition = Name;
			Collector.Request = Request;
			Board->Add(Collector);
			return Board->Num() - 1;
		}

		/** Picks the item at Cell up, as a click on it does. */
		void Hold(FIntPoint Cell)
		{
			Controller->PointerDown(Trunk.Get(), CellCentre(Cell), FVector2D(0.0, 0.0));
		}
	};

	/** The day loop's fixture: a trunk, a table, a generator of 1x1 items worth 10, three names, a board shared with the rule. */
	struct FBoardLoopFixture
	{
		TStrongObjectPtr<UInventoryGrid>					  Trunk;
		TStrongObjectPtr<UInventoryGrid>					  Table;
		TStrongObjectPtr<UFlipperWallet>					  Wallet;
		TStrongObjectPtr<UFlipperHeldItemController>		  Controller;
		TStrongObjectPtr<UFlipperTradeRule>					  Rule;
		TStrongObjectPtr<UFlipperItemGenerator>				  Generator;
		TStrongObjectPtr<UFlipperCollectorBoard>			  Board;
		TStrongObjectPtr<UFlipperDayLoop>					  Loop;
		TArray<TStrongObjectPtr<UFlipperItemDefinition>>	  Definitions;
		TArray<TStrongObjectPtr<UFlipperCollectorDefinition>> Names;

		explicit FBoardLoopFixture(int32 Seed, bool bWithBoard = true)
		{
			Trunk.Reset(NewObject<UInventoryGrid>());
			Trunk->Init(6, 4);
			Table.Reset(NewObject<UInventoryGrid>());
			Table->Init(8, 5);
			Wallet.Reset(NewObject<UFlipperWallet>());
			Wallet->Init(1000);
			Controller.Reset(NewObject<UFlipperHeldItemController>());
			Controller->Init(5.f);
			Rule.Reset(NewObject<UFlipperTradeRule>());
			Rule->Init(Table.Get(), Trunk.Get(), Wallet.Get());
			Rule->Bind(Controller.Get());

			UFlipperItemDefinition* Definition = NewObject<UFlipperItemDefinition>();
			Definition->Width = 1;
			Definition->Height = 1;
			Definition->BaseValue = 10;
			Definition->SpawnWeight = 1.f;
			Definitions.Emplace(Definition);

			Generator.Reset(NewObject<UFlipperItemGenerator>());
			Generator->Init({ Definition }, FFlipperGenerationConfig());

			TArray<UFlipperCollectorDefinition*> RawNames;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				UFlipperCollectorDefinition* Name = NewObject<UFlipperCollectorDefinition>();
				Name->DisplayName = FText::FromString(FString::Printf(TEXT("Collector %d"), Index));
				Names.Emplace(Name);
				RawNames.Add(Name);
			}

			Board.Reset(NewObject<UFlipperCollectorBoard>());

			FFlipperDayConfig Config;
			Config.DayCount = 3;
			Loop.Reset(NewObject<UFlipperDayLoop>());
			Loop->Init(Config, Trunk.Get(), Table.Get(), Generator.Get(), Controller.Get(), Rule.Get(), Seed);
			Loop->Bind(Controller.Get());
			if (bWithBoard)
			{
				Loop->SetBoard(Board.Get());
				Loop->SetCollectorDefinitions(RawNames);
				Rule->SetBoard(Board.Get());
			}
		}
	};
} // namespace FlipperBuyerTests

	#define FLIPPER_BUYER_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperBuyerTests;

// ---------------------------------------------------------------------------------------------------------------
// The board

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBoardBasicsTest, "Flipper.Buyers.BoardRollClearAndSold", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBoardBasicsTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	TArray<FFlipperItemInstance> TableItems;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		TableItems.Add(F.MakeItem({ TEXT("Item.Category.Handheld") }));
	}
	TArray<UFlipperCollectorDefinition*> NoNames;
	FRandomStream						 Random(4);

	F.Board->Roll(TableItems, NoNames, FFlipperRequestConfig(), Random);
	TestEqual(TEXT("three collectors come each day"), F.Board->Num(), 3);
	TestFalse(TEXT("none has bought yet"), F.Board->GetCollectors()[0].bSold);

	F.Board->MarkSold(1);
	TestTrue(TEXT("the sold collector is closed"), F.Board->Find(1)->bSold);
	TestFalse(TEXT("the others are not"), F.Board->Find(0)->bSold);

	TestNull(TEXT("the pawn shop is not on the board"), F.Board->Find(FlipperBuyer::PawnShop));
	TestNull(TEXT("an index past the end is not on the board"), F.Board->Find(3));
	F.Board->MarkSold(FlipperBuyer::PawnShop);
	F.Board->MarkSold(99);
	TestEqual(TEXT("marking an invalid index changes nothing"), F.Board->Num(), 3);

	F.Board->Clear();
	TestEqual(TEXT("clearing empties the board"), F.Board->Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The trade rule, per buyer

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerCanSellPawnShopTest, "Flipper.Buyers.PawnShopTakesAnythingWhileSellingIsOpen", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerCanSellPawnShopTest::RunTest(const FString& Parameters)
{
	FBuyerFixture			   F;
	const FFlipperItemInstance Plain = F.MakeItem();
	const FFlipperItemInstance Dead = F.MakeItem({}, true, false);
	const FFlipperItemInstance Untested = F.MakeItem({}, false, true);

	TestTrue(TEXT("a plain item"), F.Rule->CanSellTo(FlipperBuyer::PawnShop, Plain));
	TestTrue(TEXT("a dead item"), F.Rule->CanSellTo(FlipperBuyer::PawnShop, Dead));
	TestTrue(TEXT("an Untested item"), F.Rule->CanSellTo(FlipperBuyer::PawnShop, Untested));

	F.Rule->SetSellOpen(false);
	TestFalse(TEXT("nothing is bought while selling is closed"), F.Rule->CanSellTo(FlipperBuyer::PawnShop, Plain));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerCanSellCollectorTest, "Flipper.Buyers.CollectorTakesOnlyAMatchingItem", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerCanSellCollectorTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  WantsHandheld = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	const int32	  WantsWorkingConsole = F.AddCollector(MakeRequest({ TEXT("Item.Category.Gaming") }, true));

	const FFlipperItemInstance Handheld = F.MakeItem({ TEXT("Item.Category.Handheld") });
	const FFlipperItemInstance Console = F.MakeItem({ TEXT("Item.Category.Gaming") });
	const FFlipperItemInstance UntestedConsole = F.MakeItem({ TEXT("Item.Category.Gaming") }, false, true);

	TestTrue(TEXT("a handheld goes to the handheld collector"), F.Rule->CanSellTo(WantsHandheld, Handheld));
	TestFalse(TEXT("a console does not"), F.Rule->CanSellTo(WantsHandheld, Console));
	TestTrue(TEXT("a working console goes to the console collector"), F.Rule->CanSellTo(WantsWorkingConsole, Console));
	TestFalse(TEXT("an Untested console does not: it has not been shown to work"), F.Rule->CanSellTo(WantsWorkingConsole, UntestedConsole));

	TestFalse(TEXT("an index that is not on the board is refused"), F.Rule->CanSellTo(2, Handheld));
	TestFalse(TEXT("so is a negative index that is not the pawn shop"), F.Rule->CanSellTo(-2, Handheld));

	F.Rule->SetSellOpen(false);
	TestFalse(TEXT("a collector buys nothing while selling is closed"), F.Rule->CanSellTo(WantsHandheld, Handheld));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerPayoutTest, "Flipper.Buyers.PayoutPerBuyer", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerPayoutTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  OneClause = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	const int32	  TwoClauses = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }, true));
	const int32	  ThreeClauses = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }, true, EFlipperCondition::Good));

	const FFlipperItemInstance Item = F.MakeItem({ TEXT("Item.Category.Handheld") });
	TestEqual(TEXT("the pawn shop pays the plain offer"), F.Rule->GetPayout(FlipperBuyer::PawnShop, Item), 100);
	TestEqual(TEXT("one clause pays x1.25"), F.Rule->GetPayout(OneClause, Item), 125);
	TestEqual(TEXT("two clauses pay x1.5"), F.Rule->GetPayout(TwoClauses, Item), 150);
	TestEqual(TEXT("three clauses pay x1.75"), F.Rule->GetPayout(ThreeClauses, Item), 175);
	TestEqual(TEXT("a buyer that is not there pays nothing"), F.Rule->GetPayout(7, Item), 0);

	// The mood scales the pawn shop and the collectors alike.
	F.Rule->SetOfferMultiplier(1.25f);
	TestEqual(TEXT("a Hot day: the pawn shop"), F.Rule->GetPayout(FlipperBuyer::PawnShop, Item), 125);
	TestEqual(TEXT("a Hot day: two clauses, 100 x 1.25 x 1.5"), F.Rule->GetPayout(TwoClauses, Item), 188);

	// The pawn shop's own fraction (a data number, 1 by default).
	FFlipperPricingConfig Pricing;
	Pricing.PawnShopFraction = 0.8f;
	F.Rule->SetPricingConfig(Pricing);
	F.Rule->SetOfferMultiplier(1.f);
	TestEqual(TEXT("the pawn shop fraction applies to the pawn shop"), F.Rule->GetPayout(FlipperBuyer::PawnShop, Item), 80);
	TestEqual(TEXT("and not to a collector"), F.Rule->GetPayout(OneClause, Item), 125);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerRoundsOnceTest, "Flipper.Buyers.PayoutRoundsOnce", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerRoundsOnceTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  TwoClauses = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }, true));
	const FFlipperItemInstance Item = F.MakeItem({ TEXT("Item.Category.Handheld") }, true, true, 11);

	// Slow day: 11 x 0.8 x 1.5 = 13.2 -> 13. Rounding the mood first (8.8 -> 9) and the premium after would give 14.
	F.Rule->SetOfferMultiplier(0.8f);
	TestEqual(TEXT("mood and premium go in as one multiplier"), F.Rule->GetPayout(TwoClauses, Item), 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerBestTest, "Flipper.Buyers.ShiftClickGoesToTheBestPayingCollector", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerBestTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  OneClause = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	const int32	  TwoClauses = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }, true));
	const int32	  OtherCategory = F.AddCollector(MakeRequest({ TEXT("Item.Category.Gaming") }, true, EFlipperCondition::Good, EFlipperRarity::Rare));
	TestEqual(TEXT("test setup: the cards are 0, 1 and 2"), OneClause + TwoClauses + OtherCategory, 3);

	const FFlipperItemInstance Handheld = F.MakeItem({ TEXT("Item.Category.Handheld") });
	TestEqual(TEXT("the two-clause collector pays more than the one-clause one"), F.Rule->FindBestBuyer(Handheld), TwoClauses);

	// Untested: the two-clause card needs a working item, so only the one-clause card takes it.
	const FFlipperItemInstance UntestedHandheld = F.MakeItem({ TEXT("Item.Category.Handheld") }, false, true);
	TestEqual(TEXT("an Untested item goes to the card that does not need it working"), F.Rule->FindBestBuyer(UntestedHandheld), OneClause);

	// A closed card is skipped.
	F.Board->MarkSold(TwoClauses);
	TestEqual(TEXT("a collector that has bought is skipped"), F.Rule->FindBestBuyer(Handheld), OneClause);

	// Nothing matches: the pawn shop.
	const FFlipperItemInstance Plain = F.MakeItem({ TEXT("Item.Category.Photo") });
	TestEqual(TEXT("an item no collector wants goes to the pawn shop"), F.Rule->FindBestBuyer(Plain), FlipperBuyer::PawnShop);

	F.Board->MarkSold(OneClause);
	TestEqual(TEXT("with every matching card closed, the pawn shop"), F.Rule->FindBestBuyer(Handheld), FlipperBuyer::PawnShop);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerTieTest, "Flipper.Buyers.AFirstCardWinsATie", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerTieTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));

	const FFlipperItemInstance Handheld = F.MakeItem({ TEXT("Item.Category.Handheld") });
	TestEqual(TEXT("two identical offers: the first card"), F.Rule->FindBestBuyer(Handheld), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerCommitTest, "Flipper.Buyers.ACollectorBuysOnce", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerCommitTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  Collector = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	FFlipperItemInstance Item = F.MakeItem({ TEXT("Item.Category.Handheld") });

	F.Rule->CommitSale(Collector, Item);
	TestEqual(TEXT("the collector paid 125 on top of the 100 cash"), F.Wallet->GetCash(), 225);
	TestTrue(TEXT("and has bought: the card is closed"), F.Board->Find(Collector)->bSold);
	TestFalse(TEXT("a second item is refused"), F.Rule->CanSellTo(Collector, Item));

	F.Rule->CommitSale(FlipperBuyer::PawnShop, Item);
	F.Rule->CommitSale(FlipperBuyer::PawnShop, Item);
	TestEqual(TEXT("the pawn shop pays every time (2 x 100)"), F.Wallet->GetCash(), 425);
	TestTrue(TEXT("and is never closed"), F.Rule->CanSellTo(FlipperBuyer::PawnShop, Item));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The held-item controller, dropping on a card

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerDropTest, "Flipper.Buyers.DroppingOnACardSells", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerDropTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  Collector = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	F.PlaceInTrunk(FIntPoint(0, 0), { TEXT("Item.Category.Handheld") });

	F.Hold(FIntPoint(0, 0));
	TestTrue(TEXT("the card would accept the held item"), F.Controller->CanSellHeldTo(Collector));

	TestTrue(TEXT("the drop sells"), F.Controller->DropOnBuyer(Collector));
	TestEqual(TEXT("the collector paid 125"), F.Wallet->GetCash(), 225);
	TestFalse(TEXT("nothing is held any more"), F.Controller->IsHolding());
	TestNull(TEXT("the item left the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
	TestTrue(TEXT("the card is closed"), F.Board->Find(Collector)->bSold);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerRefusedDropTest, "Flipper.Buyers.ARefusedDropSnapsBack", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerRefusedDropTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  WantsConsole = F.AddCollector(MakeRequest({ TEXT("Item.Category.Gaming") }));
	F.PlaceInTrunk(FIntPoint(2, 1), { TEXT("Item.Category.Handheld") });

	F.Hold(FIntPoint(2, 1));
	TestFalse(TEXT("the card would refuse the held item"), F.Controller->CanSellHeldTo(WantsConsole));
	TestFalse(TEXT("the drop is refused"), F.Controller->DropOnBuyer(WantsConsole));
	TestEqual(TEXT("no money moved"), F.Wallet->GetCash(), 100);
	TestFalse(TEXT("nothing is held"), F.Controller->IsHolding());
	TestNotNull(TEXT("the item is back where it was"), F.Trunk->FindItemAt(FIntPoint(2, 1)));
	TestFalse(TEXT("and the card is still open"), F.Board->Find(WantsConsole)->bSold);

	// A card that has already bought refuses too, even for a matching item.
	const int32 Open = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	F.Board->MarkSold(Open);
	F.Hold(FIntPoint(2, 1));
	TestFalse(TEXT("a closed card refuses"), F.Controller->DropOnBuyer(Open));
	TestNotNull(TEXT("and the item is back again"), F.Trunk->FindItemAt(FIntPoint(2, 1)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerDropNothingTest, "Flipper.Buyers.DroppingNothingDoesNothing", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerDropNothingTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	TestFalse(TEXT("nothing held: no drop"), F.Controller->DropOnBuyer(FlipperBuyer::PawnShop));
	TestFalse(TEXT("and no ghost colour"), F.Controller->CanSellHeldTo(FlipperBuyer::PawnShop));
	TestEqual(TEXT("no money moved"), F.Wallet->GetCash(), 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerUnboundTest, "Flipper.Buyers.WithoutARuleNothingSells", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerUnboundTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	F.PlaceInTrunk(FIntPoint(0, 0));

	TStrongObjectPtr<UFlipperHeldItemController> Bare(NewObject<UFlipperHeldItemController>());
	Bare->Init(5.f);
	Bare->PointerDown(F.Trunk.Get(), CellCentre(0, 0), FVector2D(0.0, 0.0));
	TestFalse(TEXT("a controller with no approver sells to nobody"), Bare->DropOnBuyer(FlipperBuyer::PawnShop));
	TestNotNull(TEXT("and the item snapped back"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerQuickSellTest, "Flipper.Buyers.QuickSellPicksTheBestBuyer", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerQuickSellTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	const int32	  OneClause = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }));
	const int32	  TwoClauses = F.AddCollector(MakeRequest({ TEXT("Item.Category.Handheld") }, true));
	F.PlaceInTrunk(FIntPoint(0, 0), { TEXT("Item.Category.Handheld") });
	F.PlaceInTrunk(FIntPoint(1, 0), { TEXT("Item.Category.Handheld") });
	F.PlaceInTrunk(FIntPoint(2, 0), { TEXT("Item.Category.Handheld") });
	F.PlaceInTrunk(FIntPoint(3, 0), { TEXT("Item.Category.Photo") });

	TestTrue(TEXT("the first Shift-click sells"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(0, 0)));
	TestEqual(TEXT("to the two-clause collector, 150"), F.Wallet->GetCash(), 250);
	TestTrue(TEXT("who is now closed"), F.Board->Find(TwoClauses)->bSold);
	TestNull(TEXT("the item is gone from the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0)));

	TestTrue(TEXT("the second goes to the other collector, 125"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(1, 0)));
	TestEqual(TEXT("cash"), F.Wallet->GetCash(), 375);
	TestTrue(TEXT("whose card is closed too"), F.Board->Find(OneClause)->bSold);

	TestTrue(TEXT("the third finds both closed and goes to the pawn shop, 100"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(2, 0)));
	TestEqual(TEXT("cash"), F.Wallet->GetCash(), 475);

	TestTrue(TEXT("an item nobody wants goes to the pawn shop, 100"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(3, 0)));
	TestEqual(TEXT("cash"), F.Wallet->GetCash(), 575);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBuyerQuickSellRefusedTest, "Flipper.Buyers.QuickSellChangesNothingWhenItCannot", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBuyerQuickSellRefusedTest::RunTest(const FString& Parameters)
{
	FBuyerFixture F;
	F.PlaceInTrunk(FIntPoint(0, 0));
	F.PlaceInTrunk(FIntPoint(1, 0));

	TestFalse(TEXT("an empty cell has nothing to sell"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(5, 3)));
	TestFalse(TEXT("no grid, no sale"), F.Controller->QuickSell(nullptr, FIntPoint(0, 0)));

	F.Hold(FIntPoint(1, 0));
	TestFalse(TEXT("nothing sells while another item is held"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(0, 0)));
	F.Controller->Cancel();
	TestEqual(TEXT("no money moved"), F.Wallet->GetCash(), 100);

	F.Rule->SetSellOpen(false);
	TestFalse(TEXT("with selling closed even the pawn shop refuses"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(0, 0)));
	TestNotNull(TEXT("and the item stays in the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0)));
	TestEqual(TEXT("with no money moved"), F.Wallet->GetCash(), 100);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The board in the day loop

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBoardLoopRollTest, "Flipper.Buyers.TheDayLoopRollsTheBoardInTheMarket", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBoardLoopRollTest::RunTest(const FString& Parameters)
{
	FBoardLoopFixture F(5);
	TestEqual(TEXT("no collectors before the run"), F.Board->Num(), 0);

	F.Loop->StartRun();
	if (!TestTrue(TEXT("test setup: the table has items to build requests from"), F.Table->GetPlacedItems().Num() >= 3))
	{
		return true;
	}
	if (!TestEqual(TEXT("three collectors from the first second of the Market"), F.Board->Num(), 3))
	{
		return true;
	}

	// Every collector has a name, and the three names are different.
	TSet<const UFlipperCollectorDefinition*> Seen;
	for (const FFlipperCollector& Collector : F.Board->GetCollectors())
	{
		TestNotNull(TEXT("a collector has a name"), Collector.Definition.Get());
		TestTrue(TEXT("every request has at least one clause"), Collector.Request.GetClauseCount() >= 1);
		TestFalse(TEXT("nobody has bought yet"), Collector.bSold);
		Seen.Add(Collector.Definition);
	}
	TestEqual(TEXT("three different names"), Seen.Num(), 3);

	// The board stays the same through the Garage and the Sell phase.
	const FFlipperRequest Request0 = F.Board->GetCollectors()[0].Request;
	F.Loop->EndPhase();
	TestEqual(TEXT("still three in the Garage"), F.Board->Num(), 3);
	F.Loop->EndPhase();
	TestEqual(TEXT("it is the Sell phase"), F.Loop->GetPhase(), EFlipperPhase::Sell);
	TestEqual(TEXT("still three in the Sell phase"), F.Board->Num(), 3);
	TestTrue(TEXT("and unchanged"), SameRequest(F.Board->GetCollectors()[0].Request, Request0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBoardLoopClearTest, "Flipper.Buyers.TheBoardIsGoneWhenTheSellPhaseEnds", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBoardLoopClearTest::RunTest(const FString& Parameters)
{
	FBoardLoopFixture F(8);
	F.Loop->StartRun();

	// Day 1: a collector buys in the Sell phase.
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	F.Board->MarkSold(0);

	// Day 2 starts with a fresh board in which nobody has bought.
	F.Loop->EndPhase();
	TestEqual(TEXT("it is day 2, Market"), F.Loop->GetPhase(), EFlipperPhase::Market);
	TestEqual(TEXT("a new board of three"), F.Board->Num(), 3);
	for (const FFlipperCollector& Collector : F.Board->GetCollectors())
	{
		TestFalse(TEXT("nobody has bought on the new day"), Collector.bSold);
	}

	// After the last Sell phase the board is empty and stays empty.
	while (F.Loop->GetPhase() != EFlipperPhase::Finished)
	{
		F.Loop->EndPhase();
	}
	TestEqual(TEXT("the run is over: no collectors"), F.Board->Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBoardLoopSeedTest, "Flipper.Buyers.TheBoardFollowsTheSeed", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBoardLoopSeedTest::RunTest(const FString& Parameters)
{
	FBoardLoopFixture A(21);
	FBoardLoopFixture B(21);
	A.Loop->StartRun();
	B.Loop->StartRun();

	if (!TestEqual(TEXT("test setup: both boards are full"), A.Board->Num(), B.Board->Num()))
	{
		return true;
	}
	bool bSame = true;
	for (int32 Index = 0; Index < A.Board->Num(); ++Index)
	{
		bSame &= SameRequest(A.Board->GetCollectors()[Index].Request, B.Board->GetCollectors()[Index].Request);
		bSame &= A.Board->GetCollectors()[Index].Definition->DisplayName.EqualTo(B.Board->GetCollectors()[Index].Definition->DisplayName);
	}
	TestTrue(TEXT("a fixed seed gives the same day 1 board"), bSame);

	// Different seeds give different boards at least sometimes.
	bool bDifferent = false;
	for (int32 Seed = 100; Seed < 130 && !bDifferent; ++Seed)
	{
		FBoardLoopFixture Other(Seed);
		Other.Loop->StartRun();
		for (int32 Index = 0; Index < FMath::Min(A.Board->Num(), Other.Board->Num()); ++Index)
		{
			bDifferent |= !SameRequest(A.Board->GetCollectors()[Index].Request, Other.Board->GetCollectors()[Index].Request);
		}
	}
	TestTrue(TEXT("and other seeds do not all give the same board"), bDifferent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBoardLoopStreamTest, "Flipper.Buyers.TheBoardDoesNotMoveTheTableOrTheMood", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBoardLoopStreamTest::RunTest(const FString& Parameters)
{
	for (int32 Seed = 1; Seed <= 10; ++Seed)
	{
		FBoardLoopFixture With(Seed, true);
		FBoardLoopFixture Without(Seed, false);
		With.Loop->StartRun();
		Without.Loop->StartRun();

		TestTrue(TEXT("the mood is the same with and without a board"), With.Loop->GetMood() == Without.Loop->GetMood());

		const TArray<FFlipperPlacedItem> TableWith = With.Table->GetPlacedItems();
		const TArray<FFlipperPlacedItem> TableWithout = Without.Table->GetPlacedItems();
		if (!TestEqual(TEXT("the table has the same number of items"), TableWith.Num(), TableWithout.Num()))
		{
			continue;
		}
		for (int32 Index = 0; Index < TableWith.Num(); ++Index)
		{
			TestEqual(TEXT("the same ask price"), TableWith[Index].Item.AskPrice, TableWithout[Index].Item.AskPrice);
			TestTrue(TEXT("in the same place"), TableWith[Index].Placement.Origin == TableWithout[Index].Placement.Origin);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBoardLoopNoBoardTest, "Flipper.Buyers.ADayWithoutABoardStillRuns", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBoardLoopNoBoardTest::RunTest(const FString& Parameters)
{
	FBoardLoopFixture F(3, false);
	F.Loop->StartRun();
	while (F.Loop->GetPhase() != EFlipperPhase::Finished)
	{
		F.Loop->EndPhase();
	}
	TestEqual(TEXT("the run finished"), F.Loop->GetPhase(), EFlipperPhase::Finished);
	TestEqual(TEXT("and the pawn shop is still the only buyer"), F.Rule->FindBestBuyer(FFlipperItemInstance()), FlipperBuyer::PawnShop);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperBoardLoopSellTest, "Flipper.Buyers.ASaleInTheSellPhaseUsesTheDaysBoard", FLIPPER_BUYER_TEST_FLAGS)
bool FFlipperBoardLoopSellTest::RunTest(const FString& Parameters)
{
	for (int32 Seed = 1; Seed <= 15; ++Seed)
	{
		FBoardLoopFixture F(Seed);
		F.Loop->StartRun();

		// Buy the first item on the table, as in play, then go to the Sell phase.
		const FFlipperPlacedItem Wanted = F.Table->GetPlacedItems()[0];
		F.Controller->PointerDown(F.Table.Get(), CellCentre(Wanted.Placement.Origin), FVector2D(0.0, 0.0));
		F.Controller->PointerMove(FVector2D(50.0, 0.0));
		F.Controller->PointerUp(F.Trunk.Get(), CellCentre(0, 0), FVector2D(50.0, 0.0));
		if (!TestNotNull(TEXT("bought into the trunk"), F.Trunk->FindItemAt(FIntPoint(0, 0))))
		{
			return true;
		}
		F.Loop->EndPhase();
		F.Loop->EndPhase();

		const FFlipperItemInstance Item = F.Trunk->FindItemAt(FIntPoint(0, 0))->Item;
		const int32				   Buyer = F.Rule->FindBestBuyer(Item);
		const int32				   Payout = F.Rule->GetPayout(Buyer, Item);
		TestTrue(TEXT("the best buyer pays at least the pawn shop"), Payout >= F.Rule->GetPayout(FlipperBuyer::PawnShop, Item));

		const int32 CashBefore = F.Wallet->GetCash();
		TestTrue(TEXT("a Shift-click sells"), F.Controller->QuickSell(F.Trunk.Get(), FIntPoint(0, 0)));
		TestEqual(TEXT("and pays what the card showed"), F.Wallet->GetCash(), CashBefore + Payout);
		if (Buyer != FlipperBuyer::PawnShop)
		{
			TestTrue(TEXT("a collector who bought is closed"), F.Board->Find(Buyer)->bSold);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
