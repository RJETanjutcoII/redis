#include "expiry.h"
#include "store.h"

ExpiryJanitor::ExpiryJanitor(KeyValueStore& store, int interval_ms)
    : store_(store), interval_ms_(interval_ms) {}

ExpiryJanitor::~ExpiryJanitor() { stop(); }

void ExpiryJanitor::start() {}
void ExpiryJanitor::stop() {}
std::vector<std::string> ExpiryJanitor::drain_deletions() { return {}; }
void ExpiryJanitor::run() {}
