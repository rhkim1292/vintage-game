# Flipper selling (Phase 6): design

Grill session finished 2026-10-19. In the **Sell phase** the right-hand grid (the seller table's screen slot) becomes the **buyer's counter**. Dragging a trunk item onto it, or Shift-clicking the item, sells it: the **offer** is paid into the wallet at once and the item is removed. Sales are final. The buyer pays for what is known: a Tested working item 100 percent of Value, a Tested dead item scrap, an Untested item 50 percent of Value whether or not it works. Every offer is scaled by the day's **mood** (Slow 80 percent, Normal 100 percent, Hot 125 percent), rolled when the day starts and shown on the HUD all day. All numbers are data and are tuned in the balance pass.

## Terms

| Term | Meaning | Avoid |
|---|---|---|
| **Buyer** | The single pawn-shop customer who pays cash for trunk items during the Sell phase. | collector, customer, shop |
| **Offer** | The cash the buyer pays for one specific item, from its Value, its tested state, whether it works, and the day's mood. | sale price, bid |
| **Buyer's counter** | The right-hand grid during the Sell phase. Dropping a trunk item on it sells the item. | third grid, shop window |
| **Mood** | One multiplier per day that scales every offer the buyer makes that day. | demand, market price |

## Why

Buying only makes sense if selling pays back more on average, and the Untested gamble and the Garage test charges only matter if the buyer treats known and unknown items differently. Selling also has to be quick to do under a 45 second clock and cheap to build in a jam, so it reuses the two grids, the trade rule and the drag code that already exist.

## Locked decisions

- **How to sell:** the right-hand grid becomes the buyer's counter in the Sell phase. When the Sell phase starts, the seller's unbought items vanish. Dropping a trunk item on the counter, or Shift-clicking it in the trunk, sells it and pays the offer at once. The item is removed and sales cannot be undone. The tooltip shows the buyer's offer for a trunk item during the Sell phase. *Rejected:* Shift-click only (a stray click sells something by accident, no deliberate gesture); a separate third panel (more UI and clutter for no gain).
- **What the buyer pays:** the buyer pays for what is known. Tested and working: 100 percent of Value. Tested and dead: scrap (10 percent of Value). Untested: 50 percent of Value, whether or not it works (the buyer does not open it). With the seller's asks (Tested about 75 percent, Untested about 50 percent), a Tested flip is a safe +25 percent, an Untested item sold as it is is about break-even, and testing it first is a gamble with about +18 percent of Value on average and a real chance to lose. *Rejected:* the buyer pays the truth whatever the flag says (the Garage test would have no purpose); the buyer refuses Untested items (the 3 test charges a day become a hard limit that can strand items, too punishing for a 5 day run).

## Routine choices

- **Variation: a daily mood** instead of a fixed or per-sale random offer. One multiplier per day scales every offer, including scrap. *Rejected:* a fixed offer with no noise (still a fair option; mood is one more roll but more to plan around); a random offer per item at the moment of sale (hides the price when the player decides).
- **Mood is known from the start of the day.** The HUD shows it during the Market, Garage and Sell phases, so the player plans purchases and tests around it. Tomorrow's mood is unknown, so holding an item overnight is a gamble. *Rejected:* revealing it only when the Sell phase starts; also showing tomorrow's mood as a forecast (a good later addition).
- **Mood shape: three named moods.** Slow 80 percent (rolled 25 percent of the time), Normal 100 percent (50 percent), Hot 125 percent (25 percent). The HUD shows the name in a colour (grey, white, green). The average is about 101 percent, so the economy set in Feature 3 stays put. A Slow day still leaves a Tested flip near break-even (80 percent of Value against a 75 percent ask). *Rejected:* a continuous 80 to 120 percent (hard to read at a glance); a continuous 70 to 130 percent (a bad day can turn a safe flip into a loss).

## Implementation notes

- The offer is a pure function in `FlipperPricing`: `GetOffer(Item, Config, Mood)`. Base is the Value (Tested and working), the scrap value (Tested and dead) or `Value x UntestedOfferFraction` (Untested). The mood multiplies the base and the result is rounded once to whole dollars, never below 1 for an item with a Definition. New data in `FFlipperPricingConfig`: `UntestedOfferFraction = 0.5`. The mood table (three moods with multiplier and weight) is data too.
- Mood is rolled by the day loop at the start of each day from its own `FRandomStream` (derived from `Seed` and `Day`, with a different salt from the seller table's stream, so the table roll and the mood roll do not disturb each other). A getter `GetMood()` and the mood name serve the HUD and the tooltip.
- `FlipperTradeRule` gets a sale case: in the Sell phase a move from the trunk to the counter is a sale. The day loop opens the rule for this direction only (the Market keeps its buy and refund rules). The offer is added to the wallet in `CommitTransfer`, and the sold item must not stay on the counter.
- At the start of the Sell phase the day loop clears the seller table (the seller has left) before the counter is used.
- HUD: replace the hint "Selling is not built yet" and show the mood next to the phase name. Tooltip: show "Offer $N" for trunk items during the Sell phase.

## Verified facts

- `FlipperPricing` already has `GetValue` and `GetScrapValue`, and the held-item controller already has `TransferApprover`, `TransferCommitted` and `QuickTransfer` between the two grids.
- The day loop already has the hook points: it owns the trade rule's open state, it rolls the seller table with `FRandomStream(Seed + Day)` and it knows the phase.
- The trade rule is currently closed outside the Market, so a Sell phase move is refused today.

## Risks

- Mood weights and the Untested offer fraction are first guesses; the balance pass must check that five days at $200 start can reach a sensible rent target.
- A sold item must leave the counter. If the counter holds it, it blocks the next sale.
- At the start of the Sell phase a held item (snap back) must be handled the same way as at the end of Market.

## Deferred

- A forecast of tomorrow's mood (reopen if players want to plan holds).
- More than one buyer, requests such as "working, boxed, any handheld" (frozen in `Docs/FlipperPlan-Full.md`).

## Open threads

None.
