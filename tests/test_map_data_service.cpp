#include <string>
#include <unordered_map>
#include <variant>

#include "common/config.h"
#include "common/messages.h"
#include "gtest/gtest.h"
#include "server/game/map.h"
#include "server/game/services/map_data_service.h"

/*
 * Verifica que MapDataService responda REQUEST_MAP_DATA con un MapDataEvent
 * privado para el jugador, y que ignore mapas inexistentes.
 */
namespace {

TilemapConfig make_min_config() {
    TilemapConfig cfg;
    cfg.path = "atlas.png";
    cfg.tile_size = 32;
    cfg.map_type = MapType::CITY;
    TileDef grass;
    grass.walkable = true;
    cfg.tiles.emplace("grass", grass);
    cfg.mapa = {{"grass", "grass"}, {"grass", "grass"}};
    return cfg;
}

}  // namespace

class MapDataServiceTest: public ::testing::Test {
protected:
    TilemapConfig cfg = make_min_config();
    std::unordered_map<std::string, Map> maps;

    void SetUp() override { maps.emplace("city", Map(cfg)); }
};

TEST_F(MapDataServiceTest, RespondsWithPrivateMapDataEvent) {
    MapDataService service(maps);
    CommandResult result = service.handle_request("city");

    ASSERT_EQ(result.private_events.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<MapDataEvent>(result.private_events[0]));
    const MapLevelData& data = std::get<MapDataEvent>(result.private_events[0]).data;
    EXPECT_EQ(data.map_name, "city");
    EXPECT_EQ(data.rows, 2);
    EXPECT_EQ(data.cols, 2);
    // No es broadcast ni map_event: solo se lo manda al que lo pidió.
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.map_events.empty());
}

TEST_F(MapDataServiceTest, UnknownMapYieldsEmptyResult) {
    MapDataService service(maps);
    CommandResult result = service.handle_request("nonexistent");

    EXPECT_TRUE(result.private_events.empty());
    EXPECT_TRUE(result.broadcast_events.empty());
}
