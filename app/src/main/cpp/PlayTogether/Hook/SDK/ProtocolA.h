#ifndef SDK_PROTOCOLA_H
#define SDK_PROTOCOLA_H

#include <API/Il2CppApi.h>
#include <string>

namespace ProtocolA {

Class *get_class();
int get_Result(Object *protocol);
bool isOk(Object *protocol);
std::string describe(int result);

}

#endif // SDK_PROTOCOLA_H
