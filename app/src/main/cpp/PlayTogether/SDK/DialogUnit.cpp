#include "PlayLog.h"
#include "DialogUnit.h"
#include <API/Il2CppApi.h>
#include "enum/eDialogType.h"
#include "UILabel.h"
#include "FrameWork.h"
#include "KhoiPhucTrangThai.h"

namespace DialogUnit {
    Class *get_class() {
        return FindClass("DialogUnit");
    }
    void (*old_DialogShow)(Object *thiz);
    void DialogShow(Object *thiz) {
        old_DialogShow(thiz);
        eDialogType type = thiz->invoke_method<eDialogType>("get_DialogType");
        switch (type) {
            case eDialogType::WebView: {
                thiz->invoke_method<void>("CloseHelpWindow");
                break;
            }
            case eDialogType::BoxMessage: {
                Object *LabelTitle = thiz->get_field_object<Object *>("LabelTitle");
                Object *TextListMsg = thiz->get_field_object<Object *>("TextListMsg");
                std::string title;
                std::string msg;
                if (LabelTitle) {
                    title = UILabel::get_mText(LabelTitle);
                }
                if (TextListMsg) {
                    Object *textLabel = TextListMsg->get_field_object<Object *>("textLabel");
                    if (textLabel) {
                        msg = UILabel::get_mText(textLabel);
                    }
                }
                if (msg.find("kết nối") != std::string::npos || title.find("Kết nối") != std::string::npos) {
                    KhoiPhucTrangThai::SystemRestart();
                    FrameWork::SystemRestart();
                }
                break;
            }
            default:
                LOGI("DialogShow: type=%s", FindClass("eDialogType")->get_enum_name(type).c_str());
                break;
        }
    }
}
