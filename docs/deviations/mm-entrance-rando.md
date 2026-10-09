# MM Entrance Randomizer

Entrance randomization for Majora's Mask, built to match the feature set of SoH's
(`soh/soh/Enhancements/randomizer/entrance.cpp`). Upstream 2Ship has no entrance randomizer on the
branch we track; its draft PR #1329 (coupled swaps, rejection sampling, nothing persisted) was the
starting reference but none of its code is used. Everything lives in the port layer; no decomp
source is touched.

## Where it lives

| File | Role |
|------|------|
| `mm/2s2h/Rando/Entrance/Entrance.h` | API and types |
| `mm/2s2h/Rando/Entrance/EntranceData.cpp` | Entrance catalogue built from the logic region graph, pool membership, names, grotto hole table |
| `mm/2s2h/Rando/Entrance/EntranceShuffle.cpp` | Shuffle, validation, save and spoiler formats, discovery bits |
| `mm/2s2h/Rando/Entrance/EntranceHooks.cpp` | Runtime: rewrites the pending entrance on scene transitions |
| `mm/2s2h/Rando/Entrance/EntranceTracker.cpp` | Entrance Tracker window |
| `mm/2s2h/Rando/Entrance/EntranceCombo.cpp` | Combo exports (`COMBO_BUILD` only) |

Edits to existing files, all small:

- `Rando/Types.h`, `Rando/StaticData/Options.cpp`: 13 options, **appended** to `RandoOptionId` (see
  "Save compatibility").
- `Rando/Logic/Logic.cpp`: `FindReachableRegions` resolves each exit through
  `Entrance::ResolveExitRegion`; `GetRegionIdFromEntrance` asks the entrance module which shared
  grotto the player is in.
- `Rando/MiscBehavior/OnFileCreate.cpp`: shuffle entrances before the item fill.
- `Rando/Spoiler/Generate.cpp`, `Apply.cpp`: `"entrances"` array.
- `Rando/Rando.cpp`: `Entrance::RegisterHooks()` on save load.
- `Rando/Menu.cpp`: "Entrances" sidebar. `BenGui/BenGui.cpp`: tracker window registration.
- `include/z64save.h`, `BenJsonConversions.hpp`: layout and discovery bits in `RandoSaveInfo`.
- `BenPort.cpp`: `MM_InitRandoSaveFile` bakes the layout into the save; `MM_LoadSaveForCombo` reads
  it back for the dormant save.
- `combo/ComboShip.cpp`, `combo/ComboRandoHeadless.cpp`: call the MM exports next to the OOT ones.

## Model

An entrance is one direction through one door, identified by the two entrance ids the region graph
already stores for each exit: the entrance it loads and the entrance that leads back (`EXIT(to,
from, ...)`). That pair is unique per door even where several doors share a spawn, which the entrance
id alone is not (every grotto exit in a scene "returns" to the same placeholder). The catalogue is
derived from `Rando::Logic::Regions` at first use, so it follows the logic when upstream changes it.

The layout is a connection per entrance: the entrance whose destination it now leads to. It is held
in the module, not in `gSaveContext`, so it survives the combo oracle's `Reset`.

### Pools

