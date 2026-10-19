# Flipper Spec

Flipper is a Ludum Dare 60 entry (theme: "old tech"), built in this UE5 C++ project. You play a reseller of vintage electronics with rent due in 7 days. Each day you buy randomly rolled old tech at a flea market (Diablo-style rarity, condition and traits, some items Untested), pack it into your car trunk, test items in the garage, and sell to collectors.

This file is the spec. Each feature gets a section once its design interview is done, and later sections build on earlier ones. Features not yet specced: pricing, the day loop, collectors, win/lose and difficulty, presentation.

---

## Feature 1: Inventory grid

### Summary

The trunk and the seller table are both rectangular grids of cells holding rectangular items. Each item covers a width x height footprint and can be rotated 90° while held. One custom state machine handles both press-drag-release and click-to-pick-up/click-to-place. It shows a green or red ghost over the hovered cells, and snaps the item back to where it came from on an invalid drop, when you cancel, or when the phase timer runs out. Moving an item from the seller table into the trunk buys it on the spot. Cash may go negative down to a credit limit, and debt accrues interest. Moving it back during the same Market phase refunds it in full. Shift-click quick-transfers an item to the first spot it fits in the other grid. Items don't stack, grid sizes are fixed for the run and read from data, and item details appear in a Diablo-style hover tooltip, with rarity-colored borders on the grid.

### Terms

| Term | Definition | Avoid |
|---|---|---|
| **Grid** | A rectangular board of cells that holds items. The trunk and the seller table are each one grid (one `UInventoryGrid` instance). | inventory, bag, container |
| **Cell** | One square of a grid, addressed by an `FIntPoint` (X = column, Y = row, origin top-left). | slot, tile |
| **Item** | One rolled piece of old tech (an `FFlipperItemInstance`): its definition plus rarity, condition, traits and tested state. | object, product, loot |
| **Footprint** | The rectangle of cells an item covers: its definition's width x height, swapped when rotated. | size, bounds |
| **Placement** | Where an item sits in a grid: its origin cell (top-left of the footprint) plus whether it is rotated. | position, slot |
| **Trunk** | The player's car-trunk grid. Everything the player owns lives here. | inventory, backpack, stash |
| **Seller table** | The flea-market seller's grid, rolled fresh each Market phase. Items on it are for sale. | shop, market grid, vendor |
| **Ghost** | The preview of a held item's footprint drawn over the hovered cells: green where the drop is valid, red where it is not. | shadow, preview item |
| **Held item** | The item currently picked up (by press-drag or by click), remembered with its origin grid and placement so it can snap back. | dragged item, cursor item, selected item |
| **Debt** | A negative cash balance, allowed down to a credit limit, that accrues interest at the start of each Market phase. | loan, negative money, overdraft |
| **Phase timer** | The countdown that limits how long a Market (buying) or Collectors (selling) phase lasts. | round timer, clock, time limit |

### Why

Inventory management is the core mechanic of Flipper, not just storage. The grid creates tension on two axes: **space** (what fits in the trunk) and, together with item generation, **uncertainty** (gambling on Untested items). In the user's words, they also don't want to give the player infinite time to think about the optimal transaction, so the buying and selling phases are timed. Packing becomes "good enough, under pressure" rather than "optimal". The grid is also a deliberate small-scale rehearsal of the Diablo 2-style inventory the user plans for ProjectNS, their survival-horror loot game.

### Locked decisions

These are hard to reverse, would surprise someone without context, and involved a real tradeoff.

**1. Items are rectangles only, rotated by swapping width and height.**
- Every item's footprint is width x height. With rectangles there are only two distinct orientations, so a placement stores a `bRotated` flag, not an angle.
- *Rejected: freeform Tetris shapes (Backpack Hero style).* A deeper packing puzzle, but roughly half a day more for icon art with transparent gaps, hit-testing per cell, and justifying an L-shaped Walkman. Old tech is overwhelmingly boxy, and Resident Evil 4 shows rectangles plus rotation already make packing a real puzzle.
- *Rejected: rectangles stored as a cell list for future freeform support.* Speculative generality in a 66-hour jam: paying for logic that would never be used.

