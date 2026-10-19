// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for the rent check (Docs/FlipperSpec.md, Feature 6): one check of the cash against
// the rent when the last day's Sell phase ends, items left in the trunk not counting, and the result staying put.
// Run them with:  Automation RunTests Flipper.Rent

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "FlipperDayLoop.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperWallet.h"
	#include "InventoryGrid.h"

namespace FlipperRentTests
{
	/** A two-day run with a rent of 100 and 50 cash. There is no generator, table or controller: only the rent check is under test. */
	struct FRentFixture
	{
		TStrongObjectPtr<UInventoryGrid>		Trunk;
		TStrongObjectPtr<UFlipperWallet>		Wallet;
		TStrongObjectPtr<UFlipperDayLoop>		Loop;
		TStrongObjectPtr<UFlipperItemDefinition> Definition;

		explicit FRentFixture(bool bGiveWallet = true)
		{
			Trunk.Reset(NewObject<UInventoryGrid>());
			Trunk->Init(4, 3);
			Wallet.Reset(NewObject<UFlipperWallet>());
			Wallet->Init(50);
			Definition.Reset(NewObject<UFlipperItemDefinition>());
			Definition->Width = 1;
			Definition->Height = 1;
			Definition->BaseValue = 500;

			FFlipperDayConfig Config;
			Config.DayCount = 2;
			Config.RentAmount = 100;
			Loop.Reset(NewObject<UFlipperDayLoop>());
			Loop->Init(Config, Trunk.Get(), nullptr, nullptr, nullptr, nullptr, 1);
			if (bGiveWallet)
			{
				Loop->SetWallet(Wallet.Get());
			}
		}

		/** Ends phases until the run is Finished. */
		void PlayToTheEnd()
		{
			Loop->StartRun();
			for (int32 Guard = 0; Guard < 20 && Loop->GetPhase() != EFlipperPhase::Finished; ++Guard)
			{
				Loop->EndPhase();
			}
		}
	};
} // namespace FlipperRentTests

	#define FLIPPER_RENT_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperRentTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentRuleTest, "Flipper.Rent.ExactlyTheRentIsEnough", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentRuleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("the rent exactly covered"), UFlipperDayLoop::IsRentMet(100, 100));
	TestTrue(TEXT("more than the rent"), UFlipperDayLoop::IsRentMet(101, 100));
	TestFalse(TEXT("one dollar short"), UFlipperDayLoop::IsRentMet(99, 100));
	TestTrue(TEXT("a rent of 0 is always met"), UFlipperDayLoop::IsRentMet(0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentInProgressTest, "Flipper.Rent.NoResultUntilTheRunIsFinished", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentInProgressTest::RunTest(const FString& Parameters)
{
	FRentFixture F;
	TestEqual(TEXT("before the run starts"), F.Loop->GetResult(), EFlipperRunResult::InProgress);

	F.Wallet->Add(1000);
	F.Loop->StartRun();
	TestEqual(TEXT("in the first Market, even with plenty of cash"), F.Loop->GetResult(), EFlipperRunResult::InProgress);

	// Day 1: Market, Garage, Sell. Ending the Sell phase of a day that is not the last one is not the end of the run.
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	F.Loop->EndPhase();
	TestEqual(TEXT("it is day 2 now"), F.Loop->GetDay(), 2);
	TestEqual(TEXT("the rent is not checked after day 1"), F.Loop->GetResult(), EFlipperRunResult::InProgress);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentWinTest, "Flipper.Rent.CashAtTheRentWins", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentWinTest::RunTest(const FString& Parameters)
{
	FRentFixture F;
	F.Wallet->Add(50); // 100 cash, exactly the rent
	F.PlayToTheEnd();

	TestEqual(TEXT("the run is over"), F.Loop->GetPhase(), EFlipperPhase::Finished);
	TestEqual(TEXT("exactly the rent is a win"), F.Loop->GetResult(), EFlipperRunResult::Won);
	TestEqual(TEXT("the cash and rent the end screen shows"), F.Loop->GetCash(), 100);
	TestEqual(TEXT("the rent"), F.Loop->GetRent(), 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentLoseTest, "Flipper.Rent.OneDollarShortLoses", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentLoseTest::RunTest(const FString& Parameters)
{
	FRentFixture F;
	F.Wallet->Add(49); // 99 cash
	F.PlayToTheEnd();

	TestEqual(TEXT("one dollar short is a loss"), F.Loop->GetResult(), EFlipperRunResult::Lost);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentTrunkTest, "Flipper.Rent.ItemsLeftInTheTrunkDoNotCount", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentTrunkTest::RunTest(const FString& Parameters)
{
	FRentFixture F;
	FFlipperItemInstance Item;
	Item.Id = 1;
	Item.Definition = F.Definition.Get();
	Item.bTested = true;
	Item.bWorks = true;
	TestTrue(TEXT("a valuable item fits in the trunk"), F.Trunk->TryPlace(Item, FIntPoint(0, 0), false));

	F.PlayToTheEnd();

	TestEqual(TEXT("50 cash and a 500 item is still a loss: only cash counts"), F.Loop->GetResult(), EFlipperRunResult::Lost);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentStaysTest, "Flipper.Rent.TheResultStaysOnceFinished", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentStaysTest::RunTest(const FString& Parameters)
{
	FRentFixture F;
	F.PlayToTheEnd();
	TestEqual(TEXT("50 cash against a rent of 100"), F.Loop->GetResult(), EFlipperRunResult::Lost);

	// Cash arriving after the run cannot change the verdict, and ending the phase again does nothing.
	F.Wallet->Add(1000);
	F.Loop->EndPhase();
	TestEqual(TEXT("still Lost"), F.Loop->GetResult(), EFlipperRunResult::Lost);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentNoWalletTest, "Flipper.Rent.NoWalletNoVerdict", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentNoWalletTest::RunTest(const FString& Parameters)
{
	FRentFixture F(false);
	F.PlayToTheEnd();

	TestEqual(TEXT("the run still finishes"), F.Loop->GetPhase(), EFlipperPhase::Finished);
	TestEqual(TEXT("but with nothing to check there is no verdict"), F.Loop->GetResult(), EFlipperRunResult::InProgress);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRentConfigTest, "Flipper.Rent.TheDefaultRentIsInTheConfig", FLIPPER_RENT_TEST_FLAGS)
bool FFlipperRentConfigTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the agreed rent"), FFlipperDayConfig().RentAmount, 450);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
