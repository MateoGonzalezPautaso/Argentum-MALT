#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <functional>
#include <optional>
#include <string>

#include "../../common/messages.h"
#include "../../common/queue.h"
#include "../../common/socket.h"

#include "../network/client_protocol.h"
#include "../config/config.h"
#include "engine.h"
#include "../network/receiver.h"
#include "../network/sender.h"

class Client {
private:
    ClientConfig config;
    Socket skt;
    ClientProtocol protocol;
    Queue<ClientCommand> command_queue;
    Queue<ServerEvent> event_queue;
    Engine engine;
    Sender sender;
    Receiver receiver;

    bool run_menu();
    std::optional<LoginOkEvent> run_login();
    void game_loop();
    void shutdown();

    void frame_sync(uint32_t& last_tick, std::function<void()> render) const;

public:
    explicit Client(const ClientConfig& config);

    void run();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
};

#endif  // CLIENT_CLIENT_H
