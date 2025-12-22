#ifndef CLEANUPSTACK_H
#define CLEANUPSTACK_H
#include "common.h"
#include <stddef.h>

#define CLEANUP_BLOB_PTRS 4

struct CleanupEntry {
	void(*destroy)(void*);
    usize blob[CLEANUP_BLOB_PTRS];
};

struct CleanupStack {
	struct CleanupEntry* entries;
	u32 n;
	u32 cap;
};

void cs_init(struct CleanupStack* cs);

void cs_push(struct CleanupStack* cs, void* ctx, usize ctxsize, void(*destructor)(void*));
void cs_push_entry(struct CleanupStack* cs, struct CleanupEntry ce);

void cs_consume(struct CleanupStack* cs);
#endif