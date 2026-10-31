/* SPDX-License-Identifier: GPL-3.0-or-later
 * Parse authenticated private tables through pinned OpenBW DAT loaders.
 * This is a format probe; it does not initialize or execute a game world. */
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "bwgame.h"
using namespace bwgame;
static std::vector<uint8_t> read(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("missing DAT: " + path);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
}
int main(int argc,char **argv) {
    if (argc != 2) { std::cerr << "usage: probe verified-table-dir\n"; return 2; }
    try {
        std::string p=std::string(argv[1])+"/";
        auto units=data_loading::load_units_dat(read(p+"units.dat"));
        auto weapons=data_loading::load_weapons_dat(read(p+"weapons.dat"));
        auto upgrades=data_loading::load_upgrades_dat(read(p+"upgrades.dat"));
        auto tech=data_loading::load_techdata_dat(read(p+"techdata.dat"));
        auto flingy=data_loading::load_flingy_dat(read(p+"flingy.dat"));
        auto sprites=data_loading::load_sprites_dat(read(p+"sprites.dat"));
        auto images=data_loading::load_images_dat(read(p+"images.dat"));
        auto orders=data_loading::load_orders_dat(read(p+"orders.dat"));
        const auto &a=orders.vec.at((size_t)Orders::ScarabAttack);
        const auto &b=orders.vec.at((size_t)Orders::ReaverStop);
        const auto &c=orders.vec.at((size_t)Orders::SelfDestructing);
        if ((int)Orders::ScarabAttack != 65 || (int)Orders::ReaverStop != 7 ||
            (int)Orders::SelfDestructing != 165 || !a.targets_enemies ||
            !a.can_be_interrupted || !b.can_be_interrupted || c.can_be_interrupted ||
            a.target_order != Orders::ScarabAttack || b.target_order != Orders::ReaverStop ||
            c.target_order != Orders::SelfDestructing) return 1;
        std::cout << "units " << units.vec.size() << " weapons " << weapons.vec.size()
                  << " upgrades " << upgrades.vec.size() << " tech " << tech.vec.size()
                  << " flingy " << flingy.vec.size() << " sprites " << sprites.vec.size()
                  << " images " << images.vec.size() << " orders " << orders.vec.size()
                  << "; ScarabAttack/ReaverStop/SelfDestructing rows checked\n";
    } catch (const std::exception &e) { std::cerr << e.what() << "\n"; return 1; }
}
