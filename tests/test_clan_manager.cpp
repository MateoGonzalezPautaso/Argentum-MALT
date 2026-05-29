#include <cstdio>
#include <string>

#include "gtest/gtest.h"
#include "server/game/clan_manager.h"
#include "server/persistence/clan_persistence.h"

class ClanManagerTest: public ::testing::Test {
protected:
    std::string data_path;
    std::string index_path;
    ClanPersistence* persistence = nullptr;
    ClanManager* manager = nullptr;

    void SetUp() override {
        std::string base = "/tmp/clan_test_" + std::to_string(getpid());
        data_path = base + ".dat";
        index_path = base + ".idx";
        persistence = new ClanPersistence(data_path, index_path);
        manager = new ClanManager(*persistence);
    }

    void TearDown() override {
        delete manager;
        delete persistence;
        std::remove(data_path.c_str());
        std::remove(index_path.c_str());
    }
};

// ─────────────────────────────────────────────────────────────
// create_clan
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, CreateClan_Success) {
    auto result = manager->create_clan("alice", "LosHeroes");
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(manager->is_in_clan("alice"));
    EXPECT_EQ(manager->get_clan_name("alice"), "LosHeroes");
}

TEST_F(ClanManagerTest, CreateClan_DuplicateName) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->create_clan("bob", "LosHeroes");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, CreateClan_FounderAlreadyInClan) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->create_clan("alice", "OtroClan");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, CreateClan_FounderIsFounder) {
    manager->create_clan("alice", "LosHeroes");
    EXPECT_TRUE(manager->is_founder("alice"));
}

// ─────────────────────────────────────────────────────────────
// request_join
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, RequestJoin_Success) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->request_join("bob", "LosHeroes");
    EXPECT_TRUE(result.ok);

    auto requests = manager->get_pending_requests("LosHeroes");
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests[0], "bob");
}

TEST_F(ClanManagerTest, RequestJoin_ClanDoesNotExist) {
    auto result = manager->request_join("bob", "Inexistente");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, RequestJoin_AlreadyInClan) {
    manager->create_clan("alice", "LosHeroes");
    manager->create_clan("bob", "OtroClan");
    auto result = manager->request_join("bob", "LosHeroes");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, RequestJoin_DuplicateRequest) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    auto result = manager->request_join("bob", "LosHeroes");
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────
// accept_member
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, AcceptMember_Success) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");

    auto result = manager->accept_member("alice", "bob");
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(manager->is_in_clan("bob"));
    EXPECT_EQ(manager->get_clan_name("bob"), "LosHeroes");

    auto requests = manager->get_pending_requests("LosHeroes");
    EXPECT_TRUE(requests.empty());
}

TEST_F(ClanManagerTest, AcceptMember_NoPendingRequest) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->accept_member("alice", "bob");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, AcceptMember_NotFounder) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");
    manager->request_join("charlie", "LosHeroes");

    auto result = manager->accept_member("bob", "charlie");
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────
// reject_member
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, RejectMember_Success) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");

    auto result = manager->reject_member("alice", "bob");
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(manager->is_in_clan("bob"));
    EXPECT_TRUE(manager->get_pending_requests("LosHeroes").empty());
}

TEST_F(ClanManagerTest, RejectMember_NoPendingRequest) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->reject_member("alice", "bob");
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────
// ban_member
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, BanMember_RemovesFromClan) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");

    auto result = manager->ban_member("alice", "bob");
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(manager->is_in_clan("bob"));
}

TEST_F(ClanManagerTest, BanMember_PreventsRejoin) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");
    manager->ban_member("alice", "bob");

    auto result = manager->request_join("bob", "LosHeroes");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, BanMember_CannotBanSelf) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->ban_member("alice", "alice");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, BanMember_NotFounder) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");

    auto result = manager->ban_member("bob", "alice");
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────
// kick_member
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, KickMember_Success) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");

    auto result = manager->kick_member("alice", "bob");
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(manager->is_in_clan("bob"));
}

TEST_F(ClanManagerTest, KickMember_CannotKickSelf) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->kick_member("alice", "alice");
    EXPECT_FALSE(result.ok);
}

TEST_F(ClanManagerTest, KickMember_TargetNotMember) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->kick_member("alice", "bob");
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────
// leave_clan
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, LeaveClan_Success) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");

    auto result = manager->leave_clan("bob");
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(manager->is_in_clan("bob"));
}

TEST_F(ClanManagerTest, LeaveClan_FounderCannotLeave) {
    manager->create_clan("alice", "LosHeroes");
    auto result = manager->leave_clan("alice");
    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(manager->is_in_clan("alice"));
}

TEST_F(ClanManagerTest, LeaveClan_NotInClan) {
    auto result = manager->leave_clan("bob");
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────
// queries
// ─────────────────────────────────────────────────────────────

TEST_F(ClanManagerTest, AreInSameClan_True) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");

    EXPECT_TRUE(manager->are_in_same_clan("alice", "bob"));
}

TEST_F(ClanManagerTest, AreInSameClan_DifferentClans) {
    manager->create_clan("alice", "LosHeroes");
    manager->create_clan("bob", "OtroClan");

    EXPECT_FALSE(manager->are_in_same_clan("alice", "bob"));
}

TEST_F(ClanManagerTest, AreInSameClan_SamePlayerReturnsFalse) {
    manager->create_clan("alice", "LosHeroes");
    EXPECT_FALSE(manager->are_in_same_clan("alice", "alice"));
}

TEST_F(ClanManagerTest, GetMemberList_ContainsFounder) {
    manager->create_clan("alice", "LosHeroes");
    auto members = manager->get_member_list("LosHeroes");

    ASSERT_EQ(members.size(), 1u);
    EXPECT_EQ(members[0].username, "alice");
    EXPECT_TRUE(members[0].is_founder);
}

TEST_F(ClanManagerTest, GetMemberList_ContainsAcceptedMember) {
    manager->create_clan("alice", "LosHeroes");
    manager->request_join("bob", "LosHeroes");
    manager->accept_member("alice", "bob");

    auto members = manager->get_member_list("LosHeroes");
    ASSERT_EQ(members.size(), 2u);

    bool found_bob = false;
    for (const auto& m: members)
        if (m.username == "bob" && !m.is_founder)
            found_bob = true;
    EXPECT_TRUE(found_bob);
}
