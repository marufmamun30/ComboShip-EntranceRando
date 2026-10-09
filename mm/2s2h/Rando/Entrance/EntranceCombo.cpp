#ifdef COMBO_BUILD
// ComboShip: the combined generator never runs MM's own OnFileCreate, so MM's entrance shuffle is
// driven from here instead. The layout lives in the entrance module (not in gSaveContext), so the
// reachability oracle sees it through every Reset, MM_InitRandoSaveFile bakes it into the new save,
// and the launcher records it in the consolidated spoiler to replay on reload.
#include "Entrance.h"

#include "ComboExport.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <memory>
#include <spdlog/spdlog.h>

extern "C" {
#include "variables.h"
#include "ShipUtils.h"
}

// BenPort.cpp: the save-only item grant the reachability oracle uses. Rando::GiveItem reaches into
// parts of the game that the headless generator never initializes.
void Combo_MM_GiveItemForOracle(RandoItemId randoItemId);

// Shuffles MM's entrances for `seed` with the entrance options currently set in the menu.
// Returns 1 on success (including "nothing to shuffle"), 0 when no valid layout was found.
extern "C" COMBO_EXPORT int MM_ShuffleEntrancesForCombo(uint64_t seed) {
    // Development aid: check the entrance tables against the logic once per run.
    static bool selfTested = false;
    if (!selfTested && getenv("COMBO_ENTRANCE_SELFTEST") != nullptr) {
        selfTested = true;
        Rando::Entrance::SelfTest();
    }

    bool anyEnabled = false;
    for (RandoOptionId optionId :
         { RO_ENTRANCES_DUNGEONS, RO_ENTRANCES_BOSSES, RO_ENTRANCES_OVERWORLD, RO_ENTRANCES_INTERIORS,
           RO_ENTRANCES_GROTTOS, RO_ENTRANCES_OWL_WARPS, RO_ENTRANCES_SPAWN }) {
        auto& option = Rando::StaticData::Options[optionId];
        anyEnabled = anyEnabled || CVarGetInteger(option.cvar, option.defaultValue) != 0;
    }
    if (!anyEnabled) {
        Rando::Entrance::ClearLayout();
        return 1;
    }

    auto saved = std::make_unique<SaveContext>();
    memcpy(saved.get(), &gSaveContext, sizeof(SaveContext));
    PlayState* savedPlayState = gPlayState;
    int result = 1;

    try {
        // The same footing native generation shuffles on: a fresh rando save holding the chosen
        // options and starting items.
        memset(&gSaveContext, 0, sizeof(SaveContext));
        memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
               sizeof(gSaveContext.save.saveInfo.inventory.items));
        gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
        gSaveContext.save.shipSaveInfo.rando.finalSeed = (u32)seed;
        for (auto& [optionId, option] : Rando::StaticData::Options) {
            RANDO_SAVE_OPTIONS[optionId] = (uint32_t)CVarGetInteger(option.cvar, option.defaultValue);
        }

        auto startingItems = Rando::GetStartingItemsFromConfig();
        Rando::SetStartingItemsInSave(gSaveContext.save.shipSaveInfo.rando, startingItems);

        std::vector<RandoCheckId> checkPool;
        std::vector<RandoItemId> itemPool;
        Ship_Random_Seed((u32)seed);
        Rando::Logic::GeneratePools(gSaveContext.save.shipSaveInfo.rando, checkPool, itemPool);

        gPlayState = NULL;
        auto computedItems = Rando::GetComputedStartingItems(gSaveContext.save.shipSaveInfo.rando);
        startingItems.insert(startingItems.end(), computedItems.begin(), computedItems.end());
        for (RandoItemId startingItem : startingItems) {
            Combo_MM_GiveItemForOracle(Rando::ConvertItem(startingItem));
        }

        Rando::Entrance::Shuffle(itemPool, Combo_MM_GiveItemForOracle);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] MM entrance shuffle failed: {}", e.what());
        Rando::Entrance::ClearLayout();
        result = 0;
    }

    memcpy(&gSaveContext, saved.get(), sizeof(SaveContext));
    gPlayState = savedPlayState;
    // Pool generation and the layout checks both drew from the game's generator.
    Ship_Random_Seed((u32)seed);
    return result;
}

// The current layout as the JSON array the spoiler stores.
extern "C" COMBO_EXPORT const char* MM_DumpEntranceLayout(void) {
    static std::string buffer;
    buffer = Rando::Entrance::LayoutToJson().dump();
    return buffer.c_str();
}

// Installs a layout recorded by MM_DumpEntranceLayout. Returns 1 on success.
extern "C" COMBO_EXPORT int MM_ApplyEntranceLayoutForCombo(const char* json) {
    try {
        Rando::Entrance::LayoutFromJson(json != nullptr && json[0] != '\0' ? nlohmann::json::parse(json)
                                                                           : nlohmann::json::array());
        return 1;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] MM entrance layout could not be applied: {}", e.what());
        Rando::Entrance::ClearLayout();
        return 0;
    }
}
#endif // COMBO_BUILD
