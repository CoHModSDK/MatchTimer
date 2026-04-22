#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "CoHModSDK.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <limits>
#include <string>

namespace {
    using WorldSimulateFn = void(__thiscall*)(void*);
    using WorldGetSimManagerFn = void*(__thiscall*)(void*);
    using SimManagerClearGameTicksFn = void(__thiscall*)(void*);
    using SimManagerSetGameOverFn = void(__thiscall*)(void*);
    using SimManagerGetGameTimeFn = float(__thiscall*)(const void*);

    using ScreenManagerDrawFn = void(__thiscall*)(void*, float);
    using ScreenManagerDeactivateAllScreensFn = void(__thiscall*)(void*);
    using ScreenManagerFindScreenFn = void*(__thiscall*)(void*, const char*);
    using ScreenManagerUnloadScreenFn = void(__thiscall*)(void*, void*);
    using ScreenGetRootWidgetFn = void*(__thiscall*)(void*);

    using WidgetSetNameFn = void(__thiscall*)(void*, const char*);
    using WidgetSetPositionFn = void(__thiscall*)(void*, float, float);
    using WidgetSetSizeFn = void(__thiscall*)(void*, float, float);
    using WidgetSetParentFn = void(__thiscall*)(void*, void*);
    using WidgetProxyBindFn = void(__thiscall*)(void*, void*);
    using WidgetProxySetVisibleFn = void(__thiscall*)(void*, bool);
    using WidgetProxySetEnabledFn = void(__thiscall*)(void*, bool);
    using TextLabelCtorFn = void(__thiscall*)(void*);
    using TextLabelDtorFn = void(__thiscall*)(void*);
    using TextLabelSetTextFn = void(__thiscall*)(void*, const void*);
    using TextLabelSetAutoSizeFn = void(__thiscall*)(void*, bool);
    using TextLabelSetMultilineFn = void(__thiscall*)(void*, bool);
    using WidgetGetPresentationFn = void*(__thiscall*)(void*);
    using WidgetSetPresentationFn = void(__thiscall*)(void*, void*);
    using WidgetGetHitAreaFn = void*(__thiscall*)(void*);

    using LocStringCtorFromWideFn = void(__thiscall*)(void*, const wchar_t*);
    using LocStringDtorFn = void(__thiscall*)(void*);

    using FindWidgetExtensionFn = void*(__thiscall*)(void*, int);
    using FindWidgetByNameFn = void*(__stdcall*)(void*, const char*, int);

    constexpr char kSimEngineModuleName[] = "SimEngine.dll";
    constexpr char kUserInterfaceModuleName[] = "UserInterface.dll";
    constexpr char kLocalizerModuleName[] = "Localizer.dll";

    constexpr char kWorldSimulateExportName[] = "?Simulate@World@@UAEXXZ";
    constexpr char kWorldGetSimManagerExportName[] = "?GetSimManager@World@@QAEPAVSimManager@@XZ";
    constexpr char kClearGameTicksExportName[] = "?ClearGameTicks@SimManager@@IAEXXZ";
    constexpr char kSetGameOverExportName[] = "?SetGameOver@SimManager@@QAEXXZ";
    constexpr char kGetGameTimeExportName[] = "?GetGameTime@SimManager@@QBEMXZ";

    constexpr char kScreenManagerDrawExportName[] = "?Draw@ScreenManager@UI@@QAEXM@Z";
    constexpr char kDeactivateAllScreensExportName[] = "?DeactivateAllScreens@ScreenManager@UI@@QAEXXZ";
    constexpr char kFindScreenExportName[] = "?FindScreen@ScreenManager@UI@@QAEPAVScreen@2@PBD@Z";
    constexpr char kUnloadScreenExportName[] = "?UnloadScreen@ScreenManager@UI@@QAEXPAVScreen@2@@Z";
    constexpr char kGetRootWidgetExportName[] = "?GetRootWidget@Screen@UI@@QAEPAVWidget@2@XZ";
    constexpr char kWidgetSetNameExportName[] = "?SetName@Widget@UI@@QAEXPBD@Z";
    constexpr char kWidgetSetPositionExportName[] = "?SetPosition@Widget@UI@@QAEXMM@Z";
    constexpr char kWidgetSetSizeExportName[] = "?SetSize@Widget@UI@@QAEXMM@Z";
    constexpr char kWidgetSetParentExportName[] = "?SetParent@Widget@UI@@QAEXPAV12@@Z";
    constexpr char kWidgetProxyBindExportName[] = "?Bind@WidgetProxy@UI@@IAEXPAVWidget@2@@Z";
    constexpr char kWidgetProxySetVisibleExportName[] = "?SetVisible@WidgetProxy@UI@@UAEX_N@Z";
    constexpr char kWidgetProxySetEnabledExportName[] = "?SetEnabled@WidgetProxy@UI@@UAEX_N@Z";
    constexpr char kTextLabelCtorExportName[] = "??0TextLabel@UI@@QAE@XZ";
    constexpr char kTextLabelDtorExportName[] = "??1TextLabel@UI@@UAE@XZ";
    constexpr char kTextLabelSetTextExportName[] = "?SetText@TextLabel@UI@@QAEXABVLocString@@@Z";
    constexpr char kTextLabelSetAutoSizeExportName[] = "?SetAutoSize@TextLabel@UI@@QAEX_N@Z";
    constexpr char kTextLabelSetMultilineExportName[] = "?SetMultiline@TextLabel@UI@@QAEX_N@Z";
    constexpr char kWidgetGetPresentationExportName[] = "?GetPresentation@Widget@UI@@QAEPAVPresentation@2@XZ";
    constexpr char kWidgetSetPresentationExportName[] = "?SetPresentation@Widget@UI@@QAEXPAVPresentation@2@@Z";
    constexpr char kWidgetGetHitAreaExportName[] = "?GetHitArea@Widget@UI@@QAEPAVHitArea@2@XZ";
    constexpr char kLocStringCtorFromWideExportName[] = "??0LocString@@QAE@PB_W@Z";
    constexpr char kLocStringDtorExportName[] = "??1LocString@@QAE@XZ";

