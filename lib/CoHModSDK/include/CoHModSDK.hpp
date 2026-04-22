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

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>

#define COHMODSDK_ABI_VERSION 1u

#define COHMODSDK_HAS_FIELD(structPtr, fieldName) \
    ((structPtr)->size >= offsetof(std::remove_pointer_t<decltype(structPtr)>, fieldName) + sizeof((structPtr)->fieldName))

#if defined(COHMODSDK_RUNTIME_EXPORTS)
#define COHMODSDK_RUNTIME_API extern "C" __declspec(dllexport)
#else
#define COHMODSDK_RUNTIME_API extern "C" __declspec(dllimport)
#endif

#define COHMODSDK_MODULE_API extern "C" __declspec(dllexport)

extern "C" {
    struct CoHModSDKModContextV1;

    enum CoHModSDKLogLevel : std::uint32_t {
        CoHModSDKLogLevel_Debug = 0,
        CoHModSDKLogLevel_Info = 1,
        CoHModSDKLogLevel_Warning = 2,
        CoHModSDKLogLevel_Error = 3,
    };

    enum CoHModSDKConfigType : std::uint32_t {
        CoHModSDKConfigType_Bool = 0,
        CoHModSDKConfigType_Int = 1,
        CoHModSDKConfigType_Float = 2,
        CoHModSDKConfigType_Enum = 3,
    };

    enum CoHModSDKConfigFlags : std::uint32_t {
        CoHModSDKConfigFlags_None = 0,
        CoHModSDKConfigFlags_RestartRequired = 1u << 0,
    };

    struct CoHModSDKRuntimeInitV1 {
        std::uint32_t abiVersion;
        std::uint32_t size;
        const char* loaderDirectory;
        const char* modsDirectory;
        const char* configDirectory;
        const char* logPath;
        const char* gameModuleName;
    };

    struct CoHModSDKRuntimeInfoV1 {
        std::uint32_t abiVersion;
        std::uint32_t size;
        const char* runtimeVersion;
        const char* loaderDirectory;
        const char* modsDirectory;
        const char* configDirectory;
        const char* logPath;
        const char* gameModuleName;
    };

    struct CoHModSDKConfigValueV1 {
        CoHModSDKConfigType type;
        union {
            std::uint32_t boolValue;
            std::int32_t intValue;
            float floatValue;
            std::int32_t enumValue;
        };
    };

    using CoHModSDKConfigChangedCallback = void(*)(const char* modId, const char* optionId, const CoHModSDKConfigValueV1* value, void* userData);

    struct CoHModSDKConfigChoiceV1 {
        std::int32_t value;
        const char* valueId;
        const char* label;
    };

    struct CoHModSDKConfigOptionV1 {
        const char* optionId;
        const char* category;
        const char* label;
        const char* description;
        CoHModSDKConfigType type;
        CoHModSDKConfigValueV1 defaultValue;
        float minValue;
        float maxValue;
        float step;
        std::uint32_t flags;
        const CoHModSDKConfigChoiceV1* choices;
        std::uint32_t choiceCount;
        CoHModSDKConfigChangedCallback onChanged;
        void* userData;
    };

    struct CoHModSDKConfigSchemaV1 {
        const char* modId;
        const CoHModSDKConfigOptionV1* options;
        std::uint32_t optionCount;
    };

    using CoHModSDKConfigModVisitor = bool(*)(const char* modId, void* userData);
    using CoHModSDKConfigOptionVisitor = bool(*)(const CoHModSDKConfigOptionV1* option, const CoHModSDKConfigValueV1* currentValue, void* userData);

    struct CoHModSDKConfigModInfoV1 {
        std::uint32_t abiVersion;
        std::uint32_t size;
        const char* modId;
        const char* name;
        const char* version;
        const char* author;
    };

    struct CoHModSDKApiV1 {
        std::uint32_t abiVersion;
        std::uint32_t size;
        const CoHModSDKRuntimeInfoV1* (*GetRuntimeInfo)();
        void (*Log)(const CoHModSDKModContextV1* modContext, CoHModSDKLogLevel level, const char* message);
        void (*ShowError)(const CoHModSDKModContextV1* modContext, const char* message);
        std::optional<std::uintptr_t> (*FindPattern)(const char* moduleName, const char* signature);
        void (*PatchMemory)(void* destination, const void* source, std::size_t size);
        bool (*CreateHook)(void* targetFunction, void* detourFunction, void** originalFunction);
        bool (*RegisterConfigSchema)(const CoHModSDKConfigSchemaV1* schema);
        bool (*GetConfigValue)(const char* modId, const char* optionId, CoHModSDKConfigValueV1* outValue);
        bool (*SetConfigValue)(const char* modId, const char* optionId, const CoHModSDKConfigValueV1* value);
        bool (*EnumerateConfigMods)(CoHModSDKConfigModVisitor visitor, void* userData);
        bool (*EnumerateConfigOptions)(const char* modId, CoHModSDKConfigOptionVisitor visitor, void* userData);
        bool (*GetConfigModInfo)(const char* modId, CoHModSDKConfigModInfoV1* outInfo);
    };

    struct CoHModSDKModuleV1 {
        std::uint32_t abiVersion;
        std::uint32_t size;
        const char* modId;
        const char* name;
        const char* version;
        const char* author;
        bool (*OnInitialize)();
        void (*OnShutdown)();
    };

    COHMODSDK_RUNTIME_API bool CoHModSDKRuntime_Initialize(const CoHModSDKRuntimeInitV1* init);
    COHMODSDK_RUNTIME_API void CoHModSDKRuntime_EnableAllHooks();
    COHMODSDK_RUNTIME_API void CoHModSDKRuntime_Shutdown();
    COHMODSDK_RUNTIME_API bool CoHModSDKRuntime_RegisterMod(HMODULE modHandle, const CoHModSDKModuleV1* module, const CoHModSDKModContextV1** outContext);
    COHMODSDK_RUNTIME_API void CoHModSDKRuntime_UnregisterMod(HMODULE modHandle);
    COHMODSDK_RUNTIME_API bool CoHModSDK_GetApi(std::uint32_t abiVersion, const CoHModSDKApiV1** outApi);
}

