#include "Entrance.h"

#include <map>
#include <set>
#include <unordered_map>

extern "C" {
#include "z64scene.h"
}

namespace Rando {

namespace Entrance {

#define SCENE_OF(entrance) (((entrance) >> 9) & 0x7F)

// clang-format off

// Two-way doors are listed once, by the entrance that leads inside (or, for overworld links, by
// either direction). The other direction is found through the region graph.
static const std::set<s32> sDungeonEntrances = {
    ENTRANCE(WOODFALL_TEMPLE, 0),
    ENTRANCE(SNOWHEAD_TEMPLE, 0),
    ENTRANCE(GREAT_BAY_TEMPLE, 0),
    ENTRANCE(STONE_TOWER_TEMPLE, 0),
    ENTRANCE(STONE_TOWER_TEMPLE_INVERTED, 0),
};

// Boss rooms are one-way in MM: the only way out is the blue warp, which stays with its boss.
static const std::unordered_map<s32, const char*> sBossDoorLabels = {
    { ENTRANCE(ODOLWAS_LAIR, 0), "Woodfall Temple Boss Door" },
    { ENTRANCE(GOHTS_LAIR, 0), "Snowhead Temple Boss Door" },
    { ENTRANCE(GYORGS_LAIR, 0), "Great Bay Temple Boss Door" },
    { ENTRANCE(TWINMOLDS_LAIR, 0), "Stone Tower Temple Boss Door" },
};

// Single-door interiors with nothing behind them.
static const std::set<s32> sSimpleInteriors = {
    ENTRANCE(TREASURE_CHEST_SHOP, 0),
    ENTRANCE(HONEY_AND_DARLINGS_SHOP, 0),
    ENTRANCE(MAYORS_RESIDENCE, 0),
    ENTRANCE(TOWN_SHOOTING_GALLERY, 0),
    ENTRANCE(MILK_BAR, 0),
    ENTRANCE(FAIRY_FOUNTAIN, 0),
    ENTRANCE(FAIRY_FOUNTAIN, 1),
    ENTRANCE(FAIRY_FOUNTAIN, 2),
    ENTRANCE(FAIRY_FOUNTAIN, 3),
    ENTRANCE(FAIRY_FOUNTAIN, 4),
    ENTRANCE(SWORDMANS_SCHOOL, 0),
    ENTRANCE(TRADING_POST, 0),
    ENTRANCE(BOMB_SHOP, 0),
    ENTRANCE(POST_OFFICE, 0),
    ENTRANCE(LOTTERY_SHOP, 0),
    ENTRANCE(DEKU_SCRUB_PLAYGROUND, 0),
    ENTRANCE(SWAMP_SHOOTING_GALLERY, 0),
    ENTRANCE(TOURIST_INFORMATION, 0),
    ENTRANCE(MAGIC_HAGS_POTION_SHOP, 0),
    ENTRANCE(CUCCO_SHACK, 0),
    ENTRANCE(DOGGY_RACETRACK, 0),
    ENTRANCE(RANCH_HOUSE, 0),
    ENTRANCE(RANCH_HOUSE, 1),
    ENTRANCE(MOUNTAIN_SMITHY, 0),
    ENTRANCE(GORON_GRAVERYARD, 0),
    ENTRANCE(GORON_SHOP, 0),
    ENTRANCE(FISHERMANS_HUT, 0),
    ENTRANCE(MARINE_RESEARCH_LAB, 0),
    ENTRANCE(ZORA_HALL_ROOMS, 0),
    ENTRANCE(ZORA_HALL_ROOMS, 1),
    ENTRANCE(ZORA_HALL_ROOMS, 2),
    ENTRANCE(ZORA_HALL_ROOMS, 3),
    ENTRANCE(ZORA_HALL_ROOMS, 5),
    ENTRANCE(GHOST_HUT, 0),
    ENTRANCE(MUSIC_BOX_HOUSE, 0),
};

// Interiors with more than one door, or that are small areas in their own right. Only shuffled on
// the "All" setting, like SoH's special interiors.
static const std::set<s32> sSpecialInteriors = {
    ENTRANCE(STOCK_POT_INN, 0),
    ENTRANCE(STOCK_POT_INN, 1),
    ENTRANCE(CURIOSITY_SHOP, 0),
    ENTRANCE(CURIOSITY_SHOP, 1),
    ENTRANCE(ASTRAL_OBSERVATORY, 0),
    ENTRANCE(ASTRAL_OBSERVATORY, 1),
    ENTRANCE(GORON_SHRINE, 0),
    ENTRANCE(SECRET_SHRINE, 0),
    ENTRANCE(SWAMP_SPIDER_HOUSE, 0),
    ENTRANCE(OCEANSIDE_SPIDER_HOUSE, 0),
    ENTRANCE(DEKU_SHRINE, 0),
    ENTRANCE(BENEATH_THE_GRAVERYARD, 0),
    ENTRANCE(BENEATH_THE_GRAVERYARD, 1),
};

// Links between areas. Both directions of each are entrances of this type.
static const std::set<s32> sOverworldEntrances = {
    ENTRANCE(TERMINA_FIELD, 0),            ENTRANCE(WEST_CLOCK_TOWN, 0),
    ENTRANCE(TERMINA_FIELD, 6),            ENTRANCE(SOUTH_CLOCK_TOWN, 1),
    ENTRANCE(TERMINA_FIELD, 7),            ENTRANCE(EAST_CLOCK_TOWN, 0),
    ENTRANCE(TERMINA_FIELD, 8),            ENTRANCE(NORTH_CLOCK_TOWN, 0),
    ENTRANCE(SOUTH_CLOCK_TOWN, 7),         ENTRANCE(EAST_CLOCK_TOWN, 1),
    ENTRANCE(SOUTH_CLOCK_TOWN, 2),         ENTRANCE(EAST_CLOCK_TOWN, 3),
    ENTRANCE(SOUTH_CLOCK_TOWN, 4),         ENTRANCE(NORTH_CLOCK_TOWN, 2),
    ENTRANCE(SOUTH_CLOCK_TOWN, 5),         ENTRANCE(WEST_CLOCK_TOWN, 1),
    ENTRANCE(SOUTH_CLOCK_TOWN, 3),         ENTRANCE(WEST_CLOCK_TOWN, 2),
    ENTRANCE(SOUTH_CLOCK_TOWN, 6),         ENTRANCE(LAUNDRY_POOL, 0),
    ENTRANCE(NORTH_CLOCK_TOWN, 1),         ENTRANCE(EAST_CLOCK_TOWN, 5),
    ENTRANCE(TERMINA_FIELD, 1),            ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0),
    ENTRANCE(TERMINA_FIELD, 2),            ENTRANCE(GREAT_BAY_COAST, 0),
    ENTRANCE(TERMINA_FIELD, 3),            ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 0),
    ENTRANCE(TERMINA_FIELD, 4),            ENTRANCE(ROAD_TO_IKANA, 0),
    ENTRANCE(TERMINA_FIELD, 5),            ENTRANCE(MILK_ROAD, 0),
    ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 1),   ENTRANCE(SOUTHERN_SWAMP_POISONED, 0),
    ENTRANCE(SOUTHERN_SWAMP_POISONED, 7),  ENTRANCE(WOODS_OF_MYSTERY, 0),
    ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),  ENTRANCE(DEKU_PALACE, 0),
    ENTRANCE(SOUTHERN_SWAMP_POISONED, 4),  ENTRANCE(DEKU_PALACE, 5),
    ENTRANCE(SOUTHERN_SWAMP_POISONED, 2),  ENTRANCE(WOODFALL, 0),
    ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 1), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 6),
    ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 2),  ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0),
    ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 1), ENTRANCE(GORON_VILLAGE_WINTER, 0),
    ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 2), ENTRANCE(GORON_RACETRACK, 0),
    ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 4),  ENTRANCE(PATH_TO_SNOWHEAD, 0),
    ENTRANCE(PATH_TO_SNOWHEAD, 1),         ENTRANCE(SNOWHEAD, 0),
    ENTRANCE(MILK_ROAD, 1),                ENTRANCE(ROMANI_RANCH, 0),
    ENTRANCE(MILK_ROAD, 3),                ENTRANCE(GORMAN_TRACK, 0),
    ENTRANCE(MILK_ROAD, 2),                ENTRANCE(GORMAN_TRACK, 3),
    ENTRANCE(GREAT_BAY_COAST, 1),          ENTRANCE(ZORA_CAPE, 0),
    ENTRANCE(GREAT_BAY_COAST, 3),          ENTRANCE(PINNACLE_ROCK, 0),
    ENTRANCE(GREAT_BAY_COAST, 5),          ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0),
    ENTRANCE(ZORA_CAPE, 1),                ENTRANCE(ZORA_HALL, 0),
    ENTRANCE(ZORA_CAPE, 2),                ENTRANCE(ZORA_HALL, 1),
    ENTRANCE(ZORA_CAPE, 4),                ENTRANCE(WATERFALL_RAPIDS, 0),
    ENTRANCE(ROAD_TO_IKANA, 1),            ENTRANCE(IKANA_CANYON, 0),
    ENTRANCE(ROAD_TO_IKANA, 2),            ENTRANCE(IKANA_GRAVEYARD, 0),
    ENTRANCE(IKANA_CANYON, 3),             ENTRANCE(STONE_TOWER, 0),
    ENTRANCE(IKANA_CANYON, 5),             ENTRANCE(BENEATH_THE_WELL, 0),
    ENTRANCE(IKANA_CANYON, 8),             ENTRANCE(IKANA_CASTLE, 1),
    ENTRANCE(BENEATH_THE_WELL, 1),         ENTRANCE(IKANA_CASTLE, 0),
};

