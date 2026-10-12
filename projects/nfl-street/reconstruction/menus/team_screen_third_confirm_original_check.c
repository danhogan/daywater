#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define R13 0x803a9a40u
#define STACK 0x817df000u
#define BASE 0x81100000u

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return NULL; }
    long length = ftell(f);
    if (length < 0 || (size_t)length > RAM_SIZE) { fclose(f); return NULL; }
    rewind(f);
    uint8_t *data = malloc((size_t)length);
    if (!data || fread(data, 1, (size_t)length, f) != (size_t)length) {
        free(data); fclose(f); return NULL;
    }
    fclose(f); *size = (size_t)length; return data;
}

static int original_case(const ModernGekkoModuleDesc *module,
                         const uint8_t *baseline, size_t size, uint32_t raw) {
    uint8_t *ram = malloc(size);
    if (!ram) return 0;
    memcpy(ram, baseline, size);
    uint32_t dirty = R13 - 0x80000000u - 0x6e34u;
    uint32_t rebuild = R13 - 0x80000000u - 0x6e30u;
    if ((size_t)dirty + 2 > size || (size_t)rebuild + 2 > size) { free(ram); return 0; }
    for (unsigned i = 0; i < 2; ++i) {
        ram[dirty + i] = (uint8_t)(0x50 + i);
        ram[rebuild + i] = (uint8_t)(0x60 + i);
    }
    CPUState cpu = {0};
    cpu.ram = ram; cpu.ram_size = size; cpu.msr = 0x2000;
    cpu.gpr[1] = STACK; cpu.gpr[2] = 0x803b9a40u; cpu.gpr[13] = R13;
    cpu.gpr[3] = raw; cpu.pc = 0x8011e9ac; cpu.lr = 0;
    cpu.cycle_budget = 1000000;
    for (unsigned step = 0; cpu.pc && step < 1000; ++step) {
        uint32_t pc = cpu.pc;
        cpu.downcount = -1000000000;
        module->dispatch(&cpu, pc);
        if (cpu.exception) { free(ram); return 0; }
    }
    unsigned side = raw != 0;
    int ok = cpu.pc == 0 && cpu.gpr[1] == STACK;
    for (unsigned i = 0; ok && i < 2; ++i) {
        uint8_t dirty_expected = side == i ? 1 : (uint8_t)(0x50 + i);
        uint8_t rebuild_expected = side == i ? 1 : (uint8_t)(0x60 + i);
        if (ram[dirty + i] != dirty_expected || ram[rebuild + i] != rebuild_expected) ok = 0;
    }
    for (size_t i = 0; ok && i < size; ++i) {
        uint8_t expected = baseline[i];
        if (i == dirty) expected = side == 0 ? 1 : 0x50;
        else if (i == dirty + 1) expected = side == 1 ? 1 : 0x51;
        else if (i == rebuild) expected = side == 0 ? 1 : 0x60;
        else if (i == rebuild + 1) expected = side == 1 ? 1 : 0x61;
        if (ram[i] != expected) ok = 0;
    }
    free(ram); return ok;
}

static int native_case(uint32_t raw) {
    nflstreet_team_screen state = {0};
    state.activation.sides[0].dirty = 0x50;
    state.activation.sides[0].rebuild = 0x60;
    state.activation.sides[1].dirty = 0x51;
    state.activation.sides[1].rebuild = 0x61;
    nflstreet_team_screen expected = state;
    unsigned side = raw != 0;
    expected.activation.sides[side].dirty = 1;
    expected.activation.sides[side].rebuild = 1;
    uint8_t bytes[32], bytes_before[sizeof bytes];
    memset(bytes, 0xa7, sizeof bytes); memcpy(bytes_before, bytes, sizeof bytes);
    uint32_t words[256] = {0}, words_before[256];
    memcpy(words_before, words, sizeof words);
    uint32_t output = 0xdeadbeef;
    nflstreet_ui_service_request request = {0};
    request.kind = 1; request.property = 0x80000007u; request.argc = 1;
    request.args[0] = raw;
    if (!nflstreet_team_screen_memory(&state, bytes, sizeof bytes, BASE,
            &request, &output, STACK, words) ||
        memcmp(&state, &expected, sizeof state) ||
        memcmp(bytes, bytes_before, sizeof bytes) ||
        memcmp(words, words_before, sizeof words) || output != 0xdeadbeef) return 0;

    nflstreet_team_screen before_invalid = state;
    request.argc = 0;
    if (nflstreet_team_screen_memory(&state, bytes, sizeof bytes, BASE,
            &request, &output, STACK, words) ||
        memcmp(&state, &before_invalid, sizeof state) ||
        memcmp(bytes, bytes_before, sizeof bytes) ||
        memcmp(words, words_before, sizeof words) || output != 0xdeadbeef) return 0;
    request.argc = 2;
    if (nflstreet_team_screen_memory(&state, bytes, sizeof bytes, BASE,
            &request, &output, STACK, words) ||
        memcmp(&state, &before_invalid, sizeof state) ||
        memcmp(bytes, bytes_before, sizeof bytes) ||
        memcmp(words, words_before, sizeof words) || output != 0xdeadbeef) return 0;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: team-screen-third-confirm-original-check.exe <original-DLL> <snapshot.mem1>\n");
        return 2;
    }
    HMODULE library = LoadLibraryA(argv[1]);
    if (!library) return 3;
    ModernGekkoGetModuleFn get_module = (ModernGekkoGetModuleFn)
        GetProcAddress(library, MODERNGEKKO_GET_MODULE_SYMBOL);
    const ModernGekkoModuleDesc *module = get_module ? get_module() : NULL;
    if (!module || module->cpu_state_size != sizeof(CPUState)) return 3;
    size_t size = 0;
    uint8_t *baseline = read_file(argv[2], &size);
    if (!baseline || size != RAM_SIZE) return 4;
    const uint32_t raw_values[] = {0, 1, 2, UINT32_MAX};
    for (unsigned i = 0; i < sizeof raw_values / sizeof raw_values[0]; ++i) {
        if (!original_case(module, baseline, size, raw_values[i]) ||
            !native_case(raw_values[i])) {
            fprintf(stderr, "Third-confirm callback comparison failed raw=%08x\n", raw_values[i]);
            return 5;
        }
    }
    puts("Original8011E9AC and native high80000007 boundary agree for raw args 0,1,2,FFFFFFFF: side=(arg!=0), only dirty/rebuild bytes or fields are set to1; other side/state, source RAM, stack words, and output sentinel stay unchanged. Wrong argc0/2 reject atomically. This verifies the callback boundary only; later confirmation/lineup changes are separate.");
    free(baseline); FreeLibrary(library); return 0;
}
