/*
 * Copyright (c) 2018 naehrwert
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 (or later), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

typedef unsigned int u32;

typedef struct _hnode {
    u32 used;
    u32 size;
    struct _hnode *prev; // 64bit or 32bit ptr
    struct _hnode *next; // 64bit or 32bit ptr
#ifdef __64BIT__
    u32 align[10]; // Align to arch cache line for 64bit arch (64 Bytes).
#else
    u32 align[4]; // Align to arch cache line for 32bit arch (32 Bytes).
#endif
} __attribute__ ((packed)) hnode_t;

typedef struct _heap {
	u64* start;
	hnode_t *first;
} heap_t;

typedef struct {
    u32 total;
    u32 used;
} heap_monitor_t;


void heap_init(void *base);
void *hmalloc(u32 size);
void *hcalloc(u32 num, u32 size);
void hfree(void *page);
u32 page_size(void* page);
void heap_monitor(heap_monitor_t *mon, bool print_node_stats);
#ifdef __cplusplus
}
#endif