| Option | Contents |
|--------|----------|
| Dungeon Entrances (Off / On / On + Inverted Stone Tower) | Woodfall, Snowhead, Great Bay, Stone Tower Temple; optionally Inverted Stone Tower Temple as a fifth |
| Boss Entrances | The four boss doors. One-way in MM; blue warps stay with their boss |
| Overworld Entrances | 42 links between areas, both directions |
| Interior Entrances (Off / Simple / All) | 35 single-door interiors; "All" adds 13 multi-door or area-like ones |
| Grotto Entrances | 26 grottos, including the generic chest and cow grottos that share an interior |
| Owl Warps | The ten Song of Soaring destinations (the analogue of SoH's Warp Songs) |
| Cycle Start | Where each cycle begins (the analogue of SoH's Overworld Spawns) |
| Mixed Entrance Pools + Mix Dungeons / Overworld / Interiors / Grottos | Shuffle the chosen pools as one |
| Decouple Entrances | Place each direction independently |

Never shuffled: the Clock Tower door (it is the MM to OOT portal and the cycle hub), Sakon's Hideout
(scripted event), Dampe's house, the Moon, and the internal doors of Pirates' Fortress, Deku Palace,
Ikana Castle and Stone Tower.

SoH options with no MM counterpart: Ganon's Tower, Thieves' Hideout, Owl Drops, the age-restricted
boss mode, and Mix Bosses (MM boss doors are one-way, so they cannot join a two-way pool).

### Shuffle

Same shape as SoH's: within a pool every unplaced destination is assumed reachable from the start,
entrances are placed one at a time, and each placement is kept only if every check that is reachable
in the vanilla world with every item in hand still is. A pool that dead-ends is retried up to 20
times before generation fails. One-ways are placed last, against the finished map. In `COMBO_BUILD`
a shuffled cycle start must also reach South Clock Town with starting items alone, because the Clock
Tower door there is the way back to OOT.

The shuffle uses its own generator seeded from `finalSeed`. Validation gives items, and
`ConvertItem` reseeds `Ship_Random`, which would otherwise make every retry repeat itself.

### Runtime

One `OnPlayDestroy` hook, registered only when the loaded save has shuffled entrances. It acts only
on transitions Play itself completed (`state.init == Play_Init` and `nextEntrance ==
save.entrance`), so the day splash, quitting and the hand-off to OOT are left alone. It identifies
the door from the requested entrance plus the current scene, then rewrites `save.entrance`.

Grottos need more than that:

- Many holes load the same interior (`GROTTOS, 4` / `GROTTOS, 10`). The hole is identified from its
  scene and the chest byte it stores in the return data; the hole table in `EntranceData.cpp` was
  read from the `Door_Ana` actors in each scene.
- A grotto exit names no entrance; the game returns to wherever the hole was. Arriving at a hole
  from somewhere else therefore fills in `respawn[RESPAWN_MODE_UNK_3]` from the hole table and uses
  the game's own grotto return (`respawnFlag = 4`).
- The module remembers which grotto the player was routed into, since the game can no longer tell.

Song of Time and file load: the cycle start is rewritten in the hook when it is reached by
transition, and on save load when the file opens directly in South Clock Town.

## Save compatibility

`randoSaveOptions` is serialized by index. The new options are appended to the enum rather than
sorted in, and `from_json` now tolerates a shorter array, so saves made before this change load with
every entrance option off. Keep new options at the end.

The layout is stored as `entranceOverrides` (four entrance ids per shuffled entrance) and
`entranceDiscovered`; both are optional on load.

## Combo integration

| Export (`2ship.dll`) | Purpose |
|----------------------|---------|
| `MM_ShuffleEntrancesForCombo(seed)` | Shuffle from the menu's entrance options; 0 = no valid layout |
| `MM_DumpEntranceLayout()` | Layout as JSON, stored at `entrances.mm` in the consolidated spoiler |
| `MM_ApplyEntranceLayoutForCombo(json)` | Install a recorded layout |

The launcher shuffles MM right after OOT at each generation site and rerolls the attempt if either
fails. Unlike OOT's layout, which reload re-derives from the master seed, MM's is **replayed from the
spoiler**: `Combo_OnOOTSaveInit` applies `entrances.mm` just before `MM_InitRandoSaveFile`. The fill
needs no changes, since MM's oracle already answers from MM's logic.

## Not done

- Entrance discovery is not synced over Anchor (SoH has an `EntranceDiscovered` packet).
- No entrance hints (SoH can hint warp song destinations).
- The Entrance Tracker is a plain MM window; it is not part of the combo tracker swap model, so it
  stays visible while OOT is in the foreground.
- The combo fill does not model the return trip to OOT beyond the cycle-start rule above.

## Verification status

Checked headlessly on Windows (2026-10-08):

- `comborando` over nine setting combinations, three seeds each, all generated and judged
  completable: each pool alone, all pools together, and all pools decoupled, mixed, and mixed +
  decoupled.
- `comborando --playthrough` on a recorded mixed layout: replays `entrances.mm` and passes.
- Recorded layouts are structurally sound: no destination handed out twice, and with coupling on
  every door's way back leads to where you came from.
- `Entrance::SelfTest()` (run by setting `COMBO_ENTRANCE_SELFTEST` for a generation): all 257
  shufflable entrances are recognised from the entrance and scene the game would ask with, and each
  arrives in the region the logic expects.
- `ComboShip.exe` boots both games with the change and `COMBO_AUTOGEN_SEED` writes a spoiler with
  both games' entrances.

**Not checked: playing it.** Nobody has walked through a shuffled door yet. The transition hook, the
grotto returns and the tracker window are untested in a running game. The likeliest rough edges:
arriving at a grotto hole in a scene variant that hides it (the Mountain Village tunnel grotto in
winter), entrance cutscenes firing at unusual arrivals, the cycle start when an intro-skip
enhancement rewrites the entrance during `Play_Init`, and riding Epona through a shuffled exit.
MM's own (non-combo) generation path in `OnFileCreate.cpp` compiles but cannot run in this build.
