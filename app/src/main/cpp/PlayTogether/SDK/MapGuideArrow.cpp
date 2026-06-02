#include "MapGuideArrow.h"
#include "Config/Config.h"
#include "KinematicCharacterMotor.h"

namespace MapGuideArrow {
    Class *get_class() {
        return FindClass("MapGuideArrow");
    }

    void (*old_Update)(Object *instance);
    void Update(Object *instance) {
        old_Update(instance);
    }
}
