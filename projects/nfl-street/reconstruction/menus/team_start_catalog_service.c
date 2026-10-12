#include "team_start_catalog_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEAM_TABLE_TAG 0x4d414554u
#define TEAM_ID_TAG 0x44494754u
#define RUMT_TAG 0x52554d54u
#define CONTROLLER_TABLE_TAG 0x4f434d54u
#define CONTROLLER_KEY_TAG UINT32_MAX
#define DIGL_TAG 0x6469676cu
#define UNLS_TAG 0x554e4c53u
#define TEAM_CONTEXT_WORDS 11u

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static int fail(nflstreet_start_catalog_service *s, const char *message) {
    if (s) {
        ++s->unsupported_queries;
        snprintf(s->last_error, sizeof s->last_error, "%s", message);
    }
    return -1;
}

static int load_block(const char *path, uint8_t **bytes, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long n = ftell(f);
    if (n <= 0 || n > 16 * 1024 * 1024 || fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
    uint8_t *p = malloc((size_t)n);
    if (!p || fread(p, 1, (size_t)n, f) != (size_t)n) { free(p); fclose(f); return 0; }
    fclose(f); *bytes = p; *size = (size_t)n; return 1;
}

int nflstreet_start_catalog_load(nflstreet_start_catalog_service *s, const char *directory) {
    if (!s || !directory) return 0;
    memset(s, 0, sizeof *s);
    for (unsigned entry = 1; entry <= NFLSTREET_START_CATALOG_MAX_ROWS; ++entry) {
        char path[4096];
        int n = snprintf(path, sizeof path, "%s/%05u.bin", directory, entry);
        if (n < 0 || (size_t)n >= sizeof path) return 0;
        uint8_t *bytes = NULL; size_t size = 0;
        if (!load_block(path, &bytes, &size)) return 0;
        nflstreet_team_table table;
        if (!nflstreet_team_table_open_tag(bytes, size, TEAM_TABLE_TAG, &table) || table.used != 1) {
            free(bytes); return 0;
        }
        nflstreet_start_catalog_row *row = &s->rows[s->row_count];
        for (unsigned i = 0; i < table.field_count; ++i) {
            nflstreet_team_field field;
            if (!nflstreet_team_table_field(&table, i, &field)) { free(bytes); return 0; }
            if (field.type != 3) continue;
            if (row->field_count >= NFLSTREET_START_CATALOG_MAX_FIELDS) { free(bytes); return 0; }
            uint32_t value;
            if (!nflstreet_team_table_number(&table, 0, i, &value)) { free(bytes); return 0; }
            row->fields[row->field_count++] = (nflstreet_start_catalog_field){field.tag, value};
            if (field.tag == TEAM_ID_TAG) row->id = value;
        }
        if (!row->id || row->field_count == 0) { free(bytes); return 0; }
        ++s->row_count;
        free(bytes);
    }
    return s->row_count == NFLSTREET_START_CATALOG_MAX_ROWS;
}

static int register_context(nflstreet_start_catalog_service *s, CPUState *c,
                            uint32_t address, uint32_t table, uint32_t key,
                            uint32_t filter, uint32_t options_address) {
    if (!((table == TEAM_TABLE_TAG && key == TEAM_ID_TAG) ||
          (table == CONTROLLER_TABLE_TAG && key == CONTROLLER_KEY_TAG)))
        return fail(s, "query open was not MAET/DIGT or OCMT/FFFFFFFF");
    if (address < 0x80000000u || address - 0x80000000u > c->ram_size - TEAM_CONTEXT_WORDS * 4u)
        return fail(s, "query context pointer outside owned RAM");
    unsigned at = s->context_count;
    for (unsigned i = 0; i < s->context_count; ++i) {
        if (s->contexts[i].address == address) {
            if (s->contexts[i].active) {
                if (s->contexts[i].table_tag == table && s->contexts[i].key_tag == key) return 0;
                return fail(s, "duplicate DB query open for active context");
            }
            at = i; break;
        }
    }
    if (at == s->context_count) {
        if (s->context_count >= NFLSTREET_START_CATALOG_MAX_CONTEXTS) return fail(s, "too many DB query contexts");
        ++s->context_count;
    }
    memset(&s->contexts[at], 0, sizeof s->contexts[at]);
    s->contexts[at].address = address;
    s->contexts[at].table_tag = table;
    s->contexts[at].key_tag = key;
    s->contexts[at].cursor = UINT32_MAX;
    s->contexts[at].row_count = table == TEAM_TABLE_TAG ? s->row_count : NFLSTREET_START_CONTROLLER_ROWS;
    s->contexts[at].active = 1;
    s->contexts[at].query_filter = filter;
    if (options_address >= 0x80000000u && options_address - 0x80000000u <= c->ram_size - 16u) {
        uint8_t *p = c->ram + options_address - 0x80000000u;
        for (unsigned i = 0; i < 4; ++i) s->contexts[at].query_options[i] = be32(p + i * 4u);
    }
    ++s->contexts[at].opens;
    if (!s->next_handle) s->next_handle = 0x100u;
    return 0;
}

static int context_for(nflstreet_start_catalog_service *s, uint32_t address) {
    for (unsigned i = 0; i < s->context_count; ++i)
        if (s->contexts[i].active && s->contexts[i].address == address) return (int)i;
    return -1;
}

static int context_for_handle(nflstreet_start_catalog_service *s, CPUState *c, uint32_t handle) {
    for (unsigned i = 0; i < s->context_count; ++i) {
        const unsigned at = s->contexts[i].address - 0x80000000u;
        if (s->contexts[i].active && s->contexts[i].address >= 0x80000000u &&
            at <= c->ram_size - 4u && be32(c->ram + at) == handle)
            return (int)i;
    }
    return -1;
}

static int close_context(nflstreet_start_catalog_service *s, CPUState *c) {
    int index = context_for(s, c->gpr[3]);
    if (index < 0) return fail(s, "close for unknown context");
    uint32_t address = c->gpr[3];
    if (address < 0x80000000u || address - 0x80000000u > c->ram_size - 4u)
        return fail(s, "close context pointer outside owned RAM");
    put32(c->ram + address - 0x80000000u, 0);
    s->contexts[index].active = 0;
    ++s->contexts[index].closes;
    c->gpr[3] = 0; c->pc = c->lr;
    return 1;
}

int nflstreet_start_catalog_step(nflstreet_start_catalog_service *s, CPUState *c) {
    if (!s || !c) return 0;
    if (c->pc == 0x801509c8u && c->gpr[3] >= 0x80000000u &&
        c->gpr[3] - 0x80000000u <= c->ram_size - 44u) {
        const uint8_t *q=c->ram+c->gpr[3]-0x80000000u;
        fprintf(stderr,"509C8 query ctx=%08x target=%08x out=%08x table=%08x key=%08x filter=%08x options=%08x lr=%08x\n",c->gpr[3],c->gpr[4],c->gpr[5],be32(q+8),be32(q+12),be32(q+4),be32(q+24),c->lr);
    }
    if (c->pc == 0x801b195cu && c->gpr[7] >= 0x80000000u &&
        c->gpr[7] - 0x80000000u <= c->ram_size - 44u) {
        /* The original 48FD4 wrapper has already written its descriptor and
         * delegates the actual database lookup here. Supply only the queried
         * table record set; let 48FD4 complete its own descriptor writes. */
        uint32_t address = c->gpr[7];
        uint8_t *q = c->ram + address - 0x80000000u;
        uint32_t table = be32(q + 16u), key = be32(q + 12u);
        fprintf(stderr,"B195C open ctx=%08x table=%08x key=%08x extra=%08x filter=%08x options=%08x statusout=%08x lr=%08x\n",address,table,key,c->gpr[3],c->gpr[6],be32(q+24),c->gpr[8],c->lr);
        if ((table == TEAM_TABLE_TAG && key == TEAM_ID_TAG) ||
            (table == CONTROLLER_TABLE_TAG && key == CONTROLLER_KEY_TAG)) {
            if (c->gpr[8] < 0x80000000u || c->gpr[8] - 0x80000000u > c->ram_size - 4u)
                return fail(s, "database wrapper status output outside owned RAM");
            int result = register_context(s, c, address, table, key, c->gpr[6], be32(q + 24u));
            if (result < 0) return result;
            unsigned index = 0;
            while (index < s->context_count && s->contexts[index].address != address) ++index;
            if (index == s->context_count) return fail(s, "database context registration failed");
            put32(q, s->next_handle++);
            put32(c->ram + c->gpr[8] - 0x80000000u, 1u);
            c->gpr[3] = 0x17u;
            c->pc = c->lr;
            return 1;
        }
    }
    if (c->pc == 0x80149af0u) {
        /* 80149AF0 is the source helper used by startup to test the supplied
         * controller projection. It scans the OCMT record for UNLS==arg and
         * then checks the DIGL value through 80129AB8. For this controlled
         * fixture the only supplied record has DIGL=0, UNLS=0, and logical
         * port map[0]=0; answer only the two observed flag queries. */
        if (c->gpr[3] > 1u || s->controller_port != 0u || s->controller_enabled != 0u)
            return fail(s, "unsupported 80149AF0 controller projection query");
        unsigned at = s->profile_query_calls++;
        if (at < 4u) {
            s->profile_query_args[at] = c->gpr[3];
            s->profile_query_results[at] = c->gpr[3] == s->controller_enabled;
        }
        c->gpr[3] = c->gpr[3] == s->controller_enabled;
        c->pc = c->lr;
        return 1;
    }
    if (c->pc == 0x801b1ac8u && c->gpr[4] == 0u && (c->gpr[5] == 0u || c->gpr[5] == 1u)) {
        int index = context_for_handle(s, c, c->gpr[3]);
        if (index >= 0) {
            uint32_t context_address = s->contexts[index].address;
            if (context_address < 0x80000000u || context_address - 0x80000000u > c->ram_size - 44u)
                return fail(s, "database query context outside owned RAM");
            uint32_t cursor = be32(c->ram + context_address - 0x80000000u + 40u);
            if (cursor != UINT32_MAX && cursor > s->contexts[index].row_count + 1u) {
                char message[192];
                snprintf(message,sizeof message,"original query cursor escaped supplied catalog: cursor=%08x rows=%u tag=%08x context=%08x",cursor,s->contexts[index].row_count,c->gpr[5],context_address);
                return fail(s,message);
            }
            s->contexts[index].cursor = cursor;
            if (c->gpr[5] == 1u) {
                /* 8014919C asks B1AC8 whether the current iterator row is
                 * exhausted (0x14/0x15). Let its original wrapper increment
                 * the raw cursor only when a row remains. */
                c->gpr[3] = cursor < s->contexts[index].row_count ? 0u : 0x15u;
                c->pc = c->lr;
                return 1;
            }
            const uint32_t out = c->gpr[6];
            if (out < 0x80000000u || out - 0x80000000u > c->ram_size - 12u)
                return fail(s, "database field descriptor outside owned RAM");
            const uint32_t tag = be32(c->ram + out - 0x80000000u + 8u);
            /* Original iterator wrappers own the cursor word. Match that
             * observed value before serving the requested supplied field. */
            uint32_t value = 0;
            int found = 0;
            if (cursor < s->contexts[index].row_count &&
                s->contexts[index].table_tag == TEAM_TABLE_TAG &&
                s->contexts[index].key_tag == TEAM_ID_TAG) {
                const nflstreet_start_catalog_row *row = &s->rows[cursor];
                for (unsigned i = 0; i < row->field_count; ++i) {
                    if (row->fields[i].tag == tag) { value = row->fields[i].value; found = 1; break; }
                }
            } else if (cursor < s->contexts[index].row_count &&
                       s->contexts[index].table_tag == CONTROLLER_TABLE_TAG &&
                       s->contexts[index].key_tag == CONTROLLER_KEY_TAG) {
                if (tag == DIGL_TAG) { value = s->controller_port; found = 1; }
                else if (tag == UNLS_TAG) { value = s->controller_enabled; found = 1; }
            }
            put32(c->ram + out - 0x80000000u, value);
            if (tag == TEAM_ID_TAG || tag == 0x53495654u)
                fprintf(stderr,"B1AC8 field tag=%08x value=%08x found=%d status=%u row=%u table=%08x\n",tag,value,found,found?0u:9u,cursor,s->contexts[index].table_tag);
            c->gpr[3] = found ? 0u : 9u;
            c->pc = c->lr;
            ++s->contexts[index].reads;
            if (found) ++s->row_reads;
            else ++s->absent_rumt_reads;
            return 1;
        }
    }
    if (c->pc == 0x80148fd4u) {
        /* 80148FD4 is the verified lower query-open wrapper. Its original
         * body writes the query descriptor, then delegates only the actual
         * record search to 801B195C. Supply that database result here while
         * preserving the original source callsites and query arguments. */
        if (!((c->gpr[4] == TEAM_TABLE_TAG && c->gpr[5] == TEAM_ID_TAG) ||
              (c->gpr[4] == CONTROLLER_TABLE_TAG && c->gpr[5] == CONTROLLER_KEY_TAG)))
            return 0;
        int result = register_context(s, c, c->gpr[3], c->gpr[4], c->gpr[5], c->gpr[6], c->gpr[7]);
        if (result < 0) return result;
        uint32_t address = c->gpr[3];
        if (address < 0x80000000u || address - 0x80000000u > c->ram_size - 44u)
            return fail(s, "MAET query context outside owned RAM");
        uint8_t *q = c->ram + address - 0x80000000u;
        put32(q, s->next_handle++);
        put32(q + 4u, c->gpr[8]);
        put32(q + 8u, c->gpr[4]);
        put32(q + 12u, c->gpr[5]);
        put32(q + 16u, c->gpr[4]);
        put32(q + 20u, 2u);
        put32(q + 24u, c->gpr[7]);
        put32(q + 28u, UINT32_MAX);
        put32(q + 32u, 2u);
        put32(q + 36u, 0u);
        unsigned rows = c->gpr[4] == TEAM_TABLE_TAG ? s->row_count : NFLSTREET_START_CONTROLLER_ROWS;
        put32(q + 40u, rows ? 0u : UINT32_MAX);
        c->pc = c->lr;
        return 1;
    }
    if (c->pc == 0x80149a54u || c->pc == 0x80150780u) {
        uint32_t address = c->gpr[3];
        uint32_t table = c->pc == 0x80149a54u ? CONTROLLER_TABLE_TAG : TEAM_TABLE_TAG;
        uint32_t key = c->pc == 0x80149a54u ? CONTROLLER_KEY_TAG : TEAM_ID_TAG;
        uint32_t filter = c->pc == 0x80149a54u ? 0 : c->gpr[4];
        uint32_t options = c->pc == 0x80149a54u ? 0 : c->gpr[5];
        int result = register_context(s, c, address, table, key, filter, options);
        if (result < 0) return result;
        if (address < 0x80000000u || address - 0x80000000u > c->ram_size - 44u)
            return fail(s, "query-open context outside owned RAM");
        /* The source's subsequent 490D0/4919C/49614 calls use only this
         * handle and the cursor word. DB allocation/lookup is the boundary. */
        put32(c->ram + address - 0x80000000u, s->next_handle++);
        put32(c->ram + address - 0x80000000u + 40u, UINT32_MAX);
        c->pc = c->lr;
        return 1;
    }
    if (c->pc == 0x80149a80u || c->pc == 0x80149064u) return close_context(s, c);
    if (c->pc == 0x801490d0u) {
        int index = context_for(s, c->gpr[3]);
        if (index < 0) { char msg[512]; size_t n=(size_t)snprintf(msg,sizeof msg,"cursor reset pc=%08x ptr=%08x r4=%08x lr=%08x contexts=%u",c->pc,c->gpr[3],c->gpr[4],c->lr,s->context_count); if(c->gpr[3]>=0x80000000u&&c->gpr[3]-0x80000000u<=c->ram_size-44u){const uint8_t*q=c->ram+c->gpr[3]-0x80000000u;n+=(size_t)snprintf(msg+n,sizeof msg-n," raw=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x",be32(q),be32(q+4),be32(q+8),be32(q+12),be32(q+16),be32(q+20),be32(q+24),be32(q+28),be32(q+32),be32(q+36),be32(q+40));} for(unsigned i=0;i<s->context_count&&n<sizeof msg;++i)n+=(size_t)snprintf(msg+n,sizeof msg-n," [%u:%08x %08x/%08x active=%u cur=%08x rows=%u]",i,s->contexts[i].address,s->contexts[i].table_tag,s->contexts[i].key_tag,s->contexts[i].active,s->contexts[i].cursor,s->contexts[i].row_count); return fail(s,msg); }
        uint32_t address = c->gpr[3];
        s->contexts[index].cursor = s->contexts[index].row_count ? 0u : UINT32_MAX;
        ++s->contexts[index].resets;
        if (address < 0x80000000u || address - 0x80000000u > c->ram_size - 44u)
            return fail(s, "cursor context pointer outside owned RAM");
        put32(c->ram + address - 0x80000000u + 40u, s->contexts[index].cursor);
        c->gpr[3] = s->contexts[index].row_count != 0;
        c->pc = c->lr;
        return 1;
    }
    return 0;
}