**2. Buy on drop, with debt allowed down to a credit limit.**
- Dropping an item from the seller table into the trunk charges for it right then. The drop is refused only if it would push cash below the negative credit limit: allowed when `cash - price >= -creditLimit`.
- While cash is negative, interest is charged when each Market phase starts (`cash += cash * rate`, applied only when `cash < 0`).
- Dragging an item back from the trunk to the seller table during the **same** Market phase refunds it in full.
- The grid knows nothing about money. A separate controller (the transfer rule) approves or rejects each move between grids before the grid commits it.
- The credit limit is required. Without it, buying the whole table on credit and selling it the same day is an interest-free loan, and maxing out becomes the dominant strategy.
- *Rejected: buy on drop, refused at $0 (the original recommendation).* The user wanted the risk/reward of borrowing for a rare item. Debt costs about 1–2 hours of code.
- *Rejected: a cart model (pay at "Leave Market").* Adds a checkout step that can fail and a second money number on screen.
- *Rejected: buy on drop with no refunds.* Punishes experimenting with packing, which fights the core mechanic.
- *Open, for the economy and win/lose grills:* the credit limit value (possibly growing with reputation), the interest rate, whether debt counts against the rent target, and whether being past the limit on rent day is game over.

**3. Both input styles: press-drag-release and click-to-pick-up/click-to-place.**
- Players can hold the button and drag, releasing to drop. Or they can click once to pick up (the item follows the cursor) and click again to place, the Diablo 2 / RE4 style.
- *Rejected: press-drag-release only, on UMG's native drag-and-drop (the original recommendation).* The user chose both styles.
- *Rejected: click-click only.* The user chose both styles.

**4. Both input styles run on ONE custom held-item state machine. UMG native drag-and-drop is not used.**
- Mouse down on an item picks it up and makes it the held item.
- If the mouse moved past a small threshold (a few pixels) before release, the release tries to place it (drag style). Otherwise the item stays held until the next click (click style).
- One small controller object, shared by both grid widgets, owns the held item, its origin grid and its original placement. Each grid widget converts the mouse position to a cell (`Geometry.AbsoluteToLocal(ScreenPos) / CellSize`) and reports the hovered cell to the controller.
- The held item's visual is a widget moved to the cursor each tick.
- This gives one ghost, one rotate handler, one snap-back path and one buy/refund check, so the two styles can never behave differently. Right-click to rotate works because no Slate drag is ever in progress.
- *Rejected: two paths (UMG native drag-and-drop for dragging, plus a custom held state for clicking).* Every rule would exist twice, and bugs where the two styles disagree are exactly what shows up on day 3.

### Routine choices

