#pragma once
#ifndef OSU_2_BMS__DETAIL_UTILITIES_HPP_INCLUDED
#define OSU_2_BMS__DETAIL_UTILITIES_HPP_INCLUDED

#include <type_traits>

namespace Osu2Bms {
    namespace _Detail {

        inline void FunctionNameHelper() {

#if defined(__GNUC__) || defined(__ICC) && (__ICC >= 600)
#   define OSU_2_BMS_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#   define OSU_2_BMS_FUNCTION_SIGNATURE __FUNCSIG__
#else
#   define OSU_2_BMS_FUNCTION_SIGNATURE "(Unsupported)"
#endif

        }

    }
}

#endif // !OSU_2_BMS__DETAIL_UTILITIES_HPP_INCLUDED
