# Flipper Build Plan (FULL, original scope)

> **Frozen snapshot, 2026-10-18 night PT.** This is the original, uncut plan, kept so the game can be finished past the Ludum Dare 60 deadline. Its status lines and checkboxes are NOT kept up to date. The live jam plan, with the scope cut and the current status, is [FlipperPlan.md](FlipperPlan.md). Phases 1 and 2 were finished before this snapshot; Phases 3 to 10 here describe the full game. See "What the jam plan cuts" in FlipperPlan.md for the exact differences.

The step-by-step plan for finishing **Flipper** (Ludum Dare 60, theme "old tech") before the jam deadline. It shows the order features get built, the steps inside each feature, and the non-feature work (builds, packaging, submission).

- **Spec:** [FlipperSpec.md](FlipperSpec.md) holds the design decisions. This file holds the order and the status.
- **Repo:** https://github.com/rhkim1292/vintage-game (public, `main`, Git LFS for assets)
- **Deadline:** about **Monday 2026-10-19, ~3 PM Pacific**. This is derived from "2 days 18 hours left" on Friday evening, 2026-10-16. **Confirm the exact time on ldjam.com** and fix this line.
- **Rule of thumb:** a complete, packaged, submitted game beats a better unfinished one. Keep to the checkpoints below. If a checkpoint slips by more than ~3 hours, apply the cut list.

**Keep this file current:** tick boxes as steps finish, and update the "Where we are" block at the end of every session.

---

## Where we are

