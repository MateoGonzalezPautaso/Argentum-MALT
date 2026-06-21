#ifndef CLIENT_CHAT_COMMAND_PARSER_H
#define CLIENT_CHAT_COMMAND_PARSER_H

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../common/messages.h"

class ChatCommandParser {
public:
    ChatCommandParser();
    std::optional<ClientCommand> parse(const std::string& text) const;

private:
    using SimpleFn = ClientCommand(*)(std::string);

    std::unordered_map<std::string, ClientCommand> exact_commands_;
    std::vector<std::pair<std::string_view, SimpleFn>> prefix_commands_;
};

#endif  // CLIENT_CHAT_COMMAND_PARSER_H
