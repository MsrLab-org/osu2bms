#pragma once
#ifndef OSU_2_BMS_O2B_CONVERTER_HPP_INCLUDED
#define OSU_2_BMS_O2B_CONVERTER_HPP_INCLUDED

#include <map>
#include <typeinfo>
#include <type_traits>

#include <Osu.hpp>
#include <Bms.hpp>

#include "O2BConvertionOptions.hpp"
#include "O2BException.hpp"

namespace Osu2Bms {

    class O2BConverter {
    public:
        O2BConverter(const O2BConvertionOptions &options);
    public:
        Bms::BmsBeatmap operator()(const Osu::OsuBeatmap &osuBeatmap) throw(O2BException);
    private:
        const O2BConvertionOptions &_Options;
    private:
        struct _Note {
            enum AssociateObjectType {
                TimingPoint,
                Event,
                HitObject,
                CustomBpm
            };
            Bms::BmsChannelId Channel;
            Bms::BmsReferenceId ReferenceId;
            union {
                int32_t Time;
                double Position; // {Section}.{Persontage} [0, 1000)
            };
            AssociateObjectType AssociatedObjectType;
            size_t AssociatedObjectIndex;
        };
        std::vector<double> _GenerateBpms(const Osu::OsuBeatmap &osuBeatmap);
        std::vector<std::string> _GenerateWavs(const Osu::OsuBeatmap &osuBeatmap);
        std::vector<std::string> _GenerateBmps(const Osu::OsuBeatmap &osuBeatmap);
        std::vector<_Note> _GenerateNotes(
            const Osu::OsuBeatmap &osuBeatmap,
            const std::vector<double> &bpms,
            const std::vector<std::string> &wavs,
            const std::vector<std::string> &bmps);
        void _ConvertTimeToPosition(
            const Osu::OsuBeatmap &osuBeatmap,
            std::vector<_Note> &notes,
            const std::vector<double> &bpms);
        Bms::BmsBeatmap _GenerateBmsBeatmap(
            const Osu::OsuBeatmap &osuBeatmap,
            const std::vector<_Note> &notes,
            const std::vector<double> &bpms,
            const std::vector<std::string> &wavs,
            const std::vector<std::string> &bmps);
        void _PushBackSectionData(
            Bms::BmsBeatmap &bmsBeatmap,
            const uint16_t &section,
            const std::map<Bms::BmsChannelId, std::map<uint8_t, Bms::BmsReferenceId>> &data,
            const std::multimap<uint8_t, Bms::BmsReferenceId> &bgmNotes);
    };

}

#endif // !OSU_2_BMS_O2B_CONVERTER_HPP_INCLUDED
