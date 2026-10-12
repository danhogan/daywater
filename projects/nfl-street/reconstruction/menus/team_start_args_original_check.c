/* Controlled comparison for the original load-time FFFE/FFFC source events.
 * Reuses the bounded original-module runner and explicit resource/service
 * hooks from team_manager_original_check.c. The source bytes are native
 * constructed immediately before FFFE; only event execution is original. */
#define main team_manager_original_check_unused_main
#include "team_manager_original_check.c"
#undef main
#include "team_start_catalog_service.h"

static int load_fixture_font(const char *dir, unsigned index, uint8_t *ram) {
    char path[4096];
    int n = snprintf(path, sizeof path, "%s/font-%u.bin", dir, index);
    if (n < 0 || (size_t)n >= sizeof path) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long size = ftell(f);
    if (size <= 0 || size > 65536 || fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
    unsigned data_at = 0x01740000u + index * 0x10000u;
    if (fread(ram + data_at, 1, (size_t)size, f) != (size_t)size) { fclose(f); return 0; }
    fclose(f);
    nflstreet_font font;
    if (!nflstreet_font_open(ram + data_at, (size_t)size, &font)) return 0;
    unsigned record = 0x017f3000u + index * 0x20u;
    unsigned asset = 0x017f3100u + index * 0x20u;
    const uint32_t words[5] = {0, 0x80000000u + data_at,
        0x80000000u + data_at + (uint32_t)font.glyphs,
        0x80000000u + data_at + (uint32_t)font.kerning,
        0x80000000u + data_at + (uint32_t)font.atlas};
    for (unsigned j = 0; j < 5; ++j) put(ram + record + j * 4, words[j]);
    put(ram + asset, 0x80000000u + record);
    return 1;
}

static nflstreet_start_catalog_service catalog_service;

static int catalog_step(CPUState *c) {
    return nflstreet_start_catalog_step(&catalog_service, c);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: team-start-args-original-check.exe <original-module.dll> <fixture-dir> <db_teams-dir>\n");
        return 2;
    }
    const char *dir = argv[2];
    if (!nflstreet_start_catalog_load(&catalog_service, argv[3])) {
        fprintf(stderr, "could not parse 32 actual MAET catalog blocks\n");
        return 4;
    }
    nflstreet_team_manager_set_external_step_hook(catalog_step);
    HMODULE h = LoadLibraryA(argv[1]);
    if (!h) return 3;
    ModernGekkoGetModuleFn get = (ModernGekkoGetModuleFn)GetProcAddress(h, MODERNGEKKO_GET_MODULE_SYMBOL);
    if (!get) return 3;
    const ModernGekkoModuleDesc *m = get();
    if (!m || m->cpu_state_size != sizeof(CPUState)) return 3;

    uint8_t *ram = calloc(1, RAM_SIZE);
    uint8_t *expected = malloc(SOURCE_SIZE);
    if (!ram || !expected) return 4;
    char path[4096];
    int n = snprintf(path, sizeof path, "%s/mapped-dol.bin", dir);
    if (n < 0 || (size_t)n >= sizeof path || !load_exact(path, ram, RAM_SIZE)) return 4;
    n = snprintf(path, sizeof path, "%s/native-prestart.bin", dir);
    if (n < 0 || (size_t)n >= sizeof path || !load_exact(path, ram + SCREEN - 0x80000000u, SOURCE_SIZE)) return 4;
    n = snprintf(path, sizeof path, "%s/native-post-start.bin", dir);
    if (n < 0 || (size_t)n >= sizeof path || !load_exact(path, expected, SOURCE_SIZE)) return 4;
    for (unsigned i = 0; i < 4; ++i) if (!load_fixture_font(dir, i, ram)) return 4;

    CPUState c = {0};
    c.ram = ram; c.ram_size = RAM_SIZE; c.msr = 0x2000;
    c.gpr[1] = 0x81700000u; c.gpr[2] = 0x803b9a40u; c.gpr[13] = 0x803a9a40u;
    uint32_t a[8];
    a[0] = 12; a[1] = 9; a[2] = 0xa0; a[3] = 7; a[4] = 0x280; a[5] = 0xa0;
    uint32_t manager_size = 0;
    if (!run(m, &c, 0x801cfb4c, a, 6, &manager_size) || manager_size > 0x10000u) return 5;
    a[0] = MANAGER; a[1] = 12; a[2] = 9; a[3] = 0xa0; a[4] = 7; a[5] = 0x280; a[6] = 0xa0; a[7] = 0x10;
    if (!run(m, &c, 0x801cfb80, a, 8, NULL)) return 5;
    a[0] = MANAGER;
    if (!run(m, &c, 0x8012aa2c, a, 1, NULL)) return 5;
    static const uint32_t callbacks[9] = {0x801d60dc,0x8012cb94,0x801da46c,
        0x8012c570,0x801dc660,0x8012c5d8,0x8012ca3c,0x8012b394,0x801d9244};
    for (unsigned i = 0; i < 9; ++i) put(ram + 0x017b0380u + i * 4u, callbacks[i]);

    /* The native fixture explicitly supplied logical port 0 and its ten
     * source lookup words. Preserve that same input for the original VM. */
    for (unsigned i = 0; i < 10; ++i) put(ram + 0x00282e34u + i * 4u,
        i == 0 ? 0u : UINT32_MAX);
    put(ram + 0x003a2a50u, 6);
    /* Match the native prepared fixture's supplied query/profile state: the
     * source startup branch reads query mode and per-side persisted IDs from
     * these r13-relative globals. */
    put(ram + 0x003a2ab8u, 1);
    put(ram + 0x003a2c04u, UINT32_MAX);
    put(ram + 0x003a2c08u, UINT32_MAX);
    put(ram + 0x003a2c28u, UINT32_MAX);
    put(ram + 0x003a2c2cu, UINT32_MAX);
    /* Match the native startup fixture's explicit no-profile state. The
     * original 80127900 reader consumes these signed profile bytes. */
    ram[0x003a404cu] = 0xff;
    ram[0x003a404du] = 0xff;

    /* Construct only the already-loaded source record. Native construction is
     * represented by native-prestart.bin; the original executes both events. */
    const uint32_t row = 0x817f0100u, args_addr = 0x817f1000u, result_addr = 0x817f1100u;
    put(ram + MANAGER - 0x80000000u + 0x38, 1);
    put(ram + MANAGER - 0x80000000u + 0x3c, row);
    put(ram + row - 0x80000000u, 0);
    ram[row - 0x80000000u + 4] = 0; ram[row - 0x80000000u + 5] = 0;
    ram[row - 0x80000000u + 6] = 0; ram[row - 0x80000000u + 7] = 7;
    put(ram + row - 0x80000000u + 8, 5);
    put(ram + row - 0x80000000u + 0x10, SCREEN);
    put(ram + args_addr - 0x80000000u, 1);
    put(ram + args_addr - 0x80000000u + 4, 0x69);
    ram[result_addr - 0x80000000u] = 0;
    script_callback_boundary = 0;
    constructor_boundary = 1;
    source_relocation_boundary = 1;
    trace_game_callbacks = 0;
    trace_source_frames = 0;
    trace_source_context = 0;

    uint8_t *before_start = malloc(SOURCE_SIZE), *after_fffe = malloc(SOURCE_SIZE);
    if (!before_start || !after_fffe) return 4;
    memcpy(before_start, ram + SCREEN - 0x80000000u, SOURCE_SIZE);

    /* Exact 801CFD90 FFFE call frame: param5=-1; param6=argc2;
     * param7 points at {1,69}; FFFC follows with argc0 and null args. */
    uint32_t fffe[9] = {MANAGER, row, MANAGER + 0x68, 0, UINT32_MAX,
        0xfffffffeu, 2, args_addr, result_addr};
    if (!run(m, &c, 0x801cf34c, fffe, 9, NULL)) {
        fprintf(stderr, "original FFFE failed pc=%08x lr=%08x ex=%08x catalog=%s\n", c.pc, c.lr, c.exception,
            catalog_service.last_error[0] ? catalog_service.last_error : "none");
        return 6;
    }
    memcpy(after_fffe, ram + SCREEN - 0x80000000u, SOURCE_SIZE);
    /* D1090 passes zero as CF34C param5, then the FFFC event in param6. */
    uint32_t fffc[9] = {MANAGER, row, MANAGER + 0x68, 0, 0,
        0xfffffffcu, 0, 0, result_addr};
    if (!run(m, &c, 0x801cf34c, fffc, 9, NULL)) {
        fprintf(stderr, "original FFFC failed pc=%08x lr=%08x ex=%08x\n", c.pc, c.lr, c.exception);
        return 7;
    }
    unsigned source_diff = 0, fffe_delta = 0, fffc_delta = 0;
    for (unsigned i = 0; i < SOURCE_SIZE; ++i) {
        source_diff += ram[SCREEN - 0x80000000u + i] != expected[i];
        fffe_delta += before_start[i] != after_fffe[i];
        fffc_delta += after_fffe[i] != ram[SCREEN - 0x80000000u + i];
    }
    printf("Original load-event comparison: FFFE argc2 {1,69}, argument FFFFFFFF ran; then FFFC argc0, argument 0 ran. Source-byte deltas: FFFE=%u, FFFC=%u; original post-FFFC versus native post-start differs=%u/%u bytes.\n",
        fffe_delta, fffc_delta, source_diff, SOURCE_SIZE);
    printf("Boundary record: source relocation and class construction are supplied by native-prestart.bin; font resources are supplied from fixture bytes; original VM and class handlers execute. Async/resource/game services remain explicit hooks in the inherited runner.\n");
    printf("Bounded actual TEAM service: %u MAET rows opened, %u cursor resets, %u advances, %u numeric reads, %u absent RUMT reads; unknown query count=%u.\n",
        catalog_service.context_count, catalog_service.contexts[0].resets,
        catalog_service.contexts[0].advances, catalog_service.row_reads,
        catalog_service.absent_rumt_reads, catalog_service.unsupported_queries);
    if (source_diff) {
        unsigned shown = 0;
        for (unsigned i = 0; i < SOURCE_SIZE && shown < 32; ++i) {
            if (ram[SCREEN - 0x80000000u + i] != expected[i]) {
                printf("  diff +%x original=%02x native=%02x\n", i,
                    ram[SCREEN - 0x80000000u + i], expected[i]); ++shown;
            }
        }
    }
    free(before_start); free(after_fffe); free(expected); free(ram); FreeLibrary(h);
    return source_diff ? 1 : 0;
}