static const std::set<s32> sOwlWarpEntrances = {
    ENTRANCE(GREAT_BAY_COAST, 11),
    ENTRANCE(ZORA_CAPE, 6),
    ENTRANCE(SNOWHEAD, 3),
    ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8),
    ENTRANCE(SOUTH_CLOCK_TOWN, 9),
    ENTRANCE(MILK_ROAD, 4),
    ENTRANCE(WOODFALL, 4),
    ENTRANCE(SOUTHERN_SWAMP_POISONED, 10),
    ENTRANCE(IKANA_CANYON, 4),
    ENTRANCE(STONE_TOWER, 3),
};

// Positions come from the DoorAna actors in each scene's room data. The generic chest grotto
// (GROTTOS, 4) and cow grotto (GROTTOS, 10) are shared by many holes; the logic gives each of those
// its own id (17 and up, see Logic.cpp) and the hole's item byte picks the chest.
#define HOLE(logicId, realId, data, scene, entrScene, room, x, y, z, yaw) \
    { ENTRANCE(GROTTOS, logicId), ENTRANCE(GROTTOS, realId), data, scene, ENTRANCE(entrScene, 0), room, x, y, z, yaw }
static const std::vector<GrottoHole> sGrottoHoles = {
    HOLE(0,  0,  0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,  -2782,    48, -1654, 329),
    HOLE(1,  1,  0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,  -1592,  -222,  4622, 244),
    HOLE(2,  2,  0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,   4450,   254,   925, 211),
    HOLE(3,  3,  0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,    192,    48, -3138, 298),
    HOLE(7,  7,  0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,  -2425,  -281, -3291,  58),
    HOLE(9,  9,  0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,   3223,   219,  1417, 253),
    HOLE(11, 11, 0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,  -5159,  -281,  -571,   0),
    HOLE(13, 13, 0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,  -2317,  -221,  3418, 119),
    HOLE(17, 10, 0x1F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,   -375,  -222,  3976, 180),
    HOLE(18, 4,  0x9A, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,   2367,   315,  -192,   0),
    HOLE(19, 4,  0x3F, SCENE_00KEIKOKU,         TERMINA_FIELD,                 0,   1012,  -221,  3642, 239),
    HOLE(20, 4,  0x3E, SCENE_24KEMONOMITI,      ROAD_TO_SOUTHERN_SWAMP,        0,    104,  -182,  2202,  23),
    HOLE(21, 4,  0x3D, SCENE_20SICHITAI,        SOUTHERN_SWAMP_POISONED,       1,  -1700,    38,  1800, 330),
    HOLE(22, 4,  0x5C, SCENE_26SARUNOMORI,      WOODS_OF_MYSTERY,              2,      2,     0,  -889,   0),
    HOLE(23, 4,  0x3B, SCENE_10YUKIYAMANOMURA,  MOUNTAIN_VILLAGE_WINTER,       1,   2406,  1168, -1197, 303),
    HOLE(5,  5,  0xFF, SCENE_17SETUGEN,         PATH_TO_GORON_VILLAGE_WINTER,  0,    589,   195,    53, 240),
    HOLE(24, 4,  0x99, SCENE_17SETUGEN,         PATH_TO_GORON_VILLAGE_WINTER,  0,  -1309,   320,   143,  68),
    HOLE(25, 4,  0x33, SCENE_14YUKIDAMANOMITI,  PATH_TO_SNOWHEAD,              0,   -987,   360, -2339, 253),
    HOLE(26, 4,  0x37, SCENE_30GYOSON,          GREAT_BAY_COAST,               0,   1359,    80,  5018, 298),
    HOLE(27, 10, 0xFF, SCENE_30GYOSON,          GREAT_BAY_COAST,               0,   2077,   333,  -215, 285),
    HOLE(28, 4,  0x95, SCENE_31MISAKI,          ZORA_CAPE,                     0,   -562,    80,  2707, 283),
    HOLE(29, 4,  0xB4, SCENE_IKANA,             IKANA_CANYON,                  2,  -2475,  -505,  2475, 230),
    HOLE(30, 4,  0xB8, SCENE_BOTI,              IKANA_GRAVEYARD,               1,    106,   314, -1777, 205),
    HOLE(31, 4,  0x96, SCENE_IKANAMAE,          ROAD_TO_IKANA,                 0,   -428,   200,  -335, 253),
};
#undef HOLE

