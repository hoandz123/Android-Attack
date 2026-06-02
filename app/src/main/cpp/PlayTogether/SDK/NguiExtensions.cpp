#include "NguiExtensions.h"
#include "UILabel.h"
#include <string>

namespace NguiExtensions {
    Class *get_class() {
        return FindClass("NguiExtensions");
    }
    void SetLabelReplacelineToSpace(Object *label, int index) {
        get_class()->find_method("SetLabelReplacelineToSpace", 2)->static_invoke<void>(label, index);
    }
    std::string GetNameIDText(int nameID) {
        Object *label = UILabel::get_UILabel();
        if (!label || !nameID) return "";
        std::string textdf = UILabel::get_mText(label);
        SetLabelReplacelineToSpace(label, nameID);
        std::string text = UILabel::get_mText(label);
        UILabel::set_mText(label, String::Create(textdf.c_str()));
        return text;
    }
}

