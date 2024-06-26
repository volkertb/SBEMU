#ifndef __SOUND_UTIL_MEM_H
#define __SOUND_UTIL_MEM_H

#include <linux/mutex.h>
/*
 *  Copyright (C) 2000 Takashi Iwai <tiwai@suse.de>
 *
 *  Generic memory management routines for soundcard memory allocation
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/*
 * memory block
 */
struct snd_util_memblk {
	unsigned int size;		/* size of this block */
	unsigned int offset;		/* zero-offset of this block */
	struct list_head list;		/* link */
};

#define snd_util_memblk_argptr(blk)	(void*)((char*)(blk) + sizeof(struct snd_util_memblk))

/*
 * memory management information
 */
struct snd_util_memhdr {
	unsigned int size;		/* size of whole data */
	struct list_head block;		/* block linked-list header */
	int nblocks;			/* # of allocated blocks */
	unsigned int used;		/* used memory size */
	int block_extra_size;		/* extra data size of chunk */
	struct mutex block_mutex;	/* lock */
};

/*
 * prototypes
 */
//struct snd_util_memhdr *snd_util_memhdr_new(int memsize);
//void snd_util_memhdr_free(struct snd_util_memhdr *hdr);
struct snd_util_memblk *snd_util_mem_alloc(struct snd_util_memhdr *hdr, int size);
int snd_util_mem_free(struct snd_util_memhdr *hdr, struct snd_util_memblk *blk);
int snd_util_mem_avail(struct snd_util_memhdr *hdr);

/* functions without mutex */
struct snd_util_memblk *__snd_util_mem_alloc(struct snd_util_memhdr *hdr, int size);
//void __snd_util_mem_free(struct snd_util_memhdr *hdr, struct snd_util_memblk *blk);
//struct snd_util_memblk *__snd_util_memblk_new(struct snd_util_memhdr *hdr,
//					      unsigned int units,
//					      struct list_head *prev);

#include "linux/slab.h"
#include "linux/list.h"
#define get_memblk(p)	list_entry(p, struct snd_util_memblk, list)

/*
 * create a new memory manager
 */
static inline
struct snd_util_memhdr *
snd_util_memhdr_new(int memsize)
{
	struct snd_util_memhdr *hdr;

	hdr = kzalloc(sizeof(*hdr), GFP_KERNEL);
	if (hdr == NULL)
		return NULL;
	hdr->size = memsize;
	mutex_init(&hdr->block_mutex);
	INIT_LIST_HEAD(&hdr->block);

	return hdr;
}

/*
 * free a memory manager
 */
static inline
void snd_util_memhdr_free(struct snd_util_memhdr *hdr)
{
	struct list_head *p;

	if (!hdr)
		return;
	/* release all blocks */
	while ((p = hdr->block.next) != &hdr->block) {
		list_del(p);
		kfree(get_memblk(p));
	}
	kfree(hdr);
}

/*
 * create a new memory block with the given size
 * the block is linked next to prev
 */
static inline
struct snd_util_memblk *
__snd_util_memblk_new(struct snd_util_memhdr *hdr, unsigned int units,
		      struct list_head *prev)
{
	struct snd_util_memblk *blk;

	blk = kmalloc(sizeof(struct snd_util_memblk) + hdr->block_extra_size,
		      GFP_KERNEL);
	if (blk == NULL)
		return NULL;

	if (prev == &hdr->block)
		blk->offset = 0;
	else {
		struct snd_util_memblk *p = get_memblk(prev);
		blk->offset = p->offset + p->size;
	}
	blk->size = units;
	list_add(&blk->list, prev);
	hdr->nblocks++;
	hdr->used += units;
	return blk;
}

/*
 * remove the block from linked-list and free resource
 * (without mutex)
 */
static inline
void
__snd_util_mem_free(struct snd_util_memhdr *hdr, struct snd_util_memblk *blk)
{
	list_del(&blk->list);
	hdr->nblocks--;
	hdr->used -= blk->size;
	kfree(blk);
}

#endif /* __SOUND_UTIL_MEM_H */