// Names for spawns whose scene name alone would not tell two doors apart.
static const std::unordered_map<s32, const char*> sSpawnLabels = {
    { ENTRANCE(EAST_CLOCK_TOWN, 1), "East Clock Town (Lower South Gate)" },
    { ENTRANCE(SOUTH_CLOCK_TOWN, 7), "South Clock Town (Lower East Gate)" },
    { ENTRANCE(EAST_CLOCK_TOWN, 3), "East Clock Town (Upper South Gate)" },
    { ENTRANCE(SOUTH_CLOCK_TOWN, 2), "South Clock Town (Upper East Gate)" },
    { ENTRANCE(WEST_CLOCK_TOWN, 1), "West Clock Town (Lower South Gate)" },
    { ENTRANCE(SOUTH_CLOCK_TOWN, 5), "South Clock Town (Lower West Gate)" },
    { ENTRANCE(WEST_CLOCK_TOWN, 2), "West Clock Town (Upper South Gate)" },
    { ENTRANCE(SOUTH_CLOCK_TOWN, 3), "South Clock Town (Upper West Gate)" },
    { ENTRANCE(STOCK_POT_INN, 0), "Stock Pot Inn (Front Door)" },
    { ENTRANCE(STOCK_POT_INN, 1), "Stock Pot Inn (Balcony)" },
    { ENTRANCE(EAST_CLOCK_TOWN, 9), "East Clock Town (Inn Front Door)" },
    { ENTRANCE(EAST_CLOCK_TOWN, 10), "East Clock Town (Inn Balcony)" },
    { ENTRANCE(CURIOSITY_SHOP, 0), "Curiosity Shop (Front)" },
    { ENTRANCE(CURIOSITY_SHOP, 1), "Curiosity Shop (Back Room)" },
    { ENTRANCE(ASTRAL_OBSERVATORY, 0), "Astral Observatory (Passage)" },
    { ENTRANCE(ASTRAL_OBSERVATORY, 1), "Astral Observatory (Telescope)" },
    { ENTRANCE(TERMINA_FIELD, 9), "Termina Field (Observatory Fence)" },
    { ENTRANCE(FAIRY_FOUNTAIN, 0), "Clock Town Fairy Fountain" },
    { ENTRANCE(FAIRY_FOUNTAIN, 1), "Woodfall Fairy Fountain" },
    { ENTRANCE(FAIRY_FOUNTAIN, 2), "Snowhead Fairy Fountain" },
    { ENTRANCE(FAIRY_FOUNTAIN, 3), "Great Bay Fairy Fountain" },
    { ENTRANCE(FAIRY_FOUNTAIN, 4), "Ikana Fairy Fountain" },
    { ENTRANCE(RANCH_HOUSE, 0), "Ranch Barn" },
    { ENTRANCE(RANCH_HOUSE, 1), "Ranch House" },
    { ENTRANCE(ROMANI_RANCH, 2), "Romani Ranch (Barn)" },
    { ENTRANCE(ROMANI_RANCH, 3), "Romani Ranch (House)" },
    { ENTRANCE(GORMAN_TRACK, 0), "Gorman Track (Front)" },
    { ENTRANCE(GORMAN_TRACK, 3), "Gorman Track (Back)" },
    { ENTRANCE(MILK_ROAD, 3), "Milk Road (Gorman Track)" },
    { ENTRANCE(MILK_ROAD, 2), "Milk Road (Behind Fence)" },
    { ENTRANCE(ZORA_HALL_ROOMS, 0), "Mikau's Room" },
    { ENTRANCE(ZORA_HALL_ROOMS, 1), "Japas' Room" },
    { ENTRANCE(ZORA_HALL_ROOMS, 2), "Lulu's Room" },
    { ENTRANCE(ZORA_HALL_ROOMS, 3), "Evan's Room" },
    { ENTRANCE(ZORA_HALL_ROOMS, 5), "Zora Hall Shop" },
    { ENTRANCE(ZORA_HALL, 6), "Zora Hall (Mikau's Door)" },
    { ENTRANCE(ZORA_HALL, 5), "Zora Hall (Japas' Door)" },
    { ENTRANCE(ZORA_HALL, 3), "Zora Hall (Lulu's Door)" },
    { ENTRANCE(ZORA_HALL, 4), "Zora Hall (Evan's Door)" },
    { ENTRANCE(ZORA_HALL, 2), "Zora Hall (Shop Door)" },
    { ENTRANCE(ZORA_HALL, 0), "Zora Hall (Water Entrance)" },
    { ENTRANCE(ZORA_HALL, 1), "Zora Hall (Land Entrance)" },
    { ENTRANCE(ZORA_CAPE, 1), "Zora Cape (Hall Water Entrance)" },
    { ENTRANCE(ZORA_CAPE, 2), "Zora Cape (Hall Land Entrance)" },
    { ENTRANCE(DEKU_PALACE, 0), "Deku Palace (Front)" },
    { ENTRANCE(DEKU_PALACE, 5), "Deku Palace (Upper Ledge)" },
    { ENTRANCE(SOUTHERN_SWAMP_POISONED, 3), "Southern Swamp (Palace Front)" },
    { ENTRANCE(SOUTHERN_SWAMP_POISONED, 4), "Southern Swamp (Palace Shortcut)" },
    { ENTRANCE(BENEATH_THE_GRAVERYARD, 0), "Beneath the Graveyard (Night 2)" },
    { ENTRANCE(BENEATH_THE_GRAVERYARD, 1), "Beneath the Graveyard (Night 1)" },
    { ENTRANCE(IKANA_GRAVEYARD, 2), "Ikana Graveyard (Night 2 Grave)" },
    { ENTRANCE(IKANA_GRAVEYARD, 3), "Ikana Graveyard (Night 1 Grave)" },
    { ENTRANCE(IKANA_CASTLE, 0), "Ikana Castle (Courtyard Well)" },
    { ENTRANCE(IKANA_CASTLE, 1), "Ikana Castle (Front Gate)" },
    { ENTRANCE(BENEATH_THE_WELL, 0), "Beneath the Well (Canyon Side)" },
    { ENTRANCE(BENEATH_THE_WELL, 1), "Beneath the Well (Castle Side)" },
    { ENTRANCE(GROTTOS, 0), "Termina Field Gossip Stone Grotto #3" },
    { ENTRANCE(GROTTOS, 1), "Termina Field Gossip Stone Grotto #1" },
    { ENTRANCE(GROTTOS, 2), "Termina Field Gossip Stone Grotto #4" },
    { ENTRANCE(GROTTOS, 3), "Termina Field Gossip Stone Grotto #2" },
    { ENTRANCE(GROTTOS, 5), "Hot Spring Water Grotto" },
    { ENTRANCE(GROTTOS, 7), "Termina Field Dodongo Grotto" },
    { ENTRANCE(GROTTOS, 9), "Termina Field Scrub Grotto" },
    { ENTRANCE(GROTTOS, 11), "Termina Field Bio Baba Grotto" },
    { ENTRANCE(GROTTOS, 12), "Bean Seller Grotto" },
    { ENTRANCE(GROTTOS, 13), "Termina Field Peahat Grotto" },
    { ENTRANCE(GROTTOS, 16), "Lone Peak Shrine" },
    { ENTRANCE(GROTTOS, 17), "Termina Field Cow Grotto" },
    { ENTRANCE(GROTTOS, 18), "Termina Field Pillar Grotto" },
    { ENTRANCE(GROTTOS, 19), "Termina Field Tall Grass Grotto" },
    { ENTRANCE(GROTTOS, 20), "Road to Southern Swamp Grotto" },
    { ENTRANCE(GROTTOS, 21), "Southern Swamp Grotto" },
    { ENTRANCE(GROTTOS, 22), "Woods of Mystery Grotto" },
    { ENTRANCE(GROTTOS, 23), "Mountain Village Tunnel Grotto" },
    { ENTRANCE(GROTTOS, 24), "Path to Goron Village Ramp Grotto" },
    { ENTRANCE(GROTTOS, 25), "Path to Snowhead Grotto" },
    { ENTRANCE(GROTTOS, 26), "Great Bay Coast Fisherman Grotto" },
    { ENTRANCE(GROTTOS, 27), "Great Bay Coast Cow Grotto" },
    { ENTRANCE(GROTTOS, 28), "Zora Cape Grotto" },
    { ENTRANCE(GROTTOS, 29), "Ikana Canyon Grotto" },
    { ENTRANCE(GROTTOS, 30), "Ikana Graveyard Grotto" },
    { ENTRANCE(GROTTOS, 31), "Road to Ikana Grotto" },
    { ENTRANCE(GREAT_BAY_COAST, 11), "Great Bay Coast Owl Statue" },
    { ENTRANCE(ZORA_CAPE, 6), "Zora Cape Owl Statue" },
    { ENTRANCE(SNOWHEAD, 3), "Snowhead Owl Statue" },
    { ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8), "Mountain Village Owl Statue" },
    { ENTRANCE(SOUTH_CLOCK_TOWN, 9), "Clock Town Owl Statue" },
    { ENTRANCE(MILK_ROAD, 4), "Milk Road Owl Statue" },
    { ENTRANCE(WOODFALL, 4), "Woodfall Owl Statue" },
    { ENTRANCE(SOUTHERN_SWAMP_POISONED, 10), "Southern Swamp Owl Statue" },
    { ENTRANCE(IKANA_CANYON, 4), "Ikana Canyon Owl Statue" },
    { ENTRANCE(STONE_TOWER, 3), "Stone Tower Owl Statue" },
};

