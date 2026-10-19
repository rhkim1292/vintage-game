# Flipper Build Plan (jam scope)

The step-by-step plan for finishing **Flipper** (Ludum Dare 60, theme "old tech") before the jam deadline. It shows the order features get built, the steps inside each feature, and the non-feature work (builds, packaging, submission).

- **Spec:** [FlipperSpec.md](FlipperSpec.md) holds the design decisions. This file holds the order and the status.
- **Full plan:** [FlipperPlan-Full.md](FlipperPlan-Full.md) is a frozen copy of the ORIGINAL, uncut plan (all of Phases 3 to 10 with traits, debt and interest, collector requests, icons, CRT look, music). This file is the cut-down jam version. See "What the jam plan cuts" below, and use the full plan to keep building after the deadline.
- **Repo:** https://github.com/rhkim1292/vintage-game (public, `main`, Git LFS for assets)
- **Deadline:** about **Monday 2026-10-19, ~3 PM Pacific**. This is derived from "2 days 18 hours left" on Friday evening, 2026-10-16. **Confirm the exact time on ldjam.com** and fix this line; the whole schedule below hangs on it.
- **Rule of thumb:** a complete, packaged, submitted game beats a better unfinished one. If a checkpoint slips by more than ~1 hour, apply the cut list.

**Keep this file current:** tick boxes as steps finish, and update the "Where we are" block at the end of every session.

---

## Where we are

- **Last updated:** 2026-10-18, ~11 PM PT (end of session 3: Phases 1 to 3 built, 36 of 36 `Flipper.*` tests pass; scope cut for the jam; Phase 3 uncommitted; next is the 3.5 play-check, then Phase 4)
- **Done:** concept chosen; inventory grid grilled and specced; git repo created and pushed. Phase 1: all of `UInventoryGrid` (11 of 11 `Flipper.InventoryGrid.*` tests) and an early Shipping package (see 1.3). Phase 2: the user implemented `UFlipperHeldItemController` (14 of 14 `Flipper.HeldItem.*` tests, so 25 of 25 `Flipper.*` tests); Claude wrote the widgets and the test screen (`UInventoryGridWidget`, `UFlipperHeldItemOverlay`, `UFlipperItemTooltip`, `AFlipperTestPlayerController` / `AFlipperTestGameMode`, the level under `Content/Flipper/`), and the user wrote `ScreenToCell`. The user played it and reports everything working. Last commit 3ec99ec, pushed. `.clang-format` is in the repo; a local `.vscode/settings.json` has format-on-save.
- **Next step:** Phase 3.5 (play the test screen and check the rolled items look right, then commit), then Phase 4 (pricing and cash). Phase 3 so far: the grill is done and in the spec as Feature 2; `UFlipperItemGenerator` (RollRarity, RollDefinition, RollItem, FillSellerTable) is implemented (the user wrote RollRarity; Claude wrote the rest at their request because of the time pressure) and all 36 `Flipper.*` tests pass; the 6 item data assets are in `Content/Flipper/Items/` (made by editor Python, `DirectoriesToAlwaysCook` added to `Config/DefaultGame.ini` so they are packaged); the test screen now rolls its trunk and seller table with the generator; the tooltip shows rarity and status, the grid shows rarity borders and "?"/"X" marks.
- **Controller and widget facts later phases rely on:** a held item is in no grid; `TransferApprover` is unbound by default (unbound = every cross-grid move allowed) and is consulted only for moves between different grids (this is where the buy rule plugs in in Phase 4); `GetGhost` returns true with `bOutValid=false` when the approver refuses; `OnChanged` fires on pick-up, rotate, drop, snap-back, cancel. The grid widget paints everything itself in `NativePaint` (no child widgets, no Blueprint assets); `UUserWidget` defaults to `SelfHitTestInvisible`, so a widget that handles clicks itself must set `Visible`.
- **Small leftovers (not blocking):** `GetGhostColor` in `InventoryGridWidget.cpp` may still be the neutral white (green when valid, red when not is wanted); `OnQuickTransferRefused` only logs (no shake yet); items show a colour and a name, no icon textures; stale `// TODO` comments in the controller; `QuickTransfer` has no `From == To` handling. Escape ends a play-in-editor session, so test Cancel in Standalone.
- **Blockers:** none. The Unreal editor must be closed before command-line builds.