    constexpr std::uintptr_t kWidgetFactoryCreateRva = 0x000421A0u;
    constexpr std::uintptr_t kFindWidgetExtensionRva = 0x00031810u;
    constexpr std::uintptr_t kFindWidgetByNameRva = 0x0003BAB0u;
    constexpr std::uintptr_t kAddRenderChildRva = 0x0003DBF0u;

    constexpr int kDrawChildrenExtensionId = 1;
    constexpr int kWidgetFactoryCreateFlag = 1;

    constexpr char kTaskbarScreenName[] = "Taskbar";
    constexpr char kGameScreenName[] = "GameScreen";
    constexpr char kTimerLabelName[] = "matchtimer_taskbar_label";
    constexpr char kTextLabelWidgetTypeName[] = "TextLabel";
    constexpr char kTimerPresentationDonorName[] = "txtInfo";

    constexpr float kTimerLabelPositionX = 0.30260f;
    constexpr float kTimerLabelPositionY = 0.00000f;
    constexpr float kTimerLabelSizeX = 0.39480f;
    constexpr float kTimerLabelSizeY = 0.12600f;

    constexpr std::size_t kOpaqueTextLabelStorageSize = 8192u;
    constexpr std::size_t kLocStringStorageSize = 256u;

    WorldSimulateFn g_originalWorldSimulate = nullptr;
    WorldGetSimManagerFn g_getWorldSimManager = nullptr;
    SimManagerClearGameTicksFn g_originalClearGameTicks = nullptr;
    SimManagerSetGameOverFn g_originalSetGameOver = nullptr;
    SimManagerGetGameTimeFn g_getGameTime = nullptr;

    ScreenManagerDrawFn g_originalScreenManagerDraw = nullptr;
    ScreenManagerDeactivateAllScreensFn g_originalDeactivateAllScreens = nullptr;
    ScreenManagerFindScreenFn g_findScreen = nullptr;
    ScreenManagerUnloadScreenFn g_originalUnloadScreen = nullptr;
    ScreenGetRootWidgetFn g_getRootWidget = nullptr;
    WidgetSetNameFn g_widgetSetName = nullptr;
    WidgetSetPositionFn g_widgetSetPosition = nullptr;
    WidgetSetSizeFn g_widgetSetSize = nullptr;
    WidgetSetParentFn g_widgetSetParent = nullptr;
    WidgetProxyBindFn g_widgetProxyBind = nullptr;
    WidgetProxySetVisibleFn g_widgetProxySetVisible = nullptr;
    WidgetProxySetEnabledFn g_widgetProxySetEnabled = nullptr;
    TextLabelCtorFn g_textLabelCtor = nullptr;
    TextLabelDtorFn g_textLabelDtor = nullptr;
    TextLabelSetTextFn g_textLabelSetText = nullptr;
    TextLabelSetAutoSizeFn g_textLabelSetAutoSize = nullptr;
    TextLabelSetMultilineFn g_textLabelSetMultiline = nullptr;
    WidgetGetPresentationFn g_widgetGetPresentation = nullptr;
    WidgetSetPresentationFn g_widgetSetPresentation = nullptr;
    WidgetGetHitAreaFn g_widgetGetHitArea = nullptr;
    LocStringCtorFromWideFn g_locStringCtorFromWide = nullptr;
    LocStringDtorFn g_locStringDtor = nullptr;
    FindWidgetExtensionFn g_findWidgetExtension = nullptr;
    FindWidgetByNameFn g_findWidgetByName = nullptr;
    void* g_widgetFactoryCreateAddress = nullptr;
    void* g_addRenderChildAddress = nullptr;

