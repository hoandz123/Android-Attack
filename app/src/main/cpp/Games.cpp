#include "Games.hpp"
#include <vector>
#include <cstring>

namespace games {

namespace {
struct Entry { const char *package; ActivateFn activate; };
std::vector<Entry> &Registry() { static std::vector<Entry> r; return r; }
} // namespace

Registrar::Registrar(const char *package, ActivateFn activate) { Registry().push_back(Entry{package, activate}); }

bool Dispatch(const char *package) {
    if (!package) return false;
    for (const Entry &e : Registry()) { if (std::strcmp(e.package, package) == 0) { e.activate(); return true; } }
    return false;
}

} // namespace games
