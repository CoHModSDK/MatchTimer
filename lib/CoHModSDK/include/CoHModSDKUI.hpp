/**
 *  CoHModSDK - Shared runtime SDK for Company of Heroes
 *  Copyright (c) 2026 Tosox
 *
 *  This project is licensed under the Creative Commons
 *  Attribution-NonCommercial-NoDerivatives 4.0 International License
 *  (CC BY-NC-ND 4.0) with additional permissions.
 *
 *  Independent mods using this project only through its public interfaces
 *  are not required to use CC BY-NC-ND 4.0.
 *
 *  See the repository root LICENSE file for the full license text and
 *  additional permissions.
 */

#pragma once

#include "CoHModSDK.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

// Opaque storage sizes and alignments for game UI objects.
// Allocate buffers with at least these sizes and the specified alignment
// before passing them to the Construct/Destruct API functions.
inline constexpr std::size_t COHMODSDK_UI_LOCSTRING_STORAGE_SIZE        = 256u;
inline constexpr std::size_t COHMODSDK_UI_LOCSTRING_STORAGE_ALIGN       = alignof(void*);
inline constexpr std::size_t COHMODSDK_UI_TEXTLABEL_STORAGE_SIZE        = 8192u;
inline constexpr std::size_t COHMODSDK_UI_TEXTLABEL_STORAGE_ALIGN       = 16u;
inline constexpr std::size_t COHMODSDK_UI_BUTTON_STORAGE_SIZE           = 16384u;
inline constexpr std::size_t COHMODSDK_UI_BUTTON_STORAGE_ALIGN          = 16u;
inline constexpr std::size_t COHMODSDK_UI_CHECKBUTTON_STORAGE_SIZE      = 16384u;
inline constexpr std::size_t COHMODSDK_UI_CHECKBUTTON_STORAGE_ALIGN     = 16u;
inline constexpr std::size_t COHMODSDK_UI_ARTLABEL_STORAGE_SIZE         = 16384u;
inline constexpr std::size_t COHMODSDK_UI_ARTLABEL_STORAGE_ALIGN        = 16u;
inline constexpr std::size_t COHMODSDK_UI_GENERICWIDGET_STORAGE_SIZE    = 16384u;
inline constexpr std::size_t COHMODSDK_UI_GENERICWIDGET_STORAGE_ALIGN   = 16u;

// Extension ID passed to FindWidgetExtension to get the render child list.
inline constexpr int COHMODSDK_UI_DRAW_CHILDREN_EXTENSION_ID = 1;

// Flag passed to WidgetFactory_Create for standard widget creation.
inline constexpr int COHMODSDK_UI_WIDGET_FACTORY_CREATE_FLAG = 1;

