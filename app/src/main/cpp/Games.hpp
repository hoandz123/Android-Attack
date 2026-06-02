#pragma once

namespace games {

using ActivateFn = void (*)();

struct Registrar {
    Registrar(const char *package, ActivateFn activate);
};

bool Dispatch(const char *package);

} // namespace games

#define REGISTER_GAME(package, activate) static const games::Registrar _game_reg_##activate(package, activate)
