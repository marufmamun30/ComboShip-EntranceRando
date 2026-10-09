# ComboShip

**ComboShip is a cross-game randomizer for [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) (Ocarina of Time) and [2 Ship 2 Harkinian](https://github.com/2ship2harkinian/2ship2harkinian) (Majora's Mask).**

**DISCLAIMER: THIS IS AN UNOFFICIAL PROJECT AND USES AI AS PART OF THE DEVELOPMENT. IT IS NOT CREATED OR HANDLED BY THE HARBOUR MASTERS TEAM. BRING ANY IDEAS, QUESTIONS, OR CONCERNS TO ME DIRECTLY!**

## This fork: Majora's Mask entrance randomizer

This fork of [Varuuna/ComboShip](https://github.com/Varuuna/ComboShip) adds an entrance randomizer for Majora's Mask: dungeons, bosses, overworld, interiors, grottos, owl warps and cycle start, with mixed pools and decoupled entrances.

- **Play it (Windows):** [download the build](https://github.com/marufmamun30/ComboShip-EntranceRando/archive/refs/heads/windows-build.zip), extract it and run `ComboShip.exe`. [Setup notes and known rough edges](https://github.com/marufmamun30/ComboShip-EntranceRando/tree/windows-build#readme). You need your own Ocarina of Time and Majora's Mask ROMs.
- **Source:** this branch. The change on its own is the [`feat/mm-entrance-rando`](https://github.com/marufmamun30/ComboShip-EntranceRando/tree/feat/mm-entrance-rando) branch; design notes are in [`docs/deviations/mm-entrance-rando.md`](docs/deviations/mm-entrance-rando.md).

It is an early test build: seed generation is verified, in-game behaviour has had little play time.

## What it is

Like [OOTMM](https://ootmm.com/), ComboShip shuffles items across *both* games at once: a check in Ocarina of Time can hold a Majora's Mask item and vice-versa, and a single seed spans the two. Both games run together in one application; ComboShip builds that combined runtime on top of the existing Ship of Harkinian and 2 Ship 2 Harkinian ports.

## Features

- Anything within Ship and 2Ship is here. When those projects gets new updates, they are not far from inclusion here.
- Online Multiplayer through Anchor. And yes it works cross-game as well. Work together in OOT and MM, split or together.

## Future plans

- Possibly looking into some more OOTMM features, but it's not a priority yet.
- Possible Archipelago support as well, ideally through the existing SoH implementation

## Got issues?

Check out the nightly builds to see if your issue has already been fixed for an upcoming release:
- [Windows](https://nightly.link/Varuuna/ComboShip/workflows/build-artifacts/develop/ComboShip-windows.zip)
- [Linux](https://nightly.link/Varuuna/ComboShip/workflows/build-artifacts/develop/ComboShip-linux.zip) / [Linux AppImage](https://nightly.link/Varuuna/ComboShip/workflows/build-artifacts/develop/ComboShip-linux-appimage.zip)

If you can still reproduce it, please create an issue.

## Building

ComboShip builds on **Windows** (below) and **Linux** — see
[`docs/BUILDING_LINUX.md`](docs/BUILDING_LINUX.md) for Linux build, run and
AppImage packaging instructions. macOS support will return later.

### Prerequisites

- Windows 10/11 (x64)
- Visual Studio 2022 (MSVC, with C++20 / C23 support)
- CMake 3.26 or newer

### Configure and build

From the repository root (the clone folder, which contains this README), configure once to generate the Visual Studio solution:

```powershell
cmake -B build/x64 -A x64
```

Helper scripts in `scripts/` wrap `cmake --build` and default to a Debug build (pass `--Release` for Release):

```powershell
./scripts/build-comboship.ps1  ->  ComboShip.exe
```

## Packaging

`cpack` produces a single Windows ZIP bundling the full runtime (`ComboShip.exe`, the engine and UI DLLs, both ports, and assets):

```powershell
cpack
```

## Contributing

ComboShip is built on two upstream projects, kept as vendored copies under `soh/` and `mm/`:

- Ship of Harkinian — https://github.com/HarbourMasters/Shipwright
- 2 Ship 2 Harkinian — https://github.com/2ship2harkinian/2ship2harkinian

ComboShip-specific code lives in `combo/`, and changes to the vendored ports are kept minimal and guarded behind `COMBO_BUILD`. See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the design and [`docs/UPSTREAM_MERGES.md`](docs/UPSTREAM_MERGES.md) for how upstream changes are merged in.

## License

ComboShip combines two separately-licensed projects; each retains its own license. See the `soh/` and `mm/` directories for details.
