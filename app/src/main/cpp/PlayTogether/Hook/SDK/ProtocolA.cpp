#include "ProtocolA.h"
#include "TableMessagesImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"

namespace ProtocolA {

namespace {

unsigned errorLookupId(int result) {
    if (result <= 0) return result == 0 ? 0u : (unsigned) (-(long long) result);
    return (unsigned) result;
}

std::string enumName(int result) {
    Class *cls = FindClass(OBF("PlayTogether.Result"));
    if (!cls || !cls->find_method(OBF("ToString"), 0)) return {};
    Object *boxed = cls->value_box(&result);
    if (!boxed) return {};
    String *s = boxed->invoke_method<String *>(OBF("ToString"));
    return s ? s->to_string() : std::string{};
}

std::string popupMessage(unsigned errId) {
    if (!errId) return {};
    Object *tbl = TableSystem::GetTableUnit(eTableType::ErrorPopup);
    if (!tbl) return {};
    TableSystem::TryForceLoad(eTableType::ErrorPopup, tbl);
    Class *cls = tbl->get_class();
    if (!cls || !cls->find_method(OBF("GetByErrorId"), 1)) return {};
    Object *row = tbl->invoke_method<Object *>(OBF("GetByErrorId"), errId);
    if (!row) return {};
    Class *rc = row->get_class();
    if (!rc) return {};
    unsigned msgId = 0;
    if (rc->find_method(OBF("get_VngStringMsg"), 0))
        msgId = row->invoke_method<unsigned int>(OBF("get_VngStringMsg"));
    if (!msgId && rc->find_method(OBF("get_StringMsg"), 0))
        msgId = row->invoke_method<unsigned int>(OBF("get_StringMsg"));
    return TableMessagesImpl::GetMessageDx(msgId);
}

} // namespace

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

std::string describe(int result) {
    if (result == 0) return OBF("OK");
    std::string name = enumName(result);
    std::string msg = popupMessage(errorLookupId(result));
    if (!msg.empty() && !name.empty()) return msg + OBF(" [") + name + OBF("]");
    if (!msg.empty()) return msg;
    if (!name.empty()) return name;
    return std::string(OBF("Result=")) + std::to_string(result);
}

}
