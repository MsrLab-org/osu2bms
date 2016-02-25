#include "O2BConverter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "_Detail/Utilities.hpp"

namespace Osu2Bms {

    O2BConverter::O2BConverter(const O2BConvertionOptions &options)
        : _Options(options) {}

    Bms::BmsBeatmap O2BConverter::operator()(const Osu::OsuBeatmap &osuBeatmap) throw(O2BException) {
        using namespace std;
        cout << "Generating BPMs..." << endl;
        auto bpms = _GenerateBpms(osuBeatmap);
        cout << "Generating WAVs..." << endl;
        auto wavs = _GenerateWavs(osuBeatmap);
        cout << "Generating BMPs..." << endl;
        auto bmps = _GenerateBmps(osuBeatmap);
        cout << "Generating notes..." << endl;
        auto notes = _GenerateNotes(osuBeatmap, bpms, wavs, bmps);
        cout << "Converting time to position..." << endl;
        _ConvertTimeToPosition(osuBeatmap, notes, bpms);
        cout << "Generating BMS beatmap..." << endl;
        return _GenerateBmsBeatmap(osuBeatmap, notes, bpms, wavs, bmps);
    }

    std::vector<double> O2BConverter::_GenerateBpms(
        const Osu::OsuBeatmap &osuBeatmap) {
        using namespace std;
        vector<double> bpms;
        if (_Options.WithTimingPoints) {
            if (osuBeatmap.TimingPoints.size() == 0 || osuBeatmap.TimingPoints.front().Inherited) {
                throw O2BException(
                    std::string("in ") + OSU_2_BMS_FUNCTION_SIGNATURE
                    + ": First TimingPoint should be non-inherited");
            }
            double last = nan(nullptr);
            for (const auto &tp : osuBeatmap.TimingPoints) {
                if (!tp.Inherited || _Options.WithInheritedTimingPoints) {
                    double bpm = last;
                    if (tp.Inherited) {
                        bpm /= tp.Ratio();
                    } else {
                        last = bpm = tp.BeatsPerMinute();
                    }
                    auto it = lower_bound(bpms.begin(), bpms.end(), bpm);
                    if (it == bpms.end() || *it != bpm) {
                        bpms.insert(it, bpm);
                    }
                }
            }
        }
        return move(bpms);
    }

    std::vector<std::string> O2BConverter::_GenerateWavs(
        const Osu::OsuBeatmap &osuBeatmap) {
        using namespace std;
        vector<string> wavs;
        auto addWav = [&wavs](const std::string &wav) {
            auto it = lower_bound(wavs.begin(), wavs.end(), wav);
            if (it == wavs.end() || *it != wav) {
                wavs.insert(it, wav);
            }
        };
        addWav(osuBeatmap.AudioFilename);
        if (_Options.WithKeySounds) {
            for (auto &object : osuBeatmap.HitObjects) {
                auto wav = object->StartPoint.CustomHitSound;
                if (wav) {
                    addWav(*wav);
                }
            }
        }
        if (_Options.WithEventSounds) {
            for (auto &event : osuBeatmap.Events) {
                if (std::dynamic_pointer_cast<Osu::OsuSoundEffectEvent>(event)) {
                    addWav(event->FilePath);
                }
            }
        }
        return move(wavs);
    }

    std::vector<std::string> O2BConverter::_GenerateBmps(
        const Osu::OsuBeatmap &osuBeatmap) {
        using namespace std;
        vector<string> bmps;
        if (_Options.WithBga) {
            for (auto &event : osuBeatmap.Events) {
                if (std::dynamic_pointer_cast<Osu::OsuVideoEvent>(event)) {
                    const auto &bmp = event->FilePath;
                    auto it = lower_bound(bmps.begin(), bmps.end(), bmp);
                    if (it == bmps.end() || *it != bmp) {
                        bmps.insert(it, bmp);
                    }
                }
            }
        }
        return move(bmps);
    }

