// Copyright Epic Games, Inc. All Rights Reserved.

// Automation tests that act as the spec for collector requests (Docs/FlipperCollectors-design.md): matching, the
// premium and offer, building a request from a table item, and rolling the day's board.
// Run them with:  Automation RunTests Flipper.Requests
//
// The tags used here (Item.Category.*, Item.Trait.*) come from Config/DefaultGameplayTags.ini.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

	#include "UObject/StrongObjectPtr.h"
	#include "GameplayTagContainer.h"
	#include "FlipperCollectorDefinition.h"
	#include "FlipperItemDefinition.h"
	#include "FlipperRequests.h"
	#include "FlipperTraitDefinition.h"

namespace FlipperRequestTests
{
	static FGameplayTag Tag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}

	/** Keeps the assets alive and makes items, requests and boards. */
	struct FRequestFixture
	{
		TArray<TStrongObjectPtr<UFlipperItemDefinition>>	  Definitions;
		TArray<TStrongObjectPtr<UFlipperTraitDefinition>>	  Traits;
		TArray<TStrongObjectPtr<UFlipperCollectorDefinition>> Collectors;
		FFlipperPricingConfig								  Pricing;
		FFlipperRequestConfig								  Config;
		int32												  NextItemId = 1;

		/** A Tested, working, Common, Good item by default, with the given category tags and one trait per trait tag. */
		FFlipperItemInstance MakeItem(std::initializer_list<const TCHAR*> CategoryTags = {}, std::initializer_list<const TCHAR*> TraitTags = {}, EFlipperRarity Rarity = EFlipperRarity::Common, EFlipperCondition Condition = EFlipperCondition::Good, bool bTested = true, bool bWorks = true, int32 BaseValue = 100)
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
			Item.Rarity = Rarity;
			Item.Condition = Condition;
			Item.bTested = bTested;
			Item.bWorks = bWorks;
			for (const TCHAR* TraitTag : TraitTags)
			{
				UFlipperTraitDefinition* Trait = NewObject<UFlipperTraitDefinition>();
				Trait->Tag = Tag(TraitTag);
				Traits.Emplace(Trait);
				Item.Traits.Add(Trait);
			}
			return Item;
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

		TArray<UFlipperCollectorDefinition*> MakeCollectors(int32 Count)
		{
			TArray<UFlipperCollectorDefinition*> Result;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				UFlipperCollectorDefinition* Collector = NewObject<UFlipperCollectorDefinition>();
				Collector->DisplayName = FText::FromString(FString::Printf(TEXT("Collector %d"), Index));
				Collectors.Emplace(Collector);
				Result.Add(Collector);
			}
			return Result;
		}
	};

	/** How many tags of the request sit under the given parent tag (Item.Category or Item.Trait). */
	static int32 CountTagsUnder(const FFlipperRequest& Request, const TCHAR* Parent)
	{
		const FGameplayTag ParentTag = Tag(Parent);
		int32			   Count = 0;
		for (const FGameplayTag& RequiredTag : Request.RequiredTags)
		{
			if (RequiredTag.MatchesTag(ParentTag))
			{
				++Count;
			}
		}
		return Count;
	}

	static void Advance(FRandomStream& Random, int32 Draws)
	{
		for (int32 Index = 0; Index < Draws; ++Index)
		{
			Random.GetFraction();
		}
	}
} // namespace FlipperRequestTests

	#define FLIPPER_REQUEST_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

using namespace FlipperRequestTests;

