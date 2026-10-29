# Flipper collectors with requests (design)

Grill session: 2026-10-25, topic "Collectors with requests (Flipper step 2)". This is step 2 of the post-jam work that follows [FlipperPlan-Full.md](FlipperPlan-Full.md) (Phase 6, collectors and selling), after step 1, [FlipperItemDataFull-design.md](FlipperItemDataFull-design.md). It extends [FlipperSelling-design.md](FlipperSelling-design.md) (the single buyer, the daily mood, the offer formula) and uses the tags added in step 1. Everything in those documents stays in force except where this one says it extends them.

## Summary

Each day three **collectors** appear, each with one **request**: a list of clauses an item must satisfy ("working, Boxed, Handheld, Fair or better"). The requests are visible from the first second of the Market phase and disappear when the Sell phase ends, so the day becomes: read the board, buy toward it, test the maybe-working items in the Garage, then deliver. In the Sell phase the right-hand panel shows one **collector card** per collector plus a permanent **pawn shop** card. The player drags a trunk item onto the card of their choice (or Shift-clicks it). A matching item sells to that collector at the plain offer times a **premium** that grows with the number of clauses; a non-matching drop is refused and snaps back. A collector buys once, then the card shows Sold. The pawn shop keeps today's behaviour: it takes anything, any number of items, at the plain mood-scaled offer. Requests are built from real items on the day's seller table, so every request can be met by buying something that is actually there. Holding an item shows each card's exact payout (or the reason it is refused), and hovering a card outlines every trunk item that satisfies it. No prototype or diagram was drawn for this grill.

## Terms

| Term | Definition | Avoid |
|---|---|---|
| **Collector** | A buyer character who shows up on a given day with one request and pays a premium for an item that satisfies it. Buys one item, then is done for the day. | customer, NPC, client |
| **Request** | What one collector wants, written as clauses the item must satisfy. | order, quest, contract |
| **Clause** | One requirement inside a request: a category tag, a trait tag, must work, a minimum condition or a minimum rarity. A request has 1 to 3. | filter, rule, condition (already means item grade) |
| **Pawn shop** | The always-present last buyer card: takes any item, any number of times, at the plain mood-scaled offer (times a data number that is 100 percent for now). Today's single buyer. | the counter, default buyer |
| **Premium** | The multiplier a collector pays over the plain offer when the item satisfies the request. Set by the number of clauses. | bonus, markup, tip |
| **Collector card** | The on-screen panel for one collector in the Sell phase: name, request, premium, state (open or Sold). Items are dragged onto it. | slot, customer panel |
| **Board** | The day's list of collector requests, visible from the start of the Market phase. | quest log, order list |

## Why

The jam build sold to one anonymous buyer at a formula price, so condition, traits and categories had nothing to be for. The full plan (Phase 6) wants collectors with requests such as "Wanted: working, boxed, any handheld" so that what you buy, what you test and what you keep become decisions. In the user's words: "I want to go back to following FlipperPlan-Full", then "ready for step 2". Step 1 added the properties a request can mention (category tags, trait tags, condition, the works flag, rarity); this step adds who asks for them and how the player delivers.

## Locked decisions

These are hard to reverse, would surprise someone without context, and involved a real trade-off.

### 1. Collectors are drop-target cards (q1, answer A)

- In the Sell phase each collector is a card. The player drags a trunk item onto the card they want to sell to. A matching item is paid that collector's price; a non-matching drop is refused with the red ghost and snaps back. Shift-click sells to the best-paying matching collector (and see decision 10 for the fall-through).
- This gives the held-item controller a second kind of drop target next to the inventory grid. It is a one-time, bounded change: press-drag-release input stays as it is.
- *Rejected: keep the single counter grid with a read-only request list (B):* cheapest, but the game would pick the best match for the player, which removes the choice the feature exists for. *Rejected: one collector at a time, walking up in a queue (C):* adds queue state and animation, and a slow turn would waste the 45 second Sell phase.

### 2. Requests are posted at the start of the day and gone at its end (q2, answer A)