    std::vector<O2BConverter::_Note> O2BConverter::_GenerateNotes(
        const Osu::OsuBeatmap &osuBeatmap,
        const std::vector<double> &bpms,
        const std::vector<std::string> &wavs,
        const std::vector<std::string> &bmps) {
        using namespace std;
        using namespace Bms;
        using namespace Osu;
        vector<_Note> notes;
        double last = nan(nullptr);
        if (_Options.WithTimingPoints) {
            for (size_t i = 0; i < osuBeatmap.TimingPoints.size(); ++i) {
                const auto &tp = osuBeatmap.TimingPoints[i];
                if (!tp.Inherited || _Options.WithInheritedTimingPoints) {
                    double bpm = last;
                    if (tp.Inherited) {
                        bpm /= tp.Ratio();
                    } else {
                        last = bpm = tp.BeatsPerMinute();
                    }
                    _Note note;
                    note.Channel = BmsChannelId::Bpm2;
                    note.ReferenceId =
                        lower_bound(bpms.begin(), bpms.end(), bpm)
                        - bpms.begin() + 1;
                    note.Time = tp.Time;
                    note.AssociatedObjectType = _Note::TimingPoint;
                    note.AssociatedObjectIndex = i;
                    notes.push_back(note);
                }
            }
        }
        _Note bgmNote;
        bgmNote.Time = osuBeatmap.AudioLeadIn;
        bgmNote.Channel = BmsChannelId::Bgm;
        bgmNote.ReferenceId = lower_bound(wavs.begin(), wavs.end(), osuBeatmap.AudioFilename) - wavs.begin() + 1;
        notes.push_back(bgmNote);
        for (size_t i = 0; i < osuBeatmap.Events.size(); ++i) {
            const auto &event = osuBeatmap.Events[i];
            string fileName = event->FilePath;
            if (auto sound = dynamic_pointer_cast<OsuSoundEffectEvent>(event)) {
                if (_Options.WithEventSounds) {
                    _Note note;
                    note.Time = sound->Time;
                    note.Channel = BmsChannelId::Bgm;
                    note.ReferenceId = lower_bound(wavs.begin(), wavs.end(), fileName) - wavs.begin() + 1;
                    note.AssociatedObjectType = _Note::Event;
                    note.AssociatedObjectIndex = i;
                    notes.push_back(note);
                }
            } else if (auto video = dynamic_pointer_cast<OsuVideoEvent>(event)) {
                if (_Options.WithBga) {
                    _Note note;
                    note.Time = video->Time;
                    note.Channel = BmsChannelId::Bga;
                    note.ReferenceId = lower_bound(bmps.begin(), bmps.end(), fileName) - bmps.begin() + 1;
                    note.AssociatedObjectType = _Note::Event;
                    note.AssociatedObjectIndex = i;
                    notes.push_back(note);
                }
            }
        }
        auto keyCount = osuBeatmap.ManiaKeyCount();
        if (_Options.KeyMap.size() != keyCount) {
            throw O2BException(
                std::string("in ") + OSU_2_BMS_FUNCTION_SIGNATURE
                + ": Key map size mismatched");
        }
        for (size_t i = 0; i < osuBeatmap.HitObjects.size(); ++i) {
            const auto &o = osuBeatmap.HitObjects[i];
            auto column = o->StartPoint.ManiaColumn(keyCount);
            BmsReferenceId ref("ZZ");
            if (_Options.WithKeySounds) {
                if (const auto &wav = o->StartPoint.CustomHitSound) {
                    ref = lower_bound(wavs.begin(), wavs.end(), *wav) - wavs.begin() + 1;
                }
            }
            BmsChannelId channel = _Options.KeyMap[column];
            if (auto ln = dynamic_pointer_cast<OsuHold>(o)) {
                _Note endNote;
                channel = static_cast<BmsChannelId>(
                    static_cast<underlying_type_t<BmsChannelId>>(
                        _Options.KeyMap[column]) + 40);
                endNote.Channel = channel;
                endNote.ReferenceId = ref;
                endNote.Time = ln->EndTime;
                endNote.AssociatedObjectType = _Note::HitObject;
                endNote.AssociatedObjectIndex = i;
                notes.push_back(endNote);
            }
            _Note note;
            note.Channel = channel;
            note.ReferenceId = ref;
            note.Time = o->StartTime;
            note.AssociatedObjectType = _Note::HitObject;
            note.AssociatedObjectIndex = i;
            notes.push_back(note);
        }
        sort(notes.begin(), notes.end(), [](const _Note &lhs, const _Note &rhs) {
            return lhs.Time < rhs.Time;
        });
        return move(notes);
    }

    void O2BConverter::_ConvertTimeToPosition(
        const Osu::OsuBeatmap &osuBeatmap,
        std::vector<_Note> &notes,
        const std::vector<double> &bpms) {
        using namespace std;
        using namespace Bms;
        if (notes.size() == 0) {
            return;
        }
        double bpm;
        if (_Options.WithTimingPoints) {
            const auto &firstTp = osuBeatmap.TimingPoints.front();
            bpm = firstTp.BeatsPerMinute() / firstTp.Meter;
        } else {
            bpm = _Options.CustomBpm / _Options.CustomMeter;
        }
        double position = _Options.Offset / _Options.GridSize;
        auto time = notes.front().Time;
        for (auto &note : notes) {
            auto newPosition = position + static_cast<double>(note.Time - time) / 60000.0 * bpm;
            if (note.Channel == BmsChannelId::Bpm2) {
                const auto &tp = osuBeatmap.TimingPoints[note.AssociatedObjectIndex];
                bpm = bpms[note.ReferenceId.UnderlyingValue() - 1] / tp.Meter;
                position = newPosition;
                time = note.Time;
            }
            note.Position = newPosition;
        }
    }