- **Last updated:** 2026-10-18, evening PT (end of session 3: Phase 1 finished; Phase 2.1 held-item controller finished; widgets are next)
- **Done:** concept chosen; inventory grid grilled and specced; git repo created and pushed. Phase 1 done: the user implemented all of `UInventoryGrid` (11 of 11 `Flipper.InventoryGrid.*` tests pass) and a Shipping package built and launches (see the note under 1.3). Phase 2.1 controller done: the user implemented all of `UFlipperHeldItemController` (`Source/LD60/Flipper/FlipperHeldItemController.cpp`); all 14 `Flipper.HeldItem.*` tests pass, so 25 of 25 `Flipper.*` tests pass. Last commit beeff2b, pushed (main is level with origin/main). `.clang-format` (copied from the MCP project) is in the repo; a local `.vscode/settings.json` has format-on-save.
- **Next step:** Phase 2.1 widgets and Phase 2.2 test screen, Claude scaffolds, in mentor style for the user where there is logic worth writing: `UInventoryGridWidget` (draws cells, items, rarity borders and the ghost via `GetGhost`; turns mouse position into a cell and calls the controller's `PointerDown`/`PointerMove`/`PointerUp`, `Rotate` on R and right-click, `Cancel` on Escape, `QuickTransfer` on Shift-click), `UInventoryItemWidget`, the held-item visual, the tooltip card, then the 10x6 trunk / 8x5 seller-table test level. Ask the user before creating files. Controller facts the widgets rely on: a held item is in no grid; `TransferApprover` is unbound by default (unbound = every cross-grid move allowed) and is consulted only for moves between different grids; `GetGhost` returns true with `bOutValid=false` (red) when the approver refuses, false only when nothing is held or the grid is null; `OnChanged` fires on pick-up, rotate, drop, snap-back, cancel. Small optional leftovers in the controller: stale `// TODO` comments, no null checks / `From == To` handling in `QuickTransfer`, the ensure text in `QuickTransfer` says "dropped". The user commits manually; Claude writes the commit message.
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
- **User:** `UInventoryGrid` logic, item pricing, `UFlipperItemGenerator` weighted rolls.
- **Claude:** everything else (Unreal boilerplate, data-asset classes, widgets and input plumbing, game flow, tests), unless the user claims a piece at scaffolding time.

### Commands (verify on first use, then fix here if they differ)

```powershell
# Build the editor target (close the editor first; Live Coding blocks command-line builds)
& "C:\UnrealSource\UnrealEngine\Engine\Build\BatchFiles\Build.bat" LD60Editor Win64 Development -Project="D:\Unreal Projects\LD60\LD60.uproject" -WaitMutex

# Run the Flipper automation tests headless
& "C:\UnrealSource\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Unreal Projects\LD60\LD60.uproject" -ExecCmds="Automation RunTests Flipper; Quit" -unattended -nopause -nullrhi -nosplash -log

# Package a Shipping build for Windows
& "C:\UnrealSource\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="D:\Unreal Projects\LD60\LD60.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="D:\Unreal Projects\LD60\Packaged" -noP4 -utf8output
```

---

## Schedule at a glance

| When (PT) | Phase | Checkpoint |
|---|---|---|
| Fri night | 1. Inventory grid logic | Grid tests pass; first test package builds |
| Sat morning | 2. Grid UI and input | Two grids on screen; drag, click, rotate, Shift-click and snap-back all work |
| Sat afternoon | 3. Item data and generation | The seller table fills with rolled items that show tooltips |
| Sat evening | 4. Pricing and economy | Buying, refunds and debt work with real prices |
| Sun morning | 5. Day loop | Market → Garage → Collectors, with timers, runs for 7 days |
| Sun midday | 6. Collectors and selling | Items sell to collectors |
| Sun afternoon | 7. Win/lose and difficulty | **MVP: the full game is playable from start to finish.** Package and push a build |
| Sun evening → Mon morning | 8. Presentation and juice | Retro look, item icons, sound, title screen |
| Mon morning | 9. Balance and playtest | Feature freeze by ~9 AM PT |
| Mon, last ~6 h | 10. Ship | Submitted with ~2 h to spare |

---

## Phase 0: Setup (done)

- [x] Choose the concept (Flipper: buy, test and flip vintage electronics over 7 days to make rent)
- [x] Grill the inventory grid and write it into `Docs/FlipperSpec.md` (11 decisions)
- [x] Git repo with Unreal `.gitignore` and LFS `.gitattributes`, pushed to GitHub
- [x] Write this plan

## Phase 1: Inventory grid logic (Fri night)

- [x] **1.1 Scaffold** (Claude) — done 2026-10-16 night; builds clean, all 10 `Flipper.InventoryGrid.*` tests fail as expected
  - Add `SlateCore` to `LD60.Build.cs` and the `LD60/Flipper` include paths.
  - Create a minimal `UFlipperItemDefinition` data asset with only what the grid needs: display name, width, height and icon. Item data fills in the rest later.
  - Create a minimal `FFlipperItemInstance` (an id plus its definition) and `FFlipperPlacement` (origin cell plus `bRotated`).
  - Create `UInventoryGrid` with its full header (`CanPlace`, `TryPlace`, `Remove`, `FindItemAt`, `FindFirstFit`, the footprint helper, `OnChanged`) and `// TODO` bodies.
  - Write automation tests `Flipper.InventoryGrid.*`: bounds, overlap, rotated footprints, remove then re-place, finding the item at a cell, and first fit (unrotated, then rotated, then none).
  - Build to confirm the scaffold compiles. Tests are expected to fail.
- [x] **1.2 Implement the grid** (user, mentored) until every `Flipper.InventoryGrid` test passes.
- [x] **1.3 Early test package** (Claude). Package a Shipping build of the empty template to catch packaging and toolchain problems now, not at hour 64. Note any fixes here. *Result 2026-10-17: succeeded with no fixes needed, but the first run took 3 h 9 min wall time (compiling the engine for Shipping from source, then a full cook). Packaged folder is about 2.2 GB, of which 1.77 GB is `LD60-Win64-Shipping.pdb`: leave the .pdb out of any zip. The exe starts and stays running. Later packages should be much faster since the engine is now compiled. Budget at least 30 to 60 min for the final package anyway, and start it early.*
- [x] **1.4 Commit and push.**

## Phase 2: Grid UI and input (Sat morning)

The spec already covers this (single held-item state machine, ghost, tooltip), so no grill is needed. Raise any open UI layout questions at the start of Phase 8.

- [~] **2.1 Scaffold the widgets** (held-item controller DONE and tested, widgets still to do) (Claude): `UInventoryGridWidget` (draws cells, items, rarity borders and the ghost; turns mouse position into a cell), `UInventoryItemWidget`, the held-item controller (pick up, rotate the grab offset, place or snap back, cancel), the held-item visual and the tooltip card. *Ask the user whether they want to write the held-item state machine themselves; it's good practice for ProjectNS.*
- [ ] **2.2 Test screen** (Claude): a debug level and game mode showing a 10x6 trunk and an 8x5 seller table, filled with hard-coded test items.
- [ ] **2.3 Verify in play:**
  - press-drag-release and click-to-pick-up/click-to-place both work
  - R and right-click rotate
  - an invalid drop snaps back, and so does Escape
  - Shift-click moves an item to the first fit in the other grid, or shakes if nothing fits
  - the tooltip appears without covering the item
  - mouse position to cell is correct at different window sizes and DPI settings
- [ ] **2.4 Commit and push.**

## Phase 3: Item data and generation (Sat afternoon)

- [ ] **3.1 Grill: item data.** Fields, rarity tiers and colors, condition levels, traits, the Untested state, bundled "lots", and which fields the tooltip shows.
- [ ] **3.2 Grill: item generation.** How the seller table is rolled each day, the weights, how odds change across the 7 days, and auto-placing items with `FindFirstFit`. If time is short, combine this with 3.1 into one grill.
- [ ] **3.3 Scaffold** (Claude): extend `UFlipperItemDefinition`; add `UFlipperTraitDefinition`, the rarity and condition enums, the full `FFlipperItemInstance`, and `UFlipperItemGenerator` with `// TODO` roll logic and tests for its odds.
- [ ] **3.4 Create the data** (8 starter items, 5 traits) as data assets in the editor. Claude can script this with editor Python if that's set up; otherwise the user makes them in the editor.
- [ ] **3.5 Implement the generator** (user, mentored) until the tests pass.
- [ ] **3.6 Placeholder icons.** Flat colored rectangles with the item's name, so art doesn't block gameplay.
- [ ] **3.7 Verify in play:** the seller table fills with varied rolled items and tooltips show the rolled properties. Commit and push.

## Phase 4: Pricing and economy (Sat evening)

- [ ] **4.1 Grill: pricing and economy.** The value formula (base × rarity × condition × traits), the seller's markup, the credit limit and interest rate (debt is already decided, see the spec), and whether the limit grows.
- [ ] **4.2 Scaffold** (Claude): a pricing function with `// TODO` and tests; a wallet/economy class (signed cash, credit-limit check, interest charged at each Market start); the transfer rule controller (buy on drop, same-phase refunds, a free move within one grid).
- [ ] **4.3 Implement pricing** (user, mentored) until the tests pass.
- [ ] **4.4 Wire the transfer rule into the held-item controller and Shift-click; add the cash/debt HUD** (Claude).
- [ ] **4.5 Verify in play:** buying, refunds, a refused drop past the credit limit, and debt shown in red. Commit and push.

## Phase 5: Day loop (Sun morning)

- [ ] **5.1 Grill: day loop.** Phase lengths (Market and Collectors are timed, already decided), whether Garage testing is timed or limited by actions, how testing an Untested item works, how phases hand over to each other, and the warning near the end of a phase.
- [ ] **5.2 Implement** (Claude): a phase state machine (Market → Garage → Collectors → next day), the timer HUD with the end-of-phase warning, snapping back a held item when time runs out, and the Garage test interaction.
- [ ] **5.3 Verify:** 7 days cycle correctly, interest is charged at each Market start, and the seller table re-rolls each day. Commit and push.

## Phase 6: Collectors and selling (Sun midday)

- [ ] **6.1 Grill: collectors.** What a request looks like ("Wanted: working, boxed, any handheld"), how selling works on the grid (dragging onto a collector?), how offers are priced, and how many collectors appear each day.
- [ ] **6.2 Scaffold and implement.** Matching an item to a request and calculating the offer is pricing-adjacent, so offer it to the user. Claude does the UI.
- [ ] **6.3 Verify:** a complete buy, test and sell loop for one day. Commit and push.

## Phase 7: Win/lose and difficulty (Sun afternoon) → MVP

- [ ] **7.1 Grill: win/lose and difficulty.** The rent target, how debt counts against rent, game-over conditions, and the difficulty curve across the 7 days.
- [ ] **7.2 Implement** (Claude): rent check, win and lose screens, restart, and the difficulty values in data.
- [ ] **7.3 MVP checkpoint.** Play the whole game from start to finish. **Package a Shipping build, run it from the packaged folder, and push.** From here on there is always a submittable build.

## Phase 8: Presentation and juice (Sun evening → Mon morning)

- [ ] **8.1 Grill: presentation.** UI style (CRT scanlines, chunky pixel font, colors), layout of the market, garage and collector screens, the sound list, juice, and the title screen.
- [ ] **8.2 Item icons** for the 8 starter items, replacing the placeholders.
- [ ] **8.3 Retro UI look:** CRT/scanline material over the UI, an open-license pixel font, rarity colors.
- [ ] **8.4 Sound:** cash register, item pickup and place, refused drop, the buzz of a dead device, timer ticking, phase-end buzzer. Use generated or CC0 sound effects; note their sources and licenses for the submission page.
- [ ] **8.5 Juice:** shake on a refused drop, a "WORKS!" / "DEAD" reveal when testing, a cash counter that ticks up.
- [ ] **8.6 Title screen, a short how-to-play, and quit.**
- [ ] **8.7 Commit and push** after each item above.

## Phase 9: Balance and playtest (Mon morning)

- [ ] **9.1** Play 2–3 full runs and tune prices, odds, credit limit, interest, rent and timers (all in data, no recompiling).
- [ ] **9.2** Fix bugs found; no new features.
- [ ] **9.3 Feature freeze by ~9 AM PT Monday.**

## Phase 10: Ship (Monday, final ~6 hours)

- [ ] **10.1** Package the final Shipping build. Zip it, unzip it to a different folder, and play a full day from the unzipped copy.
- [ ] **10.2** Capture 3–5 screenshots and a short GIF of packing the trunk and testing an item (the GIF matters, since Unreal has no web build and players must download).
- [ ] **10.3** Upload the build (e.g. itch.io) and write the Ludum Dare entry page: description, controls, credits and asset licenses, and a link to the GitHub repo.
- [ ] **10.4 Submit with at least ~2 hours to spare.** Push the final commit and tag it `ld60-submission`.

---

## If behind schedule: cut list (in this order)

1. Presentation beyond the CRT look and basic sound (fewer icons, no music)
2. Shift-click quick-transfer
3. The click-to-pick-up input style (keep press-drag-release)
4. Collector requests (fall back to a single pawn-shop buyer paying the formula price)
5. Debt and interest (fall back to refusing a purchase you can't pay for)

**Never cut:** the grid with rotation, buying, the Untested gamble, selling, the rent goal, packaging and submission.

## If ahead of schedule: stretch list

- Trunk upgrades (a roof rack adds rows at the bottom)
- Diablo 2-style swap when dropping onto an item
- Bundled "lots" as items
- More item types and traits
- Music
