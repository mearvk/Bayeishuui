/*
 * fighter_sheet_demo.cpp -- C++ demonstration of the Bayeishuui SLeeLa
 * character sheet reader / interpreter / disposer. Same behaviour as the C
 * demo; shares the fighter_sheet.h contract (C11/C++17 clean).
 * Build: make build-sheet-cpp ; run: ./build/bayeishuui-sheet-cpp <sheet>...
 */
#include <cstdio>
#include <iostream>
#include "fighter_sheet.h"

namespace {
void printFighter(const char *path, const BayFighter &f) {
    std::cout << "=== " << path << " ===\n";
    std::cout << "  name       : " << f.name << "\n";
    std::cout << "  rank       : " << f.rank << " (" << f.rankTitle << ")\n";
    std::cout << "  style      : " << f.style << "\n";
    std::cout << "  tactic     : " << f.tactic << "\n";
    std::cout << "  health     : " << f.health << "\n";
    std::cout << "  stamina    : " << f.stamina << "\n";
    std::printf("  reach      : %.2f m\n", f.reach / 100.0);
    std::cout << "  baseDamage : " << f.baseDamage << "\n";
    std::cout << "  windup     : " << f.windupMs << " ms\n";
    std::printf("  pace       : %.2fx\n", f.paceCentis / 100.0);
    std::cout << "  aggression : " << f.aggression << "/100\n";
    std::cout << "  lateral    : " << f.lateral << "/100\n";
    std::cout << "  colour     : (" << f.colorR << "," << f.colorG << "," << f.colorB << ")\n";
    std::cout << "  interpreted: " << (f.valid ? "yes" : "no (defaults)") << "\n";
}
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0]
                  << " <character-sheet.sleela> [more.sleela ...]\n";
        return 2;
    }
    for (int i = 1; i < argc; ++i) {
        BayFighter f;
        if (!bay_fighter_load(argv[i], &f)) {
            std::cerr << "could not read sheet: " << argv[i] << "\n";
            continue;
        }
        printFighter(argv[i], f);
        bay_fighter_dispose(&f);
        if (i + 1 < argc) std::cout << "\n";
    }
    return 0;
}
