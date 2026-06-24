#include <string>

#include "common/config.h"
#include "common/messages.h"
#include "gtest/gtest.h"
#include "server/game/map.h"
#include "server/game/map_level_data_builder.h"

/*
 * Construye un TilemapConfig 3x3 conocido:
 *   - tiles "grass" (transitable) y "wall" (no transitable)
 *   - un prop "tree" con hitbox que bloquea su celda
 *   - un prop "door" con transition_map (puerta a otro mapa), sin hitbox
 *
 *   mapa:            prop_map:
 *     g g g            .    .    tree
 *     g W g            .    .    .
 *     g g g            door .    .
 */
namespace {

constexpr int kTileSize = 10;

TilemapConfig make_test_config() {
    TilemapConfig cfg;
    cfg.path = "atlas.png";
    cfg.tile_size = kTileSize;
    cfg.map_type = MapType::CITY;

    TileDef grass;
    grass.walkable = true;
    TileDef wall;
    wall.walkable = false;
    cfg.tiles.emplace("grass", grass);
    cfg.tiles.emplace("wall", wall);

    cfg.mapa = {
            {"grass", "grass", "grass"},
            {"grass", "wall", "grass"},
            {"grass", "grass", "grass"},
    };

    PropDef tree;
    tree.height = kTileSize;
    tree.width = kTileSize;
    tree.hitbox = {0, 0, kTileSize, kTileSize};  // cubre el centro de su celda
    cfg.props.emplace("tree", tree);

    PropDef door;
    door.height = kTileSize;
    door.width = kTileSize;
    door.transition_map = "dungeon";  // sin hitbox: transitable pero es transición
    cfg.props.emplace("door", door);

    cfg.prop_map = {
            {"", "", "tree"},
            {"", "", ""},
            {"door", "", ""},
    };

    cfg.mob_spawn_zones = {
            {false, false, false},
            {false, true, false},
            {false, false, false},
    };
    return cfg;
}

const std::string& tile_at(const MapLevelData& d, int row, int col) {
    return d.tile_id_table[d.tile_grid[static_cast<std::size_t>(row)]
                                      [static_cast<std::size_t>(col)]];
}

}  // namespace

class MapLevelDataBuilderTest: public ::testing::Test {
protected:
    TilemapConfig cfg = make_test_config();
    Map map{cfg};
    MapLevelData data = MapLevelDataBuilder::build("city", map);
};

TEST_F(MapLevelDataBuilderTest, BasicMetadata) {
    EXPECT_EQ(data.map_name, "city");
    EXPECT_EQ(data.map_type, MapType::CITY);
    EXPECT_EQ(data.tile_size, kTileSize);
    EXPECT_EQ(data.rows, 3);
    EXPECT_EQ(data.cols, 3);
}

TEST_F(MapLevelDataBuilderTest, TileDictionaryHasNoDuplicates) {
    // Solo dos ids únicos en la grilla: "grass" y "wall".
    EXPECT_EQ(data.tile_id_table.size(), 2u);
}

TEST_F(MapLevelDataBuilderTest, TileGridIndicesResolveToOriginalIds) {
    EXPECT_EQ(tile_at(data, 0, 0), "grass");
    EXPECT_EQ(tile_at(data, 1, 1), "wall");
    EXPECT_EQ(tile_at(data, 2, 2), "grass");
}

TEST_F(MapLevelDataBuilderTest, WalkableGridResolvedServerSide) {
    // Tile no transitable.
    EXPECT_FALSE(data.walkable[1][1]);
    // Prop con hitbox bloquea su celda.
    EXPECT_FALSE(data.walkable[0][2]);
    // Celda de pasto libre.
    EXPECT_TRUE(data.walkable[0][0]);
    EXPECT_TRUE(data.walkable[2][2]);
}

TEST_F(MapLevelDataBuilderTest, PropsAreSparseWithDictionary) {
    ASSERT_EQ(data.props.size(), 2u);
    EXPECT_EQ(data.prop_id_table.size(), 2u);
}

TEST_F(MapLevelDataBuilderTest, TransitionFlagSetOnlyForDoor) {
    const PropPlacement* tree = nullptr;
    const PropPlacement* door = nullptr;
    for (const auto& p: data.props) {
        const std::string& name = data.prop_id_table[p.prop_id_index];
        if (name == "tree")
            tree = &p;
        else if (name == "door")
            door = &p;
    }
    ASSERT_NE(tree, nullptr);
    ASSERT_NE(door, nullptr);
    EXPECT_FALSE(tree->is_transition);
    EXPECT_TRUE(door->is_transition);
    EXPECT_EQ(door->row, 2);
    EXPECT_EQ(door->col, 0);
}

TEST_F(MapLevelDataBuilderTest, MobSpawnZonesCopied) {
    ASSERT_EQ(data.mob_spawn_zones.size(), 3u);
    EXPECT_TRUE(data.mob_spawn_zones[1][1]);
    EXPECT_FALSE(data.mob_spawn_zones[0][0]);
}
