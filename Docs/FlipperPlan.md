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

- **Last updated:** 2026-10-25 (session 4 continued): step 1 (item data) is COMMITTED (a076515, "Add condition, traits, lots and tag categories to items"). Step 2 (collectors with requests) is GRILLED and SCAFFOLDED: the design is in `Docs/FlipperCollectors-design.md` and spec Feature 8 (12 questions, all recommendations accepted: collectors are drop-target cards you drag items onto; 3 collectors a day with 1 to 3 clauses each (category tag, trait tag, must work, minimum condition, minimum rarity); the board is visible from the start of the Market phase and gone after Sell; the pawn shop stays as a last card at 100 percent of the plain offer; an Untested item cannot satisfy must-work; each request is built from a real item on the day's seller table; a collector buys once; the premium is x1.25 / x1.5 / x1.75 by clause count on top of the mood; Shift-click goes to the best-paying matching collector else the pawn shop; names-only collectors; live payout plus a trunk highlight while choosing). Scaffold stage 1 is written, built and COMMITTED (aa7272d), and RollRequest is now written (UNCOMMITTED): `FlipperCollectorDefinition.h`, `FlipperRequests.h/.cpp` (types `FFlipperRequestConfig`, `FFlipperRequest`, `FFlipperCollector`; `GetCollectorOffer` and `RollBoard` are written; `Matches` is DONE (the user tried it first, it had an operator-precedence bug and missed bTested and trait tags, so Claude wrote it on request); `RollRequest` is DONE too (Claude wrote it on request; the user asked)), `FlipperRandom.h/.cpp` (the weighted pick moved out of the item generator, now shared), and `Tests/FlipperRequestTests.cpp` (14 tests, `Flipper.Requests.*`). Test result: the FULL `Flipper` suite passes, 119 / 119 (exit 0). One test (BoardRequests) had a wrong assumption (MakeItem defaults to Good condition, so the pool was bigger than the test said); the test items are now Poor/Common. **Next: commit the RollRequest change, then scaffold stage 2:** the board in the day loop (rolled after the table and the mood, on its own random stream, cleared after Sell), the trade rule / held-item controller learning a card as a drop target, the pawn shop fraction (a new 1.0 in the pricing config), the collector cards and the trunk highlight, about six collector name assets (made with a temporary editor automation test, like the traits) plus a cook-list entry in `DefaultGame.ini`, and the still-open layout question of where the request board shows during Market and Garage (proposal: a compact strip beside the day HUD). Then step 3 (debt, credit limit, interest) and step 4 (presentation). Earlier, 2026-10-24 (session 4 continued): the template cleanup, centred layout and scale-to-fit rule were committed (63d79c4) and the user play-checked the layout and scale-to-fit ("works"). The user is now resuming [FlipperPlan-Full.md](FlipperPlan-Full.md), in this order: (1) item data (condition, traits, lots, tag categories), (2) collectors with requests, (3) debt, credit limit and interest, (4) presentation. Step 1 (item data, full scope; `Docs/FlipperItemDataFull-design.md`, spec Feature 7) is BUILT, TESTED and PLAY-CHECKED (the user: "everything looks like it's working") but UNCOMMITTED at the time of writing; the suggested commit message is "Add condition, traits, lots and tag categories to items": 105 of 105 `Flipper.*` tests pass. The user wrote `RollCondition`, `GetTraitMultiplier` and the condition/trait multipliers in `GetValue` and made `RollRarity` take one draw on every path (a rule for every roll function: the same number of draws whatever the config); Claude wrote `RollTraits` at the user's request. The data assets are created in `Content/Flipper` (via a one-off editor automation test that was then deleted; the assets were read back from disk): `Items/` has the 6 old definitions (now with category tags) plus `DA_CassetteWalkman` (1x2 $30), `DA_VCR` (3x1 $60), `DA_BoxOfFloppies` (3x2 $45, lot of 10, weight 0.5) and `DA_ShoeboxOfCassettes` (2x2 $40, lot of 12, weight 0.5); `Traits/` has `DA_Boxed` (x1.4, weight 3), `DA_WithAccessories` (x1.2, 3), `DA_Prototype` (x1.8, 1), `DA_Modded` (x0.7, 2), `DA_MissingParts` (x0.6, 2), with colours and tags. The trait roll weights, the shoebox's lot count (12) and the colours were open details, now first guesses in data. Next: the user commits (the asset files are LFS); then step 2, collectors with requests, which starts with a grill (what a request looks like, matching on tags, works flag, rarity and condition, how offers are priced, how many collectors a day); the balance pass for the new prices (condition alone lowers average prices about 9%) can wait for Phase 9. Earlier, 2026-10-22 (session 4 continued): the Garage hide-table change was committed (4be20b7), built and tested (95 of 95 `Flipper.*`; the user has not reported the play check of the table vanishing at Garage). Since then, ALL WRITTEN BUT NOT BUILT, NOT TESTED AND UNCOMMITTED (the editor was open, so Live Coding blocked the build): (1) the third-person template is removed: `LD60Character`, `LD60GameMode`, `LD60PlayerController`, `Source/LD60/Variant_*` (82 source files) and `Content/{Characters,Input,LevelPrototyping,ThirdPerson,Variant_*}` plus their `__ExternalActors__`/`__ExternalObjects__` folders (753 content files; git history keeps them); `DefaultEngine.ini` now has `EditorStartupMap=/Game/Flipper/TestScreen`, `GlobalDefaultGameMode=/Script/LD60.FlipperTestGameMode` and the `TP_ThirdPerson` redirects are gone; `LD60.Build.cs` lost the `Variant_*` include paths and the `EnhancedInput`/`AIModule`/`StateTreeModule`/`GameplayStateTreeModule` dependencies; `LD60.uproject` lost the `StateTree`/`GameplayStateTree` plugins and the `AIModule` dependency; only `LD60.Build.cs`, `LD60.cpp`, `LD60.h` and `Flipper/` remain in `Source/LD60`. (2) Responsive layout: `AFlipperTestPlayerController::LayoutWidgets()` (called in `BeginPlay` before `StartRun`) sizes the cash panel, trunk, day HUD and seller table and anchors them as one block (956 x 386 UI units, `GetLayoutSize()`) at the viewport centre with `SetAnchorsInViewport(0.5, 0.5)` set LAST (the position and size setters reset the anchors); `UFlipperCashWidget` is now a 150x34 sized panel (`PanelWidth`/`PanelHeight`) painting at its own origin; the seller table size and the gaps are named constants. (3) Scale to fit: new `UFlipperDpiScalingRule` (`FlipperDpiScalingRule.h/.cpp`, a `UDPICustomScalingRule`) scales the whole UI so the block fits the viewport with `MarginFraction` = 5% on each side, using the tighter of height and width; chosen via `[/Script/Engine.UserInterfaceSettings] UIScaleRule=Custom, CustomScalingRuleClass=/Script/LD60.FlipperDpiScalingRule` in `DefaultEngine.ini`. Known limit: the block is about 2.5:1, so on 16:9 the width limits it (scale about 1.8 at 1920x1080, block about 65% of the height); filling the height for real needs a vertical restack (HUD on top, seller table, trunk below; about 640 x 590), which the user has NOT yet chosen. Next: the user closes the editor and says "closed"; Claude builds `LD60Editor`, reruns `Flipper.*` (expect 95 of 95), the user opens the editor (checks it starts on `TestScreen`), resizes the play window to check the centring and scaling, then decides on the vertical restack, then commits. Leftover stale lines in `DefaultEditor.ini` / `DefaultEditorPerProjectUserSettings.ini` point at a missing `ThirdPersonCPP` and were left alone. The Shipping zip still predates all of this (it should shrink after the cleanup). Earlier, 2026-10-21 (session 4 continued): the ghost snapping below was play-checked by the user ("everything looks good") and committed (89d1032). New, written but NOT built, NOT tested and UNCOMMITTED: the seller table's grid is hidden (collapsed) during the Garage phase. `AFlipperTestPlayerController::OnPhaseChanged` (bound to `DayLoop->OnPhaseChanged` before `StartRun`) sets `SellerTableWidget` to Collapsed in Garage and Visible otherwise; `UInventoryGridWidget::ScreenToCellPos` now returns false when `!IsVisible()` so a hidden grid takes no clicks, hovers, tooltips or drops (it would otherwise answer from its stale cached geometry). Spec Feature 4 day-loop paragraph updated. The editor was open, so the build and the tests have not run: first close the editor, build, rerun `Flipper.*` (expect 95 of 95; there is no test for the widget visibility, it needs the play check), then play a day and check the table disappears at Garage and returns at Sell. Suggested commit message is in the chat; use "Hide the seller table during the Garage phase". Earlier, 2026-10-20 (session 4 continued, post-deadline polish): the held item was centred on the cursor and a window-vs-desktop cursor offset in every `NativePaint` was fixed (committed 47ef66a, tests re-run 94 of 94). Then the red/green ghost was made to match the picture too: the grab offset (`GetGrabOffset`, `GetCentreCell`) is GONE, and the controller's `PointerDown` / `PointerUp` / `GetGhost` now take the pointer as an `FVector2D` in fractional cell units; `GetDropOrigin(CellPos)` = `round(CellPos - Footprint / 2)` is the one rule for the ghost and the drop; `UInventoryGridWidget::ScreenToCellPos` gives the fractional position; tests use `CellCentre(x, y)` from `Tests/FlipperTestHelpers.h`, with a new `Flipper.HeldItem.DropSnapsToNearestLine` test. Built and tested: 95 of 95 `Flipper.*` pass. NOT yet play-checked by the user and UNCOMMITTED (controller + header, grid widget + header, overlay comment, test player controller, 4 test files, the new helper header, the spec; suggested commit message "Snap the ghost and the drop to the nearest cells under the centred held item"). Earlier on 2026-10-19 (session 4 start), ~3:30 AM PT (session 3: Phases 1 to 7 built and committed (a93eb3b selling, e322c1a 4x3 trunk, fb1a0c3 rent/win-lose, ef71868 packaged game starts on the test screen); 94 of 94 `Flipper.*` tests pass; the user play-checked Phase 7 ("everything is working"); a Shipping package was built (4 min) and zipped as `Packaged\Flipper-Win64.zip` (345 MB, no .pdb, unzipped copy launches) and a submission description was written; the MVP is submittable now; the user will upload it on ldjam.com and check the real deadline; next is Phase 8)
- **Done:** concept chosen; inventory grid grilled and specced; git repo created and pushed. Phase 1: all of `UInventoryGrid` (11 of 11 `Flipper.InventoryGrid.*` tests) and an early Shipping package (see 1.3). Phase 2: the user implemented `UFlipperHeldItemController` (14 of 14 `Flipper.HeldItem.*` tests, so 25 of 25 `Flipper.*` tests); Claude wrote the widgets and the test screen (`UInventoryGridWidget`, `UFlipperHeldItemOverlay`, `UFlipperItemTooltip`, `AFlipperTestPlayerController` / `AFlipperTestGameMode`, the level under `Content/Flipper/`), and the user wrote `ScreenToCell`. The user played it and reports everything working. Last commit 3ec99ec, pushed. `.clang-format` is in the repo; a local `.vscode/settings.json` has format-on-save.
- **Next step:** (0) post-deadline polish is under way: next is whatever the user picks (candidates: the Phase 8 list below, the small leftovers, balance); first the user play-checks the ghost snapping (even-width items like the 2-wide ones are the interesting case: the ghost should flip a column once the pointer passes the middle of a cell), then commits. Session 4 gotchas: in `NativePaint` get the cursor with `FlipperPaint::CursorLocal(Args, Geometry)` (the paint geometry is in window space, the cursor and `GetCachedGeometry()` are in desktop space; a raw `AbsoluteToLocal(GetCursorPos())` is off by the window's position); the held item has no grab offset any more: the controller works from the pointer in fractional cell units and `GetDropOrigin` rounds the picture's top-left corner (spec Feature 1, updated). The earlier steps follow. (1) the user uploads `Packaged\Flipper-Win64.zip` + the description to ldjam.com and checks the REAL deadline (the ~3 PM Mon PT estimate is unconfirmed); (2) Phase 8 presentation-lite if time remains: sounds (buzzer at 0, sale, cash), sale/cash juice, grid captions ("Seller" / "Buyer's counter" / "Trunk"), a title screen, maybe a run summary on the end screen; (3) Phase 9 balance (rent $450, mood weights, `UntestedOfferFraction`, 4x3 trunk, play 2 full runs); (4) Phase 10 final package: rerun the RunUAT command in this file (~4 min when the engine is built), delete the old zip, rezip WITHOUT the .pdb (stage with `robocopy /XF *.pdb`, then `tar -a -c -f ... -C stage Flipper`), unzip to another folder and launch it. Gotchas learned: the packaging build merges source files, so two files must not define the same helper name in an anonymous namespace (e.g. `IsInside`; the editor build does not catch it); the editor must be closed to build or package; `GameDefaultMap` is `/Game/Flipper/TestScreen`. Phase 7 facts: one rent check ($450, `FFlipperDayConfig::RentAmount`) of the wallet cash when the day loop enters `Finished`; `UFlipperDayLoop::SetWallet/GetResult` (`EFlipperRunResult`)/`GetCash/GetRent/IsRentMet`; `UFlipperEndScreen` (headline, cash, rent, days, Play Again reloads the level with a new seed); HUD shows "Rent $450"; 8 tests in `Tests/FlipperRentTests.cpp`. Earlier phases are summarised below.
- **Controller and widget facts later phases rely on:** a held item is in no grid; `TransferApprover` is unbound by default (unbound = every cross-grid move allowed) and is consulted only for moves between different grids (the Phase 4 buy rule plugs in here via `UFlipperTradeRule::Bind`, plus the `TransferCommitted` hook that fires once just before a cross-grid item is placed and may stamp the item); `GetGhost` returns true with `bOutValid=false` when the approver refuses; `OnChanged` fires on pick-up, rotate, drop, snap-back, cancel. The grid widget paints everything itself in `NativePaint` (no child widgets, no Blueprint assets); `UUserWidget` defaults to `SelfHitTestInvisible`, so a widget that handles clicks itself must set `Visible`.
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
- [x] **4.5 Verify in play:** buying, refunds, and a refused drop when short of cash. Commit and push.

## Phase 5: Day loop (Mon morning)

- [x] **5.1 Grill (short).** Phase lengths (Market and Sell are timed, already decided), whether Garage testing is timed or limited by actions, how testing works, how phases hand over, the end-of-phase warning, and the number of days.
- [x] **5.2 Implement** (Claude): a phase state machine (Market, Garage, Sell, next day), the timer HUD with the end-of-phase warning, snapping back a held item when time runs out, and the Garage test interaction (reveals works or dead).
- [x] **5.3 Verify:** the days cycle correctly and the seller table re-rolls each day. Commit and push.

## Phase 6: Selling (Mon morning)

- [x] **6.1 Grill (short).** Done: `Docs/FlipperSelling-design.md`, Feature 5 in the spec.
- [x] **6.2 Implement.** Done (Claude wrote the offer on the user's request because of the time pressure): `GetOffer`, mood, counter, HUD, tooltip, 14 new tests.
- [x] **6.3 Verify:** a complete buy, test and sell loop for one day. Play-checked by the user; the commit and push are theirs (still to do).

## Phase 7: Win/lose (Mon, ~10 AM) → MVP

- [x] **7.1 Grill (short).** The rent target per day or at the end, game-over conditions, and a simple difficulty curve (all numbers in data).
- [x] **7.2 Implement** (Claude): rent check, win and lose screens, restart. Built and 94 of 94 tests pass; not yet play-checked.
- [x] **7.3 MVP checkpoint.** Done 2026-10-19: played by the user, Shipping package built, zipped without the .pdb (`Packaged\Flipper-Win64.zip`, 345 MB) and launch-tested from an unzipped copy. The user pushes and submits.

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
