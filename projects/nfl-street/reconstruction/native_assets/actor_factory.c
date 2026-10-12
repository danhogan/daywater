#include "actor_factory.h"
#include <string.h>

int nflstreet_actor_create(const nflstreet_factory_record *records, size_t count,
    uint32_t type, uint8_t header_byte, const void *input,
    nflstreet_factory_allocator allocate, void *allocation_context,
    nflstreet_factory_memory *result)
{
    if (!result) return 0;
    memset(result, 0, sizeof *result);
    if (!records || type >= count || !allocate) return 0;
    const nflstreet_factory_record *record = records + type;
    if (record->object_size < 0x12) return 0;
    nflstreet_factory_memory memory = {0};
    if (!allocate(allocation_context, record->allocation_owner,
                  record->object_size, &memory) || !memory.token ||
        !memory.bytes || memory.size < record->object_size) return 0;
    memset(memory.bytes, 0, record->object_size);
    memory.bytes[0] = (uint8_t)type;
    memory.bytes[1] = header_byte;
    memory.bytes[0x10] = 0;
    memory.bytes[0x11] = 2;
    if (record->construct)
        record->construct(record->construction_context, &memory, input);
    *result = memory;
    return 1;
}