#define COHMODSDK_EXPORT_MODULE(moduleInstance) \
    COHMODSDK_MODULE_API bool CoHMod_GetModule(std::uint32_t abiVersion, const CoHModSDKModuleV1** outModule) { \
        if ((outModule == nullptr) || (abiVersion < COHMODSDK_ABI_VERSION)) { \
            return false; \
        } \
        *outModule = &(moduleInstance); \
        return true; \
    } \
    COHMODSDK_MODULE_API void CoHMod_SetContext(const CoHModSDKModContextV1* modContext) { \
        ::ModSDK::Detail::SetModContext(modContext); \
    }

namespace ModSDK {
    namespace Detail {
        inline const CoHModSDKModContextV1*& ModContextStorage() {
            static const CoHModSDKModContextV1* modContext = nullptr;
            return modContext;
        }

        inline void SetModContext(const CoHModSDKModContextV1* modContext) {
            ModContextStorage() = modContext;
        }

        inline const CoHModSDKModContextV1* GetModContext() {
            const CoHModSDKModContextV1* modContext = ModContextStorage();
            if (modContext == nullptr) {
                throw std::runtime_error("CoHModSDK mod context is unavailable");
            }

            return modContext;
        }

        inline const CoHModSDKApiV1& GetApi() {
            static const CoHModSDKApiV1* api = []() -> const CoHModSDKApiV1* {
                const CoHModSDKApiV1* resolvedApi = nullptr;
                if (!CoHModSDK_GetApi(COHMODSDK_ABI_VERSION, &resolvedApi) || (resolvedApi == nullptr)) {
                    throw std::runtime_error("CoHModSDK runtime API is unavailable");
                }

                return resolvedApi;
            }();

            return *api;
        }
    }

    namespace Runtime {
        inline const CoHModSDKRuntimeInfoV1* GetInfo() {
            return Detail::GetApi().GetRuntimeInfo();
        }

        inline void Log(CoHModSDKLogLevel level, const char* message) {
            Detail::GetApi().Log(Detail::GetModContext(), level, message);
        }

