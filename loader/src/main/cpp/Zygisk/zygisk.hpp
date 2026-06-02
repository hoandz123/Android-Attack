// Đây là API công khai cho các mô-đun Zygisk.
// KHÔNG SỬA ĐỔI BẤT KỲ MÃ NÀO TRONG HEADER NÀY.

#pragma once

#include <jni.h>

#define ZYGISK_API_VERSION 2

/*

Xác định một lớp và kế thừa zygisk::ModuleBase để triển khai chức năng của mô-đun của bạn.
Sử dụng macro REGISTER_ZYGISK_MODULE(className) để đăng ký lớp đó với Zygisk.

Xin lưu ý rằng các mô-đun sẽ chỉ được tải sau khi zygote đã phân nhánh (fork) tiến trình con.
ĐIỀU NÀY CÓ NGHĨA LÀ TẤT CẢ MÃ CỦA BẠN CHẠY TRONG TIẾN TRÌNH ỨNG DỤNG/MÁY CHỦ HỆ THỐNG, KHÔNG PHẢI LÀ ZYGOTE DAEMON!

Mã ví dụ:

static jint (*orig_logger_entry_max)(JNIEnv *env);
static jint my_logger_entry_max(JNIEnv *env) { return orig_logger_entry_max(env); }

static void example_handler(int socket) { ... }

class ExampleModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }
    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        JNINativeMethod methods[] = {
            { "logger_entry_max_payload_native", "()I", (void*) my_logger_entry_max },
        };
        api->hookJniNativeMethods(env, "android/util/Log", methods, 1);
        *(void **) &orig_logger_entry_max = methods[0].fnPtr;
    }
private:
    zygisk::Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(ExampleModule)

REGISTER_ZYGISK_COMPANION(example_handler)

*/

namespace zygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

class ModuleBase {
public:

    // Hàm này được gọi khi mô-đun được tải vào tiến trình đích.
    // Một handle API Zygisk sẽ được gửi dưới dạng đối số; hãy gọi các hàm tiện ích hoặc giao tiếp
    // với Zygisk thông qua handle này.
    virtual void onLoad([[maybe_unused]] Api *api, [[maybe_unused]] JNIEnv *env) {}

    // Hàm này được gọi trước khi tiến trình ứng dụng được chuyên biệt hóa.
    // Tại thời điểm này, tiến trình vừa được phân nhánh từ zygote, nhưng chưa có chuyên biệt hóa
    // dành riêng cho ứng dụng nào được áp dụng. Điều này có nghĩa là tiến trình không có bất kỳ
    // hạn chế sandbox nào và vẫn chạy với cùng đặc quyền của zygote.
    //
    // Tất cả các đối số sẽ được gửi và sử dụng để chuyên biệt hóa ứng dụng được truyền dưới dạng
    // một đối tượng AppSpecializeArgs duy nhất. Bạn có thể đọc và ghi đè các đối số này để thay
    // đổi cách tiến trình ứng dụng sẽ được chuyên biệt hóa.
    //
    // Nếu bạn cần chạy một số hoạt động với quyền siêu người dùng, bạn có thể gọi Api::connectCompanion()
    // để có một socket thực hiện các cuộc gọi IPC với một tiến trình đồng hành gốc (root companion process).
    // Xem Api::connectCompanion() để biết thêm thông tin.
    virtual void preAppSpecialize([[maybe_unused]] AppSpecializeArgs *args) {}

    // Hàm này được gọi sau khi tiến trình ứng dụng được chuyên biệt hóa.
    // Tại thời điểm này, tiến trình có tất cả các hạn chế sandbox được bật cho ứng dụng này.
    // Điều này có nghĩa là hàm này chạy với cùng đặc quyền của mã của chính ứng dụng.
    virtual void postAppSpecialize([[maybe_unused]] const AppSpecializeArgs *args) {}

    // Hàm này được gọi trước khi tiến trình máy chủ hệ thống được chuyên biệt hóa.
    // Xem preAppSpecialize(args) để biết thêm thông tin.
    virtual void preServerSpecialize([[maybe_unused]] ServerSpecializeArgs *args) {}

