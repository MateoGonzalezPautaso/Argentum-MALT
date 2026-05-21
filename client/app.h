#ifndef CLIENT_APP_H
#define CLIENT_APP_H

#include <string>

#include "config.h"

namespace client_app {

void init_image();
void shutdown_image();
void init_ttf();
void shutdown_ttf();
ClientConfig load_config();

}  // namespace client_app

#endif