    std::atomic<std::uint32_t> g_lastGameTimeMilliseconds = 0u;
    std::atomic<bool> g_matchActive = false;
    std::atomic<bool> g_gameOver = false;

    std::atomic<bool> g_loggedWorldSimulate = false;
    std::atomic<bool> g_loggedTimeCapture = false;
    std::atomic<bool> g_loggedUiHooksInstalled = false;
    std::atomic<bool> g_loggedUiDrawHook = false;
    std::atomic<bool> g_loggedTimerLabelCreated = false;
    std::atomic<bool> g_loggedTimerLabelBound = false;
    std::atomic<bool> g_loggedTimerLabelUpdated = false;
    std::atomic<bool> g_loggedTimerLabelCreateFailure = false;
    std::atomic<bool> g_loggedTimerLabelBindFailure = false;
    std::atomic<bool> g_loggedTimerPresentationFailure = false;
    std::atomic<bool> g_loggedTimerLabelDetach = false;

    bool g_shutdownInProgress = false;

    void* g_taskbarScreen = nullptr;
    void* g_taskbarRootWidget = nullptr;
    void* g_timerLabelRaw = nullptr;
    alignas(16) std::array<unsigned char, kOpaqueTextLabelStorageSize> g_timerLabelProxyStorage = {};
    bool g_timerLabelProxyConstructed = false;
    std::uint32_t g_lastUiLabelSeconds = std::numeric_limits<std::uint32_t>::max();
    bool g_lastUiLabelVisible = false;
    bool g_uiLabelTextInitialized = false;

    template <typename T>
    T ResolveExport(HMODULE module, const char* exportName) {
        if ((module == nullptr) || (exportName == nullptr)) {
            return nullptr;
        }

        return reinterpret_cast<T>(GetProcAddress(module, exportName));
    }

    template <typename T>
    bool ResolveRequiredExport(HMODULE module, const char* moduleName, const char* exportName, T& outFunction) {
        outFunction = ResolveExport<T>(module, exportName);
        if (outFunction != nullptr) {
            return true;
        }

        char message[256] = {};
        std::snprintf(
            message,
            sizeof(message),
            "Match Timer failed to resolve %s from %s",
            exportName,
            moduleName == nullptr ? "<unknown>" : moduleName
        );
        ModSDK::Runtime::LogError(message);
        ModSDK::Dialogs::ShowError(message);
        return false;
    }

    HMODULE AcquireModule(const char* moduleName) {
        if (moduleName == nullptr) {
            return nullptr;
        }

        HMODULE module = GetModuleHandleA(moduleName);
        if (module != nullptr) {
            return module;
        }

        return LoadLibraryA(moduleName);
    }

    void LogOnce(std::atomic<bool>& flag, CoHModSDKLogLevel level, const char* message) {
        bool expected = false;
        if (flag.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
            ModSDK::Runtime::Log(level, message);
        }
    }

    bool ShowAndLogError(const char* message) {
        ModSDK::Runtime::LogError(message);
        ModSDK::Dialogs::ShowError(message);
        return false;
    }

    bool InstallExportHook(HMODULE module, const char* exportName, void* detour, void** originalFunction, const char* hookDisplayName) {
        void* target = ResolveExport<void*>(module, exportName);
        if (target == nullptr) {
            char message[256] = {};
            std::snprintf(message, sizeof(message), "Match Timer failed to resolve %s", hookDisplayName);
            return ShowAndLogError(message);
        }

        if (!ModSDK::Hooks::CreateHook(target, detour, originalFunction)) {
            char message[256] = {};
            std::snprintf(message, sizeof(message), "Match Timer failed to create hook for %s", hookDisplayName);
            return ShowAndLogError(message);
        }

        return true;
    }

    std::wstring FormatMatchTimeWide(std::uint32_t totalSeconds) {
        const std::uint32_t hours = totalSeconds / 3600u;
        const std::uint32_t minutes = (totalSeconds / 60u) % 60u;
        const std::uint32_t seconds = totalSeconds % 60u;

        wchar_t buffer[32] = {};
        std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%02u:%02u:%02u", hours, minutes, seconds);
        return std::wstring(buffer);
    }

    void RefreshCachedMatchTime(const void* simManager) {
        if (simManager == nullptr) {
            return;
        }

        if (g_getGameTime != nullptr) {
            float gameTimeSeconds = g_getGameTime(simManager);
            if (!std::isfinite(gameTimeSeconds) || (gameTimeSeconds < 0.0f)) {
                gameTimeSeconds = 0.0f;
            }

            const auto milliseconds = static_cast<std::uint32_t>(std::lround(gameTimeSeconds * 1000.0f));
            g_lastGameTimeMilliseconds.store(milliseconds, std::memory_order_relaxed);
            if (milliseconds > 0u) {
                char message[96] = {};
                std::snprintf(message, sizeof(message), "Match Timer captured game time %.3f seconds", gameTimeSeconds);
                LogOnce(g_loggedTimeCapture, CoHModSDKLogLevel_Info, message);
            }
        }
    }

