# Flipper day loop (Phase 5): design

Grill session finished 2026-10-19. A **day** is three phases in order: **Market** (timed, 60 s), **Garage** (no clock, ends with a Done button, limited by 3 test charges) and **Sell** (timed, 45 s). Every phase can be ended early with a button. Unsold trunk items carry into the next day, the seller table is thrown away and re-rolled each day, and a one-second banner announces each phase. The run is 5 days (a data value). All numbers are data and are tuned in the balance pass.

## Terms

| Term | Meaning | Avoid |
|---|---|---|
| **Day** | One full cycle of Market, then Garage, then Sell. | round, turn, level |
| **Market phase** | The timed phase where the player buys from the seller table into the trunk. | buying phase, shop phase |
| **Garage phase** | The phase where the player tests Untested trunk items to reveal works or dead. | workshop, testing phase |
| **Sell phase** | The timed phase where the player sells trunk items to the buyer (mechanics in Phase 6). | Collectors phase, selling round |
| **Test charge** | One use of the Garage test action; a limited number per day. | test token, test point |

## Why

Without a clock the player can deliberate forever and the Market is just a puzzle with no pressure. Without a limit on testing, the Untested gamble is meaningless because every purchase is followed by a free reveal. The loop has to make the three jobs (buy, find out, sell) feel different, and it has to fit a 10 to 12 minute run for jam judges.

## Locked decisions

- **Testing uses limited charges.** The player gets a number of test charges per day (data value, start at 3). In the Garage, clicking an Untested item in the trunk spends one charge, sets `bTested`, and reveals works or dead. Unused charges do not carry over. *Rejected:* free unlimited testing (turns the Garage into a click-everything chore and makes the Untested gamble nearly meaningless); testing that costs cash (needs a new price rule and interacts awkwardly with refunds).

## Routine choices

- **Clocks:** Market 60 s and Sell 45 s are timed. The Garage has no clock; its limit is the test charges and it ends on a Done button. In the last 10 seconds the timer turns red, and a buzzer sounds at zero. *Rejected:* all three timed; one shared clock per day.
- **Hand-over:** every phase has an End phase button that moves on early. Leftover trunk items stay for the next day. The seller table is discarded and re-rolled each day. A one-second banner announces each phase. *Rejected:* no early end for Market and Sell; unbought seller items persisting to the next day.
- **Settled before this grill, carried over:** Market and Sell are timed; a held item snaps back when its timer runs out; the day count is data, start at 5; the seller table is re-rolled each day with the same generator (new `FRandomStream` per day so item Ids stay unique); the `bBoughtThisPhase` refund flag is cleared on all trunk items when the Market phase ends.

## Implementation notes

- A phase state machine (Market, Garage, Sell, next day) owns the current phase, day number, time left, and test charges left. Phase lengths, test charges and day count live in a config struct like `FFlipperPricingConfig`.
- At the end of Market: cancel any held item (snap back), then clear `bBoughtThisPhase` on every trunk item, then enter Garage.
- Entering Market on a new day: build a new `FRandomStream`, clear the seller table, and call `FillSellerTable` on the same generator. The trunk is untouched.
- Phase rules for input: in Market, the trade rule from Phase 4 applies. In Garage, no buying or refunds (cross-grid transfers refused) and clicks on an Untested trunk item test it. In Sell, behaviour is decided in Phase 6; until then, no transfers.
- The timer HUD shows the phase name, day number, time left (red when 10 s or less), and charges left in the Garage; plus the End phase button and the banner.

## Verified facts

- `UFlipperHeldItemController::Cancel()` exists and snaps a held item back; `TransferApprover` can refuse transfers by phase.
- `FFlipperItemInstance` already has `bTested`, `bWorks`, `bBoughtThisPhase`.
- `UFlipperItemGenerator::FillSellerTable(Table, Random)` fills a full day's table; one generator hands out all Ids.

## Risks

- Widgets that handle clicks (the End button) must `SetVisibility(Visible)`; the pure HUD must be `HitTestInvisible`.
- A purchase in the last frame of Market must not leave the item refundable into Garage: the flag clear has to happen after the held item snaps back.
- Sell-phase length (45 s) is a guess made before the selling mechanics are grilled.

## Deferred

- Sell-phase mechanics and the buyer's offer formula (Phase 6).
- What happens after the last day (win or lose, rent): Phase 7.
- Sound for the buzzer and the "WORKS!/DEAD" reveal: Phase 8.

## Open threads

None.