extern "C" {
    // ScreenManager::Draw hook — fires on each render frame.
    using CoHModSDKSMDrawPreFn  = bool(*)(void* screenManager, float* deltaSeconds);
    using CoHModSDKSMDrawPostFn = void(*)(void* screenManager, float deltaSeconds);

    // ScreenManager::Update hook — fires on each logic update.
    using CoHModSDKSMUpdatePreFn  = bool(*)(void* screenManager, float* deltaTime);
    using CoHModSDKSMUpdatePostFn = void(*)(void* screenManager, float deltaTime);

    // ScreenManager::DeactivateAllScreens hook.
    using CoHModSDKSMDeactivateAllPreFn  = bool(*)(void* screenManager);
    using CoHModSDKSMDeactivateAllPostFn = void(*)(void* screenManager);

    // ScreenManager::UnloadScreen hook.
    using CoHModSDKSMUnloadScreenPreFn  = bool(*)(void* screenManager, void* screen);
    using CoHModSDKSMUnloadScreenPostFn = void(*)(void* screenManager, void* screen);

    struct CoHModSDKUiApiV1 {
        std::uint32_t abiVersion;
        std::uint32_t size;

        // --- ScreenManager hooks ---
        // Register pre/post callbacks for ScreenManager::Draw (render loop).
        // Pre returning false skips the original call and all post callbacks.
        bool (*OnScreenManagerDraw)(CoHModSDKSMDrawPreFn pre, CoHModSDKSMDrawPostFn post);
        // Register pre/post callbacks for ScreenManager::Update (logic loop).
        bool (*OnScreenManagerUpdate)(CoHModSDKSMUpdatePreFn pre, CoHModSDKSMUpdatePostFn post);
        // Register pre/post callbacks for ScreenManager::DeactivateAllScreens.
        bool (*OnScreenManagerDeactivateAll)(CoHModSDKSMDeactivateAllPreFn pre, CoHModSDKSMDeactivateAllPostFn post);
        // Register pre/post callbacks for ScreenManager::UnloadScreen.
        bool (*OnScreenManagerUnloadScreen)(CoHModSDKSMUnloadScreenPreFn pre, CoHModSDKSMUnloadScreenPostFn post);

        // --- ScreenManager / Screen ---
        // Returns the ScreenManager singleton. May be null if not yet initialized.
        void* (*ScreenManager_GetInstance)();
        // Finds a screen by name. Returns null if not found.
        void* (*ScreenManager_FindScreen)(void* screenManager, const char* name);
        // Returns the root widget of a screen.
        void* (*Screen_GetRootWidget)(void* screen);

        // --- Widget property setters/getters ---
        void  (*Widget_SetPosition)(void* widget, float x, float y);
        void  (*Widget_SetSize)(void* widget, float w, float h);
        // SetName is optional in the game — may be a no-op if the export is absent.
        void  (*Widget_SetName)(void* widget, const char* name);
        void  (*Widget_SetParent)(void* widget, void* parent);
        void* (*Widget_GetPresentation)(void* widget);
        void  (*Widget_SetPresentation)(void* widget, void* presentation);
        void* (*Widget_GetHitArea)(void* widget);
        void  (*Widget_SetHitArea)(void* widget, void* hitArea);

        // --- WidgetProxy ---
        void (*WidgetProxy_Bind)(void* proxy, void* widget);
        void (*WidgetProxy_SetVisible)(void* proxy, bool visible);
        void (*WidgetProxy_SetEnabled)(void* proxy, bool enabled);
        // hAlign: HAlign enum value (0 = left, 1 = center, 2 = right). fontTag may be null.
        void (*WidgetProxy_SetTextHAlign)(void* proxy, int hAlign, const char* fontTag);

        // --- TextLabel (caller-allocated storage) ---
        void (*TextLabel_Construct)(void* storage);
        void (*TextLabel_Destruct)(void* storage);
        void (*TextLabel_SetText)(void* textLabel, const void* locString);
        void (*TextLabel_SetAutoSize)(void* textLabel, bool autoSize);
        void (*TextLabel_SetMultiline)(void* textLabel, bool multiline);

        // --- Button (caller-allocated storage) ---
        void (*Button_Construct)(void* storage);
        void (*Button_Destruct)(void* storage);
        void (*Button_SetText)(void* button, const void* locString);

        // --- CheckButton (caller-allocated storage) ---
        void (*CheckButton_Construct)(void* storage);
        void (*CheckButton_Destruct)(void* storage);
        void (*CheckButton_SetChecked)(void* checkButton, bool checked);
        bool (*CheckButton_GetChecked)(void* checkButton);

        // --- ArtLabel (caller-allocated storage) ---
        void (*ArtLabel_Construct)(void* storage);
        void (*ArtLabel_Destruct)(void* storage);
        void (*ArtLabel_SetAllArtVisible)(void* artLabel, bool visible);

        // --- GenericWidget (caller-allocated storage) ---
        void (*GenericWidget_Construct)(void* storage);
        void (*GenericWidget_Destruct)(void* storage);

        // --- LocString (caller-allocated storage) ---
        void (*LocString_Construct)(void* storage, const wchar_t* text);
        void (*LocString_Destruct)(void* storage);

        // --- Widget factory and tree operations ---
        // Creates a raw widget by type name (e.g. "TextLabel", "Button", "CheckButton",
        // "ArtLabel", "Group"). Returns null on failure. Uses COHMODSDK_UI_WIDGET_FACTORY_CREATE_FLAG.
        void* (*WidgetFactory_Create)(const char* typeName, int flag);
        // Returns an extension object attached to a widget (e.g. the render child list).
        void* (*FindWidgetExtension)(void* widget, int extensionId);
        // Searches the widget tree under root for a widget with the given name.
        // index selects which subtree to search (try 0 then 1 for exhaustive search).
        void* (*FindWidgetByName)(void* root, const char* name, int index);
        // Appends childWidget to parentRenderObject's child list at insertIndex (-1 = append).
        // parentRenderObject must be the render extension (from FindWidgetExtension), not the widget itself.
        bool  (*AddRenderChild)(void* parentRenderObject, void* childWidget, int insertIndex);

        // --- Composite helpers ---
        // Calls Widget_SetParent (if parent != null), Widget_SetName (if name != null),
        // Widget_SetPosition, and Widget_SetSize in sequence.
        void (*ConfigureWidget)(void* widget, void* parent, const char* name,
                                float x, float y, float w, float h);
        // Gets the DrawChildren extension of parent and appends child to it.
        bool (*AttachRenderChild)(void* parent, void* child);
        // Gets the DrawChildren extension of parent and removes child from it.
        bool (*DetachRenderChild)(void* parent, void* child);
        // Copies the presentation pointer from srcWidget to dstWidget.
        void (*CopyPresentation)(void* dstWidget, void* srcWidget);
    };

    COHMODSDK_RUNTIME_API bool CoHModSDK_GetUiApi(std::uint32_t abiVersion, const CoHModSDKUiApiV1** outApi);
}