---

## How every feature is built (the loop)

1. **Grill.** Run a design interview with the `grill-with-ui` skill (the user prefers the browser page over grilling in the terminal). Treat earlier decisions in the spec as settled; ask only what's open.
2. **Spec.** On Finish, add that feature's section to `Docs/FlipperSpec.md`. Merge it into the existing file, never overwrite it.
3. **Scaffold** (Claude). Write the boilerplate fully. For the logic the user owns, write signatures with `// TODO` bodies plus automation tests that act as the spec.
4. **Implement** (user, mentored through the `socratic-mentor` skill). Claude builds and runs the tests and answers with guiding questions, not fixes, unless the user asks for the fix.
5. **Verify.** All tests pass, and the feature has been checked in the editor or in play (say what was observed, not "should work").
6. **Commit and push**, one commit per finished step.

**Who writes what:**
- **User:** `UInventoryGrid` logic (done), the held-item controller (done), item pricing, `UFlipperItemGenerator` weighted rolls.
- **Claude:** everything else (Unreal boilerplate, data-asset classes, widgets and input plumbing, game flow, tests), unless the user claims a piece at scaffolding time.

### Commands (verified)

```powershell
# Build the editor target (close the editor first; Live Coding blocks command-line builds)
& "C:\UnrealSource\UnrealEngine\Engine\Build\BatchFiles\Build.bat" LD60Editor Win64 Development -Project="D:\Unreal Projects\LD60\LD60.uproject" -WaitMutex

# Run the Flipper automation tests headless (a crash in one test kills the run: exit code 3 and "Assertion failed" in the log)
& "C:\UnrealSource\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Unreal Projects\LD60\LD60.uproject" -ExecCmds="Automation RunTests Flipper; Quit" -unattended -nopause -nullrhi -nosplash -log

# Package a Shipping build for Windows
& "C:\UnrealSource\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="D:\Unreal Projects\LD60\LD60.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="D:\Unreal Projects\LD60\Packaged" -noP4 -utf8output
```

If the build fails, the test run uses the OLD dll, so its results are stale. Check the build result first.

---

## What the jam plan cuts

Everything cut here is still described in full in [FlipperPlan-Full.md](FlipperPlan-Full.md), so it can be built after the deadline.

| Area | Full plan | Jam plan |
|---|---|---|
| Item properties | Rarity, condition levels, traits (5), Untested state, bundled lots | Rarity (3 tiers), a hidden works/dead flag that testing reveals (the Untested gamble), no traits, no lots |
| Starter items | 8 | 6 |
| Money | Signed cash, credit limit, debt, interest at each Market start | Cash only; a purchase you can't afford is refused (the cut list's fallback) |
| Buyers | Collectors with requests ("working, boxed, any handheld") | One pawn-shop style buyer paying the formula price |
| Length | 7 days | Day count is a data value; start at 5 |
| Item art | Icons for 8 items | Coloured rectangles with the name (already drawn this way) |
| Look | CRT/scanline material, pixel font, rarity colours | Pixel font and rarity colours only; no CRT |
| Sound | Cash register, pickup/place, refused drop, dead-device buzz, timer, buzzer, music | A handful: buy/sell, refused drop, dead buzz, phase-end buzzer; no music |
| Juice | Shake, "WORKS!/DEAD" reveal, ticking cash counter | Shake on refused drop and the "WORKS!/DEAD" reveal; the counter only if time allows |

---

## Schedule at a glance (jam)

Assumes the deadline is Monday ~3 PM PT and that submitting 2 hours early is the target. These are targets, not promises.

