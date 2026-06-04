#pragma once

#include <API/Il2CppApi.h>
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include <cstdint>
#include <string>

namespace lienquan {
namespace il2cpp_bypass {

namespace {

// CSProtocol.CSCMD_ID_DEF (AovTdr.dll) — dwMsgID trong CSPkgHead; dump không có literal, số từ xref/disasm
enum CsCmdId : uint32_t {
    CSID_RELAYHASHCHECK = 0x507u,                    // 1287 — LNetwork.SendGameMsg (LSynchrReport.Upload)
    CSID_ANTIDATA_REQ = 0xBB8u,                      // 3000 — lobby AC (TssdkSys)
    CSID_SEC_BATTLE_ANTI_DATA_REPORT_REQ = 0xBEAu,   // 3050 — battle AC
    CSID_SEC_LIGHT_ANTI_DATA_REPORT_REQ = 0x32AAu,   // 12970 — SenSecLightAntiDataReportMsg
};

bool hook_stub() { return false; }

static bool (*old_SendLobbyMsg)(void *, void *, bool) = nullptr;
static bool (*old_LNetwork_SendGameMsg)(void *, void *) = nullptr;

static uint32_t msg_id_from_ref(void *pkgRef) {
    if (!pkgRef) return 0;
    void *pkg = *(void **)pkgRef;
    if (!pkg) return 0;
    const size_t offHead = GET_FIELD("AovTdr.dll", "CSProtocol", "CSPkg", "stPkgHead");
    const size_t offMsgId = GET_FIELD("AovTdr.dll", "CSProtocol", "CSPkgHead", "dwMsgID");
    if (!offHead || !offMsgId) return 0;
    void *head = *(void **)((uintptr_t)pkg + offHead);
    return head ? *(uint32_t *)((uintptr_t)head + offMsgId) : 0;
}

static std::string cs_cmd_name(uint32_t id) {
    Class *cls = FindClass(OBF("CSProtocol.CSCMD_ID_DEF"));
    if (!cls) return std::to_string(id);
    const std::string name = cls->get_enum_name(id);
    return name.empty() ? std::to_string(id) : name;
}

static void log_block_egress(const char *via, uint32_t id) {
    LOGI("[LienQuan] block %s %s (id=%u 0x%X)", via, cs_cmd_name(id).c_str(), id, id);
}

bool hook_SendLobbyMsg(void *self, void *msgRef, bool showAlert) {
    const uint32_t id = msg_id_from_ref(msgRef);
    if (id == CSID_ANTIDATA_REQ || id == CSID_SEC_BATTLE_ANTI_DATA_REPORT_REQ || id == CSID_SEC_LIGHT_ANTI_DATA_REPORT_REQ) {
        log_block_egress("SendLobbyMsg", id);
        return false;
    }
    return old_SendLobbyMsg ? old_SendLobbyMsg(self, msgRef, showAlert) : false;
}

bool hook_LNetwork_SendGameMsg(void *self, void *msgRef) {
    const uint32_t id = msg_id_from_ref(msgRef);
    if (id == CSID_RELAYHASHCHECK) {
        log_block_egress("LNetwork.SendGameMsg", id);
        return false;
    }
    return old_LNetwork_SendGameMsg ? old_LNetwork_SendGameMsg(self, msgRef) : false;
}

} // namespace

inline void install() {
    static void *orig = nullptr;

    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDK", "GetReportData", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDK", "GetReportData2", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDK", "GetReportData3", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDK", "GetReprotData4Status", 1), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDK", "OnRecvSignature", 4), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDK", "GetReportData4", 1), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "TssSdkCom", "CreateSecBattleAntiDataReportMsg", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDKBroadCaster", "OpenPipe", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDKBroadCaster", "RecvMsg", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDKBroadCaster", "RegistListener", 1), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "GCloud.AnoSDK", "AnoSDKBroadCaster/IListener", "OnRecvMsg", 2), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LSynchrReport", "OnConstruct", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LSynchrReport", "FillReport", 1), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LSynchrReport", "Bind", 1), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LSynchrReport", ".ctor", 0), (void *)hook_stub, &orig);
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LStateSynchr", "OnConstruct", 0), (void *)hook_stub, &orig);

    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.Framework", "NetworkModule", "SendLobbyMsg", 2), (void *)hook_SendLobbyMsg, (void **)&old_SendLobbyMsg);
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LNetwork", "SendGameMsg", 1), (void *)hook_LNetwork_SendGameMsg, (void **)&old_LNetwork_SendGameMsg);

    LOGI("[LienQuan] il2cpp bypass: AnoSDK/TssSdk/LSynchr + SendLobbyMsg(filter) + LNetwork.SendGameMsg(0x507)");
}

} // namespace il2cpp_bypass
} // namespace lienquan
