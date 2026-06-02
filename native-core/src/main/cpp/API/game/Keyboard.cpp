#include "Keyboard.h"
#include "API/Il2CppApi.h"
#include "string"
#include "thread"

enum class TouchScreenKeyboardType : int {
    Default = 0, ASCIICapable, NumbersAndPunctuation, URL, NumberPad,
    PhonePad, NamePhonePad, EmailAddress, NintendoNetworkAccount,
    Social, Search, DecimalPad, OneTimeCode
};

namespace Keyboard {
    bool check;
    Il2CppObject*                    inst   = nullptr;
    std::function<void(std::string)> cb;


    void Open(const char *txt, const std::function<void(const std::string &)> &c) {
        Domain* domain = il2cpp_domain_get();
        domain->attach_thread();
        if (inst) {
            Reset();
        }
        cb   = std::move(c);
        static auto Open = (Il2CppObject* (*)(...)) GET_METHOD("UnityEngine.CoreModule.dll", "UnityEngine", "TouchScreenKeyboard", "Open", 8);
        inst = Open(String::Create(txt), TouchScreenKeyboardType::Default, 0, 0, 0, 0, String::Create(""), 0);
    }

    void Reset() {
        if (!inst) return;
        inst->invoke_method<void>("Destroy");
        inst->invoke_method<void>("Finalize");
        inst = nullptr;
    }

    void Update() {
        if (!inst) return;
        auto status = inst->invoke_method<TouchScreenKeyboardStatus>("get_status");
        if (status == TouchScreenKeyboardStatus::Done) {
            Il2CppString* s = inst->invoke_method<Il2CppString*>(OBF("get_text"));
            if (cb) cb(s ? s->to_string() : std::string{});
            Reset();
            SDKLOGD("Keyboard Done");
        }
        else if (status != TouchScreenKeyboardStatus::Visible) {
            Reset();
            SDKLOGD("Keyboard Canceled or LostFocus");
        }
    }
}