        inline void LogDebug(const char* message) {
            Detail::GetApi().Log(Detail::GetModContext(), CoHModSDKLogLevel_Debug, message);
        }

        inline void LogInfo(const char* message) {
            Detail::GetApi().Log(Detail::GetModContext(), CoHModSDKLogLevel_Info, message);
        }

        inline void LogWarning(const char* message) {
            Detail::GetApi().Log(Detail::GetModContext(), CoHModSDKLogLevel_Warning, message);
        }

        inline void LogError(const char* message) {
            Detail::GetApi().Log(Detail::GetModContext(), CoHModSDKLogLevel_Error, message);
        }
    }

    namespace Dialogs {
        inline void ShowError(const char* message) {
            Detail::GetApi().ShowError(Detail::GetModContext(), message);
        }
    }

    namespace Memory {
        inline std::optional<std::uintptr_t> FindPattern(const char* moduleName, const char* signature) {
            return Detail::GetApi().FindPattern(moduleName, signature);
        }

        inline void PatchMemory(void* destination, const void* source, std::size_t size) {
            Detail::GetApi().PatchMemory(destination, source, size);
        }

        inline void* GetVTableEntry(void* instance, std::size_t index) {
            return (*static_cast<void***>(instance))[index];
        }

        template <typename T>
        inline T ResolveExport(HMODULE module, const char* exportName) {
            if ((module == nullptr) || (exportName == nullptr)) {
                return nullptr;
            }

            return reinterpret_cast<T>(GetProcAddress(module, exportName));
        }
    }

    namespace Hooks {
        inline bool CreateHook(void* targetFunction, void* detourFunction, void** originalFunction) {
            return Detail::GetApi().CreateHook(targetFunction, detourFunction, originalFunction);
        }
    }

    namespace Config {
        using Value = CoHModSDKConfigValueV1;
        using Choice = CoHModSDKConfigChoiceV1;
        using Option = CoHModSDKConfigOptionV1;
        using Schema = CoHModSDKConfigSchemaV1;
        using Type = CoHModSDKConfigType;
        using Flags = CoHModSDKConfigFlags;
        using ModInfo = CoHModSDKConfigModInfoV1;
        using ChangedCallback = CoHModSDKConfigChangedCallback;
        using ModVisitor = CoHModSDKConfigModVisitor;
        using OptionVisitor = CoHModSDKConfigOptionVisitor;

        inline Value MakeBoolValue(bool value) {
            Value result = {};
            result.type = CoHModSDKConfigType_Bool;
            result.boolValue = value ? 1u : 0u;
            return result;
        }

        inline Value MakeIntValue(std::int32_t value) {
            Value result = {};
            result.type = CoHModSDKConfigType_Int;
            result.intValue = value;
            return result;
        }

        inline Value MakeFloatValue(float value) {
            Value result = {};
            result.type = CoHModSDKConfigType_Float;
            result.floatValue = value;
            return result;
        }

        inline Value MakeEnumValue(std::int32_t value) {
            Value result = {};
            result.type = CoHModSDKConfigType_Enum;
            result.enumValue = value;
            return result;
        }

        inline bool RegisterSchema(const Schema& schema) {
            return Detail::GetApi().RegisterConfigSchema(&schema);
        }

        inline bool GetValue(const char* modId, const char* optionId, Value* outValue) {
            return Detail::GetApi().GetConfigValue(modId, optionId, outValue);
        }

        inline bool SetValue(const char* modId, const char* optionId, const Value& value) {
            return Detail::GetApi().SetConfigValue(modId, optionId, &value);
        }

        inline bool EnumerateMods(ModVisitor visitor, void* userData) {
            return Detail::GetApi().EnumerateConfigMods(visitor, userData);
        }

        inline bool EnumerateOptions(const char* modId, OptionVisitor visitor, void* userData) {
            return Detail::GetApi().EnumerateConfigOptions(modId, visitor, userData);
        }

        inline bool GetModInfo(const char* modId, ModInfo* outInfo) {
            return Detail::GetApi().GetConfigModInfo(modId, outInfo);
        }
    }
}