    Bms::BmsBeatmap O2BConverter::_GenerateBmsBeatmap(
        const Osu::OsuBeatmap &osuBeatmap,
        const std::vector<_Note> &notes,
        const std::vector<double> &bpms,
        const std::vector<std::string> &wavs,
        const std::vector<std::string> &bmps) {
        using namespace std;
        using namespace Bms;
        BmsBeatmap bmsBeatmap;
        auto keyCount = osuBeatmap.ManiaKeyCount();
        uint16_t section = 0;
        map<BmsChannelId, map<uint8_t, BmsReferenceId>> data;
        multimap<uint8_t, BmsReferenceId> bgmNotes;
        for (const auto &note : notes) {
            uint16_t noteSection = floor(note.Position);
            if (noteSection != section) {
                _PushBackSectionData(bmsBeatmap, section, data, bgmNotes);
                data.clear();
                bgmNotes.clear();
                section = noteSection;
            }
            uint8_t index = static_cast<uint8_t>((note.Position - noteSection) * _Options.GridSize);
            if (note.Channel == BmsChannelId::Bgm) {
                bgmNotes.insert({index, note.ReferenceId});
            } else {
                data[note.Channel][index] = note.ReferenceId;
            }
        }
        _PushBackSectionData(bmsBeatmap, section, data, bgmNotes);
        bmsBeatmap.Artist = osuBeatmap.ArtistUnicode;
        bmsBeatmap.Bpm = _Options.WithTimingPoints ? osuBeatmap.TimingPoints.front().BeatsPerMinute() : _Options.CustomBpm;
        bmsBeatmap.Title = osuBeatmap.TitleUnicode;
        bmsBeatmap.LongNoteType = BmsLongNoteType::NotePair;
        for (const auto &event : osuBeatmap.Events) {
            if (auto backgroundEvent = dynamic_pointer_cast<Osu::OsuBackgroundEvent>(event)) {
                bmsBeatmap.Cover = backgroundEvent->FilePath;
                break;
            }
        }
        for (size_t i = 0; i < bpms.size(); ++i) {
            bmsBeatmap.BpmMap[i + 1] = bpms[i];
        }
        for (size_t i = 0; i < wavs.size(); ++i) {
            bmsBeatmap.WavMap[i + 1] = wavs[i];
        }
        if (_Options.WithBga) {
            for (size_t i = 0; i < bmps.size(); ++i) {
                bmsBeatmap.BmpMap[i + 1] = bmps[i];
            }
        }
        return move(bmsBeatmap);
    }

    void O2BConverter::_PushBackSectionData(
        Bms::BmsBeatmap &bmsBeatmap,
        const uint16_t &section,
        const std::map<Bms::BmsChannelId, std::map<uint8_t, Bms::BmsReferenceId>> &data,
        const std::multimap<uint8_t, Bms::BmsReferenceId> &bgmNotes) {
        using namespace std;
        using namespace Bms;
        auto it = bgmNotes.cbegin();
        vector<shared_ptr<BmsReferenceListDataUnit>> units;
        const BmsDataUnitId bgmUnitId(section, BmsChannelId::Bgm);
        for (uint8_t i = 0; i < _Options.GridSize; ++i) {
            if (it != bgmNotes.cend() && it->first == i) {
                if (units.empty()) {
                    units.push_back(make_shared<BmsReferenceListDataUnit>(bgmUnitId));
                    units.back()->Value = BmsReferenceListDataUnit::ValueType(_Options.GridSize, 0);
                }
                while (it != bgmNotes.cend() && it->first == i) {
                    bool inserted = false;
                    for (auto &unit : units) {
                        if (unit->Value[i].IsPlaceholder()) {
                            unit->Value[i] = it->second;
                            inserted = true;
                            break;
                        }
                    }
                    if (!inserted) {
                        units.push_back(make_shared<BmsReferenceListDataUnit>(bgmUnitId));
                        units.back()->Value = BmsReferenceListDataUnit::ValueType(_Options.GridSize, 0);
                        units.back()->Value[i] = it->second;
                    }
                    ++it;
                }
            }
        }
        for (auto &unit : units) {
            unit->Shrink();
        }
        for (const auto &unit : units) {
            bmsBeatmap.MainData.push_back(unit);
        }
        for (const auto &field : data) {
            auto &channel = field.first;
            auto &notes = field.second;
            auto it = notes.cbegin();
            auto unit = make_shared<BmsReferenceListDataUnit>(BmsDataUnitId(section, channel));
            unit->Value = BmsReferenceListDataUnit::ValueType(_Options.GridSize, 0);
            for (uint8_t i = 0; i < _Options.GridSize; ++i) {
                if (it != notes.cend() && it->first == i) {
                    unit->Value[i] = it->second;
                    ++it;
                }
            }
            unit->Shrink();
            bmsBeatmap.MainData.push_back(unit);
        }
    }

}
