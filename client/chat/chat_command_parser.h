#ifndef CLIENT_CHAT_COMMAND_PARSER_H
#define CLIENT_CHAT_COMMAND_PARSER_H

#include <optional>
#include <string>

#include "../../common/messages.h"

class ChatCommandParser {
public:
    std::optional<ClientCommand> parse(const std::string& text) const;
};

#endif  // CLIENT_CHAT_COMMAND_PARSER_H
