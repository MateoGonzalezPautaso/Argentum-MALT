#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <functional>
#include <optional>
#include <string>

#include "../../common/messages.h"
#include "../../common/queue.h"
#include "../../common/socket.h"
#include "../audio/audio_manager.h"
#include "../config/config.h"
#include "../network/client_protocol.h"
#include "../network/receiver.h"
#include "../network/sender.h"

#include "engine.h"

class Client {
private:
    ClientConfig config;
    Socket skt;
    ClientProtocol protocol;
    Queue<ClientCommand> command_queue;
    Queue<ServerEvent> event_queue;
    AudioManager audio_manager;
    Engine engine;
    Sender sender;
    Receiver receiver;

    bool run_menu();
    std::optional<LoginOkEvent> run_login();
    std::optional<LoginOkEvent> run_create_character();
    void game_loop();
    void wait_for_initial_map();
    void shutdown();

    void frame_sync(uint32_t& last_tick, std::function<void()> render) const;

public:
    explicit Client(const ClientConfig& config);

    void run();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
};

#endif  // CLIENT_CLIENT_H
