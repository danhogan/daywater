#include "team_player_labels_callback.h"

#include <stdint.h>
#include <string.h>

enum { OWNER_INVALID = 0, OWNER_SCREEN = 1, OWNER_STACK = 2 };
#define VM_STACK_BYTES 1024u
#define LABEL_TEXT_MAX 255u

typedef struct { uint32_t address; size_t size; } memory_range;

static int in_range(uint32_t base, size_t size, uint32_t address, size_t bytes) {
    return address >= base && (size_t)(address - base) <= size &&
           bytes <= size - (size_t)(address - base);
}

static int overlap(memory_range a, memory_range b) {
    uint64_t a_end = (uint64_t)a.address + a.size;
    uint64_t b_end = (uint64_t)b.address + b.size;
    return (uint64_t)a.address < b_end && (uint64_t)b.address < a_end;
}

static int owner_of(size_t screen_size, uint32_t screen_base, uint32_t stack_base,
                    uint32_t address, size_t bytes) {
    int screen = in_range(screen_base, screen_size, address, bytes);
    int stack = in_range(stack_base, VM_STACK_BYTES, address, bytes);
    if (screen == stack) return OWNER_INVALID;
    return screen ? OWNER_SCREEN : OWNER_STACK;
}

static int read_word(uint8_t *screen, size_t screen_size, uint32_t screen_base,
                     uint32_t stack_base, uint32_t words[256], uint32_t address,
                     uint32_t *value) {
    if ((address & 3u) || !value) return 0;
    int owner = owner_of(screen_size, screen_base, stack_base, address, 4);
    if (owner == OWNER_SCREEN) {
        const uint8_t *p = screen + (address - screen_base);
        *value = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
                 (uint32_t)p[2] << 8 | p[3];
    } else if (owner == OWNER_STACK) {
        *value = words[(address - stack_base) / 4];
    } else return 0;
    return 1;
}

static int write_word(uint8_t *screen, size_t screen_size, uint32_t screen_base,
                      uint32_t stack_base, uint32_t words[256], uint32_t address,
                      uint32_t value) {
    int owner = owner_of(screen_size, screen_base, stack_base, address, 4);
    if (owner == OWNER_SCREEN) {
        uint8_t *p = screen + (address - screen_base);
        p[0] = (uint8_t)(value >> 24); p[1] = (uint8_t)(value >> 16);
        p[2] = (uint8_t)(value >> 8); p[3] = (uint8_t)value;
    } else if (owner == OWNER_STACK && !(address & 3u)) {
        words[(address - stack_base) / 4] = value;
    } else return 0;
    return 1;
}

static int write_bytes(uint8_t *screen, size_t screen_size, uint32_t screen_base,
                       uint32_t stack_base, uint32_t words[256], uint32_t address,
                       const char *bytes, size_t length) {
    int owner = owner_of(screen_size, screen_base, stack_base, address, length);
    if (!owner) return 0;
    if (owner == OWNER_SCREEN) {
        memcpy(screen + (address - screen_base), bytes, length);
        return 1;
    }
    for (size_t i = 0; i < length; ++i) {
        size_t offset = (size_t)(address - stack_base) + i;
        unsigned shift = 24u - (unsigned)(offset & 3u) * 8u;
        uint32_t mask = 0xffu << shift;
        uint32_t *word = &words[offset / 4];
        *word = (*word & ~mask) | ((uint32_t)(uint8_t)bytes[i] << shift);
    }
    return 1;
}

static int descriptor(uint8_t *screen, size_t screen_size, uint32_t screen_base,
                      uint32_t stack_base, uint32_t words[256], uint32_t address,
                      uint32_t *capacity, uint32_t *buffer, int *owner) {
    uint32_t ignored;
    if ((address & 3u) || !read_word(screen, screen_size, screen_base, stack_base,
                                     words, address, &ignored) ||
        !read_word(screen, screen_size, screen_base, stack_base,
                    words, address + 4, capacity) ||
        !read_word(screen, screen_size, screen_base, stack_base,
                    words, address + 8, buffer)) return 0;
    *owner = owner_of(screen_size, screen_base, stack_base, address, 12);
    return *owner != OWNER_INVALID;
}

