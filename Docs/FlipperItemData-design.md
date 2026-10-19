# Flipper item data and generation: design

Grill session: 2026-10-18, topic "Flipper item data and generation". Scope is the **jam build** (see `Docs/FlipperPlan.md`); the full original ambitions (traits, condition, lots) are in `Docs/FlipperPlan-Full.md`.

## Summary

Every item the player sees is a rolled copy (`FFlipperItemInstance`) of one of 6 item definitions (data assets). Each copy has a **rarity** rolled per item from 3 tiers (Common grey about 70%, Uncommon blue about 25%, Rare gold about 5%) and a hidden **works/dead** flag. On the seller table each item is either **Tested** (known working, priced higher) or **Untested** (hidden; about 65% turn out to work, about 35% are dead), with about 60% of items Untested. Each day the generator rolls a fixed count of 8 items onto the 8x5 seller table, equally likely per definition, placed with `FindFirstFit`. The hover tooltip shows name, footprint, rarity and a status line. On the grid, the border colour shows rarity and a small "?" marks Untested items. No prototype or diagram was drawn for this grill.

## Terms

| Term | Definition | Avoid |
|---|---|---|
| **Item definition** | The static description of one kind of item (name, footprint, base value): a data asset such as "Game Boy". | item type, item class, template |
| **Item** | One rolled copy of a definition (`FFlipperItemInstance`): a unique id, its definition, a rolled rarity, a hidden works/dead flag and a tested flag. | object, product, loot |
| **Rarity** | A tier rolled per item (Common grey, Uncommon blue, Rare gold) that sets its border colour and a price multiplier. Two Game Boys can differ in rarity. | quality, grade |
| **Untested** | An item whose works/dead flag is hidden from the player until it is tested in the Garage. | unknown, mystery |
| **Tested** | An item whose works/dead flag has been revealed. A Tested item on the seller table is always working; an Untested one becomes Tested after the Garage test. | verified, checked |
| **Seller table** | The 8x5 grid of items the seller offers on one day, rolled fresh each day. | shop, stock, market |

## Why

The jam plan cut traits and lots, so rarity and the hidden works/dead flag are the only things that make two copies of the same item different. Together they are the "gamble" in buying: scan the table, spot a gold border, decide whether to pay more for a Tested copy or take the discount on an Untested one. The 6 starter items have a mix of footprints so packing the 10x6 trunk stays a puzzle.

## Locked decisions

These are hard to reverse, would surprise someone without context, and involved a real trade-off.

### 1. Rarity: 3 tiers, rolled per item (q1, answer A)

- Tiers: **Common** (grey, about 70%), **Uncommon** (blue, about 25%), **Rare** (gold, about 5%). Weights and price multipliers live in data so they can be tuned without recompiling. The multipliers' values are for the pricing grill (Phase 4).
- The roll happens per item at generation time, so the same definition can be Common one day and Rare the next.
- *Rejected: four tiers (add Epic, purple).* One more colour and one more weight to tune, for a tier almost nobody would see in 5 days.
- *Rejected: rarity fixed per item type.* It turns rarity into a lookup, removes the surprise, and means the seller table can never contain a lucky find.

### 2. The Untested gamble: a mixed Tested/Untested table (q2, answer A)

- Each item on the seller table is **Tested** (known to work, priced higher) or **Untested** (works/dead hidden). About **60%** of items are Untested. Of Untested items, about **65% work and 35% are dead**. A dead item is never labelled on the table; it only exists as an unlucky Untested buy. All these numbers live in data.
- The Garage test reveals an Untested item's flag (the WORKS/DEAD reveal).
- *Rejected: everything Untested.* No choice to make, so buying becomes pure luck.
- *Rejected: three labels (Tested Working, Tested Dead, Untested).* Adds a fourth state and a scrap sub-economy (who buys dead items?) the jam does not have time for.

## Routine choices

- **Starter items (q3, A):** Floppy Disk 1x1 ($6), Game Boy 1x2 ($35), Polaroid Camera 2x2 ($45), Rotary Phone 2x2 ($55), Typewriter 3x2 ($70), CRT Television 4x3 ($120). Base value is the price of a Common, working copy; the pricing grill decides the formula that uses it. All six are data assets, so they can be renamed or swapped freely. The 4x3 TV takes 12 of the trunk's 60 cells on purpose.
- **Daily seller table roll (q4, A):** a fixed count of **8 items** per day (a number in data). Each roll picks a definition with equal chance, then rolls rarity and Tested/Untested (and the works/dead flag for Untested). Items are placed with `FindFirstFit` in roll order; if one does not fit it is re-rolled once, then skipped. The odds are the same every day. The generator takes a random seed (e.g. an `FRandomStream`) as a parameter so automation tests can check the odds deterministically. The count and the definition weights live in data, so odds shifting across days can be added later without a rewrite.
- **Tooltip fields (q5, A):** four lines: name, footprint ("W x H"), rarity (the word, in the rarity colour), and a status line: "Tested: works" (green) or "Untested" (grey). After a Garage test the status line shows "WORKS" or "DEAD". A price line is added later by the pricing grill. Base value is not shown.
- **Marks on the grid (q6, A):** border colour = rarity (grey, blue, gold). A small "?" in the corner of Untested items; no mark on Tested ones. After a Garage test the "?" goes away and a red "X" appears on dead items.

## Verified facts

- `UFlipperItemDefinition` and `FFlipperItemInstance` already exist from Phase 1 (name, footprint, id). The instance does not yet carry rarity, the works flag or the tested flag; Phase 3 extends them.
- `UInventoryGrid::FindFirstFit` exists (unrotated, then rotated, row by row) and is the intended way to place generated items.
- `UFlipperItemTooltip` currently paints name and "W x H" only; `UInventoryGridWidget::NativePaint` currently draws every item with a light-grey outline and no marks. Both need updating for q5 and q6.
- The grid widgets paint everything from C++ (`NativePaint`), so the border colour and the "?" and "X" marks are paint calls, not assets.

## Risks

- **A 64-pixel cell is small:** the "?" and "X" marks and the label must stay readable on a 1x1 Floppy Disk.
- **Odds are untuned:** 60% Untested, 65% working and 5% Rare are first guesses; the balance pass decides if the gamble is fun or punishing.
- **Fixed 8 items may collide with big rolls:** two or three TVs can leave an item unplaced on the 40-cell table. The re-roll-once-then-skip rule means the table can occasionally have 7 items, so tests should assert "at most 8" with a seed that fills, not exactly 8 for every seed.

## Deferred

- **Odds shifting across days** (Rare chance or big-item weight growing per day): post-jam, or if the balance pass shows the table is too flat. Reopen if 5 days feels samey.
- **Trait system, condition and bundled lots:** cut for the jam, kept in `Docs/FlipperPlan-Full.md`.
- **Tested Dead scrap items:** cut with the pawn-shop-only economy; reopen with more buyers.

## Open threads

None. Every discussion in this grill ended in a decision.
