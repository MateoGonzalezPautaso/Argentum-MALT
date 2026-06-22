#include <cstdio>
#include <filesystem>
#include <string>
#include <variant>

#include "gtest/gtest.h"
#include "server/core/config.h"
#include "server/game/game.h"
#include "server/game/player_data_service.h"
#include "server/persistence/clan_persistence.h"

namespace {

template <typename Event>
bool has_event(const std::vector<ServerEvent>& events) {
    for (const auto& ev: events) {
        if (std::holds_alternative<Event>(ev))
            return true;
    }
    return false;
}

template <typename Event>
const Event& get_event(const std::vector<ServerEvent>& events) {
    for (const auto& ev: events) {
        if (std::holds_alternative<Event>(ev))
            return std::get<Event>(ev);
    }
    throw std::runtime_error("event not found");
}

}  // namespace

class GameTest: public ::testing::Test {
protected:
    std::string data_dir;
    ServerConfig config;
    PlayerDataService* data_service = nullptr;
    ClanPersistence* clan_persistence = nullptr;
    Game* game = nullptr;

    void SetUp() override {
        data_dir = "/tmp/game_test_" + std::to_string(getpid());
        std::filesystem::create_directories(data_dir);

        data_service = new PlayerDataService(data_dir, config);
        clan_persistence = new ClanPersistence(data_dir + "/clans.dat", data_dir + "/clans.idx");
        game = new Game(config, *data_service, *clan_persistence);
    }

    void TearDown() override {
        delete game;
        delete clan_persistence;
        delete data_service;
        std::filesystem::remove_all(data_dir);
    }

    // Crea un personaje y lo deja logueado bajo player_id, devuelve el resultado de la creación.
    CommandResult create_character(uint16_t player_id, const std::string& username, Race race,
                                   PlayerClass player_class) {
        CreateCharacterCmd cmd{username, "password", race, player_class};
        return game->process_command(player_id, cmd);
    }
};

// ─────────────────────────────────────────────────────────────
// Guerrero: no puede meditar ni lanzar hechizos (consigna "Razas y Clases")
// ─────────────────────────────────────────────────────────────

TEST_F(GameTest, Warrior_CannotMeditate) {
    auto created = create_character(1, "warrior1", Race::HUMAN, PlayerClass::WARRIOR);
    ASSERT_TRUE(has_event<CharacterCreatedEvent>(created.private_events));

    auto result = game->process_command(1, MeditateCmd{});

    ASSERT_TRUE(has_event<ChatMsgEvent>(result.private_events));
    EXPECT_EQ(get_event<ChatMsgEvent>(result.private_events).message,
              "Los guerreros no pueden meditar");
    EXPECT_FALSE(has_event<MeditationStartEvent>(result.private_events));
}

TEST_F(GameTest, Warrior_CannotCastSpell) {
    auto created = create_character(1, "warrior2", Race::DWARF, PlayerClass::WARRIOR);
    ASSERT_TRUE(has_event<CharacterCreatedEvent>(created.private_events));

    auto result = game->process_command(1, CastSpellCmd{0});

    ASSERT_TRUE(has_event<ChatMsgEvent>(result.private_events));
    EXPECT_EQ(get_event<ChatMsgEvent>(result.private_events).message,
              "Los guerreros no pueden usar magia");
}

TEST_F(GameTest, Mage_CanMeditate) {
    auto created = create_character(1, "mage1", Race::ELF, PlayerClass::MAGE);
    ASSERT_TRUE(has_event<CharacterCreatedEvent>(created.private_events));

    auto result = game->process_command(1, MeditateCmd{});

    EXPECT_TRUE(has_event<MeditationStartEvent>(result.private_events));
}
