#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

class KeyValueStore;
class AOFWriter;

using CommandArgs = std::vector<std::string>;
using CommandResult = std::string;

struct CommandContext {
    KeyValueStore& store;
    AOFWriter* aof;
    std::string server_version;
};

using CommandHandler = std::function<CommandResult(CommandContext&, const CommandArgs&)>;

class CommandDispatcher {
public:
    CommandDispatcher();
    CommandResult dispatch(CommandContext& ctx, const CommandArgs& args);

private:
    std::unordered_map<std::string, CommandHandler> handlers_;

    static CommandResult cmd_ping(CommandContext&, const CommandArgs&);
    static CommandResult cmd_set(CommandContext&, const CommandArgs&);
    static CommandResult cmd_get(CommandContext&, const CommandArgs&);
    static CommandResult cmd_del(CommandContext&, const CommandArgs&);
    static CommandResult cmd_exists(CommandContext&, const CommandArgs&);
    static CommandResult cmd_expire(CommandContext&, const CommandArgs&);
    static CommandResult cmd_ttl(CommandContext&, const CommandArgs&);
    static CommandResult cmd_persist(CommandContext&, const CommandArgs&);
    static CommandResult cmd_keys(CommandContext&, const CommandArgs&);
    static CommandResult cmd_type(CommandContext&, const CommandArgs&);
    static CommandResult cmd_dbsize(CommandContext&, const CommandArgs&);
    static CommandResult cmd_flushall(CommandContext&, const CommandArgs&);
    static CommandResult cmd_info(CommandContext&, const CommandArgs&);
};
