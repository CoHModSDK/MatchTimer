#include "CoHModSDK.hpp"
#include "CoHModSDKUI.hpp"

#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace {
    using WorldSimulateFn = void(__thiscall*)(void*);
    using WorldGetSimManagerFn = void*(__thiscall*)(void*);
    using SimManagerClearGameTicksFn = void(__thiscall*)(void*);
    using SimManagerSetGameOverFn = void(__thiscall*)(void*);
    using SimManagerGetGameTimeFn = float(__thiscall*)(const void*);

    // Present in the SDK UI API but must remain manually resolved:
    // ExtractGetterOffset() disassembles raw game machine code at the function pointer
    // address — the SDK wraps these in C++ stubs, so the API functions cannot be used.
    using WidgetGetPresentationFn = void*(__thiscall*)(void*);
    using WidgetGetHitAreaFn = void*(__thiscall*)(void*);

    constexpr float kTimerLabelPositionX = 0.47500f;
    constexpr float kTimerLabelPositionY = -0.02000f;
    constexpr float kTimerLabelSizeX = 0.14000f;
    constexpr float kTimerLabelSizeY = 0.12600f;

    constexpr int kRenderProxyHAlignLeft = 0;
    constexpr int kRenderProxyHAlignCentre = 0;
    constexpr int kRenderProxyHAlignRight = 0;

    WorldSimulateFn oFnWorldSimulate = nullptr;
    WorldGetSimManagerFn oFnWorldGetSimManager = nullptr;
    SimManagerClearGameTicksFn oFnClearGameTicks = nullptr;
    SimManagerSetGameOverFn oFnSetGameOver = nullptr;
    SimManagerGetGameTimeFn oFnGetGameTime = nullptr;
    WidgetGetPresentationFn oFnWidgetGetPresentation = nullptr;
    WidgetGetHitAreaFn oFnWidgetGetHitArea = nullptr;

    std::uint32_t lastGameTime = 0u;
    bool matchActive = false;
    bool matchOver = false;

    bool shutdownInProgress = false;

    std::optional<ModSDK::UI::TextLabel> timerLabel;
    std::uint32_t lastUiLabelSeconds = UINT32_MAX;

    void* taskbarScreen = nullptr;
    void* taskbarRootWidget = nullptr;
    void* timerLabelRaw = nullptr;
    
    bool lastUiLabelVisible = false;
    bool uiLabelTextInitialized = false;

    bool InstallExportHook(HMODULE module, const char* exportName, void* detour, void** originalFunction, const char* hookDisplayName) {
        void* target = ModSDK::Memory::ResolveExport<void*>(module, exportName);
        if (target == nullptr) {
            char message[128] = {};
            std::snprintf(message, sizeof(message), "Failed to resolve %s", hookDisplayName);
			ModSDK::Dialogs::ShowError(message);
            return false;
        }

        if (!ModSDK::Hooks::CreateHook(target, detour, originalFunction)) {
            char message[128] = {};
            std::snprintf(message, sizeof(message), "Failed to create hook for %s", hookDisplayName);
            ModSDK::Dialogs::ShowError(message);
            return false;
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
        float gameTimeSeconds = oFnGetGameTime(simManager);
        if (!std::isfinite(gameTimeSeconds) || (gameTimeSeconds < 0.0f)) {
            gameTimeSeconds = 0.0f;
        }

        lastGameTime = static_cast<std::uint32_t>(std::lround(gameTimeSeconds * 1000.0f));
    }

    bool ShouldShowTimer() {
        return matchActive || matchOver;
    }

    void ResetTimerLabelBinding(bool destroyProxy) {
        if (destroyProxy) {
            timerLabel.reset();
        }

        lastUiLabelSeconds = UINT32_MAX;
        lastUiLabelVisible = false;
        uiLabelTextInitialized = false;
    }

    void ResetTaskbarTimerState(bool destroyProxy) {
        ResetTimerLabelBinding(destroyProxy);
        taskbarScreen = nullptr;
        taskbarRootWidget = nullptr;
        timerLabelRaw = nullptr;
    }

    void ApplyTimerLabelVisibility(bool visible) {
        if (!timerLabel.has_value()) {
            return;
        }

        timerLabel->SetVisible(visible);
        timerLabel->SetEnabled(visible);
    }

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
            reinterpret_cast<std::uintptr_t>(widget) + static_cast<std::uintptr_t>(offset));
        *field = nullptr;
    }

    void DetachSharedTimerLabelFields() {
        if (timerLabelRaw == nullptr) {
            return;
        }

        const int presentationOffset = ExtractGetterOffset(reinterpret_cast<void*>(oFnWidgetGetPresentation));
        const int hitAreaOffset = ExtractGetterOffset(reinterpret_cast<void*>(oFnWidgetGetHitArea));
        if (presentationOffset < 0) {
            return;
        }

        ZeroWidgetField(timerLabelRaw, presentationOffset);
        ZeroWidgetField(timerLabelRaw, hitAreaOffset);

        ModSDK::Runtime::LogDebug("Detached shared presentation state from the native Taskbar timer label before unload");
    }

    void RetireTaskbarTimerLabel() {
        ApplyTimerLabelVisibility(false);
        DetachSharedTimerLabelFields();
        if ((taskbarRootWidget != nullptr) && (timerLabelRaw != nullptr)) {
            ModSDK::UI::DetachRenderChild(taskbarRootWidget, timerLabelRaw);
        }
        ResetTaskbarTimerState(false);
    }

    void* ResolveTimerPresentationDonor(void* screenManager) {
        if (screenManager == nullptr) {
            return nullptr;
        }

        void* const gameScreen = ModSDK::UI::ScreenManager_FindScreen(screenManager, "GameScreen");
        if (gameScreen == nullptr) {
            return nullptr;
        }

        void* const gameRootWidget = ModSDK::UI::Screen_GetRootWidget(gameScreen);
        if (gameRootWidget == nullptr) {
            return nullptr;
        }

        return ModSDK::UI::FindWidget(gameRootWidget, "txtInfo");
    }

    bool BindTimerLabelProxy(void* rawWidget) {
        if (rawWidget == nullptr) {
            return false;
        }

        ResetTimerLabelBinding(true);
        timerLabel.emplace();
        timerLabel->Construct(rawWidget);
        timerLabel->SetAutoSize(false);
        timerLabel->SetMultiline(false);
        timerLabel->SetTextHAlign(kRenderProxyHAlignLeft);
        ApplyTimerLabelVisibility(false);

        return true;
    }

    bool CreateTaskbarTimerLabel(void* screenManager, void* taskbarRootWidget) {
        if ((screenManager == nullptr) || (taskbarRootWidget == nullptr)) {
            return false;
        }

        void* const donorRawWidget = ResolveTimerPresentationDonor(screenManager);
        if (donorRawWidget == nullptr) {
            ModSDK::Runtime::LogError("Failed to find GameScreen::txtInfo as the native timer donor widget");
            return false;
        }

        void* const rawWidget = ModSDK::UI::WidgetFactory_Create("TextLabel");
        if (rawWidget == nullptr) {
            ModSDK::Runtime::LogError("Failed to create the native Taskbar timer label widget");
            return false;
        }

        ModSDK::UI::CopyPresentation(rawWidget, donorRawWidget);
        ModSDK::UI::ConfigureWidget(rawWidget, taskbarRootWidget, "matchtimer_taskbar_label",
            kTimerLabelPositionX, kTimerLabelPositionY, kTimerLabelSizeX, kTimerLabelSizeY);
        if (!ModSDK::UI::AttachRenderChild(taskbarRootWidget, rawWidget)) {
            ModSDK::Runtime::LogError("Failed to attach the native Taskbar timer label to the HUD render tree");
            return false;
        }

        timerLabelRaw = rawWidget;
        return BindTimerLabelProxy(rawWidget);
    }

    bool EnsureTaskbarTimerLabel(void* screenManager) {
        if (screenManager == nullptr) {
            return false;
        }

        void* const resolvedTaskbarScreen = ModSDK::UI::ScreenManager_FindScreen(screenManager, "Taskbar");
        if (resolvedTaskbarScreen == nullptr) {
            ResetTaskbarTimerState(true);
            return false;
        }

        void* const resolvedTaskbarRootWidget = ModSDK::UI::Screen_GetRootWidget(resolvedTaskbarScreen);
        if (resolvedTaskbarRootWidget == nullptr) {
            ResetTaskbarTimerState(true);
            return false;
        }

        if ((taskbarScreen != resolvedTaskbarScreen) || (taskbarRootWidget != resolvedTaskbarRootWidget)) {
            ResetTimerLabelBinding(true);
            taskbarScreen = resolvedTaskbarScreen;
            taskbarRootWidget = resolvedTaskbarRootWidget;
            timerLabelRaw = nullptr;
        }

        void* const desiredRawWidget = ModSDK::UI::FindWidget(taskbarRootWidget, "matchtimer_taskbar_label");
        if (desiredRawWidget == nullptr) {
            return CreateTaskbarTimerLabel(screenManager, taskbarRootWidget);
        }

        if ((!timerLabel.has_value()) || (timerLabelRaw != desiredRawWidget)) {
            timerLabelRaw = desiredRawWidget;
            if (!BindTimerLabelProxy(desiredRawWidget)) {
                ModSDK::Runtime::LogError("Failed to bind to the native Taskbar timer label");
                return false;
            }
        }

        return true;
    }

    void UpdateTimerLabelText() {
        if (!timerLabel.has_value() || (timerLabelRaw == nullptr)) {
            return;
        }

        const bool shouldShow = ShouldShowTimer();
        const std::uint32_t totalSeconds = lastGameTime / 1000u;
        if (uiLabelTextInitialized && (shouldShow == lastUiLabelVisible) &&
            (!shouldShow || (totalSeconds == lastUiLabelSeconds))) {
            return;
        }

        if (!shouldShow) {
            uiLabelTextInitialized = true;
            lastUiLabelVisible = false;
            return;
        }

        const std::wstring labelText = FormatMatchTimeWide(totalSeconds);
        timerLabel->SetText(labelText.c_str());
        uiLabelTextInitialized = true;
        lastUiLabelVisible = true;
        lastUiLabelSeconds = totalSeconds;
    }

    void __fastcall HookedWorldSimulate(void* world, void* edx) {
        oFnWorldSimulate(world);

        matchActive = true;
        matchOver = false;

        void* simManager = oFnWorldGetSimManager(world);
        RefreshCachedMatchTime(simManager);
    }

    void __fastcall HookedClearGameTicks(void* simManager, void* edx) {
        oFnClearGameTicks(simManager);

        lastGameTime = 0u;
        matchActive = false;
        matchOver = false;
    }

    void __fastcall HookedSetGameOver(void* simManager, void* edx) {
        oFnSetGameOver(simManager);

        matchActive = true;
        matchOver = true;
        RefreshCachedMatchTime(simManager);
    }

    bool OnSmDrawPre(void* screenManager, float* deltaSeconds) {
        const bool shouldShow = ShouldShowTimer();
        if (EnsureTaskbarTimerLabel(screenManager)) {
            ApplyTimerLabelVisibility(shouldShow);
            if (shouldShow) {
                UpdateTimerLabelText();
            } else {
                lastUiLabelVisible = false;
            }
        }
        return true;
    }

    bool OnSmDeactivateAllPre(void* screenManager) {
        RetireTaskbarTimerLabel();
        return true;
    }

    bool OnSmUnloadScreenPre(void* screenManager, void* screen) {
        if (shutdownInProgress) {
            return true;
        }

        if ((screen != nullptr) && (screen == taskbarScreen)) {
            RetireTaskbarTimerLabel();
        }

        return true;
    }

    bool InstallSimHooks() {
        HMODULE hSimEngine = GetModuleHandleA("SimEngine.dll");
        if (!hSimEngine) {
            ModSDK::Dialogs::ShowError("Failed to get SimEngine module handle");
            return false;
        }

        oFnGetGameTime = ModSDK::Memory::ResolveExport<SimManagerGetGameTimeFn>(
            hSimEngine, "?GetGameTime@SimManager@@QBEMXZ");
        oFnWorldGetSimManager = ModSDK::Memory::ResolveExport<WorldGetSimManagerFn>(
            hSimEngine, "?GetSimManager@World@@QAEPAVSimManager@@XZ");
        if (!oFnGetGameTime || !oFnWorldGetSimManager) {
            ModSDK::Dialogs::ShowError("Failed to resolve required exports in SimEngine.dll");
            return false;
        }

        if (!InstallExportHook(hSimEngine, "?Simulate@World@@UAEXXZ",
                reinterpret_cast<void*>(&HookedWorldSimulate),
                reinterpret_cast<void**>(&oFnWorldSimulate), "World::Simulate")) {
            return false;
        }
        if (!InstallExportHook(hSimEngine, "?ClearGameTicks@SimManager@@IAEXXZ",
                reinterpret_cast<void*>(&HookedClearGameTicks),
                reinterpret_cast<void**>(&oFnClearGameTicks), "SimManager::ClearGameTicks")) {
            return false;
        }
        if (!InstallExportHook(hSimEngine, "?SetGameOver@SimManager@@QAEXXZ",
                reinterpret_cast<void*>(&HookedSetGameOver),
                reinterpret_cast<void**>(&oFnSetGameOver), "SimManager::SetGameOver")) {
            return false;
        }

        return true;
    }

    bool InstallUiHooks() {
        HMODULE hUserInterface = GetModuleHandleA("UserInterface.dll");
        if (!hUserInterface) {
            ModSDK::Dialogs::ShowError("Failed to get UserInterface module handle");
            return false;
        }

        oFnWidgetGetPresentation = ModSDK::Memory::ResolveExport<WidgetGetPresentationFn>(
            hUserInterface, "?GetPresentation@Widget@UI@@QAEPAVPresentation@2@XZ");
        oFnWidgetGetHitArea = ModSDK::Memory::ResolveExport<WidgetGetHitAreaFn>(
            hUserInterface, "?GetHitArea@Widget@UI@@QAEPAVHitArea@2@XZ");
        if (!oFnWidgetGetPresentation || !oFnWidgetGetHitArea) {
            ModSDK::Dialogs::ShowError("Failed to resolve required exports in UserInterface.dll");
            return false;
        }

        if (!ModSDK::UI::OnScreenManagerDraw(&OnSmDrawPre, nullptr)) {
            ModSDK::Dialogs::ShowError("Failed to create hook for ScreenManager::Draw");
            return false;
        }
        if (!ModSDK::UI::OnScreenManagerDeactivateAll(&OnSmDeactivateAllPre, nullptr)) {
            ModSDK::Dialogs::ShowError("Failed to create hook for ScreenManager::DeactivateAll");
            return false;
        }
        if (!ModSDK::UI::OnScreenManagerUnloadScreen(&OnSmUnloadScreenPre, nullptr)) {
            ModSDK::Dialogs::ShowError("Failed to create hook for ScreenManager::UnloadScreen");
            return false;
        }

        return true;
    }

    bool OnInitialize() {
        shutdownInProgress = false;

        if (!InstallSimHooks() || !InstallUiHooks()) {
            return false;
        }

        return true;
    }

    void OnShutdown() {
        shutdownInProgress = true;
        RetireTaskbarTimerLabel();
    }

    const CoHModSDKModuleV1 kModule = {
        .abiVersion = COHMODSDK_ABI_VERSION,
        .size = sizeof(CoHModSDKModuleV1),
        .modId = "de.tosox.matchtimer",
        .name = "Match Timer",
        .version = "1.1.0",
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
