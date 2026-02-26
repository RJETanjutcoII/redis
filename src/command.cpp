#include "command.h"
#include "store.h"
#include "aof.h"
#include "resp.h"
#include <algorithm>
#include <chrono>
#include <sstream>

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

CommandResult CommandDispatcher::cmd_set(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 3)
        return resp_error("ERR wrong number of arguments for 'set'");

    const auto& key = args[1];
    const auto& value = args[2];
    std::optional<TimePoint> expiry;

    size_t i = 3;
    while (i < args.size()) {
        std::string opt = args[i];
        std::transform(opt.begin(), opt.end(), opt.begin(), ::toupper);

        if (opt == "EX" && i + 1 < args.size()) {
            try {
                int64_t secs = std::stoll(args[++i]);
                expiry = Clock::now() + std::chrono::seconds(secs);
            } catch (const std::exception&) {
                return resp_error("ERR value is not an integer or out of range");
            }
        } else if (opt == "PX" && i + 1 < args.size()) {
            try {
                int64_t ms = std::stoll(args[++i]);
                expiry = Clock::now() + std::chrono::milliseconds(ms);
            } catch (const std::exception&) {
                return resp_error("ERR value is not an integer or out of range");
            }
        }
        ++i;
    }

    ctx.store.set(key, value, expiry);
    if (ctx.aof) ctx.aof->append(args);
    return resp_simple_string("OK");
}

CommandResult CommandDispatcher::cmd_get(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 2)
        return resp_error("ERR wrong number of arguments for 'get'");

    auto val = ctx.store.get(args[1]);
    if (!val.has_value()) return resp_null_bulk();
    return resp_bulk_string(val.value());
}

CommandResult CommandDispatcher::cmd_del(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 2)
        return resp_error("ERR wrong number of arguments for 'del'");

    int64_t count = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        if (ctx.store.del(args[i])) ++count;
    }
    if (ctx.aof) ctx.aof->append(args);
    return resp_integer(count);
}

CommandResult CommandDispatcher::cmd_exists(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 2)
        return resp_error("ERR wrong number of arguments for 'exists'");

    auto val = ctx.store.get(args[1]);
    return resp_integer(val.has_value() ? 1 : 0);
}

CommandResult CommandDispatcher::cmd_expire(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 3)
        return resp_error("ERR wrong number of arguments for 'expire'");

    int64_t secs;
    try {
        secs = std::stoll(args[2]);
    } catch (const std::exception&) {
        return resp_error("ERR value is not an integer or out of range");
    }
    ctx.store.expire(args[1], secs);
    if (ctx.aof) ctx.aof->append(args);
    return resp_integer(1);
}

CommandResult CommandDispatcher::cmd_ttl(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 2)
        return resp_error("ERR wrong number of arguments for 'ttl'");

    return resp_integer(ctx.store.ttl(args[1]));
}

CommandResult CommandDispatcher::cmd_persist(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 2)
        return resp_error("ERR wrong number of arguments for 'persist'");

    ctx.store.persist(args[1]);
    if (ctx.aof) ctx.aof->append(args);
    return resp_integer(1);
}

CommandResult CommandDispatcher::cmd_keys(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 2)
        return resp_error("ERR wrong number of arguments for 'keys'");

    return resp_array(ctx.store.keys(args[1]));
}

CommandResult CommandDispatcher::cmd_type(CommandContext& ctx, const CommandArgs& args) {
    if (args.size() < 2)
        return resp_error("ERR wrong number of arguments for 'type'");

    auto val = ctx.store.get(args[1]);
    if (!val.has_value()) return resp_simple_string("none");
    return resp_simple_string("string");
}

CommandResult CommandDispatcher::cmd_dbsize(CommandContext& ctx, const CommandArgs&) {
    return resp_integer(static_cast<int64_t>(ctx.store.dbsize()));
}

CommandResult CommandDispatcher::cmd_flushall(CommandContext& ctx, const CommandArgs& args) {
    ctx.store.flushall();
    if (ctx.aof) ctx.aof->append(args);
    return resp_simple_string("OK");
}

CommandResult CommandDispatcher::cmd_info(CommandContext& ctx, const CommandArgs&) {
    std::ostringstream oss;
    oss << "# Server\r\n";
    oss << "redis_version:" << ctx.server_version << "\r\n";
    oss << "\r\n";
    oss << "# Keyspace\r\n";
    oss << "db0:keys=" << ctx.store.dbsize() << "\r\n";
    return resp_bulk_string(oss.str());
}
