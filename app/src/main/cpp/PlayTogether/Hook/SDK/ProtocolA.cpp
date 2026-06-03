#include "ProtocolA.h"

namespace ProtocolA {

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("PlayTogetherSocket.ProtocolA"));
    return cached;
}

int get_Result(Object *protocol) {
    if (!protocol) return -1;
    return protocol->invoke_method<int>(OBF("get_Result"));
}

bool isOk(Object *protocol) {
    return get_Result(protocol) == 0;
}

}
