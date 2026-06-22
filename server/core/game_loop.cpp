#include "game_loop.h"

#include <chrono>
#include <string>
#include <thread>

GameLoop::GameLoop(const ServerConfig& config, Queue<PlayerCommand>& input_queue,
                   ClientListMonitor& monitor, PlayerDataService& player_data_service,
                   ClanPersistence& clan_persistence):
        tick_rate_hz(config.tick_rate_hz),
        save_interval_ticks(config.save_interval_seconds * config.tick_rate_hz),
        game(config, player_data_service, clan_persistence),
        input_queue(input_queue),
        monitor(monitor) {}

void GameLoop::dispatch(const CommandResult& result, std::optional<uint16_t> origin) {
    for (const auto& [target_id, events]: result.targeted_events)
        for (const ServerEvent& ev: events) monitor.push_event(target_id, ev);

    for (const ServerEvent& ev: result.broadcast_events) monitor.broadcast(ev);

    if (!origin)
        return;  // sin originador no hay a quién enviar private_events ni mapa que resolver

    for (const ServerEvent& ev: result.private_events)
        monitor.push_event(*origin, ev);

    std::string origin_map = game.get_player_map_name(*origin);
    for (uint16_t pid: game.get_player_ids_on_map(origin_map)) {
        for (const ServerEvent& ev: result.map_events) monitor.push_event(pid, ev);
    }
}

void GameLoop::run() {
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    const milliseconds tick_duration = milliseconds(1000 / tick_rate_hz);
    auto next_tick = steady_clock::now();
    int ticks_since_save = 0;

    while (should_keep_running()) {
        next_tick += tick_duration;

        // Pop all commands and process them
        try {
            PlayerCommand pcmd;

            while (input_queue.try_pop(pcmd)) {
                CommandResult result = game.process_command(pcmd.player_id, pcmd.cmd);
                dispatch(result, pcmd.player_id);
            }
        } catch (const ClosedQueue&) {
            break;
        }

        // Tick game
        CommandResult tick_result = game.tick();
        dispatch(tick_result, std::nullopt);

        // Remove clients whose sender/receiver threads have exited
        for (uint16_t dead_id: monitor.clean_dead()) {
            CommandResult despawn = game.remove_player(dead_id);
            dispatch(despawn, std::nullopt);
        }

        // If the tick body overran, skip the missed complete ticks
        auto now = steady_clock::now();
        if (next_tick < now) {
            auto behind = now - next_tick;
            next_tick += (behind / tick_duration) * tick_duration;
            // behind / tick_duration is integer division on chrono durations
            // it gives exactly how many complete ticks were missed. Advance next_tick by that many
            // ticks.
        }

        ++ticks_since_save;
        if (ticks_since_save >= save_interval_ticks) {
            game.save_all_players();
            ticks_since_save = 0;
        }

        std::this_thread::sleep_until(next_tick);
    }

    game.save_all_players();
}
