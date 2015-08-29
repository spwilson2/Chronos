#ifdef __LINUX__
#include "types.h"
#include "stdlock.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define uint_64 uint64_t
#define uint_32 uint32_t
#define uint_16 uint16_t
#define uint_8  uint8_t
#define cprintf printf
/* need the log2 algorithm */
int log2_linux(uint value); /* defined in ext2.c*/

#define log2(val) log2_linux(val)
#else
#include "types.h"
#include "stdlock.h"
#include "stdlib.h"
#endif

#include "cacheman.h"

// #define CACHE_DEBUG

struct cache_entry
{
	int id; /* A unique identifier that can be used for search.*/
	int references;	/* How many hard pointers are there? */

	/**
	 * How many blocks after this block belong to this allocation?
	 */
	int sequence;
	void* slab; /* A pointer to the data that goes with this entry. */
};

static int cache_default_check(void* obj, int id, struct cache* cache)
{
	struct cache_entry* entry = cache->entries;

	if(cache->slab_shift)
	{
		/* We can use shifting (fast) */
		entry += (((uint)obj - (uint)cache->slabs) 
			>> cache->slab_shift);
	} else {
		/* We have to use division (slow) */
		entry += (((uint)obj - (uint)cache->slabs) 
			/ cache->slab_sz);
	}

	/* Match */
	if(entry->id == id) return 0;

	/* No match */
	return -1;
}

static int cache_default_dump(struct cache* cache)
{
	int x;
	for(x = 0;x < cache->entry_count;x++)
	{
		if(cache->entries[x].id)
		{
			cprintf("%d has %d references.\n",
				cache->entries[x].id,
				cache->entries[x].references);
		}
	}

	return 0;
}

static void* cache_search_nolock(int id, struct cache* cache);
static void* cache_alloc(int id, int slabs, struct cache* cache);
static int cache_dereference_nolock(void* ptr, struct cache* cache);

int cache_init(void* cache_area, uint sz, uint data_sz, 
		void* context, struct FSHardwareDriver* driver,
		struct cache* cache)
{
	memset(cache, 0, sizeof(struct cache));
	uint entries = sz / (sizeof(struct cache_entry) + data_sz);
	if(entries < 1) return -1;

#ifdef CACHE_DEBUG
	cprintf("cache: there are %d entries available in the cache.\n", 
		entries);
#endif

	cache->entry_shift = log2(sizeof(struct cache_entry));
	if(cache->entry_shift < 0)
	{
#ifdef CACHE_DEBUG
		cprintf("cache: entry size is not a multiple of 2.\n");
#endif
		return -1;
	}
	cache->slab_sz = data_sz;
	cache->context = context;
	cache->driver = driver;
	cache->entry_count = entries;
	cache->slabs = cache_area;
	cache->entries = (void*)(cache->slabs + sz) 
		- (entries << cache->entry_shift);
	slock_init(&cache->lock);
	memset(cache_area, 0, sz); /* Clear to 0 */

	/* Setup slab pointers */
	int x;
	for(x = 0;x < entries;x++)
		cache->entries[x].slab = cache->slabs + (data_sz * x);

	/* Try to assign a shift value */
	cache->slab_shift = log2(data_sz);
	/* Check to see if it worked */
	if(cache->slab_shift < 0) 
	{
#ifdef CACHE_DEBUG
		cprintf("cache: slab value is not a multiple of 2.\n");
#endif
		cache->slab_shift = 0;
	}

	/* Default search function */
	cache->check = cache_default_check;

	/* Other functions */
	cache->dereference = cache_dereference_nolock;
	cache->search = cache_search_nolock;
	cache->alloc = cache_alloc;
	cache->dump = cache_default_dump;

	return 0;
}

static void* cache_alloc(int id, int slabs, struct cache* cache)
{
        void* result = NULL;

	int sequence_start = -1;
	int slab_count = 0;

        int x;
        for(x = 0;x < cache->entry_count;x++)
        {
                if(!cache->entries[x].references)
                {
			if(sequence_start == -1)
				sequence_start = x;
			slab_count++;
			/* Are we done? */
                        if(slabs == slab_count) break;
                } else {
			sequence_start = -1;
			slab_count = 0;
		}
        }

	if(slab_count == slabs)
	{
		result = cache->entries[sequence_start].slab;
		cache->entries[sequence_start].sequence = slab_count - 1;

		for(x = 0;x < slab_count;x++)
		{
			cache->entries[sequence_start].references = 1;
			cache->entries[sequence_start].id = id;
		}
	}

#ifdef CACHE_DEBUG
        if(result) cprintf("cache: new sequence allocated: %d\n", id);
        else cprintf("cache: not enough room in cache!\n");
#endif

        return result;
}