struct SceneLabel {
    const char* name;
    EntranceGroup group;
};

static const std::unordered_map<s32, SceneLabel> sSceneLabels = {
    { ENTR_SCENE_SOUTH_CLOCK_TOWN,             { "South Clock Town", EG_CLOCK_TOWN } },
    { ENTR_SCENE_EAST_CLOCK_TOWN,              { "East Clock Town", EG_CLOCK_TOWN } },
    { ENTR_SCENE_WEST_CLOCK_TOWN,              { "West Clock Town", EG_CLOCK_TOWN } },
    { ENTR_SCENE_NORTH_CLOCK_TOWN,             { "North Clock Town", EG_CLOCK_TOWN } },
    { ENTR_SCENE_LAUNDRY_POOL,                 { "Laundry Pool", EG_CLOCK_TOWN } },
    { ENTR_SCENE_CLOCK_TOWER_INTERIOR,         { "Clock Tower Interior", EG_CLOCK_TOWN } },
    { ENTR_SCENE_CLOCK_TOWER_ROOFTOP,          { "Clock Tower Rooftop", EG_CLOCK_TOWN } },
    { ENTR_SCENE_TREASURE_CHEST_SHOP,          { "Treasure Chest Shop", EG_CLOCK_TOWN } },
    { ENTR_SCENE_HONEY_AND_DARLINGS_SHOP,      { "Honey & Darling's Shop", EG_CLOCK_TOWN } },
    { ENTR_SCENE_MAYORS_RESIDENCE,             { "Mayor's Residence", EG_CLOCK_TOWN } },
    { ENTR_SCENE_TOWN_SHOOTING_GALLERY,        { "Town Shooting Gallery", EG_CLOCK_TOWN } },
    { ENTR_SCENE_STOCK_POT_INN,                { "Stock Pot Inn", EG_CLOCK_TOWN } },
    { ENTR_SCENE_MILK_BAR,                     { "Milk Bar", EG_CLOCK_TOWN } },
    { ENTR_SCENE_CURIOSITY_SHOP,               { "Curiosity Shop", EG_CLOCK_TOWN } },
    { ENTR_SCENE_SWORDMANS_SCHOOL,             { "Swordsman's School", EG_CLOCK_TOWN } },
    { ENTR_SCENE_TRADING_POST,                 { "Trading Post", EG_CLOCK_TOWN } },
    { ENTR_SCENE_BOMB_SHOP,                    { "Bomb Shop", EG_CLOCK_TOWN } },
    { ENTR_SCENE_POST_OFFICE,                  { "Post Office", EG_CLOCK_TOWN } },
    { ENTR_SCENE_LOTTERY_SHOP,                 { "Lottery Shop", EG_CLOCK_TOWN } },
    { ENTR_SCENE_DEKU_SCRUB_PLAYGROUND,        { "Deku Scrub Playground", EG_CLOCK_TOWN } },
    { ENTR_SCENE_ASTRAL_OBSERVATORY,           { "Astral Observatory", EG_CLOCK_TOWN } },
    { ENTR_SCENE_FAIRY_FOUNTAIN,               { "Fairy Fountain", EG_TERMINA_FIELD } },
    { ENTR_SCENE_TERMINA_FIELD,                { "Termina Field", EG_TERMINA_FIELD } },
    { ENTR_SCENE_GROTTOS,                      { "Grotto", EG_TERMINA_FIELD } },
    { ENTR_SCENE_ROAD_TO_SOUTHERN_SWAMP,       { "Road to Southern Swamp", EG_SWAMP } },
    { ENTR_SCENE_SWAMP_SHOOTING_GALLERY,       { "Swamp Shooting Gallery", EG_SWAMP } },
    { ENTR_SCENE_SOUTHERN_SWAMP_POISONED,      { "Southern Swamp", EG_SWAMP } },
    { ENTR_SCENE_TOURIST_INFORMATION,          { "Tourist Information", EG_SWAMP } },
    { ENTR_SCENE_MAGIC_HAGS_POTION_SHOP,       { "Magic Hags' Potion Shop", EG_SWAMP } },
    { ENTR_SCENE_WOODS_OF_MYSTERY,             { "Woods of Mystery", EG_SWAMP } },
    { ENTR_SCENE_SWAMP_SPIDER_HOUSE,           { "Swamp Spider House", EG_SWAMP } },
    { ENTR_SCENE_DEKU_PALACE,                  { "Deku Palace", EG_SWAMP } },
    { ENTR_SCENE_DEKU_KINGS_CHAMBER,           { "Deku King's Chamber", EG_SWAMP } },
    { ENTR_SCENE_DEKU_SHRINE,                  { "Deku Shrine", EG_SWAMP } },
    { ENTR_SCENE_WOODFALL,                     { "Woodfall", EG_SWAMP } },
    { ENTR_SCENE_WOODFALL_TEMPLE,              { "Woodfall Temple", EG_SWAMP } },
    { ENTR_SCENE_ODOLWAS_LAIR,                 { "Odolwa's Lair", EG_SWAMP } },
    { ENTR_SCENE_PATH_TO_MOUNTAIN_VILLAGE,     { "Path to Mountain Village", EG_MOUNTAIN } },
    { ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER,      { "Mountain Village", EG_MOUNTAIN } },
    { ENTR_SCENE_MOUNTAIN_VILLAGE_SPRING,      { "Mountain Village", EG_MOUNTAIN } },
    { ENTR_SCENE_MOUNTAIN_SMITHY,              { "Mountain Smithy", EG_MOUNTAIN } },
    { ENTR_SCENE_GORON_GRAVERYARD,             { "Goron Graveyard", EG_MOUNTAIN } },
    { ENTR_SCENE_PATH_TO_GORON_VILLAGE_WINTER, { "Path to Goron Village", EG_MOUNTAIN } },
    { ENTR_SCENE_GORON_VILLAGE_WINTER,         { "Goron Village", EG_MOUNTAIN } },
    { ENTR_SCENE_GORON_SHRINE,                 { "Goron Shrine", EG_MOUNTAIN } },
    { ENTR_SCENE_GORON_SHOP,                   { "Goron Shop", EG_MOUNTAIN } },
    { ENTR_SCENE_GORON_RACETRACK,              { "Goron Racetrack", EG_MOUNTAIN } },
    { ENTR_SCENE_PATH_TO_SNOWHEAD,             { "Path to Snowhead", EG_MOUNTAIN } },
    { ENTR_SCENE_SNOWHEAD,                     { "Snowhead", EG_MOUNTAIN } },
    { ENTR_SCENE_SNOWHEAD_TEMPLE,              { "Snowhead Temple", EG_MOUNTAIN } },
    { ENTR_SCENE_GOHTS_LAIR,                   { "Goht's Lair", EG_MOUNTAIN } },
    { ENTR_SCENE_MILK_ROAD,                    { "Milk Road", EG_RANCH } },
    { ENTR_SCENE_GORMAN_TRACK,                 { "Gorman Track", EG_RANCH } },
    { ENTR_SCENE_ROMANI_RANCH,                 { "Romani Ranch", EG_RANCH } },
    { ENTR_SCENE_RANCH_HOUSE,                  { "Ranch House", EG_RANCH } },
    { ENTR_SCENE_CUCCO_SHACK,                  { "Cucco Shack", EG_RANCH } },
    { ENTR_SCENE_DOGGY_RACETRACK,              { "Doggy Racetrack", EG_RANCH } },
    { ENTR_SCENE_GREAT_BAY_COAST,              { "Great Bay Coast", EG_GREAT_BAY } },
    { ENTR_SCENE_FISHERMANS_HUT,               { "Fisherman's Hut", EG_GREAT_BAY } },
    { ENTR_SCENE_MARINE_RESEARCH_LAB,          { "Marine Research Lab", EG_GREAT_BAY } },
    { ENTR_SCENE_OCEANSIDE_SPIDER_HOUSE,       { "Oceanside Spider House", EG_GREAT_BAY } },
    { ENTR_SCENE_PINNACLE_ROCK,                { "Pinnacle Rock", EG_GREAT_BAY } },
    { ENTR_SCENE_PIRATES_FORTRESS_EXTERIOR,    { "Pirates' Fortress Exterior", EG_GREAT_BAY } },
    { ENTR_SCENE_PIRATES_FORTRESS,             { "Pirates' Fortress", EG_GREAT_BAY } },
    { ENTR_SCENE_PIRATES_FORTRESS_INTERIOR,    { "Pirates' Fortress Interior", EG_GREAT_BAY } },
    { ENTR_SCENE_ZORA_CAPE,                    { "Zora Cape", EG_GREAT_BAY } },
    { ENTR_SCENE_ZORA_HALL,                    { "Zora Hall", EG_GREAT_BAY } },
    { ENTR_SCENE_ZORA_HALL_ROOMS,              { "Zora Hall Rooms", EG_GREAT_BAY } },
    { ENTR_SCENE_WATERFALL_RAPIDS,             { "Waterfall Rapids", EG_GREAT_BAY } },
    { ENTR_SCENE_GREAT_BAY_TEMPLE,             { "Great Bay Temple", EG_GREAT_BAY } },
    { ENTR_SCENE_GYORGS_LAIR,                  { "Gyorg's Lair", EG_GREAT_BAY } },
    { ENTR_SCENE_ROAD_TO_IKANA,                { "Road to Ikana", EG_IKANA } },
    { ENTR_SCENE_IKANA_GRAVEYARD,              { "Ikana Graveyard", EG_IKANA } },
    { ENTR_SCENE_BENEATH_THE_GRAVERYARD,       { "Beneath the Graveyard", EG_IKANA } },
    { ENTR_SCENE_DAMPES_HOUSE,                 { "Dampe's House", EG_IKANA } },
    { ENTR_SCENE_IKANA_CANYON,                 { "Ikana Canyon", EG_IKANA } },
    { ENTR_SCENE_GHOST_HUT,                    { "Ghost Hut", EG_IKANA } },
    { ENTR_SCENE_MUSIC_BOX_HOUSE,              { "Music Box House", EG_IKANA } },
    { ENTR_SCENE_SAKONS_HIDEOUT,               { "Sakon's Hideout", EG_IKANA } },
    { ENTR_SCENE_SECRET_SHRINE,                { "Secret Shrine", EG_IKANA } },
    { ENTR_SCENE_BENEATH_THE_WELL,             { "Beneath the Well", EG_IKANA } },
    { ENTR_SCENE_IKANA_CASTLE,                 { "Ikana Castle", EG_IKANA } },
    { ENTR_SCENE_IGOS_DU_IKANAS_LAIR,          { "Igos du Ikana's Lair", EG_IKANA } },
    { ENTR_SCENE_STONE_TOWER,                  { "Stone Tower", EG_IKANA } },
    { ENTR_SCENE_STONE_TOWER_INVERTED,         { "Inverted Stone Tower", EG_IKANA } },
    { ENTR_SCENE_STONE_TOWER_TEMPLE,           { "Stone Tower Temple", EG_IKANA } },
    { ENTR_SCENE_STONE_TOWER_TEMPLE_INVERTED,  { "Inverted Stone Tower Temple", EG_IKANA } },
    { ENTR_SCENE_TWINMOLDS_LAIR,               { "Twinmold's Lair", EG_IKANA } },
    { ENTR_SCENE_THE_MOON,                     { "The Moon", EG_ONE_WAY } },
    { ENTR_SCENE_MAJORAS_LAIR,                 { "Majora's Lair", EG_ONE_WAY } },
};

