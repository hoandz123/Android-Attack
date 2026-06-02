#include "TableMessagesImpl.h"
#include <API/Il2CppApi.h>

namespace TableMessagesImpl {

std::string GetMessageDx(unsigned int msgId) {
    if (msgId == 0) return {};
    Class *cls = FindClass(OBF("TableMessagesImpl"));
    if (!cls || !cls->find_method(OBF("GetMessageDx"), 1)) return {};
    Il2CppMethod *m = cls->find_method(OBF("GetMessageDx"), 1);
    if (!m) return {};
    String *s = m->static_invoke<String *>(msgId);
    if (!s) return {};
    return s->to_string();
}

}