- The day's collectors and requests show from the first Market second, like the buyer mood does, and expire when the Sell phase ends. Nothing carries to the next day (the trunk's items do, as before).
- It creates the loop the full plan wants (read the board, buy toward it, test, deliver) and adds no persistence: a fresh list is rolled each day.
- *Rejected: revealed only when the Sell phase starts (B):* the Market phase would ignore requests, wasting the tag data. *Rejected: standing requests that last 2 or 3 days (C):* needs persistence, expiry and a days-left display and rebalancing for a 5-day run. It can be added on top of this later by letting an unfulfilled request roll over.

### 3. The pawn shop stays as a permanent last card (q4, answer A)

- It takes any item, any number of times, at the plain mood-scaled offer: today's rule, with one new data number, the pawn shop fraction, set to 1.0. Collectors are pure upside over it.
- It keeps the tuned economy ($450 rent, $200 start, ask prices of 50 to 75 percent of Value) intact, so a rebalance can see what collectors themselves add, and it protects the unlucky day with no matching request.
- *Rejected: no pawn shop (B):* unmatched items could only wait in the 40-cell trunk; needs trunk size, request frequency and balance reworked together. *Rejected: a fixed worse rate of 70 percent (C):* it is one number away through the data fraction if collectors turn out to be too small an upgrade.

### 4. An Untested item cannot satisfy a must-work clause (q5, answer A)

- Only an item that is Tested and working satisfies a must-work clause. An Untested item is refused on that card until it has been tested in the Garage.
- It follows the rule already in force ("the buyer pays for what is known") and gives the 3 daily test charges a second job: a test also unlocks must-work requests, so the player chooses which maybes to test.
- *Rejected: accept an Untested item as a gamble and pay scrap if it turns out dead (B):* the sale resolves a hidden roll at the till and needs a new payout rule. *Rejected: accept it at the Untested offer and ignore the flag (C):* it makes the must-work clause meaningless in practice.

### 5. Every request is built from a real item on the day's table (q6, answer A)

- The seller table is rolled first. Each request is then made from one real item on it: take that item's properties and keep a random subset of them as the clauses. Every request can therefore be met by buying something that is on today's table (a must-work clause still depends on testing).
- The request keeps only some of the source item's properties, so many other items on the table and in the trunk also qualify, and the request is not a pointer at one item.
- Requests use their own random stream, like the buyer mood, so the table roll for a given seed does not change.
- *Rejected: independent random requests (B):* the board would be decorative on bad days. *Rejected: also build from the trunk (C):* the generator's output would depend on what the player did, so it is harder to test and balance, and a rich trunk would make trivial requests.

### 6. A collector buys once (q9, answer A)

- One item per collector. After a sale the card shows Sold, takes no more drops and is greyed out for the rest of the day. The pawn shop stays open for any number of items.
- It caps collector income at (collectors x premium) a day, which makes the economy easy to balance, and it makes pairing meaningful: with two collectors who both want a handheld, the player decides which gets the better one.
- *Rejected: unlimited (B):* the premium would become a flat bonus on a whole category and the scarcity the board creates would go. *Rejected: a quantity on the request, "0/2" (C):* a good later extension, but it adds a quantity roll, a partly-filled state and a trunk-capacity problem to the first build.

### 7. The premium grows with the number of clauses (q7, answer B)

- Collector price = the plain offer (FlipperPricing::GetOffer rules) times today's mood multiplier times a premium taken from a data table by clause count: 1 clause x1.25, 2 clauses x1.5, 3 clauses x1.75. Rounded once, at the end.
- A harder request pays more, and the price still comes from the item actually handed over, so rarity, condition and traits keep mattering.
- *Rejected: one flat premium (A):* no link between effort and reward, so one-clause requests would be the best deal. *Rejected: a fixed dollar price printed on the card (C):* most readable, but it ignores rarity, condition and traits and rewards dumping the cheapest qualifying item every time.

## Routine choices

