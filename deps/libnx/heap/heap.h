/*
 * Copyright (c) 2018 naehrwert
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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
	int used;
	u32 size;
	struct _hnode *prev;
	struct _hnode *next;
	u32 align[4]; // Align to arch cache line size.
} hnode_t;

typedef struct _heap {
	u32* start;
	hnode_t *first;
} heap_t;

typedef struct {
    u32 total;
    u32 used;
} heap_monitor_t;


void heap_init(u32* base);
void *hmalloc(u32 size);
void *hcalloc(u32 num, u32 size);
void hfree(void *page);
u32 page_size(void* page);
void heap_monitor(heap_monitor_t *mon, bool print_node_stats);
#ifdef __cplusplus
}
#endif