    // Hàm này được gọi sau khi tiến trình máy chủ hệ thống được chuyên biệt hóa.
    // Tại thời điểm này, tiến trình chạy với đặc quyền của system_server.
    virtual void postServerSpecialize([[maybe_unused]] const ServerSpecializeArgs *args) {}
};

struct AppSpecializeArgs {
    // Các đối số bắt buộc. Các đối số này được đảm bảo tồn tại trên tất cả các phiên bản Android.
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jstring &instruction_set;
    jstring &app_data_dir;

    // Các đối số tùy chọn. Vui lòng kiểm tra xem con trỏ có phải là null không trước khi hủy tham chiếu
    jboolean *const is_child_zygote;
    jboolean *const is_top_app;
    jobjectArray *const pkg_data_info_list;
    jobjectArray *const whitelisted_data_info_list;
    jboolean *const mount_data_dirs;
    jboolean *const mount_storage_dirs;

    AppSpecializeArgs() = delete;
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jlong &permitted_capabilities;
    jlong &effective_capabilities;

    ServerSpecializeArgs() = delete;
};

namespace internal {
struct api_table;
template <class T> void entry_impl(api_table *, JNIEnv *);
}

// Các giá trị này được sử dụng trong Api::setOption(Option)
enum Option : int {
    // Buộc các quy trình hủy gắn kết (unmount) danh sách từ chối (denylist) của Magisk chạy trên tiến trình này.
    //
    // Việc đặt tùy chọn này chỉ có ý nghĩa trong preAppSpecialize.
    // Việc hủy gắn kết thực tế xảy ra trong quá trình chuyên biệt hóa tiến trình ứng dụng.
    //
    // Đặt tùy chọn này để buộc tất cả các tệp của Magisk và mô-đun phải được hủy gắn kết khỏi
    // không gian tên gắn kết của tiến trình, bất kể trạng thái thực thi danh sách từ chối.
    FORCE_DENYLIST_UNMOUNT = 0,

    // Khi tùy chọn này được đặt, thư viện của mô-đun của bạn sẽ bị dlclose-ed sau post[XXX]Specialize.
    // Hãy lưu ý rằng sau khi dlclose-ing mô-đun của bạn, tất cả mã của bạn sẽ bị hủy ánh xạ khỏi bộ nhớ.
    // BẠN KHÔNG ĐƯỢC BẬT TÙY CHỌN NÀY SAU KHI HOOK BẤT KỲ HÀM NÀO TRONG TIẾN TRÌNH.
    DLCLOSE_MODULE_LIBRARY = 1,
};

// Mặt nạ bit của giá trị trả về của Api::getFlags()
enum StateFlag : uint32_t {
    // Người dùng đã cấp quyền truy cập root cho tiến trình hiện tại
    PROCESS_GRANTED_ROOT = (1u << 0),

    // Tiến trình hiện tại đã được thêm vào danh sách từ chối
    PROCESS_ON_DENYLIST = (1u << 1),
};

// Tất cả các hàm API sẽ ngừng hoạt động sau post[XXX]Specialize vì Zygisk sẽ được dỡ bỏ
// khỏi tiến trình chuyên biệt hóa sau đó.
struct Api {

    // Kết nối với một tiến trình đồng hành gốc và nhận một socket miền Unix cho IPC.
    //
    // API này chỉ hoạt động trong các hàm pre[XXX]Specialize do các hạn chế của SELinux.
    //
    // Các hàm pre[XXX]Specialize chạy với cùng đặc quyền của zygote.
    // Nếu bạn muốn thực hiện một số hoạt động với quyền siêu người dùng, hãy đăng ký một hàm xử lý
    // sẽ được gọi trong tiến trình gốc với REGISTER_ZYGISK_COMPANION(func).
    // Một trường hợp sử dụng tốt khác cho tiến trình đồng hành là nếu bạn muốn chia sẻ một số tài nguyên
    // trên nhiều tiến trình, hãy giữ các tài nguyên trong tiến trình đồng hành và chuyển chúng qua.
    //
    // Tiến trình đồng hành gốc nhận biết được ABI; nghĩa là, khi gọi hàm này từ một tiến trình 32-bit,
    // bạn sẽ được kết nối với một tiến trình đồng hành 32-bit và ngược lại cho 64-bit.
    //
    // Trả về một bộ mô tả tệp cho một socket được kết nối với socket được truyền cho trình xử lý
    // yêu cầu đồng hành của mô-đun của bạn. Trả về -1 nếu nỗ lực kết nối không thành công.
    int connectCompanion();

