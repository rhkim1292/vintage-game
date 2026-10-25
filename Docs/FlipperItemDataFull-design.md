# Flipper item data, full scope: condition, traits, lots and categories (design)

Grill session: 2026-10-24, topic "Flipper item data: condition, traits and lots (full plan)". This is step 1 of the post-jam work that follows [FlipperPlan-Full.md](FlipperPlan-Full.md): the item properties the jam build cut (see "What the jam plan cuts" in [FlipperPlan.md](FlipperPlan.md)). It builds on [FlipperItemData-design.md](FlipperItemData-design.md) (rarity, Tested/Untested, the 6 starter items) and [FlipperPricing-design.md](FlipperPricing-design.md) (Value, ask price, offer), which stay in force except where this document says it extends them.

## Summary

Every item gains three properties and every item definition gains tags. **Condition** is a visible grade (Poor, Fair, Good, Mint) rolled per item. It multiplies the price and also sets the chance that an Untested item works, so the player can read risk off the table. **Traits** are five named extras (three good, two bad), each a price multiplier plus a Gameplay Tag; an item has 0, 1 or 2 of them. **Lots** are item definitions flagged as lots ("Lot of 10"), with no new mechanics. **Categories** are Gameplay Tags on each definition (Handheld, Audio, and so on), so that collector requests (the next feature) can say "working, boxed, any handheld" as a tag query. The catalogue grows from 6 to 8 regular items (a Cassette Walkman and a VCR are added) plus 2 lots. On the grid a one-letter condition badge appears beside the existing rarity border and "?" mark, and the tooltip gains a condition line and a traits line. No prototype or diagram was drawn for this grill.

## Terms

| Term | Definition | Avoid |
|---|---|---|
| **Condition** | A visible grade (Poor, Fair, Good, Mint) rolled per item, describing how worn it is. It sets a price multiplier and the works chance of an Untested item. | quality, state |
| **Trait** | A named extra property on one item (Boxed, With accessories, Prototype, Modded, Missing parts) that changes its value and carries a tag. An item has zero, one or two. | perk, modifier, affix |
| **Lot** | A bundle of identical small items sold as one bigger item (for example "Box of Floppies", "Lot of 10"). It is an ordinary item definition with a lot flag. | stack, bundle, crate |
| **Untested** | An item whose works/dead flag is hidden until it is tested in the Garage (already built). | unknown |
| **Tag** | A Gameplay Tag attached to a trait or an item definition so that collector requests can match it. | label, keyword |
| **Category** | The kind-of-item tags on a definition (what the item IS: handheld, audio, photo), as opposed to a trait (something about this one copy). | type, class, genre |

## Why

The jam build left rarity and the hidden works/dead flag as the only things that make two copies of the same item different. The full plan (Phase 3 and Phase 6) wants more to read on the table: condition levels, traits, bundled lots, and items that collector requests can ask for by property. In the user's words: "I want to go back to following FlipperPlan-Full. What features did we skip based on where we're at in the shortened FlipperPlan.md?", and then "Let's start with step 1". Item data is first because collector requests, richer pricing and the display all need these properties to exist before they can be built.

## Locked decisions

These are hard to reverse, would surprise someone without context, and involved a real trade-off.

### 1. Condition changes the works chance of Untested items (q1, answer B)

- Condition is a **visible** grade rolled per item. It does two things: it multiplies the item's Value (price), and it sets the chance that an **Untested** item works. A **Tested** item on the table still always works. All numbers are data.
- The Untested gamble stops being a flat coin flip and becomes a readable risk: a Poor-condition Untested radio is a worse bet than a Good one. That gives the tooltip and the grid badge a reason to show condition, and gives the balance pass two levers per level (multiplier and works chance).
- *Rejected: cosmetic grade only (A).* Cheapest, but condition would be a second rarity under another name, and a "Mint" item that is dead reads oddly.
- *Rejected: hidden grade until the Garage test (C).* A second layer of hidden information makes the table harder to read under a 60-second timer.
- Consequence for existing code: the flat "Untested items work" chance in `FFlipperGenerationConfig` is replaced by a per-condition chance, and `FlipperPricing::GetValue` gains the condition multiplier (see Implementation notes).

### 2. A trait is a price multiplier plus a tag, with no other gameplay effect (q2, answer B)

- Each trait is a data asset (`UFlipperTraitDefinition`) holding a name, a value multiplier, a Gameplay Tag (for example `Item.Trait.Boxed`), a colour and a roll weight. Traits can be good (raise value) or bad (lower it).
- The tag is there so the collector-requests feature can match "boxed" without reworking trait data later. The trait itself affects nothing but price.
- *Rejected: price modifier only (A).* The same data without the tag, which would likely be redone when collectors arrive.
- *Rejected: gameplay traits (C),* for example a Boxed item one cell bigger or a trait that lowers the works chance. It is the most interesting for the packing puzzle, but the footprint is read from the definition in `UInventoryGrid`, every widget and about 40 tests, so it should be its own grill once the data model has settled. Reopen it if the packing puzzle feels too easy.

