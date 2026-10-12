#include "event_signals.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int event_bytes(const char *text, uint8_t event[12])
{
    unsigned i;
    if (strlen(text) != 24) return 0;
    for (i = 0; i < 12; ++i) {
        char pair[3] = {text[2*i], text[2*i+1], 0};
        if (!strchr("0123456789abcdefABCDEF", pair[0]) ||
            !strchr("0123456789abcdefABCDEF", pair[1])) return 0;
        event[i] = (uint8_t)strtoul(pair, NULL, 16);
    }
    return 1;
}

int main(int argc, char **argv)
{
    uint32_t flags, command, current, per_team, total, count, handles[255], incoming_flags, dispatch;
    uint8_t event[12], index = 255, changed = 255;
    unsigned i;
    if (argc == 4 && !strcmp(argv[1], "C")) {
        if (!number(argv[2], &flags) || !number(argv[3], &command) ||
            !nflstreet_event_flag_command(command, &flags)) return 3;
        printf("%08x\n", (unsigned)flags);
        return 0;
    }
    if (argc < 11 || strcmp(argv[1], "G") || !event_bytes(argv[2], event) ||
        !number(argv[3], &flags) || !number(argv[4], &current) ||
        !number(argv[5], &per_team) || !number(argv[6], &total) ||
        !number(argv[7], &count) || !per_team || per_team > UINT16_MAX ||
        !total || total > UINT16_MAX || !count || count > 255 ||
        !number(argv[8], &incoming_flags) || !number(argv[9], &dispatch) ||
        (unsigned)argc != 10 + count) return 2;
    for (i = 0; i < count; ++i)
        if (!number(argv[10+i], &handles[i])) return 2;
    if (!nflstreet_stage3_record_poll(event, &flags, handles, (uint8_t)count,
        current, incoming_flags, dispatch, (uint16_t)per_team, (uint16_t)total, &index, &changed)) return 3;
    printf("%u %08x %u", (unsigned)changed, (unsigned)flags, (unsigned)index);
    for (i = 0; i < count; ++i) printf(" %08x", (unsigned)handles[i]);
    putchar('\n');
    return 0;
}