    bool ShouldShowTimer() {
        return g_matchActive.load(std::memory_order_relaxed) || g_gameOver.load(std::memory_order_relaxed);
    }

    void ResetTimerLabelBinding(bool destroyProxy = true) {
        if (destroyProxy && g_timerLabelProxyConstructed && (g_textLabelDtor != nullptr)) {
            g_textLabelDtor(g_timerLabelProxyStorage.data());
        }

        g_timerLabelProxyStorage.fill(0u);
        g_timerLabelProxyConstructed = false;
        g_lastUiLabelSeconds = std::numeric_limits<std::uint32_t>::max();
        g_lastUiLabelVisible = false;
        g_uiLabelTextInitialized = false;
    }

    void ResetTaskbarTimerState(bool destroyProxy = true) {
        ResetTimerLabelBinding(destroyProxy);
        g_taskbarScreen = nullptr;
        g_taskbarRootWidget = nullptr;
        g_timerLabelRaw = nullptr;
    }

    void ApplyTimerLabelVisibility(bool visible);
    bool RemoveRenderChild(void* parentWidget, void* childWidget);

    int ExtractGetterOffset(void* fn) {
        if (fn == nullptr) {
            return -1;
        }

        const auto* code = reinterpret_cast<const unsigned char*>(fn);
        if ((code[0] == 0x8B) && (code[1] == 0x41) && (code[3] == 0xC3)) {
            return static_cast<int>(code[2]);
        }

        if ((code[0] == 0x8B) && (code[1] == 0x81) && (code[6] == 0xC3)) {
            return static_cast<int>(*reinterpret_cast<const std::uint32_t*>(code + 2));
        }

        return -1;
    }

    void ZeroWidgetField(void* widget, int offset) {
        if ((widget == nullptr) || (offset < 0)) {
            return;
        }

        auto* const field = reinterpret_cast<void**>(
            reinterpret_cast<std::uintptr_t>(widget) + static_cast<std::uintptr_t>(offset)
        );
        *field = nullptr;
    }

    void DetachSharedTimerLabelFields() {
        if (g_timerLabelRaw == nullptr) {
            return;
        }

        const int presentationOffset = ExtractGetterOffset(reinterpret_cast<void*>(g_widgetGetPresentation));
        const int hitAreaOffset = ExtractGetterOffset(reinterpret_cast<void*>(g_widgetGetHitArea));
        if (presentationOffset < 0) {
            return;
        }

        ZeroWidgetField(g_timerLabelRaw, presentationOffset);
        ZeroWidgetField(g_timerLabelRaw, hitAreaOffset);
        LogOnce(
            g_loggedTimerLabelDetach,
            CoHModSDKLogLevel_Info,
            "Match Timer detached shared presentation state from the native Taskbar timer label before unload"
        );
    }

    void RetireTaskbarTimerLabel() {
        // The engine owns the raw widget memory after we graft it into Taskbar, so on teardown
        // we only hide it, detach borrowed fields, and unlink it from the render tree.
        ApplyTimerLabelVisibility(false);
        DetachSharedTimerLabelFields();

        if ((g_taskbarRootWidget != nullptr) && (g_timerLabelRaw != nullptr)) {
            RemoveRenderChild(g_taskbarRootWidget, g_timerLabelRaw);
        }

        ResetTaskbarTimerState(false);
    }

    void ApplyTimerLabelVisibility(bool visible) {
        if (!g_timerLabelProxyConstructed) {
            return;
        }

        if (g_widgetProxySetVisible != nullptr) {
            g_widgetProxySetVisible(g_timerLabelProxyStorage.data(), visible);
        }

        if (g_widgetProxySetEnabled != nullptr) {
            g_widgetProxySetEnabled(g_timerLabelProxyStorage.data(), visible);
        }
    }

    void* FindNamedWidget(void* searchRoot, const char* widgetName) {
        if ((searchRoot == nullptr) || (widgetName == nullptr) || (g_findWidgetByName == nullptr)) {
            return nullptr;
        }

        void* widget = g_findWidgetByName(searchRoot, widgetName, 0);
        if (widget == nullptr) {
            widget = g_findWidgetByName(searchRoot, widgetName, 1);
        }

        return widget;
    }

    void* CreateRawWidgetByType(const char* widgetTypeName) {
        if ((g_widgetFactoryCreateAddress == nullptr) || (widgetTypeName == nullptr)) {
            return nullptr;
        }

        void* widget = nullptr;
#if defined(_M_IX86)
        void* createFunction = g_widgetFactoryCreateAddress;
        __asm {
            mov edi, widgetTypeName
            mov eax, createFunction
            push kWidgetFactoryCreateFlag
            call eax
            mov widget, eax
        }
#else
        (void)widgetTypeName;
#endif
        return widget;
    }