| When (PT) | Phase | Checkpoint |
|---|---|---|
| Sun evening | 1 and 2 (done) | Grid logic, held-item controller, two grids on screen, all input working |
| Sun night | 3. Item data and generation | The seller table fills with rolled items that show tooltips |
| Sun night | 4. Pricing and cash | Buying and refunds work with real prices and a cash HUD |
| Mon morning | 5. Day loop | Market, Garage, Sell phases with timers cycle through the days |
| Mon morning | 6. Selling | Items sell to the buyer |
| Mon ~10 AM | 7. Win/lose | **MVP: the full game is playable from start to finish. Start a Shipping package straight away** (30 to 60 min) to prove packaging works |
| Mon 10 to 11:30 AM | 8. Presentation (lite) and 9. Balance | Title screen, sounds, colours; quick tuning |
| Mon ~11:30 AM | **Feature freeze** | Only bug fixes after this |
| Mon 11:30 AM to 1 PM | 10. Ship | Final package, upload, entry page, submitted with ~2 h to spare |

---

## Phase 0: Setup (done)

- [x] Choose the concept (Flipper: buy, test and flip vintage electronics over a few days to make rent)
- [x] Grill the inventory grid and write it into `Docs/FlipperSpec.md` (11 decisions)
- [x] Git repo with Unreal `.gitignore` and LFS `.gitattributes`, pushed to GitHub
- [x] Write this plan

## Phase 1: Inventory grid logic (done)

- [x] **1.1 Scaffold** (Claude)
- [x] **1.2 Implement the grid** (user, mentored): all 11 `Flipper.InventoryGrid.*` tests pass.
- [x] **1.3 Early test package** (Claude). *Result 2026-10-17: succeeded with no fixes needed, but the first run took 3 h 9 min wall time (compiling the engine for Shipping from source, then a full cook). Packaged folder is about 2.2 GB, of which 1.77 GB is `LD60-Win64-Shipping.pdb`: leave the .pdb out of any zip. The exe starts and stays running. Later packages should be much faster since the engine is now compiled. Budget at least 30 to 60 min for the final package anyway, and start it early.*
- [x] **1.4 Commit and push.**

## Phase 2: Grid UI and input (done)

- [x] **2.1 Widgets and controller.** The user wrote `UFlipperHeldItemController` (14 tests). Claude wrote `UInventoryGridWidget`, `UFlipperHeldItemOverlay` and `UFlipperItemTooltip`, all painting themselves from C++. There is no separate `UInventoryItemWidget`: items are painted by the grid widget.
- [x] **2.2 Test screen:** `AFlipperTestGameMode` and `AFlipperTestPlayerController` show a 10x6 trunk and an 8x5 seller table with hard-coded items; the level is under `Content/Flipper/`.
- [x] **2.3 Verify in play.** The user reports it all working (drag, click style, rotate, snap-back, Shift-click, tooltip). Window-size and DPI behaviour was not checked separately.
- [x] **2.4 Commit and push** (3ec99ec).

## Phase 3: Item data and generation (Sun night)

- [x] **3.1 Grill (one combined, short).** Item fields (rarity tiers and colours, the hidden works/dead flag and the Untested state, what the tooltip shows), how the seller table is rolled each day (weights, number of items, how odds change across days), and auto-placing with `FindFirstFit`. Traits and lots are cut.
- [x] **3.2 Scaffold** (Claude): extend `UFlipperItemDefinition` (base value, rarity weight); the rarity enum; the full `FFlipperItemInstance` (rolled rarity, works flag, tested flag); `UFlipperItemGenerator` with `// TODO` roll logic and tests for its odds.
- [x] **3.3 Create the data** (6 starter items) as data assets. Claude can create them in code or via editor Python; otherwise the user makes them in the editor.
- [x] **3.4 Implement the generator** (user, mentored) until the tests pass.
- [x] **3.5 Verify in play:** the seller table fills with varied rolled items and tooltips show the rolled properties (an untested item shows "Untested", not works/dead). Commit and push.

## Phase 4: Pricing and cash (Sun night)

- [x] **4.1 Grill (short).** The value formula (base value × rarity multiplier, dead items worth scrap), the seller's markup, starting cash, and the refund rule.
- [x] **4.2 Scaffold** (Claude): a pricing function with `// TODO` and tests; a wallet class (cash, can-afford check); the transfer rule as a `TransferApprover` (buy on drop from table to trunk, same-phase refund when moved back, a free move within one grid).
- [x] **4.3 Implement pricing** (user, mentored) until the tests pass.
- [x] **4.4 Wire the transfer rule into the held-item controller and Shift-click; add the cash HUD** (Claude). A purchase that cannot be afforded is a refused drop (snap back, ghost red).
- [ ] **4.5 Verify in play:** buying, refunds, and a refused drop when short of cash. Commit and push.