namespace ModSDK {
    namespace UIDetail {
        inline const CoHModSDKUiApiV1& GetUiApi() {
            static const CoHModSDKUiApiV1* api = []() -> const CoHModSDKUiApiV1* {
                const CoHModSDKUiApiV1* resolvedApi = nullptr;
                if (!CoHModSDK_GetUiApi(COHMODSDK_ABI_VERSION, &resolvedApi) || (resolvedApi == nullptr)) {
                    throw std::runtime_error("CoHModSDK UI API is unavailable");
                }
                return resolvedApi;
            }();
            return *api;
        }
    }

    namespace UI {
        // --- Hook registration ---

        inline bool OnScreenManagerDraw(CoHModSDKSMDrawPreFn pre, CoHModSDKSMDrawPostFn post) {
            return UIDetail::GetUiApi().OnScreenManagerDraw(pre, post);
        }

        inline bool OnScreenManagerUpdate(CoHModSDKSMUpdatePreFn pre, CoHModSDKSMUpdatePostFn post) {
            return UIDetail::GetUiApi().OnScreenManagerUpdate(pre, post);
        }

        inline bool OnScreenManagerDeactivateAll(CoHModSDKSMDeactivateAllPreFn pre, CoHModSDKSMDeactivateAllPostFn post) {
            return UIDetail::GetUiApi().OnScreenManagerDeactivateAll(pre, post);
        }

        inline bool OnScreenManagerUnloadScreen(CoHModSDKSMUnloadScreenPreFn pre, CoHModSDKSMUnloadScreenPostFn post) {
            return UIDetail::GetUiApi().OnScreenManagerUnloadScreen(pre, post);
        }

        // --- ScreenManager / Screen ---

        inline void* ScreenManager_GetInstance() {
            return UIDetail::GetUiApi().ScreenManager_GetInstance();
        }

        inline void* ScreenManager_FindScreen(void* screenManager, const char* name) {
            return UIDetail::GetUiApi().ScreenManager_FindScreen(screenManager, name);
        }

        inline void* Screen_GetRootWidget(void* screen) {
            return UIDetail::GetUiApi().Screen_GetRootWidget(screen);
        }

        // --- Widget ---

        inline void Widget_SetPosition(void* widget, float x, float y) {
            UIDetail::GetUiApi().Widget_SetPosition(widget, x, y);
        }

        inline void Widget_SetSize(void* widget, float w, float h) {
            UIDetail::GetUiApi().Widget_SetSize(widget, w, h);
        }

        inline void Widget_SetName(void* widget, const char* name) {
            UIDetail::GetUiApi().Widget_SetName(widget, name);
        }

        inline void Widget_SetParent(void* widget, void* parent) {
            UIDetail::GetUiApi().Widget_SetParent(widget, parent);
        }

        inline void* Widget_GetPresentation(void* widget) {
            return UIDetail::GetUiApi().Widget_GetPresentation(widget);
        }

