#ifndef NFLSTREET_DISC_FILES_H
#define NFLSTREET_DISC_FILES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct nflstreet_disc_files nflstreet_disc_files;
/* Host I/O choice: nod's C API, the same disc reader used by Aurora DVD.
   No game addresses or emulated DVD scheduling. One caller per handle. */
nflstreet_disc_files *nflstreet_disc_open(const char *iso);
void nflstreet_disc_close(nflstreet_disc_files *);
/* Read an FST game path, or decoded TERF entry archive/00000.bin (mapped to
   archive.dat, zero-based DIR1 index). Slash/backslash and leading slash
   accepted; traversal/drive paths rejected. No extracted-file fallback.
   Caller frees successful output (including zero-length entries). Failure
   leaves both outputs untouched. The most recent archive is cached. */
int nflstreet_disc_read(nflstreet_disc_files *,const char *path,uint8_t **bytes,size_t *size);
/* Query a TERF FST path, retaining it in the same one-archive cache. */
int nflstreet_disc_entry_count(nflstreet_disc_files *,const char *archive_path,unsigned *count);
#ifdef __cplusplus
}
#endif
#endif