int nflstreet_team_player_labels_callback_memory(
    void *context, uint8_t *screen, size_t screen_size, uint32_t screen_base,
    const nflstreet_ui_service_request *request, uint32_t *result,
    uint32_t stack_base, uint32_t stack_words[256]) {
    nflstreet_team_player_labels_bridge *bridge = context;
    if (!bridge || !screen || !request || !result || !stack_words ||
        !screen_base || !stack_base || request->kind != 1 ||
        request->property != 0x80000011u || request->argc != 6 ||
        screen_size > UINT32_MAX - screen_base ||
        stack_base > UINT32_MAX - VM_STACK_BYTES ||
        (screen_base < stack_base + VM_STACK_BYTES &&
         stack_base < (uint64_t)screen_base + screen_size) ||
        request->args[0] > 1) return 0;

    unsigned side = request->args[0];
    if (!(bridge->supplied_mask & (1u << side)) ||
        bridge->supplied_team_id[side] != bridge->selected_team_id[side] ||
        bridge->supplied_team_id[side] == UINT32_MAX ||
        (bridge->count[side] && !bridge->rows[side])) return 0;

    uint32_t name_capacity, name_buffer, role_capacity, role_buffer;
    uint32_t stat_dimensions, scalar_old;
    int name_desc_owner, role_desc_owner, stat_desc_owner;
    uint32_t name_desc = request->args[2], role_desc = request->args[3];
    uint32_t scalar_address = request->args[4], stats_desc = request->args[5];
    if (!descriptor(screen, screen_size, screen_base, stack_base, stack_words,
                    name_desc, &name_capacity, &name_buffer, &name_desc_owner) ||
        !descriptor(screen, screen_size, screen_base, stack_base, stack_words,
                    role_desc, &role_capacity, &role_buffer, &role_desc_owner) ||
        !read_word(screen, screen_size, screen_base, stack_base, stack_words,
                   stats_desc, &stat_dimensions)) return 0;
    if (stat_dimensions == 0 || stat_dimensions > 8 ||
        stats_desc > UINT32_MAX - (stat_dimensions + 1u) * 4u) return 0;
    size_t stats_header_bytes = (size_t)(stat_dimensions + 1u) * 4u;
    stat_desc_owner = owner_of(screen_size, screen_base, stack_base,
                               stats_desc, stats_header_bytes);
    if (!stat_desc_owner ||
        name_capacity > LABEL_TEXT_MAX || role_capacity > LABEL_TEXT_MAX ||
        name_capacity == UINT32_MAX || role_capacity == UINT32_MAX) return 0;

    uint64_t element_capacity = 1;
    for (uint32_t i = 0; i < stat_dimensions; ++i) {
        uint32_t dimension_size;
        if (!read_word(screen, screen_size, screen_base, stack_base, stack_words,
                       stats_desc + (i + 1u) * 4u, &dimension_size) ||
            dimension_size == 0) return 0;
        element_capacity *= dimension_size;
        if (element_capacity > UINT32_MAX) return 0;
    }
    if (element_capacity < NFLSTREET_LABEL_RATING_COUNT) return 0;

    uint32_t stats_buffer = stats_desc + (uint32_t)stats_header_bytes;
    size_t name_bytes = (size_t)name_capacity + 1u;
    size_t role_bytes = (size_t)role_capacity + 1u;
    size_t stats_bytes = NFLSTREET_LABEL_RATING_COUNT * sizeof(uint32_t);
    if (owner_of(screen_size, screen_base, stack_base, name_buffer, name_bytes) != name_desc_owner ||
        owner_of(screen_size, screen_base, stack_base, role_buffer, role_bytes) != role_desc_owner ||
        owner_of(screen_size, screen_base, stack_base, scalar_address, 4) == OWNER_INVALID ||
        owner_of(screen_size, screen_base, stack_base, stats_buffer, stats_bytes) != stat_desc_owner ||
        (stats_buffer & 3u) || !read_word(screen, screen_size, screen_base, stack_base,
                                         stack_words, scalar_address, &scalar_old)) return 0;

    memory_range descriptors[3] = {
        {name_desc, 12}, {role_desc, 12}, {stats_desc, stats_header_bytes}
    };
    memory_range outputs[4] = {
        {name_buffer, name_bytes}, {role_buffer, role_bytes},
        {scalar_address, 4}, {stats_buffer, stats_bytes}
    };
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < i; ++j)
            if (overlap(descriptors[i], descriptors[j])) return 0;
        for (unsigned j = 0; j < 4; ++j)
            if (overlap(descriptors[i], outputs[j])) return 0;
    }
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = 0; j < i; ++j)
            if (overlap(outputs[i], outputs[j])) return 0;

    char name[LABEL_TEXT_MAX + 1], role[LABEL_TEXT_MAX + 1];
    uint32_t scalar = 0;
    uint32_t ratings[NFLSTREET_LABEL_RATING_COUNT];
    nflstreet_label_status status = nflstreet_team_label_build(
        bridge->cursor[side], bridge->mode[side], bridge->rows[side],
        bridge->count[side], bridge->role_texts, bridge->role_text_count,
        name, name_capacity + 1u, role, role_capacity + 1u,
        &scalar, ratings);
    if (status != NFLSTREET_LABEL_OK) return 0;

    if (!write_bytes(screen, screen_size, screen_base, stack_base, stack_words,
                     name_buffer, name, strlen(name) + 1u) ||
        !write_bytes(screen, screen_size, screen_base, stack_base, stack_words,
                     role_buffer, role, strlen(role) + 1u) ||
        !write_word(screen, screen_size, screen_base, stack_base, stack_words,
                    scalar_address, scalar)) return 0;
    for (unsigned i = 0; i < NFLSTREET_LABEL_RATING_COUNT; ++i)
        if (!write_word(screen, screen_size, screen_base, stack_base, stack_words,
                        stats_buffer + i * 4u, ratings[i])) return 0;

    return 1;
}