        inline void Widget_SetPresentation(void* widget, void* presentation) {
            UIDetail::GetUiApi().Widget_SetPresentation(widget, presentation);
        }

        inline void* Widget_GetHitArea(void* widget) {
            return UIDetail::GetUiApi().Widget_GetHitArea(widget);
        }

        inline void Widget_SetHitArea(void* widget, void* hitArea) {
            UIDetail::GetUiApi().Widget_SetHitArea(widget, hitArea);
        }

        // --- WidgetProxy ---

        inline void WidgetProxy_Bind(void* proxy, void* widget) {
            UIDetail::GetUiApi().WidgetProxy_Bind(proxy, widget);
        }

        inline void WidgetProxy_SetVisible(void* proxy, bool visible) {
            UIDetail::GetUiApi().WidgetProxy_SetVisible(proxy, visible);
        }

        inline void WidgetProxy_SetEnabled(void* proxy, bool enabled) {
            UIDetail::GetUiApi().WidgetProxy_SetEnabled(proxy, enabled);
        }

        inline void WidgetProxy_SetTextHAlign(void* proxy, int hAlign, const char* fontTag = nullptr) {
            UIDetail::GetUiApi().WidgetProxy_SetTextHAlign(proxy, hAlign, fontTag);
        }

        // --- Widget factory / tree ---

        inline void* WidgetFactory_Create(const char* typeName, int flag = COHMODSDK_UI_WIDGET_FACTORY_CREATE_FLAG) {
            return UIDetail::GetUiApi().WidgetFactory_Create(typeName, flag);
        }

        inline void* FindWidgetExtension(void* widget, int extensionId = COHMODSDK_UI_DRAW_CHILDREN_EXTENSION_ID) {
            return UIDetail::GetUiApi().FindWidgetExtension(widget, extensionId);
        }

        inline void* FindWidgetByName(void* root, const char* name, int index = 0) {
            return UIDetail::GetUiApi().FindWidgetByName(root, name, index);
        }

        // Searches both the logical widget subtree (index 0) and the render child subtree
        // (index 1) for a widget with the given name, returning the first match.
        // Use this when you don't know which subtree the target widget was added to.
        inline void* FindWidget(void* root, const char* name) {
            void* widget = UIDetail::GetUiApi().FindWidgetByName(root, name, 0);
            if (widget == nullptr) {
                widget = UIDetail::GetUiApi().FindWidgetByName(root, name, 1);
            }
            return widget;
        }

        inline bool AddRenderChild(void* parentRenderObject, void* childWidget, int insertIndex = -1) {
            return UIDetail::GetUiApi().AddRenderChild(parentRenderObject, childWidget, insertIndex);
        }

        // --- Composite helpers ---

        inline void ConfigureWidget(void* widget, void* parent, const char* name,
                                    float x, float y, float w, float h) {
            UIDetail::GetUiApi().ConfigureWidget(widget, parent, name, x, y, w, h);
        }

        inline bool AttachRenderChild(void* parent, void* child) {
            return UIDetail::GetUiApi().AttachRenderChild(parent, child);
        }

        inline bool DetachRenderChild(void* parent, void* child) {
            return UIDetail::GetUiApi().DetachRenderChild(parent, child);
        }

        inline void CopyPresentation(void* dstWidget, void* srcWidget) {
            UIDetail::GetUiApi().CopyPresentation(dstWidget, srcWidget);
        }

        // --- RAII wrappers ---

        class LocString {
        public:
            LocString() = delete;
            explicit LocString(const wchar_t* text) {
                UIDetail::GetUiApi().LocString_Construct(storage_, text);
            }
            ~LocString() {
                UIDetail::GetUiApi().LocString_Destruct(storage_);
            }
            LocString(const LocString&) = delete;
            LocString& operator=(const LocString&) = delete;
            void* get() noexcept { return storage_; }
            const void* get() const noexcept { return storage_; }

        private:
            alignas(COHMODSDK_UI_LOCSTRING_STORAGE_ALIGN)
            std::byte storage_[COHMODSDK_UI_LOCSTRING_STORAGE_SIZE];
        };

