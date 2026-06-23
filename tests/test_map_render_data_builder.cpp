#include <string>

#include "client/config/map_visuals_config.h"
#include "client/core/map_render_data_builder.h"
#include "common/config.h"
#include "common/messages.h"
#include "gtest/gtest.h"

/*
 * Verifica que MapRenderDataBuilder reconstruya un TilemapConfig equivalente al
 * mapa original, combinando la estructura que llega por red (MapLevelData) con
 * el catálogo visual local (MapVisualCatalog).
 */
namespace {

MapLevelData make_level() {
    MapLevelData d;
    d.map_name = "city";
    d.map_type = MapType::CITY;
    d.tile_size = 128;
    d.rows = 2;
    d.cols = 2;
    d.tile_id_table = {"grass", "wall"};
    d.tile_grid = {
            {0, 1},
            {1, 0},
    };
    d.prop_id_table = {"tree"};
    d.props = {
            {0, 0, 1, false},  // tree en (row=0, col=1)
    };
    d.walkable = {
            {true, false},
            {false, true},
    };
    d.mob_spawn_zones = {
            {true, false},
            {false, true},
    };
    return d;
}

MapVisualCatalog make_visuals() {
    MapVisualCatalog v;
    v.tilemap_path = "atlas.png";
    TileDef grass;
    grass.path = "grass.png";
    TileDef wall;
    wall.path = "wall.png";
    v.tile_visuals.emplace("grass", grass);
    v.tile_visuals.emplace("wall", wall);
    PropDef tree;
    tree.width = 64;
    tree.height = 64;
    tree.paths = {"tree.png"};
    v.prop_visuals.emplace("tree", tree);
    return v;
}

}  // namespace

class MapRenderDataBuilderTest: public ::testing::Test {
protected:
    MapLevelData level = make_level();
    MapVisualCatalog visuals = make_visuals();
    TilemapConfig cfg = MapRenderDataBuilder::build(level, visuals);
};

TEST_F(MapRenderDataBuilderTest, MetadataFromNetworkAndVisuals) {
    EXPECT_EQ(cfg.path, "atlas.png");
    EXPECT_EQ(cfg.tile_size, 128);
    EXPECT_EQ(cfg.map_type, MapType::CITY);
}

TEST_F(MapRenderDataBuilderTest, TileGridReconstructedFromDictionary) {
    ASSERT_EQ(cfg.mapa.size(), 2u);
    EXPECT_EQ(cfg.mapa[0], (std::vector<std::string>{"grass", "wall"}));
    EXPECT_EQ(cfg.mapa[1], (std::vector<std::string>{"wall", "grass"}));
}

TEST_F(MapRenderDataBuilderTest, PropGridReconstructedFromSparseList) {
    ASSERT_EQ(cfg.prop_map.size(), 2u);
    ASSERT_EQ(cfg.prop_map[0].size(), 2u);
    EXPECT_EQ(cfg.prop_map[0][0], "");
    EXPECT_EQ(cfg.prop_map[0][1], "tree");
    EXPECT_EQ(cfg.prop_map[1][0], "");
    EXPECT_EQ(cfg.prop_map[1][1], "");
}

TEST_F(MapRenderDataBuilderTest, VisualDefinitionsCopied) {
    ASSERT_TRUE(cfg.tiles.count("grass"));
    ASSERT_TRUE(cfg.tiles.count("wall"));
    EXPECT_EQ(cfg.tiles.at("grass").path, "grass.png");
    ASSERT_TRUE(cfg.props.count("tree"));
    EXPECT_EQ(cfg.props.at("tree").height, 64);
}

TEST_F(MapRenderDataBuilderTest, MobSpawnZonesCopied) {
    ASSERT_EQ(cfg.mob_spawn_zones.size(), 2u);
    EXPECT_TRUE(cfg.mob_spawn_zones[0][0]);
    EXPECT_TRUE(cfg.mob_spawn_zones[1][1]);
    EXPECT_FALSE(cfg.mob_spawn_zones[0][1]);
}