    // Lấy bộ mô tả tệp của thư mục gốc của mô-đun hiện tại.
    //
    // API này chỉ hoạt động trong các hàm pre[XXX]Specialize.
    // Việc truy cập vào thư mục được trả về chỉ có thể thực hiện được trong các hàm pre[XXX]Specialize
    // hoặc trong tiến trình đồng hành gốc (giả sử bạn đã gửi fd qua socket).
    // Cả hai hạn chế đều do SELinux và UID.
    //
    // Trả về -1 nếu có lỗi xảy ra.
    int getModuleDir();

    // Đặt các tùy chọn khác nhau cho mô-đun của bạn.
    // Xin lưu ý rằng hàm này chấp nhận một tùy chọn duy nhất tại một thời điểm.
    // Kiểm tra zygisk::Option để biết danh sách đầy đủ các tùy chọn có sẵn.
    void setOption(Option opt);

    // Lấy thông tin về tiến trình hiện tại.
    // Trả về các giá trị zygisk::StateFlag được OR bit.
    uint32_t getFlags();

    // Hook các phương thức gốc JNI cho một lớp
    //
    // Tra cứu tất cả các phương thức gốc JNI đã đăng ký và thay thế nó bằng các hàm của riêng bạn.
    // Con trỏ hàm ban đầu sẽ được lưu trong fnPtr của mỗi JNINativeMethod.
    // Nếu không tìm thấy lớp, tên phương thức hoặc chữ ký phù hợp, JNINativeMethod.fnPtr cụ thể đó
    // sẽ được đặt thành nullptr.
    void hookJniNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *methods, int numMethods);

    // Đối với các ELF được tải trong bộ nhớ khớp với `regex`, hãy thay thế hàm `symbol` bằng `newFunc`.
    // Nếu `oldFunc` không phải là nullptr, con trỏ hàm ban đầu sẽ được lưu vào `oldFunc`.
    void pltHookRegister(const char *regex, const char *symbol, void *newFunc, void **oldFunc);

    // Đối với các ELF được tải trong bộ nhớ khớp với `regex`, hãy loại trừ các hook đã đăng ký cho `symbol`.
    // Nếu `symbol` là nullptr, thì tất cả các ký hiệu sẽ bị loại trừ.
    void pltHookExclude(const char *regex, const char *symbol);

    // Cam kết tất cả các hook đã được đăng ký trước đó.
    // Trả về false nếu có lỗi xảy ra.
    bool pltHookCommit();

private:
    internal::api_table *impl;
    template <class T> friend void internal::entry_impl(internal::api_table *, JNIEnv *);
};

// Đăng ký một lớp làm mô-đun Zygisk

#define REGISTER_ZYGISK_MODULE(clazz) \
extern "C" __attribute__((visibility("default"), used)) \
void zygisk_module_entry(zygisk::internal::api_table *table, JNIEnv *env) { \
    zygisk::internal::entry_impl<clazz>(table, env);                        \
}

// Đăng ký một hàm xử lý yêu cầu đồng hành gốc cho mô-đun của bạn
//
// Hàm chạy trong một tiến trình daemon siêu người dùng và xử lý yêu cầu đồng hành gốc từ
// mô-đun của bạn đang chạy trong một tiến trình đích. Hàm phải chấp nhận một giá trị số nguyên,
// là một socket được kết nối với tiến trình đích.
// Xem Api::connectCompanion() để biết thêm thông tin.
//
// LƯU Ý: hàm có thể chạy đồng thời trên nhiều luồng.
// Hãy cẩn thận với các điều kiện tranh đua nếu bạn có một tài nguyên được chia sẻ toàn cục.