    void ConfigureRawWidget(
        void* widget,
        const char* name,
        float positionX,
        float positionY,
        float sizeX,
        float sizeY,
        void* parent = nullptr
    ) {
        if ((widget == nullptr) || (g_widgetSetPosition == nullptr) || (g_widgetSetSize == nullptr)) {
            return;
        }

        if ((parent != nullptr) && (g_widgetSetParent != nullptr)) {
            g_widgetSetParent(widget, parent);
        }

        if ((g_widgetSetName != nullptr) && (name != nullptr) && (name[0] != '\0')) {
            g_widgetSetName(widget, name);
        }

        g_widgetSetPosition(widget, positionX, positionY);
        g_widgetSetSize(widget, sizeX, sizeY);
    }

    bool AddRenderChild(void* parentRenderObject, void* childWidget, int insertionIndex = -1) {
        if ((parentRenderObject == nullptr) || (childWidget == nullptr) || (g_addRenderChildAddress == nullptr)) {
            return false;
        }

#if defined(_M_IX86)
        void* addRenderChildFunction = g_addRenderChildAddress;
        __asm {
            mov edi, parentRenderObject
            mov eax, addRenderChildFunction
            push insertionIndex
            push childWidget
            call eax
        }
        return true;
#else
        (void)parentRenderObject;
        (void)childWidget;
        (void)insertionIndex;
        return false;
#endif
    }

    bool AttachRenderChild(void* parentWidget, void* childWidget) {
        if ((parentWidget == nullptr) || (childWidget == nullptr) || (g_findWidgetExtension == nullptr)) {
            return false;
        }

        void* const parentRenderObject = g_findWidgetExtension(parentWidget, kDrawChildrenExtensionId);
        if (parentRenderObject == nullptr) {
            return false;
        }

        return AddRenderChild(parentRenderObject, childWidget);
    }

    bool RemoveRenderChild(void* parentWidget, void* childWidget) {
        if ((parentWidget == nullptr) || (childWidget == nullptr) || (g_findWidgetExtension == nullptr)) {
            return false;
        }

        void* const parentRenderObject = g_findWidgetExtension(parentWidget, kDrawChildrenExtensionId);
        if (parentRenderObject == nullptr) {
            return false;
        }

        const std::uintptr_t renderObjectAddress = reinterpret_cast<std::uintptr_t>(parentRenderObject);
        void** children = *reinterpret_cast<void***>(renderObjectAddress + 0x1Cu);
        unsigned int& count = *reinterpret_cast<unsigned int*>(renderObjectAddress + 0x20u);
        if ((children == nullptr) || (count == 0u)) {
            return false;
        }

        for (unsigned int index = 0u; index < count; ++index) {
            if (children[index] != childWidget) {
                continue;
            }

            for (unsigned int shiftIndex = index + 1u; shiftIndex < count; ++shiftIndex) {
                children[shiftIndex - 1u] = children[shiftIndex];
            }

            children[count - 1u] = nullptr;
            --count;
            return true;
        }

        return false;
    }

    bool CopyWidgetPresentation(void* targetRawWidget, void* donorRawWidget) {
        if ((targetRawWidget == nullptr) || (donorRawWidget == nullptr) ||
            (g_widgetGetPresentation == nullptr) || (g_widgetSetPresentation == nullptr)) {
            return false;
        }

        void* const presentation = g_widgetGetPresentation(donorRawWidget);
        if (presentation == nullptr) {
            return false;
        }

        g_widgetSetPresentation(targetRawWidget, presentation);
        return true;
    }

    void* ResolveTimerPresentationDonor(void* screenManager) {
        if ((screenManager == nullptr) || (g_findScreen == nullptr) || (g_getRootWidget == nullptr)) {
            return nullptr;
        }

        void* const gameScreen = g_findScreen(screenManager, kGameScreenName);
        if (gameScreen == nullptr) {
            return nullptr;
        }

        void* const gameRootWidget = g_getRootWidget(gameScreen);
        if (gameRootWidget == nullptr) {
            return nullptr;
        }

        return FindNamedWidget(gameRootWidget, kTimerPresentationDonorName);
    }

    bool BindTimerLabelProxy(void* rawWidget) {
        if ((rawWidget == nullptr) || (g_textLabelCtor == nullptr) || (g_widgetProxyBind == nullptr)) {
            return false;
        }

        ResetTimerLabelBinding();
        g_textLabelCtor(g_timerLabelProxyStorage.data());
        g_timerLabelProxyConstructed = true;
        g_widgetProxyBind(g_timerLabelProxyStorage.data(), rawWidget);

        if (g_textLabelSetAutoSize != nullptr) {
            g_textLabelSetAutoSize(g_timerLabelProxyStorage.data(), false);
        }

        if (g_textLabelSetMultiline != nullptr) {
            g_textLabelSetMultiline(g_timerLabelProxyStorage.data(), false);
        }

        ApplyTimerLabelVisibility(false);
        LogOnce(g_loggedTimerLabelBound, CoHModSDKLogLevel_Info, "Match Timer bound the native Taskbar timer label");
        return true;
    }

