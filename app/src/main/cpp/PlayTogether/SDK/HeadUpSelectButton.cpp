#include "PlayLog.h"
#include "HeadUpSelectButton.h"
#include "Config/Config.h"
#include <Tools/Tools.h>

namespace HeadUpSelectButton {
    Class *get_class() {
        return FindClass("HeadUpSelectButton");
    }

    void (*old_SetSprite)(Object *instance, String *spriteName);
    void SetSprite(Object *instance, String *spriteName) {
        old_SetSprite(instance, spriteName);
    }

    void (*old_UpdatePosition)(Object *instance);
    void UpdatePosition(Object *instance) {
        old_UpdatePosition(instance);
    }
}