// clang-format on

static std::vector<EntranceInfo> sEntrances;
static std::unordered_map<uint64_t, s16> sEntranceLookup;

static uint64_t MakeKey(s32 to, s32 from) {
    return ((uint64_t)(uint32_t)to << 32) | (uint32_t)from;
}

static std::string LabelFor(s32 entrance) {
    auto spawn = sSpawnLabels.find(entrance);
    if (spawn != sSpawnLabels.end()) {
        return spawn->second;
    }
    auto scene = sSceneLabels.find(SCENE_OF(entrance));
    if (scene != sSceneLabels.end()) {
        return scene->second.name;
    }
    return "Entrance " + std::to_string(entrance);
}

static EntranceGroup GroupFor(s32 entrance) {
    auto scene = sSceneLabels.find(SCENE_OF(entrance));
    return scene != sSceneLabels.end() ? scene->second.group : EG_TERMINA_FIELD;
}

static EntranceType ClassifyTwoWay(s32 to, s32 from) {
    if (SCENE_OF(to) == ENTR_SCENE_GROTTOS) {
        return ET_GROTTO;
    }
    if (SCENE_OF(from) == ENTR_SCENE_GROTTOS) {
        return ET_GROTTO_REVERSE;
    }
    if (sDungeonEntrances.contains(to)) {
        return ET_DUNGEON;
    }
    if (sDungeonEntrances.contains(from)) {
        return ET_DUNGEON_REVERSE;
    }
    if (sSimpleInteriors.contains(to)) {
        return ET_INTERIOR;
    }
    if (sSimpleInteriors.contains(from)) {
        return ET_INTERIOR_REVERSE;
    }
    if (sSpecialInteriors.contains(to)) {
        return ET_SPECIAL_INTERIOR;
    }
    if (sSpecialInteriors.contains(from)) {
        return ET_SPECIAL_INTERIOR_REVERSE;
    }
    if (sOverworldEntrances.contains(to) && sOverworldEntrances.contains(from)) {
        return ET_OVERWORLD;
    }
    return ET_NONE;
}