// ---------------------------------------------------------------------------------------------------------------
// Counting clauses, the premium and the offer (these use no matching).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestClauseCountTest, "Flipper.Requests.ClauseCount", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestClauseCountTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("an empty request has no clauses"), FFlipperRequest().GetClauseCount(), 0);
	TestEqual(TEXT("one category tag is one clause"), FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld") }).GetClauseCount(), 1);
	TestEqual(TEXT("a category and a trait tag are two clauses"), FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld"), TEXT("Item.Trait.Boxed") }).GetClauseCount(), 2);
	TestEqual(TEXT("must work is one clause"), FRequestFixture::MakeRequest({}, true).GetClauseCount(), 1);
	TestEqual(TEXT("a minimum condition above Poor is one clause"), FRequestFixture::MakeRequest({}, false, EFlipperCondition::Fair).GetClauseCount(), 1);
	TestEqual(TEXT("a minimum rarity above Common is one clause"), FRequestFixture::MakeRequest({}, false, EFlipperCondition::Poor, EFlipperRarity::Uncommon).GetClauseCount(), 1);
	TestEqual(TEXT("a minimum of Poor and Common are not clauses"), FRequestFixture::MakeRequest({}, false, EFlipperCondition::Poor, EFlipperRarity::Common).GetClauseCount(), 0);
	TestEqual(TEXT("all five add up"), FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld"), TEXT("Item.Trait.Boxed") }, true, EFlipperCondition::Mint, EFlipperRarity::Rare).GetClauseCount(), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestPremiumTest, "Flipper.Requests.Premium", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestPremiumTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;

	// Defaults: 1 clause x1.25, 2 clauses x1.5, 3 clauses x1.75.
	TestEqual(TEXT("one clause pays x1.25"), F.Config.GetPremium(1), 1.25f);
	TestEqual(TEXT("two clauses pay x1.5"), F.Config.GetPremium(2), 1.5f);
	TestEqual(TEXT("three clauses pay x1.75"), F.Config.GetPremium(3), 1.75f);
	TestEqual(TEXT("no clauses pay no premium"), F.Config.GetPremium(0), 1.f);
	TestEqual(TEXT("more than three clauses use the three-clause premium"), F.Config.GetPremium(5), 1.75f);

	// The numbers come from the config.
	F.Config.TwoClausesPremium = 2.f;
	TestEqual(TEXT("the premium comes from the config"), F.Config.GetPremium(2), 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestCollectorOfferTest, "Flipper.Requests.CollectorOffer", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestCollectorOfferTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;
	const FFlipperRequest OneClause = FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld") });
	const FFlipperRequest TwoClauses = FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld"), TEXT("Item.Trait.Boxed") });

	// A Tested, working item of Value 100 pays Value x mood x premium.
	const FFlipperItemInstance Working = F.MakeItem();
	TestEqual(TEXT("one clause, Normal mood: 100 x 1.25"), FlipperRequests::GetCollectorOffer(Working, OneClause, F.Pricing, F.Config, 1.f), 125);
	TestEqual(TEXT("two clauses, Normal mood: 100 x 1.5"), FlipperRequests::GetCollectorOffer(Working, TwoClauses, F.Pricing, F.Config, 1.f), 150);
	TestEqual(TEXT("the mood scales it too: 100 x 0.8 x 1.5"), FlipperRequests::GetCollectorOffer(Working, TwoClauses, F.Pricing, F.Config, 0.8f), 120);

	// What is known still decides the base: dead is scrap (10 percent), Untested is 50 percent, both times the premium.
	const FFlipperItemInstance Dead = F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good, true, false);
	TestEqual(TEXT("a Tested dead item pays scrap x premium: 100 x 0.1 x 1.5"), FlipperRequests::GetCollectorOffer(Dead, TwoClauses, F.Pricing, F.Config, 1.f), 15);
	const FFlipperItemInstance Untested = F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good, false);
	TestEqual(TEXT("an Untested item pays 50 percent x premium: 100 x 0.5 x 1.5"), FlipperRequests::GetCollectorOffer(Untested, TwoClauses, F.Pricing, F.Config, 1.f), 75);

	// Rounded to the nearest dollar at the end: Value 6 x premium 1.25 is 7.5, which rounds up to 8.
	TestEqual(TEXT("rounded to the nearest dollar"), FlipperRequests::GetCollectorOffer(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good, true, true, 6), OneClause, F.Pricing, F.Config, 1.f), 8);

	// No definition, no offer.
	TestEqual(TEXT("an item with no Definition gets 0"), FlipperRequests::GetCollectorOffer(FFlipperItemInstance(), OneClause, F.Pricing, F.Config, 1.f), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Matching (these need FlipperRequests::Matches).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestMatchTagsTest, "Flipper.Requests.MatchTags", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestMatchTagsTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;

	// An item's tags are its definition's categories plus the tag of each trait.
	const FFlipperItemInstance GameBoy = F.MakeItem({ TEXT("Item.Category.Handheld"), TEXT("Item.Category.Gaming") }, { TEXT("Item.Trait.Boxed") });

	TestTrue(TEXT("a category it has"), FlipperRequests::Matches(GameBoy, FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld") })));
	TestTrue(TEXT("its other category"), FlipperRequests::Matches(GameBoy, FRequestFixture::MakeRequest({ TEXT("Item.Category.Gaming") })));
	TestFalse(TEXT("a category it lacks"), FlipperRequests::Matches(GameBoy, FRequestFixture::MakeRequest({ TEXT("Item.Category.Audio") })));
	TestTrue(TEXT("a trait it has"), FlipperRequests::Matches(GameBoy, FRequestFixture::MakeRequest({ TEXT("Item.Trait.Boxed") })));
	TestFalse(TEXT("a trait it lacks"), FlipperRequests::Matches(GameBoy, FRequestFixture::MakeRequest({ TEXT("Item.Trait.Prototype") })));
	TestTrue(TEXT("a category and a trait it has"), FlipperRequests::Matches(GameBoy, FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld"), TEXT("Item.Trait.Boxed") })));
	TestFalse(TEXT("every tag must be there: the category is, the trait is not"), FlipperRequests::Matches(GameBoy, FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld"), TEXT("Item.Trait.Modded") })));

	// A trait tag is not satisfied by a category, and the other way round.
	const FFlipperItemInstance PlainRadio = F.MakeItem({ TEXT("Item.Category.Audio") });
	TestFalse(TEXT("an item without traits never satisfies a trait tag"), FlipperRequests::Matches(PlainRadio, FRequestFixture::MakeRequest({ TEXT("Item.Trait.Boxed") })));

	// A null trait entry is skipped, not a crash.
	FFlipperItemInstance WithNullTrait = F.MakeItem({ TEXT("Item.Category.Audio") });
	WithNullTrait.Traits.Add(nullptr);
	TestTrue(TEXT("a null trait entry is ignored"), FlipperRequests::Matches(WithNullTrait, FRequestFixture::MakeRequest({ TEXT("Item.Category.Audio") })));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestMatchFlagsTest, "Flipper.Requests.MatchFlags", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestMatchFlagsTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;

	// Must work: only a Tested, working item.
	const FFlipperRequest MustWork = FRequestFixture::MakeRequest({}, true);
	TestTrue(TEXT("Tested and working satisfies must-work"), FlipperRequests::Matches(F.MakeItem(), MustWork));
	TestFalse(TEXT("Tested and dead does not"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good, true, false), MustWork));
	TestFalse(TEXT("Untested does not, even if it happens to work"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good, false, true), MustWork));
	TestFalse(TEXT("Untested and dead does not"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good, false, false), MustWork));

	// Without must-work the works state does not matter.
	const FFlipperRequest Anything = FRequestFixture::MakeRequest({ TEXT("Item.Category.Audio") });
	TestTrue(TEXT("without must-work an Untested item qualifies"), FlipperRequests::Matches(F.MakeItem({ TEXT("Item.Category.Audio") }, {}, EFlipperRarity::Common, EFlipperCondition::Good, false, false), Anything));
	TestTrue(TEXT("without must-work a dead item qualifies"), FlipperRequests::Matches(F.MakeItem({ TEXT("Item.Category.Audio") }, {}, EFlipperRarity::Common, EFlipperCondition::Good, true, false), Anything));

	// Minimum condition: Poor < Fair < Good < Mint.
	const FFlipperRequest FairOrBetter = FRequestFixture::MakeRequest({}, false, EFlipperCondition::Fair);
	TestFalse(TEXT("Poor is below Fair"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Poor), FairOrBetter));
	TestTrue(TEXT("Fair meets Fair"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Fair), FairOrBetter));
	TestTrue(TEXT("Good is above Fair"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good), FairOrBetter));
	TestTrue(TEXT("Mint is above Fair"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Mint), FairOrBetter));
	const FFlipperRequest MintOnly = FRequestFixture::MakeRequest({}, false, EFlipperCondition::Mint);
	TestFalse(TEXT("Good is below Mint"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Common, EFlipperCondition::Good), MintOnly));

	// Minimum rarity: Common < Uncommon < Rare.
	const FFlipperRequest UncommonOrBetter = FRequestFixture::MakeRequest({}, false, EFlipperCondition::Poor, EFlipperRarity::Uncommon);
	TestFalse(TEXT("Common is below Uncommon"), FlipperRequests::Matches(F.MakeItem(), UncommonOrBetter));
	TestTrue(TEXT("Uncommon meets Uncommon"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Uncommon), UncommonOrBetter));
	TestTrue(TEXT("Rare is above Uncommon"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Rare), UncommonOrBetter));
	const FFlipperRequest RareOnly = FRequestFixture::MakeRequest({}, false, EFlipperCondition::Poor, EFlipperRarity::Rare);
	TestFalse(TEXT("Uncommon is below Rare"), FlipperRequests::Matches(F.MakeItem({}, {}, EFlipperRarity::Uncommon), RareOnly));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestMatchCombinedTest, "Flipper.Requests.MatchCombined", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestMatchCombinedTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;

	// "Working, Boxed, Handheld, Fair or better, Uncommon or better": every clause must hold at once.
	const FFlipperRequest Request = FRequestFixture::MakeRequest({ TEXT("Item.Category.Handheld"), TEXT("Item.Trait.Boxed") }, true, EFlipperCondition::Fair, EFlipperRarity::Uncommon);

	TestTrue(TEXT("an item that meets all five"), FlipperRequests::Matches(F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Rare, EFlipperCondition::Mint), Request));
	TestFalse(TEXT("one missing tag fails it"), FlipperRequests::Matches(F.MakeItem({ TEXT("Item.Category.Handheld") }, {}, EFlipperRarity::Rare, EFlipperCondition::Mint), Request));
	TestFalse(TEXT("untested fails it"), FlipperRequests::Matches(F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Rare, EFlipperCondition::Mint, false, true), Request));
	TestFalse(TEXT("too low a condition fails it"), FlipperRequests::Matches(F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Rare, EFlipperCondition::Poor), Request));
	TestFalse(TEXT("too low a rarity fails it"), FlipperRequests::Matches(F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Common, EFlipperCondition::Mint), Request));

	// An empty request is met by any item that has a definition, and by nothing without one.
	TestTrue(TEXT("an empty request matches any item"), FlipperRequests::Matches(F.MakeItem(), FFlipperRequest()));
	TestFalse(TEXT("an item with no Definition matches nothing, not even an empty request"), FlipperRequests::Matches(FFlipperItemInstance(), FFlipperRequest()));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Building a request from a table item (these need FlipperRequests::RollRequest and Matches).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestRollSourceTest, "Flipper.Requests.RollSource", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestRollSourceTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;

	const FFlipperItemInstance Rich = F.MakeItem({ TEXT("Item.Category.Handheld"), TEXT("Item.Category.Gaming") }, { TEXT("Item.Trait.Boxed"), TEXT("Item.Trait.Modded") }, EFlipperRarity::Rare, EFlipperCondition::Mint);
	const FFlipperItemInstance Plain = F.MakeItem({ TEXT("Item.Category.Audio") });
	const FFlipperItemInstance Unproven = F.MakeItem({ TEXT("Item.Category.Video") }, {}, EFlipperRarity::Uncommon, EFlipperCondition::Fair, false, false);

	for (int32 Seed = 1; Seed <= 300; ++Seed)
	{
		for (const FFlipperItemInstance* Source : { &Rich, &Plain, &Unproven })
		{
			FRandomStream		 Random(Seed);
			const FFlipperRequest Request = FlipperRequests::RollRequest(*Source, F.Config, Random);

			const int32 Clauses = Request.GetClauseCount();
			TestTrue(TEXT("a request has 1 to 3 clauses"), Clauses >= 1 && Clauses <= 3);
			TestTrue(TEXT("at most one category tag"), CountTagsUnder(Request, TEXT("Item.Category")) <= 1);
			TestTrue(TEXT("at most one trait tag"), CountTagsUnder(Request, TEXT("Item.Trait")) <= 1);

			// The source meets the request, except that a must-work clause needs it tested and working.
			if (Request.bMustWork && !(Source->bTested && Source->bWorks))
			{
				FFlipperRequest WithoutMustWork = Request;
				WithoutMustWork.bMustWork = false;
				TestTrue(TEXT("the source meets every other clause"), FlipperRequests::Matches(*Source, WithoutMustWork));
				TestFalse(TEXT("an untested source cannot meet must-work"), FlipperRequests::Matches(*Source, Request));
			}
			else
			{
				TestTrue(TEXT("the source meets its own request"), FlipperRequests::Matches(*Source, Request));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestRollPoolTest, "Flipper.Requests.RollPool", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestRollPoolTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;

	// A Common, Poor-condition item with no traits can supply only a category and must-work, so never more than 2 clauses.
	const FFlipperItemInstance Plain = F.MakeItem({ TEXT("Item.Category.Audio") }, {}, EFlipperRarity::Common, EFlipperCondition::Poor);
	F.Config.OneClauseWeight = 0.f;
	F.Config.TwoClausesWeight = 0.f;
	F.Config.ThreeClausesWeight = 100.f;
	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		FRandomStream Random(Seed);
		const FFlipperRequest Request = FlipperRequests::RollRequest(Plain, F.Config, Random);
		TestEqual(TEXT("three clauses wanted, but the pool has two: the count is capped"), Request.GetClauseCount(), 2);
		TestEqual(TEXT("so it is the category and must-work"), CountTagsUnder(Request, TEXT("Item.Category")), 1);
		TestTrue(TEXT("must-work is in it"), Request.bMustWork);
		TestEqual(TEXT("a Poor item gives no condition clause"), static_cast<int32>(Request.MinCondition), static_cast<int32>(EFlipperCondition::Poor));
		TestEqual(TEXT("a Common item gives no rarity clause"), static_cast<int32>(Request.MinRarity), static_cast<int32>(EFlipperRarity::Common));
	}

	// A rich item can supply all five kinds. The clause kinds are picked at random, so over many rolls each one turns up.
	const FFlipperItemInstance Rich = F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Rare, EFlipperCondition::Mint);
	F.Config.ThreeClausesWeight = 0.f;
	F.Config.OneClauseWeight = 100.f;
	int32 Categories = 0, TraitsSeen = 0, MustWorks = 0, Conditions = 0, Rarities = 0;
	for (int32 Seed = 1; Seed <= 1000; ++Seed)
	{
		FRandomStream Random(Seed);
		const FFlipperRequest Request = FlipperRequests::RollRequest(Rich, F.Config, Random);
		TestEqual(TEXT("one clause wanted, one clause made"), Request.GetClauseCount(), 1);
		Categories += CountTagsUnder(Request, TEXT("Item.Category"));
		TraitsSeen += CountTagsUnder(Request, TEXT("Item.Trait"));
		MustWorks += Request.bMustWork ? 1 : 0;
		Conditions += Request.MinCondition == EFlipperCondition::Mint ? 1 : 0;
		Rarities += Request.MinRarity == EFlipperRarity::Rare ? 1 : 0;
	}
	// Each of the five kinds should come up about 200 times in 1000.
	TestTrue(TEXT("category clauses come up"), Categories > 130 && Categories < 270);
	TestTrue(TEXT("trait clauses come up"), TraitsSeen > 130 && TraitsSeen < 270);
	TestTrue(TEXT("must-work clauses come up"), MustWorks > 130 && MustWorks < 270);
	TestTrue(TEXT("condition clauses come up"), Conditions > 130 && Conditions < 270);
	TestTrue(TEXT("rarity clauses come up"), Rarities > 130 && Rarities < 270);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestRollCountTest, "Flipper.Requests.RollCount", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestRollCountTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;
	const FFlipperItemInstance Rich = F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Rare, EFlipperCondition::Mint);

	// Default weights 40 / 40 / 20, with a pool of five so the count is never capped.
	int32 Counts[4] = { 0, 0, 0, 0 };
	constexpr int32 Rolls = 3000;
	FRandomStream	Random(7);
	for (int32 Index = 0; Index < Rolls; ++Index)
	{
		const int32 Clauses = FlipperRequests::RollRequest(Rich, F.Config, Random).GetClauseCount();
		if (Clauses >= 1 && Clauses <= 3)
		{
			++Counts[Clauses];
		}
	}
	TestTrue(TEXT("one clause about 40 percent"), Counts[1] > Rolls * 0.35 && Counts[1] < Rolls * 0.45);
	TestTrue(TEXT("two clauses about 40 percent"), Counts[2] > Rolls * 0.35 && Counts[2] < Rolls * 0.45);
	TestTrue(TEXT("three clauses about 20 percent"), Counts[3] > Rolls * 0.15 && Counts[3] < Rolls * 0.25);

	// A weight of 0 never comes up.
	F.Config.OneClauseWeight = 0.f;
	for (int32 Index = 0; Index < 500; ++Index)
	{
		TestTrue(TEXT("a zero weight never wins"), FlipperRequests::RollRequest(Rich, F.Config, Random).GetClauseCount() >= 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestRollDrawsTest, "Flipper.Requests.RollDraws", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestRollDrawsTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;

	// Whatever the source, a request takes exactly 8 numbers from the stream, so a seed stays aligned.
	const FFlipperItemInstance Sources[] = {
		F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Rare, EFlipperCondition::Mint),
		F.MakeItem({ TEXT("Item.Category.Audio") }),
		F.MakeItem({ TEXT("Item.Category.Audio") }, {}, EFlipperRarity::Common, EFlipperCondition::Poor, false, false),
		FFlipperItemInstance(),
	};
	for (const FFlipperItemInstance& Source : Sources)
	{
		FRandomStream Used(99);
		FRandomStream Expected(99);
		FlipperRequests::RollRequest(Source, F.Config, Used);
		Advance(Expected, 8);
		TestEqual(TEXT("a request takes exactly 8 numbers"), Used.GetCurrentSeed(), Expected.GetCurrentSeed());
	}

	// The same seed gives the same request.
	FRandomStream A(5), B(5);
	const FFlipperRequest First = FlipperRequests::RollRequest(Sources[0], F.Config, A);
	const FFlipperRequest Second = FlipperRequests::RollRequest(Sources[0], F.Config, B);
	TestTrue(TEXT("same seed, same tags"), First.RequiredTags == Second.RequiredTags);
	TestEqual(TEXT("same seed, same must-work"), First.bMustWork, Second.bMustWork);
	TestEqual(TEXT("same seed, same minimum condition"), static_cast<int32>(First.MinCondition), static_cast<int32>(Second.MinCondition));
	TestEqual(TEXT("same seed, same minimum rarity"), static_cast<int32>(First.MinRarity), static_cast<int32>(Second.MinRarity));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The day's board (these need RollRequest and Matches).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestBoardSizeTest, "Flipper.Requests.BoardSize", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestBoardSizeTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;
	const TArray<UFlipperCollectorDefinition*> Names = F.MakeCollectors(5);

	TArray<FFlipperItemInstance> Table;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		Table.Add(F.MakeItem({ TEXT("Item.Category.Audio") }));
	}

	{
		FRandomStream Random(1);
		TestEqual(TEXT("3 collectors from a table of 8"), FlipperRequests::RollBoard(Table, Names, F.Config, Random).Num(), 3);
	}
	{
		F.Config.CollectorsPerDay = 5;
		FRandomStream Random(1);
		TestEqual(TEXT("CollectorsPerDay comes from the config"), FlipperRequests::RollBoard(Table, Names, F.Config, Random).Num(), 5);
		F.Config.CollectorsPerDay = 3;
	}
	{
		TArray<FFlipperItemInstance> Small = { Table[0], Table[1] };
		FRandomStream				 Random(1);
		TestEqual(TEXT("a table of 2 gives 2 collectors"), FlipperRequests::RollBoard(Small, Names, F.Config, Random).Num(), 2);
	}
	{
		TArray<FFlipperItemInstance> Empty;
		FRandomStream				 Random(1);
		TestEqual(TEXT("an empty table gives no collectors"), FlipperRequests::RollBoard(Empty, Names, F.Config, Random).Num(), 0);
	}
	{
		// An item with no definition cannot be a source.
		TArray<FFlipperItemInstance> Broken = { FFlipperItemInstance(), Table[0] };
		FRandomStream				 Random(1);
		TestEqual(TEXT("an item with no Definition is not a source"), FlipperRequests::RollBoard(Broken, Names, F.Config, Random).Num(), 1);
	}
	{
		FRandomStream Random(1);
		for (const FFlipperCollector& Collector : FlipperRequests::RollBoard(Table, Names, F.Config, Random))
		{
			TestFalse(TEXT("a new collector has not bought yet"), Collector.bSold);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestBoardRequestsTest, "Flipper.Requests.BoardRequests", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestBoardRequestsTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;
	const TArray<UFlipperCollectorDefinition*> Names = F.MakeCollectors(5);

	// Every request can be met by something on the table (the table items here are Tested and working).
	TArray<FFlipperItemInstance> Table;
	Table.Add(F.MakeItem({ TEXT("Item.Category.Handheld") }, { TEXT("Item.Trait.Boxed") }, EFlipperRarity::Rare, EFlipperCondition::Mint));
	Table.Add(F.MakeItem({ TEXT("Item.Category.Audio") }, {}, EFlipperRarity::Uncommon, EFlipperCondition::Fair));
	Table.Add(F.MakeItem({ TEXT("Item.Category.Photo") }, { TEXT("Item.Trait.Modded") }));
	Table.Add(F.MakeItem({ TEXT("Item.Category.Video") }));
	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		FRandomStream Random(Seed);
		for (const FFlipperCollector& Collector : FlipperRequests::RollBoard(Table, Names, F.Config, Random))
		{
			bool bMet = false;
			for (const FFlipperItemInstance& Item : Table)
			{
				bMet = bMet || FlipperRequests::Matches(Item, Collector.Request);
			}
			TestTrue(TEXT("a request can be met by an item on the table"), bMet);
		}
	}

	// No two collectors are built from the same item. Here every item can only give a category and must-work, and each
	// request takes both, so the categories on a board must all differ.
	FRequestFixture G;
	G.Config.OneClauseWeight = 0.f;
	G.Config.TwoClausesWeight = 100.f;
	G.Config.ThreeClausesWeight = 0.f;
	const TArray<UFlipperCollectorDefinition*> GNames = G.MakeCollectors(5);
	TArray<FFlipperItemInstance> Distinct;
	for (const TCHAR* CategoryTag : { TEXT("Item.Category.Audio"), TEXT("Item.Category.Video"), TEXT("Item.Category.Photo"), TEXT("Item.Category.Office"), TEXT("Item.Category.Telecom") })
	{
		Distinct.Add(G.MakeItem({ CategoryTag }));
	}
	for (int32 Seed = 1; Seed <= 100; ++Seed)
	{
		FRandomStream Random(Seed);
		FGameplayTagContainer Seen;
		for (const FFlipperCollector& Collector : FlipperRequests::RollBoard(Distinct, GNames, G.Config, Random))
		{
			TestEqual(TEXT("each request has a category tag"), CountTagsUnder(Collector.Request, TEXT("Item.Category")), 1);
			for (const FGameplayTag& RequiredTag : Collector.Request.RequiredTags)
			{
				TestFalse(TEXT("no category appears on two requests of one board"), Seen.HasTagExact(RequiredTag));
				Seen.AddTag(RequiredTag);
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestBoardNamesTest, "Flipper.Requests.BoardNames", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestBoardNamesTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;
	TArray<FFlipperItemInstance> Table;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		Table.Add(F.MakeItem({ TEXT("Item.Category.Audio") }));
	}

	// With enough names every collector gets a different one.
	{
		const TArray<UFlipperCollectorDefinition*> Names = F.MakeCollectors(5);
		FRandomStream							   Random(3);
		TSet<const UFlipperCollectorDefinition*>   Used;
		for (const FFlipperCollector& Collector : FlipperRequests::RollBoard(Table, Names, F.Config, Random))
		{
			TestNotNull(TEXT("a collector has a definition"), Collector.Definition.Get());
			Used.Add(Collector.Definition.Get());
		}
		TestEqual(TEXT("three different names"), Used.Num(), 3);
	}

	// With fewer names than collectors the pool refills, so a name repeats but every collector still has one.
	{
		const TArray<UFlipperCollectorDefinition*> Names = F.MakeCollectors(2);
		FRandomStream							   Random(3);
		const TArray<FFlipperCollector>			   Board = FlipperRequests::RollBoard(Table, Names, F.Config, Random);
		TestEqual(TEXT("still three collectors"), Board.Num(), 3);
		for (const FFlipperCollector& Collector : Board)
		{
			TestNotNull(TEXT("every collector has a definition"), Collector.Definition.Get());
		}
		TestTrue(TEXT("the first two use both names"), Board[0].Definition != Board[1].Definition);
	}

	// No names: the cards are still made, with a null definition.
	{
		FRandomStream					Random(3);
		const TArray<FFlipperCollector> Board = FlipperRequests::RollBoard(Table, {}, F.Config, Random);
		TestEqual(TEXT("three collectors without names"), Board.Num(), 3);
		for (const FFlipperCollector& Collector : Board)
		{
			TestNull(TEXT("no names to give"), Collector.Definition.Get());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlipperRequestBoardDrawsTest, "Flipper.Requests.BoardDraws", FLIPPER_REQUEST_TEST_FLAGS)
bool FFlipperRequestBoardDrawsTest::RunTest(const FString& Parameters)
{
	FRequestFixture F;
	const TArray<UFlipperCollectorDefinition*> Names = F.MakeCollectors(5);

	TArray<FFlipperItemInstance> Table;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		Table.Add(F.MakeItem({ TEXT("Item.Category.Audio") }));
	}

	// 10 numbers per collector built: 1 for the source, 8 for the request, 1 for the name.
	{
		FRandomStream Used(21), Expected(21);
		FlipperRequests::RollBoard(Table, Names, F.Config, Used);
		Advance(Expected, 30);
		TestEqual(TEXT("3 collectors take 30 numbers"), Used.GetCurrentSeed(), Expected.GetCurrentSeed());
	}
	{
		TArray<FFlipperItemInstance> Small = { Table[0], Table[1] };
		FRandomStream				 Used(21), Expected(21);
		FlipperRequests::RollBoard(Small, Names, F.Config, Used);
		Advance(Expected, 20);
		TestEqual(TEXT("2 collectors take 20 numbers"), Used.GetCurrentSeed(), Expected.GetCurrentSeed());
	}
	{
		TArray<FFlipperItemInstance> Empty;
		FRandomStream				 Used(21), Expected(21);
		FlipperRequests::RollBoard(Empty, Names, F.Config, Used);
		TestEqual(TEXT("no collectors take no numbers"), Used.GetCurrentSeed(), Expected.GetCurrentSeed());
	}
	{
		// The name pool being empty does not change the count.
		FRandomStream Used(21), Expected(21);
		FlipperRequests::RollBoard(Table, {}, F.Config, Used);
		Advance(Expected, 30);
		TestEqual(TEXT("with no names the stream still moves by 30"), Used.GetCurrentSeed(), Expected.GetCurrentSeed());
	}

	// The same seed gives the same board.
	FRandomStream A(8), B(8);
	const TArray<FFlipperCollector> First = FlipperRequests::RollBoard(Table, Names, F.Config, A);
	const TArray<FFlipperCollector> Second = FlipperRequests::RollBoard(Table, Names, F.Config, B);
	TestEqual(TEXT("same seed, same size"), First.Num(), Second.Num());
	for (int32 Index = 0; Index < FMath::Min(First.Num(), Second.Num()); ++Index)
	{
		TestTrue(TEXT("same seed, same name"), First[Index].Definition == Second[Index].Definition);
		TestTrue(TEXT("same seed, same tags"), First[Index].Request.RequiredTags == Second[Index].Request.RequiredTags);
		TestEqual(TEXT("same seed, same clause count"), First[Index].Request.GetClauseCount(), Second[Index].Request.GetClauseCount());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