#define REGISTER_ZYGISK_COMPANION(func) \
void zygisk_companion_entry(int client) { func(client); }

/************************************************************************************
 * Tất cả mã sau điểm này là mã nội bộ được sử dụng để giao tiếp với Zygisk
 * và đảm bảo tính ổn định của ABI. Bạn không cần phải hiểu nó đang làm gì.
 ************************************************************************************/

namespace internal {

struct module_abi {
    long api_version;
    ModuleBase *_this;

    void (*preAppSpecialize)(ModuleBase *, AppSpecializeArgs *);
    void (*postAppSpecialize)(ModuleBase *, const AppSpecializeArgs *);
    void (*preServerSpecialize)(ModuleBase *, ServerSpecializeArgs *);
    void (*postServerSpecialize)(ModuleBase *, const ServerSpecializeArgs *);

    module_abi(ModuleBase *module) : api_version(ZYGISK_API_VERSION), _this(module) {
        preAppSpecialize = [](auto self, auto args) { self->preAppSpecialize(args); };
        postAppSpecialize = [](auto self, auto args) { self->postAppSpecialize(args); };
        preServerSpecialize = [](auto self, auto args) { self->preServerSpecialize(args); };
        postServerSpecialize = [](auto self, auto args) { self->postServerSpecialize(args); };
    }
};

struct api_table {
    // 2 mục đầu tiên này là vĩnh viễn, sẽ không bao giờ thay đổi
    void *_this;
    bool (*registerModule)(api_table *, module_abi *);

    // Các hàm tiện ích
    void (*hookJniNativeMethods)(JNIEnv *, const char *, JNINativeMethod *, int);
    void (*pltHookRegister)(const char *, const char *, void *, void **);
    void (*pltHookExclude)(const char *, const char *);
    bool (*pltHookCommit)();

    // Các hàm Zygisk
    int  (*connectCompanion)(void * /* _this */);
    void (*setOption)(void * /* _this */, Option);
    int  (*getModuleDir)(void * /* _this */);
    uint32_t (*getFlags)(void * /* _this */);
};

template <class T>
void entry_impl(api_table *table, JNIEnv *env) {
    ModuleBase *module = new T();
    if (!table->registerModule(table, new module_abi(module)))
        return;
    auto api = new Api();
    api->impl = table;
    module->onLoad(api, env);
}

} // namespace internal

inline int Api::connectCompanion() {
    return impl->connectCompanion ? impl->connectCompanion(impl->_this) : -1;
}
inline int Api::getModuleDir() {
    return impl->getModuleDir ? impl->getModuleDir(impl->_this) : -1;
}
inline void Api::setOption(Option opt) {
    if (impl->setOption) impl->setOption(impl->_this, opt);
}
inline uint32_t Api::getFlags() {
    return impl->getFlags ? impl->getFlags(impl->_this) : 0;
}
inline void Api::hookJniNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *methods, int numMethods) {
    if (impl->hookJniNativeMethods) impl->hookJniNativeMethods(env, className, methods, numMethods);
}
inline void Api::pltHookRegister(const char *regex, const char *symbol, void *newFunc, void **oldFunc) {
    if (impl->pltHookRegister) impl->pltHookRegister(regex, symbol, newFunc, oldFunc);
}
inline void Api::pltHookExclude(const char *regex, const char *symbol) {
    if (impl->pltHookExclude) impl->pltHookExclude(regex, symbol);
}
inline bool Api::pltHookCommit() {
    return impl->pltHookCommit != nullptr && impl->pltHookCommit();
}

} // namespace zygisk

extern "C"
[[gnu::visibility("default")]] [[gnu::used]]
void zygisk_module_entry(zygisk::internal::api_table *, JNIEnv *);