static void BuildEntrances() {
    sEntrances.clear();
    sEntranceLookup.clear();

    for (auto& [regionId, region] : Rando::Logic::Regions) {
        for (auto& [to, regionExit] : region.exits) {
            s32 from = regionExit.returnEntrance;
            if (sEntranceLookup.contains(MakeKey(to, from))) {
                continue;
            }

            EntranceInfo info = {};
            info.to = to;
            info.from = from;
            info.parent = regionId;
            info.connected = RR_MAX;
            info.type = ET_NONE;
            info.reverse = -1;
            info.optional = false;

            if (from == ONE_WAY_EXIT) {
                if (regionId == RR_MAX) {
                    if (sOwlWarpEntrances.contains(to)) {
                        info.type = ET_OWL_WARP;
                        info.source = "Song of Soaring: " + LabelFor(to);
                    } else {
                        info.type = ET_SPAWN;
                        info.source = "Dawn of the First Day";
                    }
                    info.group = EG_ONE_WAY;
                } else {
                    auto bossDoor = sBossDoorLabels.find(to);
                    if (bossDoor != sBossDoorLabels.end()) {
                        info.type = ET_BOSS;
                        info.source = bossDoor->second;
                    } else {
                        info.source = region.name[0] != '\0' ? region.name : "One-Way Exit";
                    }
                    info.group = GroupFor(to);
                }
                info.destination = LabelFor(to);
            } else {
                info.type = ClassifyTwoWay(to, from);
                info.source = LabelFor(from);
                info.destination = LabelFor(to);
                // Group a door with the area it is found in; for the way back out of an interior
                // that is the area it leads to.
                bool leavingInterior = info.type == ET_DUNGEON_REVERSE || info.type == ET_INTERIOR_REVERSE ||
                                       info.type == ET_SPECIAL_INTERIOR_REVERSE || info.type == ET_GROTTO_REVERSE;
                info.group = GroupFor(leavingInterior ? to : from);
            }

            if (to == ENTRANCE(STONE_TOWER_TEMPLE_INVERTED, 0) || from == ENTRANCE(STONE_TOWER_TEMPLE_INVERTED, 0)) {
                info.optional = true;
            }

            sEntranceLookup[MakeKey(to, from)] = (s16)sEntrances.size();
            sEntrances.push_back(info);
        }
    }

    for (size_t i = 0; i < sEntrances.size(); i++) {
        auto& info = sEntrances[i];
        if (info.from != ONE_WAY_EXIT) {
            auto reverse = sEntranceLookup.find(MakeKey(info.from, info.to));
            if (reverse != sEntranceLookup.end()) {
                info.reverse = reverse->second;
                // The region holding the way back is exactly where this entrance arrives, which is
                // more precise than the entrance id when several doors share a spawn.
                info.connected = sEntrances[reverse->second].parent;
            }
        }
        if (info.connected == RR_MAX) {
            info.connected = Rando::Logic::GetRegionIdFromEntrance(info.to);
        }
        // A two-way type with no way back in the graph cannot be coupled, so leave it alone.
        if (info.reverse < 0 && info.type != ET_SPAWN && info.type != ET_OWL_WARP && info.type != ET_BOSS) {
            info.type = ET_NONE;
        }
    }

    // Names are what the spoiler and tracker key on, so make sure no two entrances share one.
    std::map<std::string, int> seen;
    for (auto& info : sEntrances) {
        std::string name = info.source + " -> " + info.destination;
        int count = ++seen[name];
        if (count > 1) {
            info.destination += " #" + std::to_string(count);
        }
    }
}

