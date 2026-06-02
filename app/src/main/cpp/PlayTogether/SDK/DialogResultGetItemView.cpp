#include "DialogResultGetItemView.h"
#include <API/Il2CppApi.h>
#include "Config/Config.h"
#include <Tools/Tools.h>
#include "kittymemory/MemoryPatch.h"
#include <vector>
#include <unordered_set>

namespace DialogResultGetItemView {
    Class *get_class() {
        return FindClass("DialogResultGetItemView");
    }

    void Update() {
        RATE_LIMIT(1500); // check mỗi 500ms
        if (!gPLConfig.general.isMoHopQua) return;

        Object *Self = DialogResultGetItemView::get_class()->get_static_field_value<Object *>(OBF("Self"));
        if (Self && Self->invoke_method<bool>("get_IsVisible")) {
            Self->invoke_method<void>("OnClick_ButtonSkip");
            Self->invoke_method<void>("OnPress_GachaBoxOpen");
        }
    }
}

