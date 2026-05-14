#include <exception>
#include <iostream>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "../common/socket.h"
#include "client_protocol.h"
#include "config.h"
#include "engine.h"

int main() try {
	const int img_flags = IMG_INIT_PNG;
	if ((IMG_Init(img_flags) & img_flags) != img_flags) {
		throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
	}

	const std::string config_path =
			"client_config.toml";
	ClientConfig config = load_client_config(config_path);

	Socket skt("127.0.0.1", "1234");
	ClientProtocol proto(std::move(skt));
	ClientEngine client(config, proto);

	bool running = true;
	SDL_Event event{};
	const uint32_t tick_ms = config.tick_ms;
	uint32_t last_tick = SDL_GetTicks();

	while (running) {
		while (SDL_PollEvent(&event)) {
			running = client.handle_event(event);
			if (!running) {
				break;
			}
		}
		if (!running) {
			break;
		}

		const uint32_t now = SDL_GetTicks();
		const uint32_t elapsed = now - last_tick;
		if (elapsed >= tick_ms) {
			last_tick = now;
			client.tick();
			client.show_sprite();
		}

		const uint32_t sleep_ms = (elapsed < tick_ms) ? (tick_ms - elapsed) : 0;
		if (sleep_ms > 0) {
			SDL_Delay(sleep_ms);
		}
	}

	IMG_Quit();
	return 0;
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
