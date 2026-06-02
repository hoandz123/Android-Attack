#include "PlayLog.h"
#include "MailSystem.h"
#include "Config/Config.h"
#include "CacheSystem.h"
#include "NetNativeProtocol.h"
#include <Tools/Tools.h>

namespace MailSystem {
    void Update() {
        RATE_LIMIT(5000);
        if (!gPLConfig.general.isNhanThu) return;

        Object *cachePost = CacheSystem::get_CachePost();
        if (!cachePost) return;
        if (!cachePost->invoke_method<bool>("get_NewMailCheck")) return;

        List<Object *> *userMails = cachePost->get_field_object<List<Object *> *>("userMails");
        if (!userMails || userMails->get_Count() < 1) return;

        Class *mailChoiceListClass = FindClass("System.Collections.Generic.List<PlayTogether.MailChoiceInfo>");
        Class *mailChoiceClass = FindClass("PlayTogether.MailChoiceInfo");
        if (!mailChoiceListClass || !mailChoiceClass) {
            LOGE("MailSystem: MailChoiceInfo types not found");
            return;
        }

        List<Object *> *choiceList = (List<Object *> *) mailChoiceListClass->new_object();
        if (!choiceList) return;

        int mailCount = userMails->get_Count();
        int addCount = 0;
        for (int i = 0; i < mailCount; i++) {
            Object *mail = userMails->get_item(i);
            if (!mail) continue;

            Object *choice = mailChoiceClass->new_object();
            if (!choice) continue;
            int64_t seq = mail->invoke_method<int64_t>("get_seq");
            uint32_t choiceVal = mail->invoke_method<uint32_t>("get_Choice");
            choice->invoke_method<void>("set_Seq", seq);
            choice->invoke_method<void>("set_Choice", choiceVal);
            choiceList->Add(choice);
            addCount++;
        }

        if (addCount < 1) return;
        LOGI("SendToMailReceive count=%d", addCount);
        NetNativeProtocol::SendToMailReceive(choiceList);
    }
}