## Phase 5: Day loop (Mon morning)

- [ ] **5.1 Grill (short).** Phase lengths (Market and Sell are timed, already decided), whether Garage testing is timed or limited by actions, how testing works, how phases hand over, the end-of-phase warning, and the number of days.
- [ ] **5.2 Implement** (Claude): a phase state machine (Market, Garage, Sell, next day), the timer HUD with the end-of-phase warning, snapping back a held item when time runs out, and the Garage test interaction (reveals works or dead).
- [ ] **5.3 Verify:** the days cycle correctly and the seller table re-rolls each day. Commit and push.

## Phase 6: Selling (Mon morning)

- [ ] **6.1 Grill (short).** How selling works on the grid (a third grid you drop items on, or Shift-click to the buyer?) and the buyer's offer formula.
- [ ] **6.2 Implement.** The offer is pricing-adjacent, so offer it to the user; Claude does the UI.
- [ ] **6.3 Verify:** a complete buy, test and sell loop for one day. Commit and push.

## Phase 7: Win/lose (Mon, ~10 AM) → MVP

- [ ] **7.1 Grill (short).** The rent target per day or at the end, game-over conditions, and a simple difficulty curve (all numbers in data).
- [ ] **7.2 Implement** (Claude): rent check, win and lose screens, restart.
- [ ] **7.3 MVP checkpoint.** Play the whole game from start to finish. **Package a Shipping build, run it from the packaged folder, and push.** From here on there is always a submittable build.

## Phase 8: Presentation (lite) (Mon, 10 to 11:30 AM)

- [ ] **8.1 Title screen with a short how-to-play, and quit.**
- [ ] **8.2 Colours and font:** rarity colours, an open-license pixel font.
- [ ] **8.3 Sound:** buy/sell, refused drop, dead-device buzz, phase-end buzzer. Generated or CC0 effects; note sources and licenses for the submission page.
- [ ] **8.4 Juice:** shake on a refused drop, a "WORKS!" / "DEAD" reveal when testing.
- [ ] **8.5 Commit and push** after each item.

## Phase 9: Balance and playtest (Mon, before the freeze)

- [ ] **9.1** Play 2 full runs and tune prices, odds, rent and timers (all in data, no recompiling).
- [ ] **9.2** Fix bugs found; no new features.
- [ ] **9.3 Feature freeze at ~11:30 AM PT Monday.**

## Phase 10: Ship (Mon, ~11:30 AM to 1 PM)

- [ ] **10.1** Package the final Shipping build. Zip it (no .pdb), unzip it to a different folder, and play a full day from the unzipped copy.
- [ ] **10.2** Capture 3 to 5 screenshots and a short GIF of packing the trunk and testing an item (the GIF matters, since Unreal has no web build and players must download).
- [ ] **10.3** Upload the build (e.g. itch.io) and write the Ludum Dare entry page: description, controls, credits and asset licenses, and a link to the GitHub repo.
- [ ] **10.4 Submit with at least ~2 hours to spare.** Push the final commit and tag it `ld60-submission`.

---

## If behind schedule: cut list (in this order)

1. Sound beyond the dead buzz and a buy/sell effect
2. The "WORKS!/DEAD" reveal animation (just show the result)
3. The day count (5 down to 3)
4. The pixel font and rarity colours (system font, plain colours)
5. The title screen (start straight into the game, with the how-to-play text on the Ludum Dare page)

**Never cut:** the grid with rotation, buying, the Untested gamble, selling, the rent goal, packaging and submission.

## After the jam / if ahead of schedule

Build these back in from [FlipperPlan-Full.md](FlipperPlan-Full.md):
- Debt, the credit limit and interest
- Traits and bundled lots
- Collectors with requests
- Item icons, the CRT/scanline look, music
- Trunk upgrades (a roof rack adds rows at the bottom)
- Diablo 2-style swap when dropping onto an item
- More item types