    bool CreateTaskbarTimerLabel(void* screenManager, void* taskbarRootWidget) {
        if ((screenManager == nullptr) || (taskbarRootWidget == nullptr)) {
            return false;
        }

        void* const donorRawWidget = ResolveTimerPresentationDonor(screenManager);
        if (donorRawWidget == nullptr) {
            LogOnce(
                g_loggedTimerPresentationFailure,
                CoHModSDKLogLevel_Error,
                "Match Timer failed to find GameScreen::txtInfo as the native timer donor widget"
            );
            return false;
        }

        void* const rawWidget = CreateRawWidgetByType(kTextLabelWidgetTypeName);
        if (rawWidget == nullptr) {
            LogOnce(
                g_loggedTimerLabelCreateFailure,
                CoHModSDKLogLevel_Error,
                "Match Timer failed to create the native Taskbar timer label widget"
            );
            return false;
        }

        if (!CopyWidgetPresentation(rawWidget, donorRawWidget)) {
            LogOnce(
                g_loggedTimerPresentationFailure,
                CoHModSDKLogLevel_Error,
                "Match Timer failed to copy donor presentation onto the native Taskbar timer label"
            );
            return false;
        }

        ConfigureRawWidget(
            rawWidget,
            kTimerLabelName,
            kTimerLabelPositionX,
            kTimerLabelPositionY,
            kTimerLabelSizeX,
            kTimerLabelSizeY,
            taskbarRootWidget
        );

        if (!AttachRenderChild(taskbarRootWidget, rawWidget)) {
            LogOnce(
                g_loggedTimerLabelCreateFailure,
                CoHModSDKLogLevel_Error,
                "Match Timer failed to attach the native Taskbar timer label to the HUD render tree"
            );
            return false;
        }

        g_timerLabelRaw = rawWidget;
        LogOnce(g_loggedTimerLabelCreated, CoHModSDKLogLevel_Info, "Match Timer created a native Taskbar timer label");
        return BindTimerLabelProxy(rawWidget);
    }

    bool EnsureTaskbarTimerLabel(void* screenManager) {
        if ((screenManager == nullptr) || (g_findScreen == nullptr) || (g_getRootWidget == nullptr)) {
            return false;
        }

        void* const taskbarScreen = g_findScreen(screenManager, kTaskbarScreenName);
        if (taskbarScreen == nullptr) {
            ResetTaskbarTimerState();
            return false;
        }

        void* const taskbarRootWidget = g_getRootWidget(taskbarScreen);
        if (taskbarRootWidget == nullptr) {
            ResetTaskbarTimerState();
            return false;
        }

        if ((taskbarScreen != g_taskbarScreen) || (taskbarRootWidget != g_taskbarRootWidget)) {
            ResetTimerLabelBinding();
            g_taskbarScreen = taskbarScreen;
            g_taskbarRootWidget = taskbarRootWidget;
            g_timerLabelRaw = nullptr;
        }

        void* const desiredRawWidget = FindNamedWidget(taskbarRootWidget, kTimerLabelName);
        if (desiredRawWidget == nullptr) {
            return CreateTaskbarTimerLabel(screenManager, taskbarRootWidget);
        }

        if ((!g_timerLabelProxyConstructed) || (g_timerLabelRaw != desiredRawWidget)) {
            g_timerLabelRaw = desiredRawWidget;
            if (!BindTimerLabelProxy(desiredRawWidget)) {
                LogOnce(
                    g_loggedTimerLabelBindFailure,
                    CoHModSDKLogLevel_Error,
                    "Match Timer failed to bind to the native Taskbar timer label"
                );
                return false;
            }
        }

        return true;
    }

    void UpdateTimerLabelText() {
        if (!g_timerLabelProxyConstructed || g_timerLabelRaw == nullptr ||
            (g_textLabelSetText == nullptr) || (g_locStringCtorFromWide == nullptr) || (g_locStringDtor == nullptr)) {
            return;
        }

        const bool shouldShow = ShouldShowTimer();
        const std::uint32_t totalSeconds = g_lastGameTimeMilliseconds.load(std::memory_order_relaxed) / 1000u;
        if (g_uiLabelTextInitialized && (shouldShow == g_lastUiLabelVisible) &&
            (!shouldShow || (totalSeconds == g_lastUiLabelSeconds))) {
            return;
        }

        if (!shouldShow) {
            g_uiLabelTextInitialized = true;
            g_lastUiLabelVisible = false;
            return;
        }

        const std::wstring labelText = FormatMatchTimeWide(totalSeconds);
        alignas(16) std::array<unsigned char, kLocStringStorageSize> locStringStorage = {};

        g_locStringCtorFromWide(locStringStorage.data(), labelText.c_str());
        g_textLabelSetText(g_timerLabelProxyStorage.data(), locStringStorage.data());
        g_locStringDtor(locStringStorage.data());

        g_uiLabelTextInitialized = true;
        g_lastUiLabelVisible = true;
        g_lastUiLabelSeconds = totalSeconds;
        LogOnce(g_loggedTimerLabelUpdated, CoHModSDKLogLevel_Info, "Match Timer updated the native Taskbar timer label");
    }

