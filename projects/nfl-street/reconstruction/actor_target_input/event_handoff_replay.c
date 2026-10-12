/* Derive event targets from actor descriptors, then replay record handoffs.
   Actions come from the reference trace; native scheduling is absent. */
#include "event_descriptor.h"
#include "target_selection.h"

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

static int hexbytes(const char *text, uint8_t *output, unsigned count)
{
    unsigned i;
    if (strlen(text) != count * 2) return 0;
    for (i = 0; i < count; ++i) {
        char pair[3] = {text[2*i], text[2*i+1], 0};
        char *end;
        unsigned long value;
        if (!strchr("0123456789abcdefABCDEF", pair[0]) ||
            !strchr("0123456789abcdefABCDEF", pair[1])) return 0;
        value = strtoul(pair, &end, 16);
        if (*end) return 0;
        output[i] = (uint8_t)value;
    }
    return 1;
}

static uint32_t be32(const uint8_t *data)
{
    return (uint32_t)data[0] << 24 | (uint32_t)data[1] << 16 |
           (uint32_t)data[2] << 8 | data[3];
}

int main(int argc, char **argv)
{
    uint32_t per_team, total, count, handles[255], roster[255], flags, current;
    uint8_t events[255][12], ready[255] = {0}, index, mode;
    uint8_t header[16], descriptor[6];
    uint16_t slot;
    unsigned i;
    int cursor;
    if (argc < 5 || !number(argv[1], &per_team) || !number(argv[2], &total) ||
        !number(argv[3], &count) || !per_team || per_team > 255 ||
        !total || total > 255 || !count || count > 255 ||
        (unsigned)argc <= 4 + count + total) return 2;
    cursor = 4;
    for (i = 0; i < count; ++i)
        if (!number(argv[cursor++], &handles[i])) return 2;
    for (i = 0; i < total; ++i)
        if (!number(argv[cursor++], &roster[i])) return 2;
    while (cursor < argc) {
        const char *action = argv[cursor++];
        if (!strcmp(action, "B")) {
            if (argc - cursor < 3 || !hexbytes(argv[cursor++], header, 16) ||
                !hexbytes(argv[cursor++], descriptor, 6) ||
                !nflstreet_actor_slot_from_handle(be32(header), (uint16_t)per_team,
                                                  (uint16_t)total, &slot) ||
                !hexbytes(argv[cursor++], events[slot], 12) ||
                !nflstreet_event_from_descriptor(header, descriptor, roster,
                    (uint16_t)per_team, (uint16_t)total, events[slot], &flags, &mode)) return 3;
            ready[slot] = 1;
            printf("event %u ", (unsigned)slot);
            for (i = 0; i < 12; ++i) printf("%02x", events[slot][i]);
            printf(" %08x %u\n", (unsigned)flags, (unsigned)mode);
        } else if (!strcmp(action, "H")) {
            if (cursor == argc || !number(argv[cursor++], &current) ||
                !nflstreet_actor_slot_from_handle(current, (uint16_t)per_team,
                                                  (uint16_t)total, &slot) || !ready[slot] ||
                !nflstreet_record_handle_handoff(handles, (uint8_t)count, current,
                    be32(events[slot]), (uint16_t)per_team, (uint16_t)total, &index)) return 3;
            printf("record %u", (unsigned)index);
            for (i = 0; i < count; ++i) printf(" %08x", (unsigned)handles[i]);
            putchar('\n');
        } else return 2;
    }
    return 0;
}
