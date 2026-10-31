/* SPDX-License-Identifier: GPL-3.0-or-later
 * Diagnostic loader probe: requires an ignored include prepared from the
 * pinned OpenBW source and a separately authenticated private IScript file.
 * This is not yet a Scarab scheduler or gameplay test. */
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include "bwgame.h"
using namespace bwgame;

int main(int argc, char **argv) {
    if (argc != 2) { std::cerr << "usage: probe authenticated-iscript.bin\n"; return 2; }
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) { std::cerr << "cannot open IScript\n"; return 2; }
    std::string input((std::istreambuf_iterator<char>(file)), {});
    if (!file.eof() && file.fail()) { std::cerr << "IScript read failed\n"; return 2; }
    if (input.size() != 40482) { std::cerr << "unexpected IScript size\n"; return 2; }
    bwgame::global_state st{};
    auto load_data_file = [&](auto &out, const char *name) {
        if (std::string(name) != "scripts/iscript.bin") throw std::runtime_error("unexpected input request");
        out.assign(input.begin(), input.end());
    };
#include "openbw_iscript_loader.inc"
    for (int id : {160, 286, 287, 292}) {
        auto it = st.iscript.scripts.find(id);
        if (it == st.iscript.scripts.end()) { std::cerr << "missing required script " << id << "\n"; return 1; }
        std::cout << "script " << id << " animations " << it->second.animation_pc.size() << "\n";
    }
    for (auto requirement : {std::pair<int, int>{160, 0}, {160, 1}, {160, 11},
                             {160, 12}, {160, 13}, {286, 0}, {286, 1},
                             {287, 0}, {287, 1}, {292, 0}, {292, 1}}) {
        const auto &animation = st.iscript.scripts.at(requirement.first).animation_pc;
        if (requirement.second >= static_cast<int>(animation.size()) ||
            animation[requirement.second] == 0 ||
            animation[requirement.second] >= st.iscript.program_data.size()) {
            std::cerr << "invalid required animation " << requirement.first << ":" << requirement.second << "\n";
            return 1;
        }
    }
    std::cout << "program words " << st.iscript.program_data.size() << "\n";
}
