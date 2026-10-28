/**
 * @file export.hpp
 * @brief 共享库导出宏。`DALYNCE_BUILD_SHARED=ON` 时定义 `DALYNCE_SDK_SHARED`。
 * @author ehow
 */
#pragma once

#if defined(DALYNCE_SDK_SHARED)
#    if defined(_WIN32)
#        if defined(DALYNCE_SDK_EXPORTS)
#            define DALYNCE_SDK_API __declspec(dllexport)
#        else
#            define DALYNCE_SDK_API __declspec(dllimport)
#        endif
#    else
#        define DALYNCE_SDK_API __attribute__((visibility("default")))
#    endif
#else
#    define DALYNCE_SDK_API
#endif