static int cache_force_free(void* ptr, struct cache* cache)
{
        struct cache_entry* entry = cache->entries;
        uint val = (uint)ptr - (uint)cache->slabs;
	if(val > cache->entry_count)
		return -1;
        /* If shift is available then use it (fast) */
        if(cache->slab_shift)
                entry += (uint)(val >> cache->slab_shift);
        else {
                /* The division instruction is super slow. */
                entry += (val / cache->slab_sz);
        }

	entry->references = 0;
	entry->id = 0;

	return 0;
}

static int cache_dereference_nolock(void* ptr, struct cache* cache)
{
        int result = 0;
        struct cache_entry* entry = cache->entries;
        uint val = (uint)ptr - (uint)cache->slabs;
	if(val > cache->entry_count)
		return -1;
        /* If shift is available then use it (fast) */
        if(cache->slab_shift)
                entry += (uint)(val >> cache->slab_shift);
        else {
                /* The division instruction is super slow. */
                entry += (val / cache->slab_sz);
        }

        entry->references--;
        if(entry->references <= 0)
        {
                entry->references = 0;
#ifdef CACHE_DEBUG
                cprintf("cache: object %d fully dereferenced.\n",
                                entry->id);
#endif
		if(cache->sync)
		{
#ifdef CACHE_DEBUG
			cprintf("cache: syncing data to system.\n");
#endif
			cache->sync(ptr, entry->id, cache);
		} else {
#ifdef CACHE_DEBUG
			cprintf("cache: sync is disabled.\n");
#endif
		}
	} else {
#ifdef CACHE_DEBUG
		cprintf("cache: object dereferenced: %d\n", entry->id);
#endif
	}

	return result;
}

int cache_dereference(void* ptr, struct cache* cache)
{
	slock_acquire(&cache->lock);
	int result = cache_dereference_nolock(ptr, cache);

	slock_release(&cache->lock);
	return result;
}

static void* cache_search_nolock(int id, struct cache* cache)
{
	void* result = NULL;

	int x;
	for(x = 0;x < cache->entry_count;x++)
	{
		if(!cache->check(cache->entries[x].slab, id, cache))
		{
			result = cache->entries[x].slab;
			if(cache->entries[x].references <= 0)
				cache->entries[x].references = 1;
			else cache->entries[x].references++;
			break;
		}

		/* Skip over the sequence to save time */
		if(cache->entries[x].references)
			x += cache->entries[x].sequence;
	}

#ifdef CACHE_DEBUG
	if(result) cprintf("cache: search success.\n");
	else cprintf("cache: search failure.\n");
#endif

	return result;
}

void* cache_search(int id, struct cache* cache)
{
	slock_acquire(&cache->lock);
	void* result = cache_search_nolock(id, cache);
	slock_release(&cache->lock);
	return result;
}

void* cache_addreference(int id, int slabs, struct cache* cache)
{
	void* result = NULL;
        slock_acquire(&cache->lock);
        /* First search */
        if(!(result = cache_search_nolock(id, cache)))
        {
                /* Not already cached. */
                result = cache_alloc(id, slabs, cache);
		/* Do not populate. */
        }
        slock_release(&cache->lock);
        return result;
}

void* cache_reference(int id, int slabs, struct cache* cache)
{
	void* result = NULL;
	slock_acquire(&cache->lock);
	/* First search */
	if(!(result = cache_search_nolock(id, cache)))
	{
		/* Not already cached. */
		result = cache_alloc(id, slabs, cache);
		/* Call the populate function */
		if(cache->populate)
		{
			/* Populate the entry */
			if(cache->populate(id, cache))
			{
				/* The resource is unavailable. */
				cache_force_free(result, cache);
				result = NULL;
			}
		} else {
			cprintf("cache: non populate function assigned.\n");
		}
	}
	slock_release(&cache->lock);
	return result;
}	

void* cache_soft_search(int id, struct cache* cache)
{
	slock_acquire(&cache->lock);
	void* result = NULL;

	int x;
	for(x = 0;x < cache->entry_count;x++)
	{
		if(!cache->check(cache->entries[x].slab, id, cache))
		{
			result = cache->entries[x].slab;
			break;
		}
	}

	slock_release(&cache->lock);
	return result;
}