const std::vector<EntranceInfo>& GetEntrances() {
    if (sEntrances.empty() && !Rando::Logic::Regions.empty()) {
        BuildEntrances();
    }
    return sEntrances;
}

s16 FindEntrance(s32 to, s32 from) {
    GetEntrances();
    auto it = sEntranceLookup.find(MakeKey(to, from));
    return it != sEntranceLookup.end() ? it->second : -1;
}

std::string GetArrivalName(s16 index) {
    auto& entrances = GetEntrances();
    if (index < 0 || index >= (s16)entrances.size()) {
        return "";
    }
    auto& info = entrances[index];
    if (info.from == ONE_WAY_EXIT) {
        return info.destination;
    }
    return info.destination + " (from " + info.source + ")";
}

const char* GetTypeName(EntranceType type) {
    switch (type) {
        case ET_SPAWN:
            return "Spawn";
        case ET_OWL_WARP:
            return "Owl Warp";
        case ET_BOSS:
            return "Boss";
        case ET_DUNGEON:
        case ET_DUNGEON_REVERSE:
            return "Dungeon";
        case ET_INTERIOR:
        case ET_INTERIOR_REVERSE:
        case ET_SPECIAL_INTERIOR:
        case ET_SPECIAL_INTERIOR_REVERSE:
            return "Interior";
        case ET_GROTTO:
        case ET_GROTTO_REVERSE:
            return "Grotto";
        case ET_OVERWORLD:
            return "Overworld";
        default:
            return "None";
    }
}