- **What a request can say (q3, B):** the five clause kinds: one category tag, one trait tag, must work, at least a condition and at least a rarity. An item satisfies a request when it meets every clause. *Rejected:* tags and works only (A; the safe subset and a fallback if this plays too hard); naming one item (C; it ignores the tags just added).
- **Collectors per day (q8, A):** 3 collectors every day, each request with 1, 2 or 3 clauses at weights 40 / 40 / 20. The numbers are data and are stored so that a per-day curve can be added later. *Rejected:* a ramp of 2, 2, 3, 3, 4 collectors with harder requests later (a balance-pass question, cheap to add afterwards); 4 collectors with 1 or 2 clauses (more simultaneous decisions under a 45 second timer).
- **Shift-click (q10, A):** sells to the best-paying open collector the item matches; if it matches none, it sells to the pawn shop, exactly as Shift-click sells today. Dragging onto a specific card is how the player chooses who gets the item. *Rejected:* Shift-click does nothing when no collector matches (it would sometimes silently do nothing, which reads as a bug).
- **Collector identity (q11, A):** a small data asset per collector holds a name (and later a portrait). Each request is given an unused collector at random. No gameplay effect. *Rejected:* archetypes with interests (B; a pairing step that changes no decision, and it can be added later on top without changing request data); unnamed "Collector 1, 2, 3" (C).
- **Feedback (q12, C):** while an item is held, each card shows the exact payout for it, or a short reason it is refused ("needs working"), and the card under the cursor lights green or red. Hovering a card also outlines every trunk item that satisfies it. *Rejected:* request text only, found out by dropping (A; makes the player memorise the trunk); live payout without the highlight (B; the minimum that stops drops feeling like guesses).
- **Daily mood applies to collectors too** (assumption stated in q7, accepted): one mood for all buyers, scaling every offer including the premium price.
- All numbers are data.

## Implementation notes (derived from the decisions above)

Not yet built. These follow from the decisions and from reading the current code.

- **Request type.** A request holds: a Gameplay Tag container of required tags (at most one category tag and one trait tag), a must-work flag, a minimum condition (Poor means no clause) and a minimum rarity (Common means no clause). The clause count is the number of those that are set. An item's tags are its definition's Categories plus the Tag of each of its traits; a request is met when the item carries all required tags, meets the minimum condition and rarity, and, if must-work is set, is Tested and working.
- **Pure functions** (like FlipperPricing): `Matches(Item, Request)`, the premium by clause count, and the collector price. The collector price reuses `FlipperPricing::GetOffer` with the multiplier passed in as mood times premium, so rounding still happens once. A dead Tested item that matches (a request without must-work) is paid scrap times the premium, and an Untested one 50 percent of Value times the premium; this follows from reusing GetOffer and was not asked separately.
- **Generation.** After the seller table is rolled for the day, pick a distinct item from it for each collector (fewer collectors if the table has fewer items). The pool of clauses available from that source item is: one of its category tags (always at least one), one of its trait tags (only if it has one; about 45 percent of items do), must-work, its condition (only if above Poor) and its rarity (only if above Common). Pick the clause count from the 40 / 40 / 20 weights, capped at the pool size, then that many distinct clause kinds from the pool at random. Two collectors may want the same category. The request uses its own FRandomStream, rolled after the table and the mood. The fixed-draws rule from step 1 applies: each request takes the same number of draws on every path, whatever the pool, so a seed stays aligned.
- **Collector data asset.** `UFlipperCollectorDefinition` (a primary data asset) with a display name; about six to start. Each request is given an unused definition at random, reused if there are more collectors than definitions. Needs the cook-list entry in `DefaultGame.ini`, like the traits.
- **Day state.** A collector is a definition, a request and a sold flag. The day loop rolls the board when a day starts (Market), keeps it through Garage and Sell, and clears it when the Sell phase ends. The seller table is already emptied when Sell starts.
- **Selling.** The trade rule's existing sale concept (`IsSale`, `GetOffer`, the controller's `TransferConsumes`) generalises from "trunk to the seller table" to "trunk to a chosen buyer": a collector card or the pawn shop card. A refused drop uses the existing red ghost and snap-back. Shift-click picks the best-paying open matching collector, with ties going to the first card, and falls back to the pawn shop.
- **Pawn shop fraction.** A new number in the pricing config, 1.0, multiplied into the plain offer for the pawn shop only.
- **UI.** In the Sell phase the right-hand panel shows the collector cards and the pawn shop card in place of the seller table grid. Each card shows the name, the request in words, the premium, the live payout for a held item, and Sold when closed. Widgets paint from C++ in `NativePaint` with `FlipperPaint`, and use `FlipperPaint::CursorLocal` for the cursor. The trunk grid outlines items that satisfy a hovered card. Tooltips are unchanged.
- **Tests (planned).** Matching for each clause kind and for several together; the Poor and Common thresholds; Untested against must-work; premium by clause count and the rounding-once rule; the clause count weights; request generation for a seed (deterministic, every request met by its source item except for the works status, distinct sources, own stream so the table roll does not change); the fixed draw count; sale to a card (paid once, second drop refused, Sold state); Shift-click choosing the best-paying card and falling back to the pawn shop; the board's lifecycle across the day loop.

