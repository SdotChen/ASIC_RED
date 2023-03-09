#ifndef __LIST_H__
#define __LIST_H__

struct mempool {
	int id;
	struct rte_mempool *mbuf_pool;
	struct rte_ring *in;
	struct rte_ring *out;

	struct mempool *prev;
	struct mempool *next;
};

struct mem_table {
	int count;
	struct mempool *mpool_set;
};

struct mem_table *memInstance(void);

struct mempool *mempool_create(void);

#endif