        // All proxy widget classes (TextLabel, Button, CheckButton, ArtLabel, GenericWidget)
        // use deferred construction: the default constructor zeros storage but does NOT call
        // the game constructor. Call Construct() explicitly when the widget tree is ready.
        // This makes it safe to declare instances as struct members or globals.

        class TextLabel {
        public:
            TextLabel() = default;
            ~TextLabel() {
                if (constructed_) { UIDetail::GetUiApi().TextLabel_Destruct(storage_); }
            }
            TextLabel(const TextLabel&) = delete;
            TextLabel& operator=(const TextLabel&) = delete;
            void* get() noexcept { return storage_; }

            // Calls game ctor; optionally binds to rawWidget in one step.
            void Construct(void* rawWidget = nullptr) {
                UIDetail::GetUiApi().TextLabel_Construct(storage_);
                if (rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
                constructed_ = true;
            }
            void Reset() {
                if (constructed_) { UIDetail::GetUiApi().TextLabel_Destruct(storage_); constructed_ = false; }
            }
            void Bind(void* rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
            void SetVisible(bool v) { UIDetail::GetUiApi().WidgetProxy_SetVisible(storage_, v); }
            void SetEnabled(bool v) { UIDetail::GetUiApi().WidgetProxy_SetEnabled(storage_, v); }
            void SetTextHAlign(int hAlign, const char* fontTag = nullptr) { UIDetail::GetUiApi().WidgetProxy_SetTextHAlign(storage_, hAlign, fontTag); }
            void SetText(const LocString& ls) { UIDetail::GetUiApi().TextLabel_SetText(storage_, ls.get()); }
            void SetText(const wchar_t* text) { LocString ls(text); UIDetail::GetUiApi().TextLabel_SetText(storage_, ls.get()); }
            void SetAutoSize(bool v) { UIDetail::GetUiApi().TextLabel_SetAutoSize(storage_, v); }
            void SetMultiline(bool v) { UIDetail::GetUiApi().TextLabel_SetMultiline(storage_, v); }

        private:
            alignas(COHMODSDK_UI_TEXTLABEL_STORAGE_ALIGN)
            std::byte storage_[COHMODSDK_UI_TEXTLABEL_STORAGE_SIZE] = {};
            bool constructed_ = false;
        };

        class Button {
        public:
            Button() = default;
            ~Button() {
                if (constructed_) { UIDetail::GetUiApi().Button_Destruct(storage_); }
            }
            Button(const Button&) = delete;
            Button& operator=(const Button&) = delete;
            void* get() noexcept { return storage_; }

            void Construct(void* rawWidget = nullptr) {
                UIDetail::GetUiApi().Button_Construct(storage_);
                if (rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
                constructed_ = true;
            }
            void Reset() {
                if (constructed_) { UIDetail::GetUiApi().Button_Destruct(storage_); constructed_ = false; }
            }
            void Bind(void* rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
            void SetVisible(bool v) { UIDetail::GetUiApi().WidgetProxy_SetVisible(storage_, v); }
            void SetEnabled(bool v) { UIDetail::GetUiApi().WidgetProxy_SetEnabled(storage_, v); }
            void SetTextHAlign(int hAlign, const char* fontTag = nullptr) { UIDetail::GetUiApi().WidgetProxy_SetTextHAlign(storage_, hAlign, fontTag); }
            void SetText(const LocString& ls) { UIDetail::GetUiApi().Button_SetText(storage_, ls.get()); }
            void SetText(const wchar_t* text) { LocString ls(text); UIDetail::GetUiApi().Button_SetText(storage_, ls.get()); }

        private:
            alignas(COHMODSDK_UI_BUTTON_STORAGE_ALIGN)
            std::byte storage_[COHMODSDK_UI_BUTTON_STORAGE_SIZE] = {};
            bool constructed_ = false;
        };

        class CheckButton {
        public:
            CheckButton() = default;
            ~CheckButton() {
                if (constructed_) { UIDetail::GetUiApi().CheckButton_Destruct(storage_); }
            }
            CheckButton(const CheckButton&) = delete;
            CheckButton& operator=(const CheckButton&) = delete;
            void* get() noexcept { return storage_; }

            void Construct(void* rawWidget = nullptr) {
                UIDetail::GetUiApi().CheckButton_Construct(storage_);
                if (rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
                constructed_ = true;
            }
            void Reset() {
                if (constructed_) { UIDetail::GetUiApi().CheckButton_Destruct(storage_); constructed_ = false; }
            }
            void Bind(void* rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
            void SetVisible(bool v) { UIDetail::GetUiApi().WidgetProxy_SetVisible(storage_, v); }
            void SetEnabled(bool v) { UIDetail::GetUiApi().WidgetProxy_SetEnabled(storage_, v); }
            void SetTextHAlign(int hAlign, const char* fontTag = nullptr) { UIDetail::GetUiApi().WidgetProxy_SetTextHAlign(storage_, hAlign, fontTag); }
            void SetChecked(bool v) { UIDetail::GetUiApi().CheckButton_SetChecked(storage_, v); }
            bool GetChecked() { return UIDetail::GetUiApi().CheckButton_GetChecked(storage_); }

        private:
            alignas(COHMODSDK_UI_CHECKBUTTON_STORAGE_ALIGN)
            std::byte storage_[COHMODSDK_UI_CHECKBUTTON_STORAGE_SIZE] = {};
            bool constructed_ = false;
        };

        class ArtLabel {
        public:
            ArtLabel() = default;
            ~ArtLabel() {
                if (constructed_) { UIDetail::GetUiApi().ArtLabel_Destruct(storage_); }
            }
            ArtLabel(const ArtLabel&) = delete;
            ArtLabel& operator=(const ArtLabel&) = delete;
            void* get() noexcept { return storage_; }

            void Construct(void* rawWidget = nullptr) {
                UIDetail::GetUiApi().ArtLabel_Construct(storage_);
                if (rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
                constructed_ = true;
            }
            void Reset() {
                if (constructed_) { UIDetail::GetUiApi().ArtLabel_Destruct(storage_); constructed_ = false; }
            }
            void Bind(void* rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
            void SetVisible(bool v) { UIDetail::GetUiApi().WidgetProxy_SetVisible(storage_, v); }
            void SetEnabled(bool v) { UIDetail::GetUiApi().WidgetProxy_SetEnabled(storage_, v); }
            void SetTextHAlign(int hAlign, const char* fontTag = nullptr) { UIDetail::GetUiApi().WidgetProxy_SetTextHAlign(storage_, hAlign, fontTag); }
            void SetAllArtVisible(bool v) { UIDetail::GetUiApi().ArtLabel_SetAllArtVisible(storage_, v); }

        private:
            alignas(COHMODSDK_UI_ARTLABEL_STORAGE_ALIGN)
            std::byte storage_[COHMODSDK_UI_ARTLABEL_STORAGE_SIZE] = {};
            bool constructed_ = false;
        };

        class GenericWidget {
        public:
            GenericWidget() = default;
            ~GenericWidget() {
                if (constructed_) { UIDetail::GetUiApi().GenericWidget_Destruct(storage_); }
            }
            GenericWidget(const GenericWidget&) = delete;
            GenericWidget& operator=(const GenericWidget&) = delete;
            void* get() noexcept { return storage_; }

            void Construct(void* rawWidget = nullptr) {
                UIDetail::GetUiApi().GenericWidget_Construct(storage_);
                if (rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
                constructed_ = true;
            }
            void Reset() {
                if (constructed_) { UIDetail::GetUiApi().GenericWidget_Destruct(storage_); constructed_ = false; }
            }
            void Bind(void* rawWidget) { UIDetail::GetUiApi().WidgetProxy_Bind(storage_, rawWidget); }
            void SetVisible(bool v) { UIDetail::GetUiApi().WidgetProxy_SetVisible(storage_, v); }
            void SetEnabled(bool v) { UIDetail::GetUiApi().WidgetProxy_SetEnabled(storage_, v); }
            void SetTextHAlign(int hAlign, const char* fontTag = nullptr) { UIDetail::GetUiApi().WidgetProxy_SetTextHAlign(storage_, hAlign, fontTag); }

        private:
            alignas(COHMODSDK_UI_GENERICWIDGET_STORAGE_ALIGN)
            std::byte storage_[COHMODSDK_UI_GENERICWIDGET_STORAGE_SIZE] = {};
            bool constructed_ = false;
        };
    }
}