## Verified facts

- The Sell phase today turns the seller table grid into a single buyer's counter; a drop or Shift-click sells and pays `GetOffer` at once (`FlipperTradeRule`, `IsSale`, `CommitTransfer`). The seller table is emptied when Sell starts, so the right-hand panel has nothing else to show then. The day loop already rolls the buyer mood on its own random stream when a day starts.
- `FlipperPricing::GetOffer(Item, Config, MoodMultiplier)`: Tested and working pays 100 percent of Value, Tested and dead 10 percent (scrap), Untested 50 percent, times the multiplier, rounded once, minimum 1.
- Items carry category tags (on the definition, 1 to 2 each, plus a Lot tag on lots), trait tags (0, 1 or 2 per item at 55 / 35 / 10 percent), condition, rarity and a works flag. The Market is 60 s, Garage untimed with 3 test charges, Sell 45 s, a run is 5 days.
- The held-item controller currently resolves drops against inventory grids only.

## Risks

- **The held-item controller refactor.** A second kind of drop target touches the controller, the trade rule and their tests (about 95 tests exist around grids, the controller and trading). Mitigation: keep grid-to-grid behaviour unchanged and add the card path beside it; run the full suite after each step.
- **Screen space.** The board must be visible during Market and Garage as well as Sell (decision 2), and there is no place for it yet. See Open threads.
- **Balance.** Collectors add up to 3 sales a day at x1.25 to x1.75 on top of today's economy, while condition already lowers average prices about 9 percent. The $450 rent will likely need re-tuning in Phase 9; the pawn shop fraction, the premium table, the collector count and the clause weights are all data for that.
- **Satisfiable but not trivial.** A must-work clause from an Untested source item may be dead, and a trait clause exists only on items that have a trait. Watch in play checks that boards are neither impossible nor always trivial; clause weights and the pool rules are the levers.
- **Float and draw alignment.** Request generation must follow the fixed-draws rule, or seeded boards will drift whenever the pool changes.

## Deferred

- **Requests that last several days** (q2 C). Reopen if a 5-day run feels too reactive; a rolled-over unfulfilled request is the natural addition.
- **Quantity requests, "wants 2"** (q9 C). Reopen once the single-item cards play well.
- **Collector archetypes with interests** (q11 B). Reopen in the presentation step if the cards need more character.
- **A ramp of collector count and difficulty across days** (q8 B). Reopen in the balance pass.
- **A worse pawn shop rate** (q4 C). One data number; change it in the balance pass.

## Open threads

- **Where the board lives during Market and Garage (SETTLED 2026-10-28, at scaffold stage 2).** One board widget sits directly under the day HUD in the right column: a compact strip of three request lines in Market and Garage that grows into the full collector cards plus the pawn shop card in Sell, with the seller table below it. The screen block gets about 90 px taller. Rejected: a narrow strip under the trunk (only 256 px wide, in a different place from the Sell cards); full cards in Garage and Sell with a strip only in Market (two looks for the same data).
- **How the premium is shown on a card.** Written as a multiplier ("x1.5") or as the live payout only. Left to the presentation step.
