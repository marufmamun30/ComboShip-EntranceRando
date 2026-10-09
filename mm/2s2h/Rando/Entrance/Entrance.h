#ifndef RANDO_ENTRANCE_H
#define RANDO_ENTRANCE_H

#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/Logic/Logic.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Entrance randomizer for MM, modelled on SoH's (soh/Enhancements/randomizer/entrance.cpp):
// typed entrance pools, coupled or decoupled two-way shuffles, one-way pools, optional mixing of
// pools, placement validated against the item logic, and a layout that is persisted to the save and
// the spoiler instead of being re-derived.
//
// An "entrance" here is one direction of travel through one door. It is identified by the pair of
// entrance ids the region graph already records for every exit: the entrance it loads (`to`) and the
// entrance used to come back through the same door (`from`, ONE_WAY_EXIT if there is none).
namespace Rando {

namespace Entrance {

enum EntranceType : uint8_t {
    ET_NONE,
    ET_SPAWN,
    ET_OWL_WARP,
    ET_BOSS,
    ET_DUNGEON,
    ET_DUNGEON_REVERSE,
    ET_INTERIOR,
    ET_INTERIOR_REVERSE,
    ET_SPECIAL_INTERIOR,
    ET_SPECIAL_INTERIOR_REVERSE,
    ET_GROTTO,
    ET_GROTTO_REVERSE,
    ET_OVERWORLD,
    ET_MAX,
};

enum EntranceGroup : uint8_t {
    EG_CLOCK_TOWN,
    EG_TERMINA_FIELD,
    EG_SWAMP,
    EG_MOUNTAIN,
    EG_RANCH,
    EG_GREAT_BAY,
    EG_IKANA,
    EG_ONE_WAY,
    EG_MAX,
};

struct EntranceInfo {
    s32 to;
    s32 from;
    RandoRegionId parent;
    // Region the player stands in after taking this entrance unshuffled.
    RandoRegionId connected;
    EntranceType type;
    EntranceGroup group;
    // Index of the entrance going back through the same door, -1 for one-ways.
    s16 reverse;
    // Inverted Stone Tower Temple only joins the dungeon pool on the setting that asks for it.
    bool optional;
    std::string source;
    std::string destination;
};

// Where a grotto hole sits in its overworld scene, so the player can be put back at it.
struct GrottoHole {
    s32 grotto;        // Logic id of the grotto interior, ENTRANCE(GROTTOS, n)
    s32 realEntrance;  // Entrance the game actually loads for the interior
    u8 data;           // Chest contents/flag byte the interior reads from the return data
    s16 sceneId;       // Overworld scene holding the hole
    s32 sceneEntrance; // Entrance that loads that scene
    s8 room;
    s16 x, y, z;
    s16 yawDegrees;
};

const std::vector<EntranceInfo>& GetEntrances();
s16 FindEntrance(s32 to, s32 from);
const char* GetTypeName(EntranceType type);
// Where taking an entrance puts you, named so two arrivals in the same area can be told apart.
std::string GetArrivalName(s16 index);
const char* GetGroupName(EntranceGroup group);
const GrottoHole* GetGrottoHole(s32 grotto);
const GrottoHole* FindGrottoHoleInScene(s16 sceneId, s32 realEntrance, u8 data);
s16 NormalizeSceneId(s16 sceneId);
s32 NormalizeEntrance(s32 entrance);

// Layout state. Every entrance has a connection: the index of the entrance whose destination it now
// leads to. Unshuffled entrances are connected to themselves.
bool IsShuffled();
bool IsEntranceShuffled(s16 index);
s16 GetConnection(s16 index);
void ClearLayout();
// Region reached through the exit `exitId` (with return entrance `returnEntrance`), `vanilla` when the
// exit is not part of the shuffle.
RandoRegionId ResolveExitRegion(s32 exitId, s32 returnEntrance, RandoRegionId vanilla);

// Generation. Reads the entrance options from RANDO_SAVE_OPTIONS and leaves the RNG wherever the
// shuffle left it, so callers that need a reproducible item fill must reseed afterwards. `itemPool`
// is every item the player can ever own; placement is validated with all of them in hand. Throws
// std::runtime_error when no valid layout is found. `giveItem` grants an item into gSaveContext;
// it defaults to Rando::GiveItem, which headless callers cannot use.
using ItemGiver = void (*)(RandoItemId);
bool AnyOptionEnabled();
void Shuffle(const std::vector<RandoItemId>& itemPool, ItemGiver giveItem = nullptr);

// Persistence
void WriteLayoutToSave();
void ReadLayoutFromSave();
nlohmann::json LayoutToJson();
void LayoutFromJson(const nlohmann::json& json);

// Discovery, for the tracker
bool IsDiscovered(s16 index);
void SetDiscovered(s16 index);

// Runtime
void RegisterHooks();
// Entrance most recently taken, -1 if the last transition was not through a known door.
s16 GetLastEntranceTaken();
// Logic id of the grotto the player was last routed into, -1 when not in one.
s32 GetActiveGrotto();
// Checks that every shufflable entrance can be recognised at runtime from what the game will ask
// for, and that where it arrives is where the logic believes. Logs and returns the problem count.
int SelfTest();

} // namespace Entrance

} // namespace Rando

#endif // RANDO_ENTRANCE_H
