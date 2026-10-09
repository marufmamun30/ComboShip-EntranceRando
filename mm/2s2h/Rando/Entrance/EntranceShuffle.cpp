#include "Entrance.h"

#include <algorithm>
#include <memory>
#include <random>
#include <set>
#include <unordered_map>
#include <spdlog/spdlog.h>

extern "C" {
#include "variables.h"
}

// The owl-save buffer holds a whole Save in each half; the entrance layout must not outgrow it.
static_assert(sizeof(Save) <= SAVE_BUFFER_SIZE_HALF, "Save no longer fits the save buffer");

namespace Rando {

namespace Entrance {

// sConnections[i] is the entrance whose destination entrance i leads to: i itself when unshuffled,
// -1 while it is waiting to be placed.
static std::vector<s16> sConnections;
static bool sShuffled = false;
static bool sShuffling = false;
// Destinations not handed out yet. While a pool is being placed these count as reachable from the
// start, so a placement is only rejected once nothing left in the pool could repair it.
static std::vector<s16> sAssumed;
static std::vector<bool> sDiscovered;
static ItemGiver sGiveItem = nullptr;

static void EnsureState() {
    size_t count = GetEntrances().size();
    if (sConnections.size() != count) {
        sConnections.resize(count);
        for (size_t i = 0; i < count; i++) {
            sConnections[i] = (s16)i;
        }
        sDiscovered.assign(count, false);
        sShuffled = false;
    }
}

static void RefreshShuffled() {
    sShuffled = false;
    for (size_t i = 0; i < sConnections.size(); i++) {
        if (sConnections[i] != (s16)i) {
            sShuffled = true;
            return;
        }
    }
}

bool IsShuffled() {
    return sShuffled;
}

bool IsEntranceShuffled(s16 index) {
    EnsureState();
    return index >= 0 && index < (s16)sConnections.size() && sConnections[index] != index;
}

s16 GetConnection(s16 index) {
    EnsureState();
    if (index < 0 || index >= (s16)sConnections.size()) {
        return -1;
    }
    return sConnections[index];
}

void ClearLayout() {
    sConnections.clear();
    EnsureState();
}

RandoRegionId ResolveExitRegion(s32 exitId, s32 returnEntrance, RandoRegionId vanilla) {
    if (!sShuffled && !sShuffling) {
        return vanilla;
    }
    s16 index = FindEntrance(exitId, returnEntrance);
    if (index < 0 || index >= (s16)sConnections.size()) {
        return vanilla;
    }
    s16 connection = sConnections[index];
    if (connection == index) {
        return vanilla;
    }
    if (connection < 0) {
        // Not placed yet. The root region is always reachable, so pointing there adds nothing.
        return RR_MAX;
    }
    return GetEntrances()[connection].connected;
}

bool AnyOptionEnabled() {
    return RANDO_SAVE_OPTIONS[RO_ENTRANCES_DUNGEONS] != RO_ENTRANCES_DUNGEONS_OFF ||
           RANDO_SAVE_OPTIONS[RO_ENTRANCES_BOSSES] != RO_GENERIC_OFF ||
           RANDO_SAVE_OPTIONS[RO_ENTRANCES_OVERWORLD] != RO_GENERIC_OFF ||
           RANDO_SAVE_OPTIONS[RO_ENTRANCES_INTERIORS] != RO_ENTRANCES_INTERIORS_OFF ||
           RANDO_SAVE_OPTIONS[RO_ENTRANCES_GROTTOS] != RO_GENERIC_OFF ||
           RANDO_SAVE_OPTIONS[RO_ENTRANCES_OWL_WARPS] != RO_GENERIC_OFF ||
           RANDO_SAVE_OPTIONS[RO_ENTRANCES_SPAWN] != RO_GENERIC_OFF;
}

// MARK: - Reachability

struct SearchResult {
    std::set<RandoCheckId> checks;
    std::set<RandoRegionId> regions;
};

// Everything reachable starting from `base`, picking up the item preset at each check on the way
// (checks that are not part of the item pool keep their vanilla item, and logic can depend on it).
// This mirrors the loop in GlitchlessLogic.cpp without placing anything. Leaves gSaveContext dirty.
static void Search(const SaveContext& base, SearchResult& result) {
    memcpy(&gSaveContext, &base, sizeof(SaveContext));
    result.checks.clear();
    result.regions = { RR_MAX };

    auto regionTimeStates = Rando::Logic::InitializeRegionTimeStates(RR_MAX);
    for (s16 assumed : sAssumed) {
        RandoRegionId regionId = GetEntrances()[assumed].connected;
        if (result.regions.insert(regionId).second) {
            Rando::Logic::EnsureRegionTimeState(regionTimeStates, regionId);
        }
    }

    std::set<std::pair<RandoEvent, std::function<bool()>>*> eventsInLogic;
    bool changed = true;
    while (changed) {
        changed = false;

        size_t previousRegionCount = result.regions.size();
        for (RandoRegionId regionId : std::set<RandoRegionId>(result.regions)) {
            Rando::Logic::FindReachableRegions(regionId, result.regions, regionTimeStates);
        }
        if (result.regions.size() != previousRegionCount) {
            changed = true;
        }

        for (RandoRegionId regionId : result.regions) {
            auto regionIt = Rando::Logic::Regions.find(regionId);
            if (regionIt == Rando::Logic::Regions.end()) {
                continue;
            }
            auto& region = regionIt->second;
            Rando::Logic::SetCurrentRegionTime(regionTimeStates, regionId);

            for (auto& randoEvent : region.events) {
                if (!eventsInLogic.contains(&randoEvent) && randoEvent.second()) {
                    RANDO_EVENTS[randoEvent.first]++;
                    eventsInLogic.insert(&randoEvent);
                    changed = true;
                }
            }

            for (auto& [randoCheckId, checkLogic] : region.checks) {
                if (result.checks.contains(randoCheckId) || !checkLogic.first()) {
                    continue;
                }
                result.checks.insert(randoCheckId);
                changed = true;

                RandoItemId randoItemId = RANDO_SAVE_CHECKS[randoCheckId].randoItemId;
                if (randoItemId == RI_UNKNOWN || randoItemId == RI_NONE || randoItemId == RI_TRAP) {
                    continue;
                }
                auto itemIt = Rando::StaticData::Items.find(randoItemId);
                if (itemIt == Rando::StaticData::Items.end() || itemIt->second.randoItemType == RITYPE_JUNK) {
                    continue;
                }
                sGiveItem(Rando::ConvertItem(randoItemId));
            }
        }
    }
}

// MARK: - Shuffling

struct ShuffleContext {
    std::mt19937 rng;
    // The save with every item the player can ever own already in hand.
    std::unique_ptr<SaveContext> fullInventory;
    // The save as the player starts with it.
    std::unique_ptr<SaveContext> startingInventory;
    std::set<RandoCheckId> baselineChecks;
    SearchResult scratch;
    int validations = 0;
};

static bool Validate(ShuffleContext& ctx) {
    ctx.validations++;
    Search(*ctx.fullInventory, ctx.scratch);
    return std::includes(ctx.scratch.checks.begin(), ctx.scratch.checks.end(), ctx.baselineChecks.begin(),
                         ctx.baselineChecks.end());
}

template <typename T> static void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    // std::shuffle's output differs between standard libraries, and a seed has to mean the same
    // layout on every platform.
    for (size_t i = values.size(); i > 1; i--) {
        size_t j = rng() % i;
        std::swap(values[i - 1], values[j]);
    }
}

static void RemoveOne(std::vector<s16>& values, s16 value) {
    auto it = std::find(values.begin(), values.end(), value);
    if (it != values.end()) {
        values.erase(it);
    }
}

// Shuffles one pool of two-way entrances among themselves. Coupled: a door and the door that comes
// out the other side are linked both ways, so walking back always returns you where you came from.
// Decoupled: every direction is placed independently.
static bool TryShuffleTwoWay(ShuffleContext& ctx, const std::vector<s16>& pool, bool coupled) {
    auto& entrances = GetEntrances();
    std::vector<s16> savedConnections = sConnections;

    std::vector<s16> pending = pool;
    std::vector<s16> targets = pool;
    sAssumed = pool;
    for (s16 entrance : pool) {
        sConnections[entrance] = -1;
        s16 reverse = entrances[entrance].reverse;
        // Pools that only list the way in (interiors, dungeons, grottos) drag the way out along.
        if (coupled && reverse >= 0 && std::find(pool.begin(), pool.end(), reverse) == pool.end()) {
            sConnections[reverse] = -1;
            sAssumed.push_back(reverse);
        }
    }

    ShuffleVector(pending, ctx.rng);
    while (!pending.empty()) {
        s16 entrance = pending.back();
        s16 reverse = entrances[entrance].reverse;

        std::vector<s16> candidates = targets;
        ShuffleVector(candidates, ctx.rng);

        bool placed = false;
        for (s16 target : candidates) {
            // Walking through a door and coming straight back out of it is never interesting.
            if (target == reverse) {
                continue;
            }
            s16 targetReverse = entrances[target].reverse;
            std::vector<s16> assumedBefore = sAssumed;

            sConnections[entrance] = target;
            RemoveOne(sAssumed, target);
            if (coupled && reverse >= 0 && targetReverse >= 0) {
                sConnections[targetReverse] = reverse;
                RemoveOne(sAssumed, reverse);
            }

            if (Validate(ctx)) {
                RemoveOne(targets, target);
                RemoveOne(pending, entrance);
                if (coupled && reverse >= 0 && targetReverse >= 0) {
                    RemoveOne(targets, reverse);
                    RemoveOne(pending, targetReverse);
                }
                placed = true;
                break;
            }

            sConnections[entrance] = -1;
            if (coupled && reverse >= 0 && targetReverse >= 0) {
                sConnections[targetReverse] = -1;
            }
            sAssumed = assumedBefore;
        }

        if (!placed) {
            sConnections = savedConnections;
            sAssumed.clear();
            return false;
        }
    }

    // Every destination has to have been handed to a door. One left over would mean the layout
    // only passed because that destination was still being assumed.
    if (!sAssumed.empty()) {
        sConnections = savedConnections;
        sAssumed.clear();
        return false;
    }
    return true;
}

// Shuffles entrances with no way back (owl warps, the cycle start, boss doors). Their destinations
// are drawn from `candidates`, each used at most once. `permutation` says the candidates are the
// pool's own destinations, each reachable only through whichever door ends up leading to it (boss
// rooms); otherwise they are places that exist regardless (landing spots for a warp).
static bool TryShuffleOneWay(ShuffleContext& ctx, const std::vector<s16>& pool, const std::vector<s16>& candidates,
                             bool permutation, bool requireStartAccessToClockTown) {
    std::vector<s16> savedConnections = sConnections;

    std::vector<s16> pending = pool;
    std::vector<s16> targets = candidates;
    sAssumed = pool;
    for (s16 entrance : pool) {
        sConnections[entrance] = -1;
    }

    ShuffleVector(pending, ctx.rng);
    while (!pending.empty()) {
        s16 entrance = pending.back();
        std::vector<s16> shuffledTargets = targets;
        ShuffleVector(shuffledTargets, ctx.rng);

        bool placed = false;
        for (s16 target : shuffledTargets) {
            std::vector<s16> assumedBefore = sAssumed;
            sConnections[entrance] = target;
            RemoveOne(sAssumed, permutation ? target : entrance);

            bool valid = Validate(ctx);
            if (valid && requireStartAccessToClockTown) {
                // The way back to the other game is the Clock Tower door, and Song of Time now leads
                // to wherever the cycle starts, so that has to connect to South Clock Town unaided.
                std::vector<s16> assumedDuringShuffle = sAssumed;
                sAssumed.clear();
                Search(*ctx.startingInventory, ctx.scratch);
                valid = ctx.scratch.regions.contains(RR_CLOCK_TOWN_SOUTH);
                sAssumed = assumedDuringShuffle;
            }

            if (valid) {
                RemoveOne(targets, target);
                pending.pop_back();
                placed = true;
                break;
            }

            sConnections[entrance] = -1;
            sAssumed = assumedBefore;
        }

        if (!placed) {
            sConnections = savedConnections;
            sAssumed.clear();
            return false;
        }
    }

    sAssumed.clear();
    return true;
}

template <typename F> static void ShuffleWithRetries(const char* poolName, F attempt) {
    constexpr int maxAttempts = 20;
    for (int i = 0; i < maxAttempts; i++) {
        if (attempt()) {
            return;
        }
        SPDLOG_DEBUG("Entrance shuffle: {} attempt {} found no valid placement, retrying", poolName, i + 1);
    }
    throw std::runtime_error(std::string("Entrance shuffle could not place the ") + poolName +
                             " entrances, try different entrance settings");
}

void Shuffle(const std::vector<RandoItemId>& itemPool, ItemGiver giveItem) {
    ClearLayout();
    if (!AnyOptionEnabled()) {
        return;
    }
    sGiveItem = giveItem != nullptr ? giveItem : static_cast<ItemGiver>(Rando::GiveItem);

    auto& entrances = GetEntrances();
    auto original = std::make_unique<SaveContext>();
    memcpy(original.get(), &gSaveContext, sizeof(SaveContext));
    // Item_Give takes its headless path when there is no play state.
    PlayState* savedPlayState = gPlayState;
    gPlayState = NULL;

    ShuffleContext ctx;
    // Its own generator: giving items below reseeds the game's, which would otherwise make every
    // retry replay the same choices.
    ctx.rng.seed(gSaveContext.save.shipSaveInfo.rando.finalSeed ^ 0x454E5452);
    ctx.startingInventory = std::make_unique<SaveContext>();
    memcpy(ctx.startingInventory.get(), &gSaveContext, sizeof(SaveContext));

    for (RandoItemId randoItemId : itemPool) {
        if (randoItemId == RI_UNKNOWN || randoItemId == RI_NONE || randoItemId == RI_TRAP ||
            Rando::StaticData::Items[randoItemId].randoItemType == RITYPE_JUNK) {
            continue;
        }
        sGiveItem(Rando::ConvertItem(randoItemId));
    }
    ctx.fullInventory = std::make_unique<SaveContext>();
    memcpy(ctx.fullInventory.get(), &gSaveContext, sizeof(SaveContext));

    auto restore = [&]() {
        memcpy(&gSaveContext, original.get(), sizeof(SaveContext));
        gPlayState = savedPlayState;
        sShuffling = false;
        sAssumed.clear();
    };

    try {
        sShuffling = true;
        sAssumed.clear();
        Search(*ctx.fullInventory, ctx.scratch);
        ctx.baselineChecks = ctx.scratch.checks;

        u32 dungeons = RANDO_SAVE_OPTIONS[RO_ENTRANCES_DUNGEONS];
        u32 interiors = RANDO_SAVE_OPTIONS[RO_ENTRANCES_INTERIORS];
        bool overworld = RANDO_SAVE_OPTIONS[RO_ENTRANCES_OVERWORLD] != RO_GENERIC_OFF;
        bool grottos = RANDO_SAVE_OPTIONS[RO_ENTRANCES_GROTTOS] != RO_GENERIC_OFF;
        bool decoupled = RANDO_SAVE_OPTIONS[RO_ENTRANCES_DECOUPLE] != RO_GENERIC_OFF;
        bool mixed = RANDO_SAVE_OPTIONS[RO_ENTRANCES_MIXED_POOLS] != RO_GENERIC_OFF;

        bool mixDungeons = mixed && dungeons != RO_ENTRANCES_DUNGEONS_OFF &&
                           RANDO_SAVE_OPTIONS[RO_ENTRANCES_MIX_DUNGEONS] != RO_GENERIC_OFF;
        bool mixInteriors = mixed && interiors != RO_ENTRANCES_INTERIORS_OFF &&
                            RANDO_SAVE_OPTIONS[RO_ENTRANCES_MIX_INTERIORS] != RO_GENERIC_OFF;
        bool mixGrottos = mixed && grottos && RANDO_SAVE_OPTIONS[RO_ENTRANCES_MIX_GROTTOS] != RO_GENERIC_OFF;
        bool mixOverworld = mixed && overworld && RANDO_SAVE_OPTIONS[RO_ENTRANCES_MIX_OVERWORLD] != RO_GENERIC_OFF;
        // A single flagged pool has nothing to mix with.
        if ((int)mixDungeons + (int)mixInteriors + (int)mixGrottos + (int)mixOverworld < 2) {
            mixDungeons = mixInteriors = mixGrottos = mixOverworld = false;
        }
        // On its own the overworld pool holds both directions of every link, and a placement pairs
        // two links up. Mixed with pools that only hold the way in, it has to follow the same
        // convention or links and doors could not pair with each other, so one direction of each
        // link stands for it. (Decoupled, every direction of everything is in the pool anyway.)
        bool overworldOneDirection = mixOverworld && !decoupled;

        // Two-way pools hold the way in; decoupling adds the way out as an entrance of its own.
        auto collect = [&](std::initializer_list<EntranceType> primary, std::initializer_list<EntranceType> reverse,
                           bool includeOptional) {
            std::vector<s16> pool;
            for (size_t i = 0; i < entrances.size(); i++) {
                auto& info = entrances[i];
                if (info.optional && !includeOptional) {
                    continue;
                }
                if (info.type == ET_OVERWORLD && overworldOneDirection && info.reverse < (s16)i) {
                    continue;
                }
                bool isPrimary = std::find(primary.begin(), primary.end(), info.type) != primary.end();
                bool isReverse = std::find(reverse.begin(), reverse.end(), info.type) != reverse.end();
                if (isPrimary || (decoupled && isReverse)) {
                    pool.push_back((s16)i);
                }
            }
            return pool;
        };

        struct NamedPool {
            const char* name;
            std::vector<s16> entrances;
            bool mix;
        };
        std::vector<NamedPool> pools;
        if (dungeons != RO_ENTRANCES_DUNGEONS_OFF) {
            pools.push_back(
                { "dungeon",
                  collect({ ET_DUNGEON }, { ET_DUNGEON_REVERSE }, dungeons == RO_ENTRANCES_DUNGEONS_ON_PLUS_INVERTED),
                  mixDungeons });
        }
        if (interiors != RO_ENTRANCES_INTERIORS_OFF) {
            bool all = interiors == RO_ENTRANCES_INTERIORS_ALL;
            pools.push_back({ "interior",
                              all ? collect({ ET_INTERIOR, ET_SPECIAL_INTERIOR },
                                            { ET_INTERIOR_REVERSE, ET_SPECIAL_INTERIOR_REVERSE }, true)
                                  : collect({ ET_INTERIOR }, { ET_INTERIOR_REVERSE }, true),
                              mixInteriors });
        }
        if (grottos) {
            pools.push_back({ "grotto", collect({ ET_GROTTO }, { ET_GROTTO_REVERSE }, true), mixGrottos });
        }
        if (overworld) {
            pools.push_back({ "overworld", collect({ ET_OVERWORLD }, {}, true), mixOverworld });
        }

        // Pools flagged for mixing are placed as one.
        std::vector<s16> mixedPool;
        for (auto& pool : pools) {
            if (pool.mix) {
                mixedPool.insert(mixedPool.end(), pool.entrances.begin(), pool.entrances.end());
            } else if (pool.entrances.size() > 1) {
                ShuffleWithRetries(pool.name, [&]() { return TryShuffleTwoWay(ctx, pool.entrances, !decoupled); });
            }
        }
        if (mixedPool.size() > 1) {
            ShuffleWithRetries("mixed", [&]() { return TryShuffleTwoWay(ctx, mixedPool, !decoupled); });
        }

        if (RANDO_SAVE_OPTIONS[RO_ENTRANCES_BOSSES] != RO_GENERIC_OFF) {
            std::vector<s16> bosses;
            for (size_t i = 0; i < entrances.size(); i++) {
                if (entrances[i].type == ET_BOSS) {
                    bosses.push_back((s16)i);
                }
            }
            ShuffleWithRetries("boss", [&]() { return TryShuffleOneWay(ctx, bosses, bosses, true, false); });
        }

        // One-ways go last so they are checked against the finished map. They can land at any
        // overworld arrival point or owl statue.
        bool owlWarps = RANDO_SAVE_OPTIONS[RO_ENTRANCES_OWL_WARPS] != RO_GENERIC_OFF;
        bool spawn = RANDO_SAVE_OPTIONS[RO_ENTRANCES_SPAWN] != RO_GENERIC_OFF;
        if (owlWarps || spawn) {
            std::vector<s16> landings;
            std::vector<s16> owlPool;
            std::vector<s16> spawnPool;
            for (size_t i = 0; i < entrances.size(); i++) {
                auto type = entrances[i].type;
                if (type == ET_OVERWORLD || type == ET_OWL_WARP || type == ET_SPAWN) {
                    landings.push_back((s16)i);
                }
                if (type == ET_OWL_WARP) {
                    owlPool.push_back((s16)i);
                } else if (type == ET_SPAWN) {
                    spawnPool.push_back((s16)i);
                }
            }
            if (spawn) {
#ifdef COMBO_BUILD
                bool requireClockTown = true;
#else
                bool requireClockTown = false;
#endif
                ShuffleWithRetries(
                    "spawn", [&]() { return TryShuffleOneWay(ctx, spawnPool, landings, false, requireClockTown); });
            }
            if (owlWarps) {
                ShuffleWithRetries("owl warp",
                                   [&]() { return TryShuffleOneWay(ctx, owlPool, landings, false, false); });
            }
        }

        if (!Validate(ctx)) {
            throw std::runtime_error("Entrance shuffle produced a layout that cuts off part of the world");
        }
    } catch (...) {
        restore();
        ClearLayout();
        throw;
    }

    restore();
    RefreshShuffled();

    int shuffledCount = 0;
    for (size_t i = 0; i < sConnections.size(); i++) {
        shuffledCount += sConnections[i] != (s16)i ? 1 : 0;
    }
    SPDLOG_INFO("Entrance shuffle: {} entrances moved, {} layouts tested", shuffledCount, ctx.validations);
}

// MARK: - Persistence

static u16 PackEntrance(s32 entrance) {
    return entrance == ONE_WAY_EXIT ? 0xFFFF : (u16)entrance;
}

static s32 UnpackEntrance(u16 entrance) {
    return entrance == 0xFFFF ? ONE_WAY_EXIT : (s32)entrance;
}

// Slot of each shuffled entrance in the save's override array, which is what the discovered bits
// are indexed by.
static std::unordered_map<s16, u16> sOverrideSlots;

void WriteLayoutToSave() {
    EnsureState();
    auto& entrances = GetEntrances();
    auto& rando = gSaveContext.save.shipSaveInfo.rando;

    rando.entranceOverrideCount = 0;
    memset(rando.entranceOverrides, 0, sizeof(rando.entranceOverrides));
    memset(rando.entranceDiscovered, 0, sizeof(rando.entranceDiscovered));
    sOverrideSlots.clear();

    for (size_t i = 0; i < sConnections.size(); i++) {
        s16 connection = sConnections[i];
        if (connection == (s16)i || connection < 0) {
            continue;
        }
        if (rando.entranceOverrideCount >= RANDO_ENTRANCE_OVERRIDES_MAX) {
            SPDLOG_ERROR("Entrance shuffle: more shuffled entrances than the save can hold");
            break;
        }
        u16 slot = rando.entranceOverrideCount++;
        rando.entranceOverrides[slot] = { PackEntrance(entrances[i].to), PackEntrance(entrances[i].from),
                                          PackEntrance(entrances[connection].to),
                                          PackEntrance(entrances[connection].from) };
        sOverrideSlots[(s16)i] = slot;
        if (sDiscovered[i]) {
            rando.entranceDiscovered[slot / 8] |= (1 << (slot % 8));
        }
    }
}

void ReadLayoutFromSave() {
    ClearLayout();
    sOverrideSlots.clear();
    if (!IS_RANDO) {
        return;
    }

    auto& rando = gSaveContext.save.shipSaveInfo.rando;
    u16 count = std::min<u16>(rando.entranceOverrideCount, RANDO_ENTRANCE_OVERRIDES_MAX);
    for (u16 slot = 0; slot < count; slot++) {
        auto& entry = rando.entranceOverrides[slot];
        s16 source = FindEntrance(UnpackEntrance(entry.fromTo), UnpackEntrance(entry.fromReturn));
        s16 target = FindEntrance(UnpackEntrance(entry.toTo), UnpackEntrance(entry.toReturn));
        if (source < 0 || target < 0) {
            SPDLOG_WARN("Entrance shuffle: save has an override for an entrance the logic no longer knows");
            continue;
        }
        sConnections[source] = target;
        sOverrideSlots[source] = slot;
        sDiscovered[source] = (rando.entranceDiscovered[slot / 8] >> (slot % 8)) & 1;
    }
    RefreshShuffled();
}

nlohmann::json LayoutToJson() {
    EnsureState();
    auto& entrances = GetEntrances();
    nlohmann::json json = nlohmann::json::array();
    for (size_t i = 0; i < sConnections.size(); i++) {
        s16 connection = sConnections[i];
        if (connection == (s16)i || connection < 0) {
            continue;
        }
        // The ids are what gets read back. The names are there for people reading the spoiler.
        json.push_back({
            { "from", entrances[i].source + " -> " + entrances[i].destination },
            { "to", GetArrivalName(connection) },
            { "type", GetTypeName(entrances[i].type) },
            { "ids", { entrances[i].to, entrances[i].from, entrances[connection].to, entrances[connection].from } },
        });
    }
    return json;
}

void LayoutFromJson(const nlohmann::json& json) {
    ClearLayout();
    if (!json.is_array()) {
        return;
    }
    for (auto& entry : json) {
        if (!entry.is_object() || !entry.contains("ids") || !entry["ids"].is_array() || entry["ids"].size() != 4) {
            throw std::runtime_error("Malformed entrance in spoiler");
        }
        s16 source = FindEntrance(entry["ids"][0].get<s32>(), entry["ids"][1].get<s32>());
        s16 target = FindEntrance(entry["ids"][2].get<s32>(), entry["ids"][3].get<s32>());
        if (source < 0 || target < 0) {
            throw std::runtime_error("Spoiler has an entrance this version does not know: " +
                                     entry.value("from", std::string("?")));
        }
        sConnections[source] = target;
    }
    RefreshShuffled();
}

// MARK: - Discovery

bool IsDiscovered(s16 index) {
    EnsureState();
    return index >= 0 && index < (s16)sDiscovered.size() && sDiscovered[index];
}

void SetDiscovered(s16 index) {
    EnsureState();
    if (index < 0 || index >= (s16)sDiscovered.size() || sDiscovered[index]) {
        return;
    }
    sDiscovered[index] = true;
    auto slot = sOverrideSlots.find(index);
    if (slot != sOverrideSlots.end()) {
        gSaveContext.save.shipSaveInfo.rando.entranceDiscovered[slot->second / 8] |= (1 << (slot->second % 8));
    }
}

} // namespace Entrance

} // namespace Rando