    void __fastcall HookedWorldSimulate(void* world, void*) {
        g_originalWorldSimulate(world);

        void* simManager = world;
        if (g_getWorldSimManager != nullptr) {
            simManager = g_getWorldSimManager(world);
        }

        if (simManager == nullptr) {
            return;
        }

        g_matchActive.store(true, std::memory_order_relaxed);
        g_gameOver.store(false, std::memory_order_relaxed);
        RefreshCachedMatchTime(simManager);
        LogOnce(g_loggedWorldSimulate, CoHModSDKLogLevel_Info, "Match Timer World::Simulate hook is active");
    }

    void __fastcall HookedClearGameTicks(void* simManager, void*) {
        g_originalClearGameTicks(simManager);

        g_lastGameTimeMilliseconds.store(0u, std::memory_order_relaxed);
        g_matchActive.store(false, std::memory_order_relaxed);
        g_gameOver.store(false, std::memory_order_relaxed);
    }

    void __fastcall HookedSetGameOver(void* simManager, void*) {
        g_originalSetGameOver(simManager);

        g_matchActive.store(true, std::memory_order_relaxed);
        g_gameOver.store(true, std::memory_order_relaxed);
        RefreshCachedMatchTime(simManager);
    }

    void __fastcall HookedScreenManagerDraw(void* screenManager, void*, float deltaSeconds) {
        LogOnce(g_loggedUiDrawHook, CoHModSDKLogLevel_Info, "Match Timer UI::ScreenManager::Draw hook is active");

        const bool shouldShow = ShouldShowTimer();
        if (EnsureTaskbarTimerLabel(screenManager)) {
            ApplyTimerLabelVisibility(shouldShow);
            if (shouldShow) {
                UpdateTimerLabelText();
            } else {
                g_lastUiLabelVisible = false;
            }
        }

        g_originalScreenManagerDraw(screenManager, deltaSeconds);
    }

    void __fastcall HookedDeactivateAllScreens(void* screenManager, void*) {
        RetireTaskbarTimerLabel();

        if (g_originalDeactivateAllScreens != nullptr) {
            g_originalDeactivateAllScreens(screenManager);
        }
    }

    void __fastcall HookedUnloadScreen(void* screenManager, void*, void* screen) {
        if (g_shutdownInProgress) {
            if (g_originalUnloadScreen != nullptr) {
                g_originalUnloadScreen(screenManager, screen);
            }
            return;
        }

        if ((screen != nullptr) && (screen == g_taskbarScreen)) {
            RetireTaskbarTimerLabel();

            if (g_originalUnloadScreen != nullptr) {
                g_originalUnloadScreen(screenManager, screen);
            }
            return;
        }

        if (g_originalUnloadScreen != nullptr) {
            g_originalUnloadScreen(screenManager, screen);
        }
    }

    bool InstallSimHooks() {
        HMODULE simEngineModule = AcquireModule(kSimEngineModuleName);
        if (simEngineModule == nullptr) {
            return ShowAndLogError("Match Timer failed to load SimEngine.dll");
        }

        g_getGameTime = ResolveExport<SimManagerGetGameTimeFn>(simEngineModule, kGetGameTimeExportName);
        g_getWorldSimManager = ResolveExport<WorldGetSimManagerFn>(simEngineModule, kWorldGetSimManagerExportName);
        if ((g_getGameTime == nullptr) || (g_getWorldSimManager == nullptr)) {
            return ShowAndLogError("Match Timer failed to resolve SimManager timer exports");
        }

        if (!InstallExportHook(
                simEngineModule,
                kWorldSimulateExportName,
                reinterpret_cast<void*>(&HookedWorldSimulate),
                reinterpret_cast<void**>(&g_originalWorldSimulate),
                "World::Simulate")) {
            return false;
        }

        if (!InstallExportHook(
                simEngineModule,
                kClearGameTicksExportName,
                reinterpret_cast<void*>(&HookedClearGameTicks),
                reinterpret_cast<void**>(&g_originalClearGameTicks),
                "SimManager::ClearGameTicks")) {
            return false;
        }

        if (!InstallExportHook(
                simEngineModule,
                kSetGameOverExportName,
                reinterpret_cast<void*>(&HookedSetGameOver),
                reinterpret_cast<void**>(&g_originalSetGameOver),
                "SimManager::SetGameOver")) {
            return false;
        }

		ModSDK::Runtime::LogInfo("Match Timer hooked SimEngine timer exports");
        return true;
    }