- **Invalid drops are refused:** while an item is held, the ghost shows over the hovered cells, green if the drop is valid, red if not. Dropping on a red ghost (overlapping another item or off the grid's edge) snaps the item back to its origin grid and placement. *Rejected:* the Diablo 2 swap (it creates a held item with no origin to return to, which conflicts with buy on drop; possible day-3 polish) and auto-nudging to the nearest fit (places items where the player didn't aim).
- **Rotation happens only while holding:** press **R** or **right-click** to toggle `bRotated`, and the ghost updates immediately. When rotating, rotate the grab offset too, so the cell you grabbed stays under the cursor. Turning clockwise, offset `(x, y)` in a W x H footprint becomes `(H-1-y, x)` in the new H x W footprint. *Rejected:* rotating placed items in place (a second code path with an unclear pivot) and no rotation (removes about half the packing puzzle).
- **No stacking:** every item is a unique rolled instance with its own placement. One item = one placement. *Rejected:* stacking identical common items (needs an equality rule across all rolled properties, plus split/merge UI). *Raised for the item-data grill:* pre-bundled "lots" (e.g. "box of 10 floppies") that act as one bigger item. They cost the grid nothing.
- **Grid sizes are fixed for the run and read from data:** starting values are a trunk of 10 x 6 and a seller table of 8 x 5, editable without recompiling. *Stretch goal:* trunk upgrades (e.g. a roof rack adding rows at the bottom, so existing cells never move). *Rejected:* choosing a vehicle at the start (a start screen and three balance targets).
- **Shift-click quick-transfer:** moves an item to the first fit in the other grid, following the same buy, refund and credit-limit rules as a drop. If nothing fits, the item shakes and stays put. "First fit" scans cells row by row from the top-left, trying unrotated and then rotated. The same `FindFirstFit` places generated items onto the seller table, so the shortcut is nearly free. *Rejected:* an Auto-pack button (solves the core puzzle for the player; good packing is an NP-hard problem anyway) and having no shortcut.
- **When the phase timer runs out:** the held item snaps back to its origin, exactly like an invalid drop. An item taken from the seller table therefore stays unbought. Then the phase ends. Pair this with a warning in the last ~10 seconds (a ticking sound, the timer flashing red); the details are for the day-loop grill. *Rejected:* a grace period of about 3 seconds (makes the deadline negotiable and adds a "phase ending" state) and placing the item wherever it's hovered (could buy an item and push you into debt without a click).
- **Item details appear in a hover tooltip:** hovering any item, or holding one, shows a Diablo-style card beside its footprint, offset to the side rather than covering it and kept on screen, with the name colored by rarity. Each item on the grid also gets a rarity-colored border, so the player can scan the seller table before hovering anything. Which fields the card shows is for the item-data grill. *Rejected:* a fixed side panel (pulls the player's eyes away from the grid under time pressure) and both a tooltip and a panel (two layouts to keep in sync).

### Implementation notes (derived from the decisions above)

- **`UInventoryGrid` (a UObject; plain logic, no widgets):** width and height, cell occupancy, and a list of items with their placements. Core API: `CanPlace(Item, Origin, bRotated)`, `TryPlace(...)`, `Remove(Item)`, `FindItemAt(Cell)`, `FindFirstFit(Item, OutOrigin, OutRotated)`, plus an `OnChanged` event. The user implements the logic; automation tests act as its spec.
- **Footprint helper:** `GetFootprint(Definition, bRotated)` returns `(W, H)`, or `(H, W)` when rotated.
- **Transfer rule (controller):** decides whether a move between grids is allowed and what it costs (buy, refund, or a free move within one grid), and applies the credit-limit check. The grid only answers "does it fit?".
- **Held-item controller:** holds the held item, its origin grid and placement, the grab offset and `bRotated`. Handles pick up, rotate, place-or-snap-back and timer-expiry snap-back. Shared by the trunk and seller-table widgets.
- **Widgets:** the grid widget (draws cells, items, rarity borders and the ghost; reports the hovered cell), the item widget, the held-item visual (follows the cursor), and the tooltip card.

### Verified facts (found by exploring the project)

- `Source/LD60/LD60.Build.cs` already lists `UMG` and `Slate`. `SlateCore` should be added explicitly, since widget input overrides use its types (`FGeometry`, `FPointerEvent`).
- The project was made from the UE5 third-person template (with the Combat, Platforming and SideScrolling variants) on a source-built engine at `C:\UnrealSource\UnrealEngine`. `Build.bat` and `UnrealEditor-Cmd.exe` are present, so building and running automation tests from the command line works.
- The editor tends to be open with Live Coding running. Live Coding can't pick up new classes or header changes, and a command-line build fails while it's active, so close the editor before each build that adds files.
- The project is not a git repository.

### Risks

- **Custom input routing:** with UMG drag-and-drop skipped, the held-item visual, hover-to-cell conversion and clicks across two grid widgets are hand-built (about 2–3 hours). Test mouse position to cell conversion early, across DPI scaling and different viewport sizes.
- **Tooltip covering items:** a badly placed card hides neighboring items. Offset it beside the footprint and keep it on screen.
- **Credit-limit balance:** debt depends on the limit and interest rate being tuned against item margins. Untuned, it's either free money or a death spiral.
- **Timer frustration:** a held item snapping back when time runs out feels unfair without a clear warning before the end.

### Handed to later grills

- **Item data:** which fields the tooltip shows, the rarity color palette, and "lots" as bundled items.
- **Economy / pricing:** the credit limit, the interest rate, and whether the limit grows.
- **Day loop:** phase timer lengths, whether Garage testing uses a clock, and the end-of-phase warning.
- **Win/lose:** how debt interacts with rent and game over.
- **Stretch:** trunk upgrades (rows added at the bottom), and the Diablo 2 swap-on-drop as polish.

### Open threads

None. Every discussion in this grill ended in a decision.

---

## Feature 2: Item data and generation (jam scope)

Full write-up: `Docs/FlipperItemData-design.md`. Items are rolled copies of 6 item definitions (data assets). Each has a rarity rolled per item (Common grey ~70%, Uncommon blue ~25%, Rare gold ~5%) and a hidden works/dead flag. The seller table mixes Tested items (known working, priced higher) with Untested ones (about 60% of the table; about 65% of those work, 35% are dead). The generator rolls a fixed 8 items a day onto the 8x5 seller table with `FindFirstFit`, taking a random seed so tests are deterministic.

### Locked decisions

- **Rarity:** 3 tiers, rolled per item, weights and price multipliers in data. *Rejected:* 4 tiers (an extra tier nobody sees in 5 days), rarity fixed per item type (turns rarity into a lookup).
- **Untested gamble:** a mixed table, Tested (known working) or Untested (hidden). A dead item is never labelled on the table. *Rejected:* everything Untested (no choice), a "Tested Dead" scrap label (a scrap sub-economy the jam can't afford).

### Routine choices

- **Starter items:** Floppy Disk 1x1 $6, Game Boy 1x2 $35, Polaroid Camera 2x2 $45, Rotary Phone 2x2 $55, Typewriter 3x2 $70, CRT Television 4x3 $120 (base value = a Common working copy).
- **Daily table:** 8 items, equal chance per definition, rarity and Tested/Untested rolled per item, re-roll once if it doesn't fit then skip; the same odds every day.
- **Tooltip:** name, footprint, rarity (coloured), status line ("Tested: works" / "Untested", then "WORKS"/"DEAD" after a Garage test). Price is added by the pricing grill; base value is not shown.
- **On the grid:** border colour = rarity, "?" on Untested items, a red "X" on dead items after testing.

### Handed to later grills

- **Pricing:** the rarity multipliers, the Tested price premium and the base-value formula.
- **Day loop:** the Garage test action that flips Untested to Tested.
- **Post-jam:** odds shifting across days, traits, condition, lots (see `Docs/FlipperPlan-Full.md`).

---

## Feature 3: Pricing and cash (jam scope)

Full write-up: `Docs/FlipperPricing-design.md`. Value = base value x rarity multiplier (Common x1, Uncommon x2, Rare x4), rounded to whole dollars; a dead item is worth 10 percent of Value (scrap). The seller's ask price is fixed when the item is rolled: Tested 75 percent of Value, Untested 50 percent, then a random plus or minus 20 percent. Buying is buy-on-drop and a drop you cannot afford is refused. Moving an item bought this Market phase back to the table refunds what you paid; anything older cannot go back. The player starts with $200 and an empty trunk.

### Locked decisions

- **Value:** x1 / x2 / x4 by rarity, dead = 10 percent scrap. *Rejected:* gentler x1 / x1.5 / x2.5; dead worth nothing.
- **Ask price:** Tested 75 percent, Untested 50 percent, plus or minus 20 percent per item. *Rejected:* no variation; 80 / 40 percent.

### Routine choices

- **Start:** $200 cash, empty trunk. *Rejected:* free starter items; $100.
- **Refund:** only for items bought this Market phase, exactly what was paid. Moves inside one grid are free.
- **Cash HUD and an ask-price line in the tooltip.** All numbers are data.

### Handed to later grills

- **Selling:** the buyer's offer formula (Phase 6).
- **Day loop:** clearing the bought-this-phase flag at the end of each Market phase (done in Feature 4).

---

## Feature 4: Day loop (jam scope)

Full write-up: `Docs/FlipperDayLoop-design.md`. A day is Market (timed, 60 s), then Garage (no clock, ends with a Done button), then Sell (timed, 45 s). Every phase can be ended early with a button. In the Garage the player has 3 test charges per day; clicking an Untested trunk item spends one and reveals works or dead. Unsold trunk items carry into the next day; the seller table is thrown away and re-rolled each day. A one-second banner announces each phase, and the timer turns red in the last 10 seconds with a buzzer at zero. The run is 5 days. At the end of Market a held item snaps back and the bought-this-phase flag is cleared on every trunk item.

### Locked decisions

- **Testing:** limited charges per day (3), unused ones lost. *Rejected:* free unlimited testing; cash-costed testing.

### Routine choices

- **Clocks:** Market 60 s, Sell 45 s, Garage untimed. *Rejected:* all timed; one shared daily clock.
- **Hand-over:** End phase button on every phase, trunk persists, table re-rolled, banner. *Rejected:* no early end; seller items persisting.
- All numbers are data.

### Handed to later grills

- **Selling:** Sell-phase mechanics and the buyer's offer formula (Phase 6).
- **Win/lose:** what happens after the last day (Phase 7).
