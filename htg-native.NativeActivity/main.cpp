// dear imgui: standalone example application for Android + OpenGL ES 3
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
#define ALIB_ANDROID
#include "json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_texture.h"
#include "imgui/imgui_uielement.h"
#include "imgui/imgui_format.h"
#include "imgui/imgui_markdown.h"
#include "imgui/imgui_impl_android.h"
#include "imgui/imgui_impl_opengl3.h"

#include "utils.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cwerror.h"


#include "rsm_hooks.h"
#include "rsm_ui_man.h"
#include "rsm_renderer.h"

#include "image_parse_face.h"

#include "ani.h"
#include <zlib.h>

// Data
static EGLDisplay           g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface           g_EglSurface = EGL_NO_SURFACE;
static EGLContext           g_EglContext = EGL_NO_CONTEXT;
static struct android_app* g_App = NULL;
static bool                 g_Initialized = false;
static char                 g_LogTag[] = "ImGuiExample";

// Forward declarations of helper functions
static void AndroidSetWakeLockState(int state);
static int AndroidGetUnicodeChar(int keyCode, int metaState);
static int AndroidKeyboardShown(bool return_height = true);
static void AndroidToggleKeyboard();
static void AndroidHideKeyboard();
static void AndroidShowKeyboard();
static int AndroidKeyboardHeight();
static int AndroidNavigationBarHeight();
static int AndroidStatusBarHeight();
static int AndroidNavigationBarShown(int* out = nullptr);
static int AndroidStatusBarShown(int* out = nullptr);
const char* root_asset_folder = "/storage/emulated/0/avetharun/htg/";
AAssetManager* AndroidGetAssetManager() {
    return g_App->activity->assetManager;
}
ANativeActivity* AndroidGetActivity() {
    return g_App->activity;
}
static const char* ReadRootFileBytes(const char* filename) {
    std::string _f_tmp = root_asset_folder;
    _f_tmp += filename;
    return alib_file_read(filename).c_str();
}
static void WriteRootFileBytes(const char* filename, const char* bytes, size_t len = 0) {
    alib_reqlen(&len, bytes);
    std::string _f_tmp = root_asset_folder;
    _f_tmp += filename;
    alib_file_write(filename, bytes, len);
}
static bool AssetExists(const char* filename) {
    AAsset* asset_descriptor = AAssetManager_open(g_App->activity->assetManager, filename, AASSET_MODE_UNKNOWN);
    if (asset_descriptor) {
        AAsset_close(asset_descriptor);
        return true;
    }
    return false;
}
// Helper to retrieve data placed into the /raw/ directory (res/raw)
static const char* ReadAssetBytes(const char* filename, size_t* num_bytes_out = 0)
{
    char* out_data = nullptr;
    size_t num_bytes = 0;
    AAsset* asset_descriptor = AAssetManager_open(g_App->activity->assetManager, filename, AASSET_MODE_BUFFER);
    if (asset_descriptor)
    {
    retry_read:
        num_bytes = AAsset_getLength(asset_descriptor);
        out_data = new char[num_bytes + 1];
        out_data[num_bytes] = '\0';
        size_t num_bytes_read = AAsset_read(asset_descriptor, out_data, num_bytes);
        AAsset_close(asset_descriptor);
        if (num_bytes != num_bytes_read) {
            alib_LOGW("Asset %s was not read correctly. Bytes expected: %zx, Bytes read: %zx", filename, num_bytes, num_bytes_read);
            goto retry_read;
        }
    }
    else {
        stbi__g_failure_reason = alib_strfmt("Asset %s not found.", filename);
        //alib_LOGE("Asset %s not found.", filename);
        return 0;
    }
    if (num_bytes_out) { *num_bytes_out = num_bytes; }
    return out_data;
}
nlohmann::json* _assets_strings_json__ = nullptr;

static std::string ReadStringAsset(const char* string_id) {
    if (!_assets_strings_json__) {
        if (!AssetExists("strings.json")) {
            return "No string asset.";
        }
        _assets_strings_json__ = alib_malloct(nlohmann::json);
        *_assets_strings_json__ = nlohmann::json::parse(ReadAssetBytes("strings.json"));
    }
    if (_assets_strings_json__->contains(string_id)) {
        return _assets_strings_json__->at(string_id).get<std::string>();
    }
    return "not found";
}
#include <sys/stat.h>

