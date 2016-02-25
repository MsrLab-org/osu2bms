#pragma once
#ifndef OSU_2_BMS_O2B_CONVERTION_OPTIONS_HPP_INCLUDED
#define OSU_2_BMS_O2B_CONVERTION_OPTIONS_HPP_INCLUDED

#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include <Bms.hpp>

namespace Osu2Bms {

    struct O2BConvertionOptions {
        double CustomBpm = 0;
        uint8_t CustomMeter = 4;
        uint8_t GridSize = 192;
        double Offset = 0;
        bool WithTimingPoints = true;
        bool WithInheritedTimingPoints = true;
        bool WithBmps = true;
        bool WithEventSounds = true;
        bool WithKeySounds = true;
        bool WithBga = true;
        std::vector<Bms::BmsChannelId> KeyMap;
    };

    struct O2BCommand {
        std::ifstream InputFile;
        std::ofstream OutputFile;
        O2BConvertionOptions Options;
    };

}

#endif // !OSU_2_BMS_O2B_CONVERTION_OPTIONS_HPP_INCLUDED
