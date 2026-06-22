file(GLOB SERVER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB SERVER_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(taller_tests
    PRIVATE
    ${SERVER_SRC}
    ${CMAKE_SOURCE_DIR}/client/network/client_protocol.cpp
    ${CMAKE_SOURCE_DIR}/server/network/server_protocol.cpp
    ${CMAKE_SOURCE_DIR}/server/network/client_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/network/sender.cpp
    ${CMAKE_SOURCE_DIR}/server/network/receiver.cpp
    ${CMAKE_SOURCE_DIR}/server/game/player.cpp
    ${CMAKE_SOURCE_DIR}/server/game/player_inventory.cpp
    ${CMAKE_SOURCE_DIR}/server/game/inventory.cpp
    ${CMAKE_SOURCE_DIR}/server/game/clan_manager.cpp
    ${CMAKE_SOURCE_DIR}/server/game/clan_command_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/game/combat_controller.cpp
    ${CMAKE_SOURCE_DIR}/server/game/enemy_npc.cpp
    ${CMAKE_SOURCE_DIR}/server/game/entity.cpp
    ${CMAKE_SOURCE_DIR}/server/game/map.cpp
    ${CMAKE_SOURCE_DIR}/server/game/prop_grid.cpp
    ${CMAKE_SOURCE_DIR}/server/game/game_formulas.cpp
    ${CMAKE_SOURCE_DIR}/server/game/game.cpp
    ${CMAKE_SOURCE_DIR}/server/game/services/bank_service.cpp
    ${CMAKE_SOURCE_DIR}/server/game/services/cheat_service.cpp
    ${CMAKE_SOURCE_DIR}/server/game/services/ground_item_service.cpp
    ${CMAKE_SOURCE_DIR}/server/game/services/map_transition_service.cpp
    ${CMAKE_SOURCE_DIR}/server/game/services/player_session_service.cpp
    ${CMAKE_SOURCE_DIR}/server/game/services/merchant_service.cpp
    ${CMAKE_SOURCE_DIR}/server/game/services/spawn_service.cpp
    ${CMAKE_SOURCE_DIR}/server/game/player_data_service.cpp
    ${CMAKE_SOURCE_DIR}/server/persistence/clan_persistence.cpp
    ${CMAKE_SOURCE_DIR}/server/persistence/clan_record.cpp
    ${CMAKE_SOURCE_DIR}/server/persistence/player_persistence.cpp
    ${CMAKE_SOURCE_DIR}/server/persistence/player_record.cpp
    ${CMAKE_SOURCE_DIR}/server/persistence/inventory_persistence.cpp
    ${CMAKE_SOURCE_DIR}/editor/io/toml_serializer.cpp
    PUBLIC
    ${SERVER_HEADERS}
)