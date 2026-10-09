# ComboShip + MM Entrance Randomizer (Windows build)

A ready-to-run test build of [ComboShip](https://github.com/Varuuna/ComboShip), the Ocarina of Time +
Majora's Mask combo randomizer, with an entrance randomizer added for Majora's Mask.

## Download and run

1. **[Download the zip](https://github.com/marufmamun30/ComboShip-EntranceRando/archive/refs/heads/windows-build.zip)**
   and extract it to a folder you can write to (not Program Files).
2. Run `ComboShip.exe`. Windows may warn about an unrecognised app: choose "More info", then "Run anyway".
3. On the first launch it asks for your own Ocarina of Time and Majora's Mask ROMs and extracts the
   game data from them. No ROMs or game data are included here.

**Already have ComboShip?** Extract this into a new folder instead of over your existing install.
You can skip step 3 by copying `oot.o2r` and `mm.o2r` from your existing folder into this one. Copy
the `Save` folder and `comboship.json` as well if you want your save files and settings; older
saves load with every entrance option off.

## Turning on the entrance randomizer

Open the Majora's Mask randomizer settings (the **Rando** section of the menu). There is a new
**Entrances** page:

| Option | What it shuffles |
|--------|------------------|
| Dungeon Entrances | The four temples; optionally Inverted Stone Tower Temple as a fifth |
| Boss Entrances | Which boss is behind each boss door |
| Overworld Entrances | The links between areas |
| Interior Entrances | Houses, shops and fountains (Simple or All) |
| Grotto Entrances | Which grotto each hole leads to |
| Owl Warps | Where each Song of Soaring destination takes you |
| Cycle Start | Where each three-day cycle begins |
| Decouple Entrances | Going back through a door no longer returns you where you came from |
| Mixed Entrance Pools | Shuffle the chosen kinds together |

Pick your options, generate a new seed and start a new file. The layout is fixed when the file is
created. **Show Entrance Tracker** on the same page opens a window that lists every shuffled entrance
and fills in where it leads once you have been through it.

## Playing together

Everyone in the same Anchor room needs this same build. The entrance tracker is not shared between
players yet: each player fills in their own.

## Status

This is an early test build. Seed generation and logic were checked across every option
combination, but the in-game side has had very little play time. If a door sends you somewhere
broken, note the seed and which door it was. The places most likely to misbehave:

- arriving at a grotto hole in a version of a scene that hides it (the Mountain Village tunnel
  grotto in winter)
- entrance cutscenes playing after an unusual arrival
- the cycle start when an intro-skip enhancement is on
- riding Epona through a shuffled exit

There are no entrance hints yet.

## What is in this folder

- `2ship.dll` and `ComboShip.exe` are built from the
  [`feat/mm-entrance-rando`](https://github.com/marufmamun30/ComboShip-EntranceRando/tree/feat/mm-entrance-rando)
  branch (commit `9310c74d`).
- Everything else is unchanged from the ComboShip Windows build of 2026-09-28.

Source and design notes:
[docs/deviations/mm-entrance-rando.md](https://github.com/marufmamun30/ComboShip-EntranceRando/blob/feat/mm-entrance-rando/docs/deviations/mm-entrance-rando.md)