// From assets/
static std::string ReadAPKFileBytes(const char* file, size_t* len = 0) {
    if (AssetExists(file)) {
        FILE* apk_f = android_fopen(file, AndroidGetAssetManager());
        // Determine file size
        fseek(apk_f, 0, SEEK_END);
        size_t size = ftell(apk_f);
        char* out = new char[size + 4];
        rewind(apk_f);
        fread(out, sizeof(char), size, apk_f);
        out[size + 1] = '\0';
        std::string __out(out, size);
        delete[] out;
        if (len) { *len = size; }
        return __out;
    }
    return alib_strfmt("Could not read or open file: %s", file);
}
bool ImGui::LoadTextureFromAPK(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
    size_t bytes_len = 0;
    unsigned char* bytes = (unsigned char*)ReadAPKFileBytes(filename, &bytes_len).c_str();
    if (!bytes) {
        return 0;
    }
    return LoadTextureFromMemory(bytes, bytes_len, out_texture, out_width, out_height);
}
const char* test_asset_bytes;
void init(struct android_app* app)
{
    if (g_Initialized)
        return;

    g_App = app;
    ANativeWindow_acquire(g_App->window);

    // Initialize EGL
    // This is mostly boilerplate code for EGL...
    {
        g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (g_EglDisplay == EGL_NO_DISPLAY)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");

        if (eglInitialize(g_EglDisplay, 0, 0) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglInitialize() returned with an error");

        const EGLint egl_attributes[] = { EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };
        EGLint num_configs = 0;
        if (eglChooseConfig(g_EglDisplay, egl_attributes, nullptr, 0, &num_configs) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned with an error");
        if (num_configs == 0)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned 0 matching config");

        // Get the first matching config
        EGLConfig egl_config;
        eglChooseConfig(g_EglDisplay, egl_attributes, &egl_config, 1, &num_configs);
        EGLint egl_format;
        eglGetConfigAttrib(g_EglDisplay, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
        ANativeWindow_setBuffersGeometry(g_App->window, 0, 0, egl_format);

        const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        g_EglContext = eglCreateContext(g_EglDisplay, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

        if (g_EglContext == EGL_NO_CONTEXT)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglCreateContext() returned EGL_NO_CONTEXT");

        g_EglSurface = eglCreateWindowSurface(g_EglDisplay, egl_config, g_App->window, NULL);
        eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext);
        rsm::HookManager::RunInitGL();
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Disable loading/saving of .ini file from disk.
    // FIXME: Consider using LoadIniSettingsFromMemory() / SaveIniSettingsToMemory() to save in appropriate location for Android.
    io.IniFilename = NULL;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplAndroid_Init(g_App->window);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    // Load Fonts
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 64.0f;
    rsm::Fonts::default_font = io.Fonts->AddFontDefault(&font_cfg);

    void* arial_font_data = (void*)ReadAssetBytes("arial.ttf");
    void* proggy_font_data = (void*)ReadAssetBytes("proggyclean.ttf");
    void* symbols_font_data = (void*)ReadAssetBytes("symbols.ttf");
    rsm::Fonts::arial = io.Fonts->AddFontFromMemoryTTF(arial_font_data, 12, 64);
    rsm::Fonts::proggy = io.Fonts->AddFontFromMemoryTTF(proggy_font_data, 12, 64);
    rsm::Fonts::symbols = io.Fonts->AddFontFromMemoryTTF(symbols_font_data, 24, 64);
    io.Fonts->Build();
    // Arbitrary scale-up
    // FIXME: Put some effort into DPI awareness
    ImGui::GetStyle().ScaleAllSizes(6.0f);

    g_Initialized = true;
    test_asset_bytes = ReadAssetBytes("test.txt");
    size_t uimglen = 0;
    const char* uimgbytes = ReadAssetBytes("icon.png", &uimglen);
    if (uimgbytes) {
        ImGui::UnknownImageTexture = ImGui::ImageTexture::LoadTextureFromMemory(uimgbytes, uimglen);
    }
    // Initialize Android Native Interface
    ImGui::GetStyle().ScrollbarSize = 12;
    rsm::doubletap_detector_ = {};
    rsm::drag_detector_ = {};
    rsm::pinch_detector_ = {};
    io.MouseDoubleClickMaxDist = 128/*px*/;
    ANIEnv::init(g_App->activity->vm, "com.avetharun.rosemary", g_App->activity->clazz, g_App);
    // run starter hooks
    rsm::HookManager::RunInitPost();

}
float _time = 0.0f;
namespace Icon {
    rsm::ImageAsset console_64{};
    rsm::ImageAsset settings{};
    rsm::ImageAsset add_character{};
    rsm::ImageAsset camera{};
    rsm::ImageAsset app{};
};
struct DebugConsole {
    static inline ImGuiTextBuffer debugWindowConsoleText = {};
    // Deletes buffer in sizes of 512 if the size of the buffer exceeds 4012
    static void pushuf(std::string cstr) {
        if (&debugWindowConsoleText == nullptr) { return; }
        debugWindowConsoleText.append(cstr.c_str());
    }
    static void pushf(std::string fmt, ...) {
        if (&debugWindowConsoleText == nullptr) { return; }
        va_list args;
        va_start(args, fmt);
        debugWindowConsoleText.appendfv(fmt.c_str(), args);
        va_end(args);
    }
    static void cwErrorHandler(const char* errs, uint32_t errid) {
        // errid not used
        pushf("%s\n%s", errs, ImRGB::resetstr());
    }

};
static inline float vertices[5 * 6 * 6] = {
    -0.1f, -0.1f, -0.1f,  0.0f, 0.0f,
     0.1f, -0.1f, -0.1f,  0.2f, 0.0f,
     0.1f,  0.1f, -0.1f,  0.2f, 0.2f,
     0.1f,  0.1f, -0.1f,  0.2f, 0.2f,
    -0.1f,  0.1f, -0.1f,  0.0f, 0.2f,
    -0.1f, -0.1f, -0.1f,  0.0f, 0.0f,

    -0.1f, -0.1f,  0.1f,  0.0f, 0.0f,
     0.1f, -0.1f,  0.1f,  0.2f, 0.0f,
     0.1f,  0.1f,  0.1f,  0.2f, 0.2f,
     0.1f,  0.1f,  0.1f,  0.2f, 0.2f,
    -0.1f,  0.1f,  0.1f,  0.0f, 0.2f,
    -0.1f, -0.1f,  0.1f,  0.0f, 0.0f,

    -0.1f,  0.1f,  0.1f,  0.2f, 0.0f,
    -0.1f,  0.1f, -0.1f,  0.2f, 0.2f,
    -0.1f, -0.1f, -0.1f,  0.0f, 0.2f,
    -0.1f, -0.1f, -0.1f,  0.0f, 0.2f,
    -0.1f, -0.1f,  0.1f,  0.0f, 0.0f,
    -0.1f,  0.1f,  0.1f,  0.2f, 0.0f,

     0.1f,  0.1f,  0.1f,  0.2f, 0.0f,
     0.1f,  0.1f, -0.1f,  0.2f, 0.2f,
     0.1f, -0.1f, -0.1f,  0.0f, 0.2f,
     0.1f, -0.1f, -0.1f,  0.0f, 0.2f,
     0.1f, -0.1f,  0.1f,  0.0f, 0.0f,
     0.1f,  0.1f,  0.1f,  0.2f, 0.0f,

    -0.1f, -0.1f, -0.1f,  0.0f, 0.2f,
     0.1f, -0.1f, -0.1f,  0.2f, 0.2f,
     0.1f, -0.1f,  0.1f,  0.2f, 0.0f,
     0.1f, -0.1f,  0.1f,  0.2f, 0.0f,
    -0.1f, -0.1f,  0.1f,  0.0f, 0.0f,
    -0.1f, -0.1f, -0.1f,  0.0f, 0.2f,

    -0.1f,  0.1f, -0.1f,  0.0f, 0.2f,
     0.1f,  0.1f, -0.1f,  0.2f, 0.2f,
     0.1f,  0.1f,  0.1f,  0.2f, 0.0f,
     0.1f,  0.1f,  0.1f,  0.2f, 0.0f,
    -0.1f,  0.1f,  0.1f,  0.0f, 0.0f,
    -0.1f,  0.1f, -0.1f,  0.0f, 0.2f
};
struct RSMI : rsm::GenericHook {
    static inline rsm::Renderer::Camera activecamera;
    struct T {
        static inline nlohmann::json TranslationKeys{};
        static inline bool hasTranslations;
        static void setd(std::string translation_dict_name = "en-us") {
            std::string fname = "translations/" + translation_dict_name + ".json";
            size_t translation_file_len = 0;
            if (!AssetExists(fname.c_str())) {
                cwError::serrof("Translation file %s not found!", fname.c_str());
                return;
            }
            const char* data = ReadAssetBytes(fname.c_str(), &translation_file_len);
            if (!data) { cwError::serrof("Unable to unpack translation file."); return; }

            TranslationKeys = nlohmann::json::parse(data, data + translation_file_len);

        }
        static std::string getk(std::string key) {
            if (TranslationKeys.size() == 0) { return "No translation keys."; }
            if (!TranslationKeys.contains(key)) { return ("No translation key found for ") + key; }
            return alib_j_getstr(TranslationKeys.at(key));
        }
    };
    void InitPost() {
        ImGui::SetCurrentFont(rsm::Fonts::default_font);
    }
    static inline ani::SharedPreferences getSharedPreferences(const char* name = "alib:settings") {
        JNIEnv* jni;
        g_App->activity->vm->AttachCurrentThread(&jni, NULL);
        return ani::SharedPreferences(jni, g_App->activity->clazz, name);
    }
    void Render() {
    }
    
    static inline rsm::Renderer::Shader shader;
    static inline GLuint VBO = 0;
    static inline GLuint VAO = 0;

    void Start() {
        cwError::onError = DebugConsole::cwErrorHandler;
        // get any settings variables
        RSMI::getSharedPreferences().getBoolean("use_usb", &GlobalState::UseSerialUSB, true);
        RSMI::getSharedPreferences().getBoolean("use_websock", &GlobalState::UseSerialWebsocket, false);
        RSMI::getSharedPreferences().getInt("websock_port", &GlobalState::SerialWebsocketPort, 8042);
        cwError::serrof("%sUnpacked shared prefs", ImRGB(0, 255, 0).tostring());
        cwError::serrof("test/test.txt returned %s", ReadAPKFileBytes("test/test.txt").c_str());
        // Eventually, use a dropdown & shared prefs to initialize this! Though that'll take community effort because i'm not bilingual </3
        if (T::TranslationKeys.size() == 0) {
            T::setd("en-us");
        }
        cwError::serrof("Translation key translation_test returned: %s", T::getk("translation_test").c_str());

        shader.compile();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 5 * 6 * 6, vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
    }
    static inline std::array<glm::mat4, 3> matrix = {};
    void PreSwap() {

        ImGuiContext* g = ImGui::GetCurrentContext();
        float font_sz = g->FontSize;
        g->FontSize = font_sz * 0.5f;
        g->DrawListSharedData.FontSize = font_sz * 0.5f;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            cwError::serror("user double tapped");
        }
        g->FontSize = font_sz;
        GlobalState::screen_size = ImGui::GetIO().DisplaySize;
        GlobalState::ui_begin = { ImGui::GetStyle().ItemSpacing.x * 0.01f,(float)AndroidStatusBarHeight() * 0.85f };
        if (rsm::GenericHook::__hook_stack.top()->ShowStatusBar) {
            float status_bar_height = ImGui::CalcTextSize("#").y * 1.55f;
            ImVec2 end_pos = { GlobalState::screen_size.x, GlobalState::ui_begin.y + status_bar_height };
            ImGui::GetForegroundDrawList()->AddLine({ 0,end_pos.y}, end_pos, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
            ImGui::GetBackgroundDrawList()->AddRectFilled({ 0,0 }, end_pos, ImGui::GetColorU32(ImGuiCol_Button));
        }
        shader.use();
        // create transformations
        activecamera.setRotation({ 0,0,180 });
        shader.setMat4("matrix", activecamera.GetFullPerspectiveMatrix());
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    static bool BackArrowModal() { 
        bool retv = false;
        ImGui::PushFont(rsm::Fonts::symbols);
        float fsz_last = ImGui::GetCurrentWindow()->FontWindowScale;
        ImGui::SetWindowFontScale(1.0f);
        ImVec2 tsz = ImGui::CalcTextSize("n");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0,0 });
        ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
        ImGui::Button("n##back");
        retv = ImGui::IsItemClicked();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopFont();
        ImGui::SetWindowFontScale(fsz_last);
        return retv;
    }
    static bool ForwardArrowModal() {
        bool retv = false;
        ImGui::PushFont(rsm::Fonts::symbols);
        float fsz_last = ImGui::GetCurrentWindow()->FontWindowScale;
        ImGui::SetWindowFontScale(1.0f);
        ImVec2 tsz = ImGui::CalcTextSize("m");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0,0 });
        ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
        ImGui::Button("m##forward");
        retv = ImGui::IsItemClicked();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopFont();
        ImGui::SetWindowFontScale(fsz_last);

        return retv;
    }
};
RSM_HOOK_ENABLE(RSMI);
struct ui_impl_debug : rsm::GenericHook {
    void Render() {
        ImGui::BeginFullscreen();
        ImGui::SetCursorPos(GlobalState::ui_begin);
        if (RSMI::BackArrowModal()) {
            this->pop();
        }
        float fsz_last = ImGui::GetCurrentWindow()->FontWindowScale;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0,0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::BeginDragScrollableChild("##debug_console_window", { 0,0 }, false);
        ImGui::SetWindowFontScale(0.5f);
        ImGui::TextMulticolored(DebugConsole::debugWindowConsoleText.c_str());
        ImGui::EndDragScrollableChild();
        ImGui::PopStyleVar(2);
        ImGui::SetWindowFontScale(fsz_last);

        ImGui::EndFullscreen();
    }
};
RSM_HOOK_ENABLE(ui_impl_debug);
struct ui_impl_licenses : rsm::GenericHook {
    const char* __license_text;
    void Start() {
    }
    void Render() {
        if (!__license_text) {
            __license_text = ReadAssetBytes("licenses.txt");
        }
        ImGui::BeginFullscreen();
        ImGui::SetCursorPos(GlobalState::ui_begin);
        if (RSMI::BackArrowModal()) {
            this->pop();
        }
        ImGui::BeginDragScrollableChild("##license_window_pane", { 0,0 }, true);
        float fsz_last = ImGui::GetCurrentWindow()->FontWindowScale;
        ImGui::SetWindowFontScale(0.35f);
        ImGui::TextMulticolored(__license_text);
        ImGui::SetWindowFontScale(fsz_last);
        ImGui::EndDragScrollableChild();
        ImGui::EndFullscreen();
    }
};
RSM_HOOK_ENABLE(ui_impl_licenses);
struct ui_impl_settings : rsm::GenericHook {
    char __itoa_websocket_num[16];
    int e;
    void Start() {
    }
    void Render() {
        ImGui::BeginFullscreen();
        ImGui::SetCursorPos(GlobalState::ui_begin);
        if (RSMI::BackArrowModal()) {
            // Commit any values from the settings dialog into shared prefs
            RSMI::getSharedPreferences()
                .edit()
                .putBoolean("use_usb", GlobalState::UseSerialUSB)
                .putBoolean("use_websock", GlobalState::UseSerialWebsocket)
                .putInt("websock_port", GlobalState::SerialWebsocketPort)
                .commit();
            ;
            this->pop();
        }
        
        ImGui::BeginDragScrollableChild("##settings_window_pane", { 0,0 }, true);
        ImGui::SetCursorPos({ 32, 16 });
        // Begin networking settings
        if (ImGui::CollapsingHeader(RSMI::T::getk("settings_adv_modal").c_str())) {
            if (ani::adb_enabled()) {
                ImGui::ThumbSwitch("##settings_use_serial_USB", &GlobalState::UseSerialUSB, RSMI::T::getk("sett_use_usb").c_str());
            }
            else {
                ImGui::BeginDisabled();
                ImGui::ThumbSwitch("##settings_use_serial_USB", &GlobalState::UseSerialUSB, RSMI::T::getk("sett_use_usb").c_str());
                ImGui::EndDisabled();
                ImGui::PushStyleColor(ImGuiCol_Text, { 185, 0,0,255 });
                ImGui::UrlHTTPText("https://www.digitaltrends.com/mobile/how-to-get-developer-options-on-android/", ani::Networking::openURLBrowser,
                    RSMI::T::getk("sett_adb_required").c_str());
                ImGui::PopStyleColor();

            }
            ImGui::ThumbSwitch("##settings_use_serial_WS", &GlobalState::UseSerialWebsocket, RSMI::T::getk("sett_use_websocket").c_str());
            
            ImGui::PushItemWidth(ImGui::CalcTextSize("######").x);
            if (!GlobalState::UseSerialWebsocket) { ImGui::BeginDisabled(); }
            bool __in_ws_port = ImGui::InputInt("##settings_serial_websocket_port", &GlobalState::SerialWebsocketPort, 0, 0, ImGuiInputTextFlags_CharsDecimal);
            ImGui::PopItemWidth();
            if (__in_ws_port) {
                if (GlobalState::SerialWebsocketPort < 802) {
                    GlobalState::SerialWebsocketPort = 802;
                }
                if (GlobalState::SerialWebsocketPort > 32755) {
                    GlobalState::SerialWebsocketPort = 32755;
                }
            }
            if (!GlobalState::UseSerialWebsocket) { ImGui::EndDisabled(); }
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 512);
        if (ImGui::TextButton("license_btn", RSMI::T::getk("view_licenses_modal").c_str())) {
            this->push(ui_impl_licenses_runner);
        }
        ImGui::UnderlineLast();

        ImGui::EndDragScrollableChild();
        ImGui::EndFullscreen();
    }

};
RSM_HOOK_ENABLE(ui_impl_settings);
struct ui_impl_renderer : rsm::GenericHook {
    void Start() {
    }
    void Render() {
        float _angle;
        ImGui::BeginFullscreen();
        ImGui::SetCursorPos(GlobalState::ui_begin);
        if (RSMI::BackArrowModal()) {
            this->pop();
        }
        ImGui::EndFullscreen();
    }
};
RSM_HOOK_ENABLE(ui_impl_renderer);
struct ui_impl_charman : rsm::GenericHook {

    void Start() {
    }
    void Render() {
        ImGui::BeginFullscreen();
        ImGui::SetCursorPos(GlobalState::ui_begin);
        if (RSMI::BackArrowModal()) {
            this->pop();
        }
        ImGui::BeginDragScrollableChild("##character_mgr", { 0,0 }, true);
        ImGui::EndDragScrollableChild();
        ImGui::EndFullscreen();

    }

};
RSM_HOOK_ENABLE(ui_impl_charman);
struct ui_impl_main : rsm::GenericHook {
    float __percent;
    void Start() {
        AndroidSetWakeLockState(true);
        if (rsm::GenericHook::__hook_stack.size() == 0) {
            this->push(this);
        }
    }
    void Render() {
        __percent += ImGui::GetIO().DeltaTime * 8.0f;
        ImGui::BeginFullscreen();
        ImGui::SetCursorPos(GlobalState::ui_begin);
        ImGui::PushFont(rsm::Fonts::symbols);
        float fsz_last = ImGui::GetCurrentWindow()->FontWindowScale;
        ImGui::SetWindowFontScale(1.0f);
        ImVec2 tsz = ImGui::CalcTextSize("9$|B");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0,0 });
        ImGui::SetCursorPos(GlobalState::ui_begin);

        ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
        if (ImGui::Button("$##settings_btn")) {
            this->push(ui_impl_settings_runner);
        }
        ImGui::SameLine();
        if (ImGui::Button("9##debug_btn")) {
            cwError::serrof("%sDebug window opened.", ImRGB(255, 255, 0, 0).tostring());
            this->push(ui_impl_debug_runner);
        }
        ImGui::SameLine();
        if (ImGui::Button("|##charman_btn")) {
            this->push(ui_impl_charman_runner);
        }
        ImGui::SameLine();
        if (ImGui::Button("B##charman_btn")) {
            this->push(ui_impl_renderer_runner);
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopFont();
        ImGui::SetWindowFontScale(fsz_last);

        // End topnav
        ImGui::BeginDragScrollableChild("__main_scrollable", { 0,0 }, true);
        glm::vec2 tmp = rsm::getDragDelta();
        RSMI::activecamera.transform_position.xy(tmp.x * ImGui::GetIO().DeltaTime, tmp.y * ImGui::GetIO().DeltaTime);
        ImGui::EndDragScrollableChild();
        ImGui::EndFullscreen();
    }
    void PreSwap() {
    }
};
RSM_HOOK_ENABLE(ui_impl_main);
static bool touch_l = 0;
void tick()
{
    if (g_EglDisplay == EGL_NO_DISPLAY)
        return;
    if (_time == 0.0f) {
        rsm::HookManager::RunStart();
        rsm::HookManager::RunPostStart();
    }
    ImGuiIO& io = ImGui::GetIO();
    
    // account previous frames for the gestures. Used for delta processing.
    rsm::m_DragPositionL = rsm::DragPosition;
    rsm::m_DoubleTapPositionL = rsm::DoubleTapPosition;
    rsm::m_PinchPositionBeginL = rsm::PinchPositionBegin;
    rsm::m_PinchPositionEndL = rsm::PinchPositionEnd;

    ndk_helper::Vec2 drag; rsm::drag_detector_.GetPointer(drag);
    drag.Value(rsm::DragPosition.x, rsm::DragPosition.y);
    ndk_helper::Vec2 pinch1, pinch2; rsm::pinch_detector_.GetPointers(pinch1, pinch2);
    pinch1.Value(rsm::PinchPositionBegin.x, rsm::PinchPositionBegin.y); pinch2.Value(rsm::PinchPositionEnd.x, rsm::PinchPositionEnd.y);
    int32_t window_width = ANativeWindow_getWidth(g_App->window);
    int32_t window_height = ANativeWindow_getHeight(g_App->window);
    int32_t half_width = lroundf((float)window_width * 0.5f);
    int32_t half_height = lroundf((float)window_height * 0.5f);
    _time += io.DeltaTime / 1000.0f;
    // Open on-screen (soft) input if requested by Dear ImGui
    static bool WantTextInputLast = false;
    if ((io.WantTextInput && !WantTextInputLast) || (!io.WantTextInput && WantTextInputLast))
        AndroidShowKeyboard();
    if (!io.WantTextInput && !WantTextInputLast) {
        AndroidHideKeyboard();
    }
    WantTextInputLast = io.WantTextInput;

    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, window_width, window_height);
    glClearColor(0x2a / 255.0f, 0x29 / 255.0f, 0x2e / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    rsm::HookManager::RunPreUpdate();
    rsm::HookManager::RunUpdate();
    rsm::HookManager::RunPostUpdate();
    // Rendering
    rsm::HookManager::RunPreRender();
    rsm::HookManager::RunRender();
    rsm::HookManager::RunPreSwap();
    RSMI_runner->PreSwap();

    rsm::HookManager::RunPreUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(g_EglDisplay, g_EglSurface);
    rsm::HookManager::RunPostRender();
    ImGui::ElementStubImpl::ResetOffsets();
}

