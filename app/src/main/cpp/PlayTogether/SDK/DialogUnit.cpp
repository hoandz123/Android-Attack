#include "PlayLog.h"
#include "DialogUnit.h"
#include <API/Il2CppApi.h>
#include "enum/eDialogType.h"
#include "JsonConvert.h"
#include "enum/Item_Type.h"
#include "NguiExtensions.h"
#include "Config/Config.h"
#include "UILabel.h"
#include "FrameWork.h"
#include "KhoiPhucTrangThai.h"
#include "kittymemory/MemoryPatch.h"
#include <Tools/Tools.h>
namespace DialogUnit {
    Class *get_class() {
        return FindClass("DialogUnit");
    }
    void (*old_DialogShow)(Object *thiz);
    void DialogShow(Object *thiz) {
        old_DialogShow(thiz);
        eDialogType type = thiz->invoke_method<eDialogType>("get_DialogType");
        switch (type) {
            case eDialogType::FishingGetItem: { //Bảo Quản
                auto _item = thiz->get_field_object<Object *>("_item");
                if (!_item) return;

                auto _sellItem = thiz->get_field_object<Object *>("_sellItem");
                if (_sellItem) {
                    LOGI("FishingGetItem: _sellItem: %s", JsonConvert::SerializeObject(_sellItem)->to_string().c_str());
                } else {
                    LOGI("FishingGetItem: _sellItem is null");
                }
                auto _sellPrice = thiz->get_field_value<int>("_sellPrice");
                LOGI("FishingGetItem: _sellPrice: %d", _sellPrice);

                LOGI("FishingGetItem: %s", JsonConvert::SerializeObject(_item)->to_string().c_str());

                int grade = _item->get_field_value<int>("<Grade>k__BackingField");
                Item_Type ItemType = _item->get_field_value<Item_Type>("<ItemType>k__BackingField");
                int ItemId = _item->get_field_value<int>("<ItemId>k__BackingField");
                int NameId = _item->get_field_value<int>("<NameId>k__BackingField");
                std::string name = NguiExtensions::GetNameIDText(NameId);

//                if (ItemType == Item_Type::Fish || ItemType ==  Item_Type::Junk) {
//
//                    if (PLConfig::FishingConfig::curFishLevel > 0 && PLConfig::FishingConfig::curFishShadowLevel > 0) {
//                        nlohmann::json data;
//                        static std::string versionName = JNIHelper::GetVersionName();
//                        data[OBFS("id")] = PLConfig::FishingConfig::curFishLevel;
//                        data[OBFS("grade")] = grade;
//                        data[OBFS("ItemType")] = ItemType;
//                        data[OBFS("ItemId")] = ItemId;
//                        data[OBFS("NameId")] = NameId;
//                        data[OBFS("name")] = name;
//                        data[OBFS("version")] = versionName;
//                        std::string url = OBFS("https://hackviet.io/api/fish");
//                        std::string body = data.dump();
//                        std::thread([url, body]() {
//                            try {
//                                HttpClient client;
//                                client.url(url).header(OBFS("Content-Type: application/json"))
//                                        .post(body).perform();
//                                if (client.code() == 200) {
//                                    LOGD("Upload Done");
//                                } else {
//                                    LOGE("Upload Failed: HTTP %ld", client.code());
//                                }
//                            } catch (const std::exception &e) {
//                                LOGE("HttpClient error: %s", e.what());
//                            }
//                        }).detach();
//                    }
//                }

                LOGI("FishingGetItem: name=%s, grade=%d, ItemType=%d, ItemId=%d, NameId=%d", name.c_str(), grade, ItemType, ItemId, NameId);

                if (gPLConfig.general.isBaoQuan) {
                    thiz->invoke_method<void>("OnClick_ButtonClose");
                } else if (gPLConfig.general.isBanGoi) {
                    if (gPLConfig.general.isDuB67 && PLConfig::FishingConfig::curFishShadowLevel > 5) {
                    } else if (gPLConfig.general.isDuNenVip && grade > 3) {
                    } else if (thiz->invoke_method<bool>("CheckItemTypeForMembershipPopup", ItemType)) {
                        static bool isModified = false;
                        if (!isModified) {
                            isModified = true;
                            for (auto m: {OBFS("IsFindGradeItem"), OBFS("IsFindFishItem"), OBFS("IsFindInsectItem")}) {
                                MemoryPatch::createWithHex((uintptr_t) IL2Cpp::Il2CppGetMethodOffset(OBF("DialogFishingGetItem"), m.c_str(), 1), HEX_RET_FALSE).Modify();
                            }
                        }
                        thiz->invoke_method<void>("OnClick_Selling");
                    }
                    thiz->invoke_method<void>("OnClick_ButtonClose");
                }

                if (ItemType == Item_Type::Fish) {
                    PLConfig::FishingConfig::gFishLogger.markSuccess((int) ItemType, ItemId, NameId, name);
                }

                break;
            }
//            case eDialogType::ResultGetItemView: {
////                Object* runBT = thiz->get_field_object<Object*>("runBT");
////                if (runBT) {
////                    LOGD("ResultGetItemView: EndBT");
////                    thiz->invoke_method<void>("EndBT", runBT);
////                }
//                thiz->invoke_method<void>("OnGachaBoxEndAction");
//                thiz->invoke_method<void>("OnClick_ButtonSkip");
//                thiz->invoke_method<void>("OnPress_GachaBoxOpen");
//                thiz->invoke_method<void>("OnClick_ButtonRevealItem");
//                break;
//            }
            case eDialogType::ResultGetItemList: {
                if (gPLConfig.general.isMoHopQua) {
                    thiz->invoke_method<void>("OnClick_ButtonOk");
                }
                break;
            }

            case eDialogType::WebView: {
                thiz->invoke_method<void>("CloseHelpWindow");
                break;
            }
            case eDialogType::GetItemRoulette: {
                int _currentState = thiz->get_field_value<int>("_currentState");
                if (_currentState == 0) {
                    thiz->invoke_method<void>("OnClick_Start");
                    thiz->invoke_method<void>("OnClick_Close");
                }
                break;
            }

            case eDialogType::BoxMessage: {
                //class DialogBoxMessageNgui : public UILabel LabelTitle; // 0x128
                //class DialogBoxMessageNgui : public NGUI_TextList TextListMsg; // 0x130
                //class UITextList : public UILabel textLabel; // 0x20

                Object *LabelTitle = thiz->get_field_object<Object *>("LabelTitle");
                Object *TextListMsg = thiz->get_field_object<Object *>("TextListMsg");
                std::string title;
                std::string msg;
                if (LabelTitle) {
                    title = UILabel::get_mText(LabelTitle);
                    LOGI("DialogBoxMessage: Title: %s", title.c_str());
                }
                if (TextListMsg) {
                    Object *textLabel = TextListMsg->get_field_object<Object *>("textLabel");
                    if (textLabel) {
                        msg = UILabel::get_mText(textLabel);
                        LOGI("DialogBoxMessage: Msg: %s", msg.c_str());
                    }
                }

                if (title.find("Hoàn tất bán hàng") != std::string::npos) {
                    LOGD("DialogBoxMessage::InitMessage - OnClick_Close");
                    thiz->invoke_method<void>("OnClick_Close");
                    return;
                }
                if (msg.find("kết nối") != std::string::npos || title.find("Kết nối") != std::string::npos) {
                    LOGD("DialogBoxMessage::InitMessage - OnClick_Close");
                    KhoiPhucTrangThai::SystemRestart();
                    FrameWork::SystemRestart();
                }
                if (msg.find("Không thể thực hiện") != std::string::npos
                    || msg.find("Lỗi không xác định") != std::string::npos
                    || msg.find("Yêu cầu không hợp lệ") != std::string::npos
                    || msg.find("Cây phải lớn lên mới thu hoạch được") != std::string::npos
                    ) {
                    LOGD("DialogBoxMessage::InitMessage - OnClick_Close");
                    thiz->invoke_method<void>("OnClick_Close");
                }
                break;
            }

            default:
                LOGI("DialogShow: type=%s", FindClass("eDialogType")->get_enum_name(type).c_str());
                break;
        }
    }
}

