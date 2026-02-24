#include "command.h"
#include "resp.h"
#include <algorithm>

CommandDispatcher::CommandDispatcher() {
    handlers_["PING"] = cmd_ping;
    handlers_["SET"] = cmd_set;
    handlers_["GET"] = cmd_get;
    handlers_["DEL"] = cmd_del;
    handlers_["EXISTS"] = cmd_exists;
    handlers_["EXPIRE"] = cmd_expire;
    handlers_["TTL"] = cmd_ttl;
    handlers_["PERSIST"] = cmd_persist;
    handlers_["KEYS"] = cmd_keys;
    handlers_["TYPE"] = cmd_type;
    handlers_["DBSIZE"] = cmd_dbsize;
    handlers_["FLUSHALL"] = cmd_flushall;
    handlers_["INFO"] = cmd_info;
}

CommandResult CommandDispatcher::dispatch(CommandContext& ctx, const CommandArgs& args) {
    if (args.empty()) return resp_error("ERR empty command");

    std::string name = args[0];
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    auto it = handlers_.find(name);
    if (it == handlers_.end())
        return resp_error("ERR unknown command '" + args[0] + "'");

    return it->second(ctx, args);
}

CommandResult CommandDispatcher::cmd_ping(CommandContext&, const CommandArgs& args) {
    if (args.size() > 1) return resp_bulk_string(args[1]);
    return resp_simple_string("PONG");
}

CommandResult CommandDispatcher::cmd_set(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_get(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_del(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_exists(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_expire(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_ttl(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_persist(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_keys(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_type(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_dbsize(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_flushall(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}

CommandResult CommandDispatcher::cmd_info(CommandContext&, const CommandArgs&) {
    return resp_error("ERR not implemented yet");
}
