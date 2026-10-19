# Flipper pricing and cash: design

Grill session: 2026-10-18, topic "Flipper Phase 4: pricing and cash". Scope is the **jam build** (see `Docs/FlipperPlan.md`). Debt, interest and the credit limit are cut for the jam and stay in `Docs/FlipperPlan-Full.md`. No prototype or diagram was drawn for this grill.

## Summary

Every item has a **Value**: its definition's base value times a rarity multiplier (Common x1, Uncommon x2, Rare x4), rounded to whole dollars. A working item is worth its Value to a buyer; a dead item is worth only **scrap value**, 10 percent of Value. The seller charges an **ask price** fixed when the item is rolled: Tested items ask 75 percent of Value, Untested items ask 50 percent, then each item gets a random variation of plus or minus 20 percent. Buying happens when you drop an item from the Seller table into the trunk: it pays the ask price, and the drop is refused if you cannot afford it. Moving an item you bought **this Market phase** back to the table refunds exactly what you paid; anything older cannot go back. The player starts with $200 and an empty trunk. All numbers live in data.

## Terms

| Term | Definition | Avoid |
|---|---|---|
| **Value** | What an item is worth to a buyer when it works: base value x the rarity multiplier, rounded to whole dollars. | price, worth (unqualified) |
| **Scrap value** | What a dead item is worth: 10 percent of its Value, rounded. | junk price |
| **Ask price** | What the seller charges for an item on the Seller table. Fixed when the item is rolled and shown in the tooltip. | buy price, cost |
| **Refund** | Moving an item you bought this Market phase back onto the Seller table returns exactly what you paid. | sell-back, return |
| **Cash** | The player's money. A whole-dollar number that never goes below zero in the jam build. | balance, funds |

## Why

Phase 3 gave each item a base value, a rarity and a hidden works/dead flag, but nothing says what any of it costs. This grill fixes the numbers so that buying is a real decision: Tested items are the safe, thinner margin, Untested ones are cheaper and riskier, and the per-item variation makes some items bargains and others overpriced, so scanning the table matters beyond space and rarity.

## Locked decisions

### 1. Value: rarity multipliers and dead items (q1, answer A)

- Value = base value x rarity multiplier, rounded to whole dollars. Multipliers: **Common x1, Uncommon x2, Rare x4**. A **dead** item is worth **10 percent of its Value** (scrap), rounded.
- Example: a Rare CRT Television is worth $480; a dead Common Game Boy is worth $4 (3.5 rounded).
- *Rejected:* gentler multipliers x1 / x1.5 / x2.5 (Rare is only about 5 percent of rolls, so it needs a big multiplier to feel like a jackpot); dead items worth nothing (a dead buy would be a total loss with no recovery, and the 10 percent keeps the sting without erasing the gamble).

### 2. Ask price: Tested vs Untested, with per-item variation (q2, answer A)

- Tested items ask **75 percent** of Value; Untested items ask **50 percent**; then each item gets a random variation of **plus or minus 20 percent**, rolled once with the item and stored on it. The ask price is rounded to whole dollars (minimum $1).
- The ask for an Untested item does not depend on whether it works: the seller does not know.
- Worked example (Common Game Boy, Value $35): Tested asks about $26 and sells for $35 (profit about $9). Untested asks about $18; it sells for $35 about 65 percent of the time and for $4 scrap otherwise, about $24 on average (profit about $6, with real swings). Averaged over the variation, Tested returns about +25 percent of Value and Untested about +18 percent, so neither is a trap.
- *Rejected:* no variation (the best buy is always the same item type, so there is little to decide besides space); 80 / 40 percent (the safe margin gets thin and the gamble too cheap).

### 3. Starting cash, starting trunk and refunds (q3, answer A)

- The player starts with **$200 cash** and an **empty trunk**.
- Buying is **buy on drop**: moving an item from the Seller table into the trunk (drag-drop or Shift-click) pays its ask price. If cash is less than the ask, the drop is **refused**: the item snaps back and the ghost is red.
- **Refund:** moving an item you bought **this Market phase** back onto the Seller table returns exactly what you paid. An item bought on an earlier day, or any item that did not come from the table, cannot go onto the table (the drop is refused). Moves inside one grid are always free.
- Why the refund is limited to the phase: otherwise a player could buy an Untested item, test it in the Garage and return it for a full refund if it is dead, which removes the whole gamble.
- *Rejected:* two free Untested starter items in the trunk (skips the first Market lesson); $100 start (a tighter start would be a balance-pass choice, not a first guess).

## Routine choices

- **Cash display:** a cash HUD in a corner of the screen showing the current whole-dollar amount.
- **Tooltip:** gains an ask-price line for items on the Seller table (the tooltip never shows base value).
- **Numbers in data:** rarity multipliers, scrap fraction, Tested and Untested ask fractions, variation range and starting cash are all config values so the balance pass can retune them without recompiling.
- **Rounding:** all money is whole dollars (`int32`); Value, scrap and ask are each rounded to the nearest dollar.

## Implementation notes (derived from the decisions above)

- **Instance fields:** `FFlipperItemInstance` gains `AskPrice` (set by the generator when the item is rolled), `PaidPrice` and `bBoughtThisPhase` (set when bought, cleared when a Market phase ends; the day loop owns the clearing).
- **Pricing functions:** `GetValue(Item)` and `GetScrapValue(Item)` and `GetAskPrice` roll (the variation is rolled in the generator with its `FRandomStream`, so tests stay deterministic).
- **Wallet:** a small class holding cash, with `CanAfford`, `Spend` and `Add`.
- **Transfer rule:** plugs into `TransferApprover` on the held-item controller: table to trunk requires `CanAfford(AskPrice)` and then charges it; trunk to table requires `bBoughtThisPhase` and then refunds `PaidPrice`.
- **Generator change:** `RollItem` must now also set `AskPrice`, so the pricing config reaches the generator.

## Verified facts

- `FFlipperItemInstance` currently holds `Id`, `Definition`, `Rarity`, `bTested` and `bWorks`; `UFlipperItemDefinition` has `BaseValue` (the 6 starter assets are $6 to $120). The generator takes an `FRandomStream&` and is covered by 11 tests.
- `TransferApprover` is a `TFunction` on the held-item controller, unbound by default (every cross-grid move allowed), and consulted only for moves between different grids.

## Risks

- The numbers are first guesses; the balance pass will move them. The rent target in the win/lose grill must be set against the actual profit per day.
- `bBoughtThisPhase` needs the day loop to clear it at the end of each Market phase, or old items could be refunded.

## Deferred

- **The buyer's offer** (what the pawn-shop buyer pays, and whether it equals Value): the selling grill, Phase 6.
- **Debt, interest, credit limit:** cut for the jam, back in the post-jam build.

## Open threads

None. Every discussion in this grill ended in a decision.
