#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <toml++/toml.h>

#include "common/config.h"
#include "editor/io/toml_serializer.h"

namespace {

std::string temp_path() {
    char fn[] = "/tmp/toml_test_XXXXXX";
    int fd = mkstemp(fn);
    if (fd == -1) {
        throw std::runtime_error("mkstemp failed");
    }
    close(fd);
    return fn;
}

void roundtrip(const TilemapConfig& input) {
    std::string path = temp_path();
    TomlSerializer::save(path, input);
    TilemapConfig output = TomlSerializer::load(path);

    EXPECT_EQ(input.path, output.path);
    EXPECT_EQ(input.tile_size, output.tile_size);
    EXPECT_EQ(input.map_type, output.map_type);

    ASSERT_EQ(input.tiles.size(), output.tiles.size());
    for (const auto& [name, def]: input.tiles) {
        auto it = output.tiles.find(name);
        ASSERT_NE(it, output.tiles.end()) << "Missing tile: " << name;
        EXPECT_EQ(def.x, it->second.x);
        EXPECT_EQ(def.y, it->second.y);
        EXPECT_EQ(def.walkable, it->second.walkable);
        EXPECT_EQ(def.path, it->second.path);
    }

    ASSERT_EQ(input.mapa.size(), output.mapa.size());
    for (std::size_t r = 0; r < input.mapa.size(); ++r) {
        ASSERT_EQ(input.mapa[r].size(), output.mapa[r].size());
        for (std::size_t c = 0; c < input.mapa[r].size(); ++c) {
            EXPECT_EQ(input.mapa[r][c], output.mapa[r][c]) << " at (" << r << "," << c << ")";
        }
    }

    ASSERT_EQ(input.props.size(), output.props.size());
    for (const auto& [name, def]: input.props) {
        auto it = output.props.find(name);
        ASSERT_NE(it, output.props.end()) << "Missing prop: " << name;
        EXPECT_EQ(def.paths, it->second.paths);
        EXPECT_EQ(def.src_x, it->second.src_x);
        EXPECT_EQ(def.src_y, it->second.src_y);
        EXPECT_EQ(def.src_w, it->second.src_w);
        EXPECT_EQ(def.src_h, it->second.src_h);
        EXPECT_EQ(def.width, it->second.width);
        EXPECT_EQ(def.height, it->second.height);
        EXPECT_EQ(def.frame_ms, it->second.frame_ms);
        EXPECT_EQ(def.hitbox.x, it->second.hitbox.x);
        EXPECT_EQ(def.hitbox.y, it->second.hitbox.y);
        EXPECT_EQ(def.hitbox.w, it->second.hitbox.w);
        EXPECT_EQ(def.hitbox.h, it->second.hitbox.h);
        EXPECT_EQ(def.transition_map, it->second.transition_map);
        EXPECT_EQ(def.transition_x, it->second.transition_x);
        EXPECT_EQ(def.transition_y, it->second.transition_y);
        ASSERT_EQ(def.parts.size(), it->second.parts.size());
        for (std::size_t i = 0; i < def.parts.size(); ++i) {
            EXPECT_EQ(def.parts[i].path, it->second.parts[i].path);
            EXPECT_EQ(def.parts[i].src_x, it->second.parts[i].src_x);
            EXPECT_EQ(def.parts[i].src_y, it->second.parts[i].src_y);
            EXPECT_EQ(def.parts[i].src_w, it->second.parts[i].src_w);
            EXPECT_EQ(def.parts[i].src_h, it->second.parts[i].src_h);
            EXPECT_EQ(def.parts[i].offset_x, it->second.parts[i].offset_x);
            EXPECT_EQ(def.parts[i].offset_y, it->second.parts[i].offset_y);
        }
    }

    ASSERT_EQ(input.prop_map.size(), output.prop_map.size());
    for (std::size_t r = 0; r < input.prop_map.size(); ++r) {
        ASSERT_EQ(input.prop_map[r].size(), output.prop_map[r].size());
        for (std::size_t c = 0; c < input.prop_map[r].size(); ++c) {
            EXPECT_EQ(input.prop_map[r][c], output.prop_map[r][c])
                    << " at prop_map (" << r << "," << c << ")";
        }
    }

    ASSERT_EQ(input.mob_spawn_zones.size(), output.mob_spawn_zones.size());
    for (std::size_t r = 0; r < input.mob_spawn_zones.size(); ++r) {
        ASSERT_EQ(input.mob_spawn_zones[r].size(), output.mob_spawn_zones[r].size());
        for (std::size_t c = 0; c < input.mob_spawn_zones[r].size(); ++c) {
            EXPECT_EQ(input.mob_spawn_zones[r][c], output.mob_spawn_zones[r][c])
                    << " at mob_spawn_zones (" << r << "," << c << ")";
        }
    }

    std::remove(path.c_str());
}

TilemapConfig make_full_config() {
    TilemapConfig cfg;
    cfg.path = "maps/test_map";
    cfg.tile_size = 64;
    cfg.map_type = MapType::CITY;

    cfg.tiles["grass"] = {.x = 0, .y = 0, .walkable = true, .path = ""};
    cfg.tiles["wall"] = {.x = 32, .y = 64, .walkable = false, .path = ""};
    cfg.tiles["water"] = {.x = 128, .y = 256, .walkable = false, .path = "custom_atlas.png"};

    cfg.mapa = {
            {"grass", "wall", "grass"},
            {"water", "grass", "wall"},
    };

    {
        PropDef npc;
        npc.paths = {"npc_atlas.png"};
        npc.src_x = 0;
        npc.src_y = 0;
        npc.src_w = 64;
        npc.src_h = 64;
        npc.width = 64;
        npc.height = 64;
        npc.frame_ms = 200;
        npc.hitbox = {.x = 8, .y = 16, .w = 48, .h = 32};
        cfg.props["merchant"] = std::move(npc);
    }

    {
        PropDef portal;
        portal.paths = {"portal_atlas.png"};
        portal.src_x = 10;
        portal.src_y = 20;
        portal.src_w = 30;
        portal.src_h = 40;
        portal.width = 60;
        portal.height = 80;
        portal.hitbox = {.x = 0, .y = 0, .w = 60, .h = 80};
        portal.transition_map = "city";
        portal.transition_x = 5;
        portal.transition_y = 3;
        cfg.props["portal"] = std::move(portal);
    }

    {
        PropDef multi;
        multi.width = 128;
        multi.height = 128;
        PropPartDef part1;
        part1.path = "atlas_a.png";
        part1.src_x = 0;
        part1.src_y = 0;
        part1.src_w = 64;
        part1.src_h = 64;
        part1.offset_x = 0;
        part1.offset_y = 64;
        PropPartDef part2;
        part2.path = "atlas_b.png";
        part2.src_x = 10;
        part2.src_y = 20;
        part2.src_w = 80;
        part2.src_h = 80;
        part2.offset_x = 64;
        part2.offset_y = 0;
        multi.parts = {std::move(part1), std::move(part2)};
        cfg.props["multi_part"] = std::move(multi);
    }

    cfg.prop_map = {
            {"merchant", "", "portal"},
            {"", "multi_part", ""},
    };

    cfg.mob_spawn_zones = {
            {true, true, false},
            {false, false, true},
    };

    return cfg;
}

}  // namespace