    bool InstallUiHooks() {
        HMODULE userInterfaceModule = AcquireModule(kUserInterfaceModuleName);
        if (userInterfaceModule == nullptr) {
            return ShowAndLogError("Match Timer failed to load UserInterface.dll");
        }

        HMODULE localizerModule = AcquireModule(kLocalizerModuleName);
        if (localizerModule == nullptr) {
            return ShowAndLogError("Match Timer failed to load Localizer.dll");
        }

        const bool resolved =
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kDeactivateAllScreensExportName, g_originalDeactivateAllScreens) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kFindScreenExportName, g_findScreen) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kUnloadScreenExportName, g_originalUnloadScreen) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kGetRootWidgetExportName, g_getRootWidget) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetSetNameExportName, g_widgetSetName) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetSetPositionExportName, g_widgetSetPosition) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetSetSizeExportName, g_widgetSetSize) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetSetParentExportName, g_widgetSetParent) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetProxyBindExportName, g_widgetProxyBind) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetProxySetVisibleExportName, g_widgetProxySetVisible) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetProxySetEnabledExportName, g_widgetProxySetEnabled) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kTextLabelCtorExportName, g_textLabelCtor) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kTextLabelDtorExportName, g_textLabelDtor) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kTextLabelSetTextExportName, g_textLabelSetText) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetGetPresentationExportName, g_widgetGetPresentation) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetSetPresentationExportName, g_widgetSetPresentation) &&
            ResolveRequiredExport(userInterfaceModule, kUserInterfaceModuleName, kWidgetGetHitAreaExportName, g_widgetGetHitArea) &&
            ResolveRequiredExport(localizerModule, kLocalizerModuleName, kLocStringCtorFromWideExportName, g_locStringCtorFromWide) &&
            ResolveRequiredExport(localizerModule, kLocalizerModuleName, kLocStringDtorExportName, g_locStringDtor);
        if (!resolved) {
            return false;
        }

        g_textLabelSetAutoSize = ResolveExport<TextLabelSetAutoSizeFn>(userInterfaceModule, kTextLabelSetAutoSizeExportName);
        g_textLabelSetMultiline = ResolveExport<TextLabelSetMultilineFn>(userInterfaceModule, kTextLabelSetMultilineExportName);

        const std::uintptr_t userInterfaceBase = reinterpret_cast<std::uintptr_t>(userInterfaceModule);
        g_widgetFactoryCreateAddress = reinterpret_cast<void*>(userInterfaceBase + kWidgetFactoryCreateRva);
        g_addRenderChildAddress = reinterpret_cast<void*>(userInterfaceBase + kAddRenderChildRva);
        g_findWidgetExtension = reinterpret_cast<FindWidgetExtensionFn>(userInterfaceBase + kFindWidgetExtensionRva);
        g_findWidgetByName = reinterpret_cast<FindWidgetByNameFn>(userInterfaceBase + kFindWidgetByNameRva);

        if (!InstallExportHook(
                userInterfaceModule,
                kScreenManagerDrawExportName,
                reinterpret_cast<void*>(&HookedScreenManagerDraw),
                reinterpret_cast<void**>(&g_originalScreenManagerDraw),
                "UI::ScreenManager::Draw")) {
            return false;
        }

        if (!InstallExportHook(
                userInterfaceModule,
                kDeactivateAllScreensExportName,
                reinterpret_cast<void*>(&HookedDeactivateAllScreens),
                reinterpret_cast<void**>(&g_originalDeactivateAllScreens),
                "UI::ScreenManager::DeactivateAllScreens")) {
            return false;
        }

        if (!InstallExportHook(
                userInterfaceModule,
                kUnloadScreenExportName,
                reinterpret_cast<void*>(&HookedUnloadScreen),
                reinterpret_cast<void**>(&g_originalUnloadScreen),
                "UI::ScreenManager::UnloadScreen")) {
            return false;
        }

        LogOnce(g_loggedUiHooksInstalled, CoHModSDKLogLevel_Info, "Match Timer hooked UI::ScreenManager::Draw for a native Taskbar timer");
        return true;
    }

    bool OnInitialize() {
        g_shutdownInProgress = false;

        if (!InstallSimHooks()) {
            return false;
        }

        if (!InstallUiHooks()) {
            return false;
        }

        ModSDK::Runtime::LogInfo("Match Timer initialized");
        return true;
    }

    void OnShutdown() {
        g_shutdownInProgress = true;
        RetireTaskbarTimerLabel();
    }

    const CoHModSDKModuleV1 kModule = {
        .abiVersion = COHMODSDK_ABI_VERSION,
        .size = sizeof(CoHModSDKModuleV1),
        .modId = "de.tosox.matchtimer",
        .name = "Match Timer",
        .version = "1.0.0",
        .author = "Tosox",
        .OnInitialize = &OnInitialize,
        .OnShutdown = &OnShutdown,
    };
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    DisableThreadLibraryCalls(hModule);
    return TRUE;
}

COHMODSDK_EXPORT_MODULE(kModule);
