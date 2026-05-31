LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Here is the name of your lib.
# When you change the lib name, change also on System.loadLibrary("") under OnCreate method on StaticActivity.java
# Both must have same name
LOCAL_MODULE    := MyLibName

# -std=c++17 is required to support AIDE app with NDK
LOCAL_CFLAGS := -w -Wno-error=format-security -fvisibility=hidden -fpermissive -fexceptions
LOCAL_CPPFLAGS := -w -Wno-error=format-security -fvisibility=hidden -Werror -std=c++17
LOCAL_CPPFLAGS += -Wno-error=c++11-narrowing -fpermissive -Wall -fexceptions
ifeq ($(APP_OPTIM),release)
LOCAL_CFLAGS += -s
LOCAL_CPPFLAGS += -s
LOCAL_LDFLAGS += -Wl,--gc-sections,--strip-all
endif
LOCAL_LDFLAGS += -llog
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ImGui
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ImGui/backends

LOCAL_SRC_FILES := Main.cpp \
	Core/Bootstrap.cpp \
	UI/Overlay.cpp \
	UI/OverlaySurface.cpp \
	UI/ImGuiRenderer.cpp \
	UI/Menu.cpp \
	UI/TouchInput.cpp \
	UI/FontRoboto.cpp \
	ImGui/imgui.cpp \
	ImGui/imgui_demo.cpp \
	ImGui/imgui_draw.cpp \
	ImGui/imgui_tables.cpp \
	ImGui/imgui_widgets.cpp \
	ImGui/backends/imgui_impl_opengl3.cpp \
	Substrate/hde64.c \
	Substrate/SubstrateDebug.cpp \
	Substrate/SubstrateHook.cpp \
	Substrate/SubstratePosixMemory.cpp \
	Substrate/SymbolFinder.cpp \
	KittyMemory/KittyMemory.cpp \
	KittyMemory/MemoryPatch.cpp \
    KittyMemory/KittyUtils.cpp \
	And64InlineHook/And64InlineHook.cpp \

include $(BUILD_SHARED_LIBRARY)