TEST(TomlSerializerTest, RoundTripFull) { roundtrip(make_full_config()); }

TEST(TomlSerializerTest, RoundTripEmpty) {
    TilemapConfig cfg;
    cfg.mapa = {{"", ""}, {"", ""}};
    roundtrip(cfg);
}

TEST(TomlSerializerTest, RoundTripDungeonWithSpawnZones) {
    TilemapConfig cfg;
    cfg.map_type = MapType::DUNGEON;
    cfg.tile_size = 128;
    cfg.mapa = {{"floor", "floor"}, {"floor", "wall"}};
    cfg.tiles["floor"] = {.x = 0, .y = 0, .walkable = true, .path = ""};
    cfg.tiles["wall"] = {.x = 128, .y = 0, .walkable = false, .path = ""};
    cfg.mob_spawn_zones = {{true, false}, {false, true}};
    roundtrip(cfg);
}

TEST(TomlSerializerTest, RoundTripNpcPropNoPaths) {
    TilemapConfig cfg;
    cfg.tile_size = 32;
    cfg.mapa = {{"tile1"}};
    cfg.tiles["tile1"] = {.x = 16, .y = 16, .walkable = true, .path = ""};

    PropDef npc;
    npc.width = 32;
    npc.height = 32;
    cfg.props["simple_npc"] = std::move(npc);

    cfg.prop_map = {{"simple_npc"}};
    roundtrip(cfg);
}

TEST(TomlSerializerTest, RoundTripNoGridNoZones) {
    TilemapConfig cfg;
    cfg.path = "template_only";
    cfg.tile_size = 128;
    cfg.tiles["dirt"] = {.x = 0, .y = 128, .walkable = true, .path = "overworld.png"};
    roundtrip(cfg);
}
