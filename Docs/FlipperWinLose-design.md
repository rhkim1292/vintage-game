# Flipper Phase 7: rent target and win/lose (design)

Grilled on 2026-10-19 (grill-with-ui session `20261019-022004`). Project Flipper, Ludum Dare 60, branch `main`.

## Summary

A run lasts `DayCount` days (5). After the last Sell phase of the last day the game checks the player's **cash** against a single **rent** amount ($450). Cash at least equal to the rent is a win ("You made rent!"); anything less is a loss ("Evicted!"). Items left in the trunk do not count. The end screen shows the headline, the final cash, the rent and the number of days played, and has a **Play Again** button that starts a fresh run with a new random seed. The rent amount lives in data (`FFlipperDayConfig`), so the balance pass can retune it without recompiling.

## Terms

- **Rent**: the cash amount the player must have at the end of the run. The goal of the run. Avoid: target, quota, debt.
- **Rent check**: the single test of cash against the rent, done once when the run reaches `Finished`. Avoid: deadline, checkpoint.
- **Run result**: InProgress, Won or Lost. Avoid: score, grade.
- **End screen**: the full-screen Win/Lose panel shown in the `Finished` phase. Avoid: game over screen (it also shows wins).

## Why

The theme is making rent by flipping old tech. So far the run just stops after day 5 with no verdict. A rent goal turns buying, the Untested gamble and selling into a decision with something at stake, and gives the player a reason to try again.

## Locked decisions

- **One rent check, at the end of the last day** (q1, option A). Rejected: rent every day, rising (B), and rent on a few days only (C). Both end the run early on a single miss, which punishes the Untested gamble that is the heart of the game, and their numbers are much harder to balance in the time left. Switching later is a data and logic change, not a rewrite of the screens, because the check lives in one place.

## Routine choices

- **Cash only counts** (q2, A). Unsold trunk items are worth nothing at the check, so the player must sell them in the last Sell phase. Rejected: cash plus the items' would-be offers (an Untested offer is a guess and it needs a second HUD number).
- **End screen** (q3, A): headline, final cash, rent, days played, Play Again. Play Again starts a fresh run with a new random seed. Rejected: a run summary with items bought, sold and best sale (needs new counters; a candidate for Phase 8) and replaying the same seed (teaches the player the answers and flattens the gamble).
- **Rent is $450** (q4, A), i.e. a net gain of $250 over the $200 starting cash. A first guess; Phase 9 tunes it from play-throughs.

## Implementation notes (for Phase 7.2)

- `FFlipperDayConfig` gets `RentAmount` (default 450, ClampMin 0).
- `UFlipperDayLoop` is given the wallet (a `SetWallet`, null allowed so existing tests do not change). Entering `Finished` sets a result: Won when `Wallet->GetCash() >= RentAmount`, otherwise Lost. A getter `GetResult()` returns InProgress before that. With no wallet the result stays InProgress.
- A pure helper in `FlipperPricing` or the day loop (`IsRentMet(Cash, Rent)`) so the rule is one testable line.
- New `UFlipperEndScreen` widget, painted from C++ like the other panels: full-screen dim, headline, three numbers, one Play Again button (small, `Visible`, handles its own click). Shown only in `Finished`. The phase banner is left alone.
- The day HUD shows the rent for the whole run ("Rent $450") next to the cash or the day, so the player always knows the goal. In the last day it can turn red if cash is below the rent.
- Play Again reloads the level, which builds a new controller and picks a new `FMath::Rand()` seed. The trunk, wallet, day loop and mood all start fresh.
- Tests (spec): the result is InProgress before `Finished`; exactly the rent wins; one dollar below loses; items left in the trunk do not change the result; the result is set only once the last Sell phase ends; no wallet leaves it InProgress.

## Verified facts

- `EFlipperPhase::Finished` exists and the day loop already enters it after the last day's Sell phase. It has no screen yet.
- `UFlipperWallet` holds the cash and never goes negative. The starting cash is $200 (`FFlipperPricingConfig::StartingCash`).
- The seed for a run is `FMath::Rand()` at `AFlipperTestPlayerController::BeginPlay`, so a level reload gives a new run.

## Risks

- The rent number is untested. If too high or too low the game is dull; tune it in Phase 9 after at least two full runs.
- A player with $0 and an empty trunk cannot recover; they will watch the rest of the run. Accepted for the jam (a quick "give up" is the End phase button).
- The end screen is C++ painted and click handling needs the small, `Visible` widget pattern used by the day HUD.

## Deferred

- A run summary (items bought and sold, best sale, profit). Reopen if there is time in Phase 8.
- Rent every day or on checkpoint days with a rising amount (difficulty curve). Reopen if the single check feels too relaxed.
- Giving up early or ending the run early on a hopeless state.

## Open threads

None.
