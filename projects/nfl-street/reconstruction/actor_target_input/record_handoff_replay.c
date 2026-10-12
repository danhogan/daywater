/* Native replay of observed record-handle handoffs; no emulator services. */
#include "target_selection.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int number(const char *text, uint32_t *value)
{
    char *end;
    unsigned long long parsed;
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno || end == text || *end || parsed > UINT32_MAX) return 0;
    *value = (uint32_t)parsed;
    return 1;
}

int main(int argc, char **argv)
{
    uint32_t per_team, total, count, handles[255], current, next;
    uint8_t index;
    unsigned i;
    int cursor;
    if (argc < 7 || !number(argv[1], &per_team) || !number(argv[2], &total) ||
        !number(argv[3], &count) || !per_team || per_team > UINT16_MAX ||
        !total || total > UINT16_MAX || !count || count > 255 ||
        (unsigned)argc < 4 + count + 2 || ((unsigned)argc - 4 - count) % 2) {
        fputs("usage: replay per_team total count initial_handles... current next [current next ...]\n", stderr);
        return 2;
    }
    for (i = 0; i < count; ++i)
        if (!number(argv[4 + i], &handles[i])) return 2;
    for (cursor = 4 + (int)count; cursor < argc; cursor += 2) {
        if (!number(argv[cursor], &current) || !number(argv[cursor + 1], &next) ||
            !nflstreet_record_handle_handoff(handles, (uint8_t)count, current,
                next, (uint16_t)per_team, (uint16_t)total, &index)) return 3;
        printf("%u", (unsigned)index);
        for (i = 0; i < count; ++i) printf(" %08x", (unsigned)handles[i]);
        putchar('\n');
    }
    return 0;
}