void shutdown()
{
    if (!g_Initialized)
        return;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (g_EglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (g_EglContext != EGL_NO_CONTEXT)
            eglDestroyContext(g_EglDisplay, g_EglContext);

        if (g_EglSurface != EGL_NO_SURFACE)
            eglDestroySurface(g_EglDisplay, g_EglSurface);

        eglTerminate(g_EglDisplay);
    }

    g_EglDisplay = EGL_NO_DISPLAY;
    g_EglContext = EGL_NO_CONTEXT;
    g_EglSurface = EGL_NO_SURFACE;
    ANativeWindow_release(g_App->window);

    g_Initialized = false;
}

static void handleAppCmd(struct android_app* app, int32_t appCmd)
{
    switch (appCmd)
    {
    case APP_CMD_SAVE_STATE:
        break;
    case APP_CMD_INIT_WINDOW:
        init(app);
        break;
    case APP_CMD_TERM_WINDOW:
        shutdown();
        break;
    case APP_CMD_GAINED_FOCUS:
        break;
    case APP_CMD_LOST_FOCUS:
        break;
    }
}
void rsm::Renderer::TransformPosition(ndk_helper::Vec2& vec) {
    ImGuiIO io = ImGui::GetIO();
        vec = ndk_helper::Vec2(2.0f, 2.0f) * vec
            / ndk_helper::Vec2(io.DisplaySize.x,
                io.DisplaySize.y) - ndk_helper::Vec2(1.f, 1.f);

}
static int32_t handleInputEvent(struct android_app* app, AInputEvent* inputEvent)
{
    
    
    if (AInputEvent_getType(inputEvent) == AINPUT_EVENT_TYPE_MOTION)
    {

        
        ndk_helper::GESTURE_STATE doubleTapState = rsm::doubletap_detector_.Detect(inputEvent);
        ndk_helper::GESTURE_STATE dragState = rsm::drag_detector_.Detect(inputEvent);
        ndk_helper::GESTURE_STATE pinchState = rsm::pinch_detector_.Detect(inputEvent);
        rsm::DoubleTapped = false;

        //Double tap detector has a priority over other detectors
        if (doubleTapState == ndk_helper::GESTURE_STATE_ACTION)
        {
            rsm::DoubleTapped = rsm::rsm_DoubleTapped_First;
        }
        else
        {
            //Handle drag state
            if (dragState & ndk_helper::GESTURE_STATE_START)
            {
                //Otherwise, start dragging
                ndk_helper::Vec2 v;
                rsm::drag_detector_.GetPointer(v);
                rsm::Renderer::TransformPosition(v);
                rsm::tap_camera_.BeginDrag(v);
            }
            // ...else other possible drag states...

            //Handle pinch state
            if (pinchState & ndk_helper::GESTURE_STATE_START)
            {
                //Start new pinch
                ndk_helper::Vec2 v1;
                ndk_helper::Vec2 v2;
                rsm::pinch_detector_.GetPointers(v1, v2);
                rsm::Renderer::TransformPosition(v1);
                rsm::Renderer::TransformPosition(v2);
                rsm::tap_camera_.BeginPinch(v1, v2);
            }
            // ...else other possible pinch states...
        }
    }

    if (AKeyEvent_getAction(inputEvent))
    {
        int code = AKeyEvent_getKeyCode(inputEvent);
        int meta_state = AMotionEvent_getMetaState(inputEvent);
        int unicode_key = AndroidGetUnicodeChar(code, meta_state);
        ImGui::GetIO().AddInputCharacter(unicode_key);
    }
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
}
void android_main(struct android_app* app)
{
    app->onAppCmd = handleAppCmd;
    app->onInputEvent = handleInputEvent;
    while (true)
    {
        int out_events;
        struct android_poll_source* out_data;

        // Poll all events. If the app is not visible, this loop blocks until g_Initialized == true.
        while (ALooper_pollAll(g_Initialized ? 0 : -1, NULL, &out_events, (void**)&out_data) >= 0)
        {
            // Process one event
            if (out_data != NULL)
                out_data->process(app, out_data);

            // Exit the app by returning from within the infinite loop
            if (app->destroyRequested != 0)
            {
                // shutdown() should have been called already while processing the
                // app command APP_CMD_TERM_WINDOW. But we play save here
                if (!g_Initialized)
                    shutdown();

                return;
            }
        }

        // Initiate a new frame
        tick();
    }
}
/// <summary>
/// 
/// </summary>
/// <returns>the rect taken up by the camera cutout</returns>
static ImRect AndroidCutoutRect() {
    ImRect out;
    JNIEnv* jni;
    g_App->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass cls = jni->GetObjectClass(g_App->activity->clazz);
    jmethodID getResourcesMID = jni->GetMethodID(cls, "getRootWindowInsets", "()Landroid/view/WindowInsets;");

    g_App->activity->vm->DetachCurrentThread();
    return out;
}
static int AndroidGenericDimenShown(const char* bar_name, int* height) {
    JNIEnv* jni;
    g_App->activity->vm->AttachCurrentThread(&jni, NULL);
    bool state = false;jclass cls = jni->GetObjectClass(g_App->activity->clazz);
    jmethodID getResourcesMID = jni->GetMethodID(cls, "getResources", "()Landroid/content/res/Resources;");
    jobject resources = jni->CallObjectMethod(g_App->activity->clazz, getResourcesMID);
    jclass resources_cls = jni->GetObjectClass(resources);
    jmethodID getIDMID = jni->GetMethodID(resources_cls, "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
    jmethodID getDimensionPixelSizeMID = jni->GetMethodID(resources_cls, "getDimensionPixelSize", "(I)I");
    jstring id0 = jni->NewStringUTF(bar_name);
    jstring id1 = jni->NewStringUTF("dimen");
    jstring id2 = jni->NewStringUTF("android");
    /*get the resource id for the status bar */
    jint resid = jni->CallIntMethod(resources, getIDMID, id0, id1, id2);
    int pxheight = 0;
    if (resid > 0) {
        pxheight = jni->CallIntMethod(resources, getDimensionPixelSizeMID, resid);
        /*get status bar height*/
    }
    if (height != NULL) {
        *height = pxheight;
    }
    jni->DeleteLocalRef(id0);
    jni->DeleteLocalRef(id1);
    jni->DeleteLocalRef(id2);
    g_App->activity->vm->DetachCurrentThread();
    return pxheight > 0;
}
static int AndroidStatusBarShown(int* height) {
    return AndroidGenericDimenShown("status_bar_height", height);
}
static int AndroidStatusBarHeight() {
    int out;
    AndroidStatusBarShown(&out);
    return out;
}
static int AndroidNavigationBarShown(int* height) {
    return AndroidGenericDimenShown("navigation_bar_height", height);
}
static int AndroidNavigationBarHeight() {
    int out;
    AndroidNavigationBarShown(&out);
    return out;
}
static int AndroidKeyboardShown(bool return_height) {
    JNIEnv* jni;
    g_App->activity->vm->AttachCurrentThread(&jni, NULL);
    bool state = false;
    jclass cls = jni->GetObjectClass(g_App->activity->clazz);
    jmethodID methodID = jni->GetMethodID(cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring service_name = jni->NewStringUTF("input_method");
    jobject input_service = jni->CallObjectMethod(g_App->activity->clazz, methodID, service_name);
    jclass input_service_cls = jni->GetObjectClass(input_service);
    methodID = jni->GetMethodID(input_service_cls, "getInputMethodWindowVisibleHeight", "()I");
    GlobalState::KeyboardHeight = jni->CallIntMethod(input_service, methodID);
    if (GlobalState::KeyboardHeight > 0) {
        GlobalState::keyboard_visible = true;
        state = true;
    }
    else {
        GlobalState::keyboard_visible = false;
    }
    g_App->activity->vm->DetachCurrentThread();
    if (!return_height) {
        return state;
    }
    else { return GlobalState::KeyboardHeight; }
}
static int AndroidKeyboardHeight() {
    return AndroidKeyboardShown(/*return_height = */true);
}
static void AndroidToggleKeyboard()
{
    JNIEnv* jni;
    g_App->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass cls = jni->GetObjectClass(g_App->activity->clazz);
    jmethodID methodID = jni->GetMethodID(cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring service_name = jni->NewStringUTF("input_method");
    jobject input_service = jni->CallObjectMethod(g_App->activity->clazz, methodID, service_name);

    jclass input_service_cls = jni->GetObjectClass(input_service);
    methodID = jni->GetMethodID(input_service_cls, "toggleSoftInput", "(II)V");
    jni->CallVoidMethod(input_service, methodID, 0, 0);

    jni->DeleteLocalRef(service_name);
    g_App->activity->vm->DetachCurrentThread();
}
static int AndroidGetUnicodeChar(int keyCode, int metaState)
{
    //https://stackoverflow.com/questions/21124051/receive-complete-android-unicode-input-in-c-c/43871301

    int eventType = AKEY_EVENT_ACTION_DOWN;
    JNIEnv* jni;
    g_App->activity->vm->AttachCurrentThread(&jni, NULL);

    jclass class_key_event = jni->FindClass("android/view/KeyEvent");

    jmethodID method_get_unicode_char = jni->GetMethodID(class_key_event, "getUnicodeChar", "(I)I");
    jmethodID eventConstructor = jni->GetMethodID(class_key_event, "<init>", "(II)V");
    jobject eventObj = jni->NewObject(class_key_event, eventConstructor, eventType, keyCode);

    int unicodeKey = jni->CallIntMethod(eventObj, method_get_unicode_char, metaState);

    g_App->activity->vm->DetachCurrentThread();

    return unicodeKey;
}
static void AndroidShowKeyboard() {
    if (AndroidKeyboardShown()) { return; }
    // Keyboard hidden, enable it.
    AndroidToggleKeyboard();
}
static void AndroidHideKeyboard() {
    if (!AndroidKeyboardShown()) { return; }
    // Keyboard shown, disable it
    AndroidToggleKeyboard();
}
static void AndroidSetWakeLockState(int state) {
    if (state) {
        // add flag
        ANativeActivity_setWindowFlags(g_App->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
    }
    if (!state) {
        // remove flag
        ANativeActivity_setWindowFlags(g_App->activity, 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
    }
}