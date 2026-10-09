#include "Entrance.h"

#include "2s2h/GameInteractor/GameInteractor.h"

#include <map>
#include <spdlog/spdlog.h>

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64horse.h"
#include "z64scene.h"
}

namespace Rando {

namespace Entrance {

#define SCENE_OF(entrance) (((entrance) >> 9) & 0x7F)

// Logic id of the grotto the player is standing in. The generic grottos all load the same entrance,
// so once a hole has been redirected the game itself no longer knows which one this is.
static s32 sActiveGrotto = -1;
static s16 sLastEntranceTaken = -1;

s16 GetLastEntranceTaken() {
    return sLastEntranceTaken;
}

s32 GetActiveGrotto() {
    return sActiveGrotto;
}

static bool SameScene(RandoRegionId regionId, s16 sceneId) {
    auto region = Rando::Logic::Regions.find(regionId);
    return region != Rando::Logic::Regions.end() && NormalizeSceneId(region->second.sceneId) == sceneId;
}

// Which door the player just went through, from the entrance the game asked for and the scene it
// asked from. `grottoData` is the chest byte a hole leaves in the return data, `activeGrotto` the
// grotto the player is in, and `grottoReturn` whether the game is doing its return-to-the-hole.
// -1 when it is not a door the shuffle knows.
static s16 IdentifySource(s32 requested, s16 sceneId, u8 grottoData, s32 activeGrotto, bool grottoReturn) {
    auto& entrances = GetEntrances();

    // Leaving a grotto through its exit, or by climbing out, does not name an entrance at all: the
    // game just returns to where the hole was.
    if (sceneId == SCENE_KAKUSIANA && activeGrotto >= 0) {
        for (size_t i = 0; i < entrances.size(); i++) {
            // The few grottos with a real exit (the bean seller's, Lone Peak Shrine) do name their
            // entrance. Anything else asked for from in here, like a warp, is not the way out.
            if (entrances[i].type == ET_GROTTO_REVERSE && entrances[i].from == activeGrotto &&
                (grottoReturn || entrances[i].to == requested)) {
                return (s16)i;
            }
        }
    }

    // Holes that share an interior are told apart by the scene they are in and the chest they carry.
    if (SCENE_OF(requested) == ENTR_SCENE_GROTTOS) {
        const GrottoHole* hole = FindGrottoHoleInScene(sceneId, requested, grottoData);
        if (hole != nullptr) {
            requested = hole->grotto;
        }
    }

    s16 match = -1;
    s16 anywhere = -1;
    int anywhereCount = 0;
    for (size_t i = 0; i < entrances.size(); i++) {
        auto& info = entrances[i];
        if (info.to != requested) {
            continue;
        }
        if (info.parent == RR_MAX) {
            // Owl warps and the cycle start can be triggered from any scene.
            anywhere = (s16)i;
            anywhereCount++;
        } else if (SameScene(info.parent, sceneId)) {
            if (match >= 0) {
                return -1; // Two doors in this scene load the same entrance, cannot tell which
            }
            match = (s16)i;
        }
    }
    if (match >= 0) {
        return match;
    }
    return anywhereCount == 1 ? anywhere : -1;
}

// Points the game's grotto return at a hole.
static void SetGrottoReturn(const GrottoHole* hole) {
    RespawnData& respawn = gSaveContext.respawn[RESPAWN_MODE_UNK_3];
    respawn.entrance = hole->sceneEntrance;
    respawn.roomIndex = hole->room;
    respawn.pos.x = hole->x;
    respawn.pos.y = hole->y;
    respawn.pos.z = hole->z;
    respawn.yaw = DEG_TO_BINANG(hole->yawDegrees);
    respawn.playerParams = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_GROTTO);
    respawn.tempSwitchFlags = 0;
    respawn.unk_18 = 0;
    respawn.tempCollectFlags = 0;
}

// The player is about to be put into a grotto they did not drop into through its own hole, so the
// return data the game recorded (if any) belongs to somewhere else. Aim it at where this grotto's
// way out leads, so that climbing out is right even before the transition hook looks at it, and so
// the entrance the game reads back during the fade is always a real one.
static void PrepareGrottoReturn(s32 grotto) {
    auto& entrances = GetEntrances();
    for (size_t i = 0; i < entrances.size(); i++) {
        if (entrances[i].type != ET_GROTTO_REVERSE || entrances[i].from != grotto) {
            continue;
        }
        s16 connection = GetConnection((s16)i);
        auto& exitTarget = entrances[connection >= 0 ? connection : (s16)i];
        const GrottoHole* hole = exitTarget.type == ET_GROTTO_REVERSE ? GetGrottoHole(exitTarget.from) : nullptr;
        if (hole != nullptr) {
            SetGrottoReturn(hole);
        } else {
            gSaveContext.respawn[RESPAWN_MODE_UNK_3].entrance = exitTarget.to;
        }
        return;
    }
}

static void ArriveAt(const EntranceInfo& target) {
    if (SCENE_OF(target.to) == ENTR_SCENE_GROTTOS) {
        PrepareGrottoReturn(target.to);
        const GrottoHole* hole = GetGrottoHole(target.to);
        if (hole != nullptr) {
            gSaveContext.save.entrance = hole->realEntrance;
            gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = (s8)hole->data;
        } else {
            gSaveContext.save.entrance = target.to;
        }
        gSaveContext.respawnFlag = 0;
        sActiveGrotto = target.to;
    } else if (target.type == ET_GROTTO_REVERSE && GetGrottoHole(target.from) != nullptr) {
        // Coming up out of a hole: the same return the game does after a grotto, aimed at this hole.
        const GrottoHole* hole = GetGrottoHole(target.from);
        SetGrottoReturn(hole);
        gSaveContext.save.entrance = hole->sceneEntrance;
        gSaveContext.respawnFlag = 4;
        sActiveGrotto = -1;
    } else {
        gSaveContext.save.entrance = target.to;
        gSaveContext.respawnFlag = 0;
        sActiveGrotto = -1;
    }

    // Whatever cutscene or day variant was queued belonged to the vanilla destination. Left alone it
    // is added to the entrance as a layer the new spawn may not have. This value also lets Play_Init
    // pick the right version of the scenes that change with the story (see z_play.c).
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    // The horse only exists in a few scenes, and those are no longer the ones this exit leads to.
    gHorseIsMounted = false;
}

static void MarkDiscovered(s16 source, s16 target) {
    auto& entrances = GetEntrances();
    SetDiscovered(source);
    // When doors are coupled, seeing one side tells you the other.
    s16 sourceReverse = entrances[source].reverse;
    s16 targetReverse = entrances[target].reverse;
    if (sourceReverse >= 0 && targetReverse >= 0 && GetConnection(targetReverse) == sourceReverse) {
        SetDiscovered(targetReverse);
    }
}

static void OnTransition() {
    if (gPlayState == NULL || gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return;
    }
    // Play is torn down for more than scene changes (the day splash, quitting, handing over to the
    // other game). Only a transition that Play itself finished leaves these two in agreement.
    if (gPlayState->state.init != Play_Init || gPlayState->nextEntrance != (u16)gSaveContext.save.entrance) {
        return;
    }

    s16 sceneId = NormalizeSceneId(gPlayState->sceneId);
    s32 respawnFlag = gSaveContext.respawnFlag;
    bool leavingGrotto = sceneId == SCENE_KAKUSIANA && respawnFlag == 4;
    // 0 is an ordinary exit and -2 one taken by falling. Every other value reloads a spot the game
    // recorded earlier (void out, death, song landing), which was already resolved when it was stored.
    if (respawnFlag != 0 && respawnFlag != -2 && !leavingGrotto) {
        return;
    }
    s32 requested = NormalizeEntrance(gSaveContext.save.entrance);

    s16 source = IdentifySource(requested, sceneId, (u8)gSaveContext.respawn[RESPAWN_MODE_UNK_3].data, sActiveGrotto,
                                leavingGrotto);
    sLastEntranceTaken = source;
    if (source < 0) {
        // Wherever this leads, it is not a grotto the shuffle routed the player into.
        sActiveGrotto = -1;
        return;
    }

    auto& entrances = GetEntrances();
    s16 target = GetConnection(source);
    if (target < 0 || target == source) {
        // Not shuffled. Still note which grotto this is, in case its way out is.
        sActiveGrotto = SCENE_OF(entrances[source].to) == ENTR_SCENE_GROTTOS ? entrances[source].to : -1;
        return;
    }

    ArriveAt(entrances[target]);
    MarkDiscovered(source, target);
}

// The cycle start is the one entrance that is also reached without a transition, when a file loads.
static void OnSaveLoaded() {
    sActiveGrotto = -1;
    sLastEntranceTaken = -1;
    if (!IS_RANDO || !IsShuffled() || gSaveContext.save.isOwlSave) {
        return;
    }
    if (gSaveContext.save.entrance != ENTRANCE(SOUTH_CLOCK_TOWN, 0)) {
        return;
    }
    s16 source = FindEntrance(ENTRANCE(SOUTH_CLOCK_TOWN, 0), ONE_WAY_EXIT);
    s16 target = GetConnection(source);
    if (source < 0 || target < 0 || target == source) {
        return;
    }
    gSaveContext.save.entrance = GetEntrances()[target].to;
    SetDiscovered(source);
}

int SelfTest() {
    auto& entrances = GetEntrances();
    int problems = 0;
    int tested = 0;
    for (size_t i = 0; i < entrances.size(); i++) {
        auto& info = entrances[i];
        if (info.type == ET_NONE) {
            continue;
        }
        tested++;
        std::string name = info.source + " -> " + info.destination;

        // What the game asks for when the player takes this entrance.
        s32 requested = info.to;
        u8 grottoData = 0;
        s32 activeGrotto = -1;
        s16 sceneId = SCENE_MAX;
        auto parent = Rando::Logic::Regions.find(info.parent);
        if (parent != Rando::Logic::Regions.end()) {
            sceneId = NormalizeSceneId(parent->second.sceneId);
        }
        if (info.type == ET_GROTTO_REVERSE) {
            sceneId = SCENE_KAKUSIANA;
            activeGrotto = info.from;
        } else if (info.type == ET_GROTTO) {
            const GrottoHole* hole = GetGrottoHole(info.to);
            if (hole != nullptr) {
                requested = hole->realEntrance;
                grottoData = hole->data;
                if (hole->sceneId != sceneId) {
                    SPDLOG_ERROR("Entrance self test: hole table has {} in a different scene than the logic", name);
                    problems++;
                }
            }
        }
        if (info.parent == RR_MAX) {
            sceneId = SCENE_SPOT00; // Any scene that does not have a door of its own to this spawn
        }

        s16 identified = IdentifySource(requested, sceneId, grottoData, activeGrotto, info.type == ET_GROTTO_REVERSE);
        if (identified != (s16)i) {
            SPDLOG_ERROR("Entrance self test: {} is recognised as {}", name,
                         identified < 0 ? std::string("nothing")
                                        : entrances[identified].source + " -> " + entrances[identified].destination);
            problems++;
        }

        // Where arriving through it lands, against the region the logic connects it to.
        bool landsAtHole = info.type == ET_GROTTO_REVERSE && GetGrottoHole(info.from) != nullptr;
        bool landsInGrotto = info.type == ET_GROTTO;
        if (!landsAtHole && !landsInGrotto) {
            RandoRegionId landed = Rando::Logic::GetRegionIdFromEntrance(info.to);
            if (landed != info.connected) {
                auto regionName = [](RandoRegionId regionId) {
                    auto region = Rando::Logic::Regions.find(regionId);
                    return std::string(region != Rando::Logic::Regions.end() ? region->second.name : "?") + " #" +
                           std::to_string((int)regionId);
                };
                SPDLOG_ERROR("Entrance self test: arriving via {} lands in region {} but the logic expects {}", name,
                             regionName(landed), regionName(info.connected));
                problems++;
            }
        }
    }
    SPDLOG_INFO("Entrance self test: {} entrances, {} problems", tested, problems);
    return problems;
}

void RegisterHooks() {
    ReadLayoutFromSave();
    OnSaveLoaded();

    COND_HOOK(OnPlayDestroy, IS_RANDO && IsShuffled(), OnTransition);
}

} // namespace Entrance

} // namespace Rando