### 3. Item definitions carry Gameplay Tags as categories (q6, answer B)

- Each definition has a Gameplay Tag container (for example `Item.Category.Handheld`, `Item.Category.Audio`). Traits use the same tag system, so a collector request is a single tag query (does the item carry all of these tags), and new request kinds ("anything from the 80s") are data, not code.
- This adds the `GameplayTags` engine module to `LD60.Build.cs` (it ships with the engine; no plugin to enable) and a small amount of tag setup in the editor.
- *Rejected: one category per definition from a fixed enum (A).* A smaller step, but a request could then only ask about one category, and an enum change means recompiling.
- *Rejected: no categories (C).* Requests could only name a specific item, a trait, a rarity or a condition and would feel thin.

## Routine choices

- **Lots are plain item definitions (q3, A):** a definition gets a lot flag and a count, the tooltip says "Lot of N", and the price comes from the lot's own base value. No new mechanics, and it does not close off a later "open a lot in the Garage" feature, which would add a contents list to the same definition. *Rejected:* opening in the Garage to spill N random items into the trunk (a second Garage interaction with a "what if the trunk is full" problem, kept for its own grill); skipping lots.
- **Condition scale (q4, B):** 4 levels. Poor x0.6 price / 45% works / weight 20, Fair x0.85 / 60% / 35, Good x1.0 / 75% / 30, Mint x1.3 / 90% / 15. With these weights the average works chance is about 66% (today's flat 65% roughly holds) and the average price multiplier is about 0.91, so prices fall about 9% on average; that is the first number to check in the balance pass. Good is the baseline (the base value already means a Common, Good, working copy). All twelve numbers are data. *Rejected:* 3 levels (flattens the table); 5 levels with Junk and Pristine (two more numbers per level for little extra readability).
- **Traits (q5, A):** five traits. Good: Boxed x1.4, With accessories x1.2, Prototype x1.8 (low roll weight). Bad: Modded x0.7, Missing parts x0.6. An item gets 0 / 1 / 2 traits at 55% / 35% / 10%, chosen by weight, no duplicates. Traits are visible on the table; only the works/dead flag stays hidden. An average table of 8 items has about 4 with a trait and about 1 with two. Trait multipliers multiply together and with the other multipliers. *Rejected:* at most one trait per item (removes the mixed decisions such as Boxed plus Missing parts); a per-definition list of which traits it may roll (authoring work with little visible effect on 8 items; can be added later because the roll reads from data).
- **Display (q7, A):** the tooltip gains a condition line (the grade as a word, coloured) and a traits line. On the grid, condition appears as a one-letter badge (P / F / G / M) in a corner, next to the rarity border, the "?" mark and the dead "X". Traits appear only in the tooltip. *Rejected:* trait pips on the grid (a fourth mark on a 64-pixel cell where the 1x1 Floppy Disk is already crowded; easy to add after a play check if scanning for traits feels slow); tooltip only (the player would hover every item under the timer).
- **Starter items (q8, A):** the set grows from 6 to 8 with a **Cassette Walkman** (1x2, $30; Handheld, Audio) and a **VCR** (3x1, $60; Video). Tags for the existing six: Floppy Disk 1x1 $6 = Computing; Game Boy 1x2 $35 = Handheld, Gaming; Polaroid Camera 2x2 $45 = Photo; Rotary Phone 2x2 $55 = Telecom; Typewriter 3x2 $70 = Office; CRT Television 4x3 $120 = Display. The Walkman gives "any handheld" two members and adds Audio; the VCR adds the 3x1 shape the set lacked. *Rejected:* Boombox 3x2 $90 and Pocket Calculator 1x1 $12 (two shapes already in the set). All values are data assets and can be swapped after a play check.
- **Lots in the catalogue (q9, A):** two lots on top of the 8 regular items: **Box of Floppies** (3x2, $45; Computing, Lot, "Lot of 10") and **Shoebox of Cassettes** (2x2, $40; Audio, Lot), each with spawn weight 0.5, so each appears half as often as a regular item (about 1 lot per day across the 8-item table with 10 definitions). *Rejected:* one lot only (the smallest step); two lots at weight 1 (a lot on most tables, crowding the 40-cell grid).
- **Lots roll like any item (q10, A):** rarity, condition, traits and Tested/Untested all apply to a lot. An Untested lot can be dead (the disks are corrupted) and a lot can be Boxed. No special cases in the generator, pricing or the Garage test; only the tooltip adds "Lot of N". *Rejected:* lots skipping traits and always working (B), or skipping only traits (C): each needs a lot check in the generator and a rule to explain on the tooltip. The "Boxed box" oddity is cosmetic; the per-definition trait lists in the trait choice above would remove it later.

## Implementation notes (derived from the decisions above)

- **Data model:**
  - `EFlipperCondition` (Poor, Fair, Good, Mint) next to `EFlipperRarity` in `FlipperTypes.h`, with a name, a badge letter and a colour helper in a `FlipperCondition` namespace like `FlipperRarity`.
  - `FFlipperItemInstance` gains `Condition` and the item's traits (an array of at most two `UFlipperTraitDefinition` pointers).
  - `UFlipperItemDefinition` gains `FGameplayTagContainer Categories`, `bool bIsLot` and `int32 LotCount`.
  - `UFlipperTraitDefinition` is a new data asset: name, multiplier, tag, colour, roll weight.
- **Config:**
  - The per-level condition numbers (multiplier, works chance, roll weight) and the trait count odds (55 / 35 / 10) live in data beside `FFlipperGenerationConfig` and `FFlipperPricingConfig`.
  - The old flat "Untested items work" chance in `FFlipperGenerationConfig` is replaced by the per-condition chance.
  - The trait data assets are listed on the generator the same way definitions are.
- **Pricing:** Value = BaseValue x rarity multiplier x condition multiplier x the product of the trait multipliers, rounded once to the nearest whole dollar. Scrap value (dead item), the seller's ask price and the buyer's offer are still computed from Value exactly as before, so they pick up the new multipliers with no other change.
- **Roll order (one `FRandomStream`, every roll takes a fixed number of draws so a seed stays deterministic):** definition, rarity, condition, Tested/Untested, works/dead (using the condition's chance, only for an Untested item), traits (count, then picks), ask price last.
- **Tests:** the generator tests assert odds statistically (condition weights, works chance per level, trait count odds, no duplicate traits, a lot rolls like an item) and pricing tests cover the new multipliers. Tests that assert an exact value at a fixed seed will need re-baselining because the roll sequence changes.
- **Widgets:** `UInventoryGridWidget::NativePaint` draws the condition badge through `FlipperPaint::Label`; `UFlipperItemTooltip` gets the condition and traits lines and the "Lot of N" text.
- **Tag namespaces:** the examples in the grill differed (`Item.Trait.Boxed` and `Category.Handheld`). Use `Item.Trait.*` and `Item.Category.*` so the two never collide with other tags.

## Verified facts

- `FFlipperGenerationConfig`, `UFlipperItemGenerator` (`RollRarity`, `RollDefinition`, `RollItem`, and the function that places up to `ItemsPerDay` items with `FindFirstFit`) and `FlipperPricing::GetValue` / `GetAskPrice` / `GetOffer` already exist in `Source/LD60/Flipper/`. All of them take a `FRandomStream` or a config struct.
- `Source/LD60/LD60.Build.cs` lists Core, CoreUObject, Engine, InputCore, UMG, SlateCore and Slate: `GameplayTags` is not a dependency yet.
- `UFlipperItemDefinition` holds DisplayName, Width, Height, BaseValue, SpawnWeight and Icon; it has no tags, no lot fields and no condition.
- The Flipper tests live in `Source/LD60/Flipper/Tests/` (8 test files plus a helper header); 95 of 95 passed on the last run before the template cleanup, and the cleanup commit (63d79c4) was not seen to build or be tested in this session.

## Risks

- **Unit tuning:** condition multipliers, works chances, trait multipliers and trait odds are first guesses. Average prices fall about 9% from condition alone, and traits shift them again; the balance pass must re-check rent ($450), the $200 start and the mood multipliers.
- **A 64-pixel cell is small:** the Floppy Disk is 1x1 and must hold a name, a "?" or "X" and now a condition letter. Check it in a play pass and move or drop the badge if it is unreadable.
- **Existing seeded tests:** inserting new rolls changes the random sequence, so any test that asserts exact values at a seed will fail until it is re-baselined.
- **Lot crowding:** a 3x2 lot takes 6 of the 40 table cells; with 10 definitions the odds are fine, but a bad roll with two lots and a CRT can leave an item unplaced (the existing re-roll-once-then-skip rule covers it).
- **New dependency:** the `GameplayTags` module touches `LD60.Build.cs` and needs the editor closed to build; the Gameplay Tag entries live in a config file (`Config/Tags/` or `DefaultGameplayTags.ini`) that must be committed.

## Deferred

- **Gameplay traits** (Boxed makes the item bigger, a trait that lowers the works chance): reopen if the packing puzzle feels too easy or traits feel like only a price tag. It changes the footprint in the grid, widgets and tests.
- **Opening a lot in the Garage** to spill N items into the trunk: its own grill, together with the "trunk is full" rule.
- **Trait pips on the grid:** reopen if scanning the table for traits feels slow in play.
- **Per-definition trait lists:** reopen if "Boxed Box of Floppies" or similar oddities bother you.
- **Odds shifting across days** (already deferred in the item-data design).

## Open threads

No discussion ended without a decision. Small details were left to the scaffold step and are for the implementation to fix, then record here:
- The roll weights of the five traits (Prototype is "low"; no other number was set).
- The `LotCount` of the Shoebox of Cassettes (the Box of Floppies is 10).
- The condition colours and badge placement on the cell.
- Whether the "Lot" tag is derived from the lot flag or set by hand on the two lot assets.