const char* GetGroupName(EntranceGroup group) {
    switch (group) {
        case EG_CLOCK_TOWN:
            return "Clock Town";
        case EG_TERMINA_FIELD:
            return "Termina Field";
        case EG_SWAMP:
            return "Southern Swamp";
        case EG_MOUNTAIN:
            return "Snowhead";
        case EG_RANCH:
            return "Romani Ranch";
        case EG_GREAT_BAY:
            return "Great Bay";
        case EG_IKANA:
            return "Ikana";
        case EG_ONE_WAY:
            return "One-Way";
        default:
            return "";
    }
}

const GrottoHole* GetGrottoHole(s32 grotto) {
    for (auto& hole : sGrottoHoles) {
        if (hole.grotto == grotto) {
            return &hole;
        }
    }
    return nullptr;
}

const GrottoHole* FindGrottoHoleInScene(s16 sceneId, s32 realEntrance, u8 data) {
    sceneId = NormalizeSceneId(sceneId);
    const GrottoHole* onlyMatch = nullptr;
    int matches = 0;
    for (auto& hole : sGrottoHoles) {
        if (hole.sceneId != sceneId || hole.realEntrance != realEntrance) {
            continue;
        }
        // The low five bits are the chest flag, which is what tells the generic grottos apart. The
        // upper bits are the chest contents and differ between a scene's alternate setups.
        if ((hole.data & 0x1F) == (data & 0x1F)) {
            return &hole;
        }
        onlyMatch = &hole;
        matches++;
    }
    return matches == 1 ? onlyMatch : nullptr;
}

// The region graph only knows one variant of the scenes that change with the story.
s16 NormalizeSceneId(s16 sceneId) {
    switch (sceneId) {
        case SCENE_10YUKIYAMANOMURA2:
            return SCENE_10YUKIYAMANOMURA;
        case SCENE_11GORONNOSATO2:
            return SCENE_11GORONNOSATO;
        case SCENE_17SETUGEN2:
            return SCENE_17SETUGEN;
        case SCENE_20SICHITAI2:
            return SCENE_20SICHITAI;
        default:
            return sceneId;
    }
}

s32 NormalizeEntrance(s32 entrance) {
    s32 scene = SCENE_OF(entrance);
    switch (scene) {
        case ENTR_SCENE_MOUNTAIN_VILLAGE_SPRING:
            scene = ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_GORON_VILLAGE_SPRING:
            scene = ENTR_SCENE_GORON_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_PATH_TO_GORON_VILLAGE_SPRING:
            scene = ENTR_SCENE_PATH_TO_GORON_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_SOUTHERN_SWAMP_CLEARED:
            scene = ENTR_SCENE_SOUTHERN_SWAMP_POISONED;
            break;
        default:
            break;
    }
    // Drop the layer bits, the graph's ids never carry them.
    return (scene << 9) | (entrance & 0x1F0);
}

} // namespace Entrance

} // namespace Rando
