#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options.hpp>

#include "Bms.hpp"
#include "Osu.hpp"
#include "O2BConverter.hpp"
#include "O2BException.hpp"

using namespace std;
using namespace experimental::filesystem;
using namespace boost::program_options;
using namespace command_line_style;
using namespace Osu;
using namespace Bms;
using namespace Osu2Bms;

options_description allOptions("options");
options_description visibleOptions("Usage: osu2bms <input-file> [<output-file>] [options]");

// Should just be called once
void InitializeOptions() {
    // Make options description
    options_description generic("generic options");
    generic.add_options()
        ("help", "show help message")
        ("version,v", "show version information");
    options_description config("configuration");
    config.add_options()
        ("bpm", value<double>(), "required by --no-timing-points, manually provide BPM value")
        ("key-map,m", value<vector<int>>()->multitoken(), "customize key map by providing a channel ID sequence")
        ("key-map-default", "use default BMS key definations")
        ("key-map-o2mania", "use BMS key definations in O2Mania")
        ("max-grid-size", value<int>()->default_value(192), "maximum grid partition size, [1, 255]")
        ("meter", value<int>()->default_value(4), "required by --no-timing-points, manually provide meter value")
        ("no-bga", "ignore BGAs")
        ("no-event-sounds", "ignore background sounds")
        ("no-inherited-timing-points", "ignore inherited timing points")
        ("no-key-sounds", "ignore key sounds")
        ("no-timing-points", "ignore timing points")
        ("offset", value<double>()->default_value(0.5), "an offset value of all notes, [0, 1)");
    options_description hidden("hidden options");
    hidden.add_options()
        ("input-file", value<string>(), "path to osu! beatmap file")
        ("output-file", value<string>(), "path to BMS beatmap file");
    allOptions.add(generic).add(config).add(hidden);
    visibleOptions.add(generic).add(config);
}

variables_map ParseArguments(const size_t argc, const char *argv[]) {
    // Parse arguments
    positional_options_description p;
    p.add("input-file", 1);
    p.add("output-file", 1);
    p.add("key-map", -1);
    variables_map vm;
    try {
        store(command_line_parser(argc, argv)
            .options(allOptions)
            .positional(p)
            .style(unix_style | allow_slash_for_short)
            .run(), vm);
    } catch (const error &e) {
        throw O2BException(e.what());
    }
    notify(vm);
    return move(vm);
}

int main(int argc, const char *argv[]) {
    try {
        InitializeOptions();
        auto vm = ParseArguments(argc, argv);
        if (vm.count("help")) {
            cout << visibleOptions << endl;
            return EXIT_SUCCESS;
        }
        if (vm.count("version")) {
            cout << "osu2bms version 0.0.1\n" << endl;
            return EXIT_SUCCESS;
        }
        ifstream fin;
        ofstream fout;
        string outputPath;
        if (!vm.count("input-file")) {
            throw O2BException("No input file");
        }
        auto inputPath = vm["input-file"].as<string>();
        auto len = inputPath.length();
        if (len >= 4 && inputPath.substr(len - 4) == ".osu") {
            fin.open(inputPath);
            if (!fin) {
                throw O2BException("Could not open file at " + inputPath);
            }
            outputPath = inputPath.substr(0, len - 4) + ".bms";
        } else {
            throw O2BException("Input file type must be .osu");
        }
        if (vm.count("output-file")) {
            outputPath = vm["output-file"].as<string>();
        }
        len = outputPath.length();
        if (len >= 4 && outputPath.substr(len - 4) == ".bms") {
            fout.open(outputPath);
            if (!fout) {
                throw O2BException("Could not open file at " + outputPath);
            }
        } else {
            throw O2BException("Output file type must be .bms");
        }
        OsuBeatmap osuBeatmap;
        fin >> osuBeatmap;
        O2BConvertionOptions options;
        auto keyCount = osuBeatmap.ManiaKeyCount();
        if (vm.count("key-map-o2mania")) {
            options.KeyMap = GetBmsChannels(keyCount, false, true, true);
        }
        if (vm.count("key-map-default")) {
            if (!options.KeyMap.empty()) {
                throw O2BException("Key maps are in conflict.");
            }
            options.KeyMap = GetBmsChannels(keyCount, false, true, false);
        }
        if (vm.count("key-map")) {
            if (!options.KeyMap.empty()) {
                throw O2BException("Key maps are in conflict.");
            }
            auto &channels = vm["key-map"].as<vector<int>>();
            for (auto channel : channels) {
                if (channel < 11 || channel > 19) {
                    throw O2BException("Key channels should be in range [11, 19]");
                }
                options.KeyMap.push_back(static_cast<BmsChannelId>(channel));
            }
        }
        auto gridSize = vm["max-grid-size"].as<int>();
        if (gridSize <= 0 || gridSize >= 256) {
            throw O2BException("Maximum grid partition size should be in range [1, 255]");
        }
        options.GridSize = static_cast<uint8_t>(gridSize);
        options.WithBga = vm.count("no-bga") == 0;
        options.WithEventSounds = vm.count("no-event-sounds") == 0;
        options.WithInheritedTimingPoints = vm.count("no-inherited-timing-points") == 0;
        options.WithKeySounds = vm.count("no-key-sounds") == 0;
        options.WithTimingPoints = vm.count("no-timing-points") == 0;
        if (!options.WithTimingPoints) {
            if (!vm.count("bpm")) {
                throw O2BException("BPM must be provided if --no-timing-points is set");
            }
            options.CustomBpm = vm["bpm"].as<double>();
            if (options.CustomBpm <= 0) {
                throw O2BException("BPM value must be greater than 0");
            }
            auto meter = vm["meter"].as<int>();
            if (meter <= 0) {
                throw O2BException("Meter value must be greater than 0");
            }
            options.CustomMeter = static_cast<uint8_t>(meter);
        }
        options.Offset = vm["offset"].as<double>();
        O2BConverter convert(options);
        auto bmsBeatmap = convert(osuBeatmap);
        fout << bmsBeatmap.StringValue() << endl;
    } catch (const OsuException &e) {
        cerr << e.Description() << endl;
    } catch (const BmsException &e) {
        cerr << e.Description() << endl;
    } catch (const O2BException &e) {
        cerr << e.Description() << endl;
    }
    return EXIT_SUCCESS;
}
