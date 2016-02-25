#pragma once
#ifndef OSU_2_BMS_O2B_EXCEPTION_HPP_INCLUDED
#define OSU_2_BMS_O2B_EXCEPTION_HPP_INCLUDED

#include <string>

namespace Osu2Bms {

    class O2BException {
    public:
        O2BException(const std::string &description);
    public:
        const std::string &Description() const;
    private:
        std::string _Description;
    };

}

#endif // !OSU_2_BMS_O2B_EXCEPTION_HPP_INCLUDED
