#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>

#include "list.h"
#include "server.h"

#ifndef NUM_MBUF
#define NUM_MBUF 4095
#endif

#ifndef RING_SIZE
#define RING_SIZE 4096
#endif

struct mem_table *memInst = NULL;

struct mem_table *memInstance(void) {
	if(memInst == NULL) {
		memInst = rte_malloc("mem table", sizeof(struct mem_table), 0);
		memset(memInst,0,sizeof(struct mem_table));
		memInst->count = 0;
		memInst->mpool_set = NULL;
	}
	return memInst;
}


struct mempool *mempool_create(void) {
	struct mem_table *mtable = memInstance();
	printf("Current mpoll num:%d\n",mtable->count);
	int id = mtable->count;

	char namebuf[32];

	sprintf(namebuf,"mempool %d",id);
	struct mempool *mpool = rte_malloc(namebuf,sizeof(struct mempool),0);

	if(mpool == NULL)
		return NULL;

	mpool->id = id;	
	sprintf(namebuf,"mbuf pool %d",id);
	mpool->mbuf_pool = rte_pktmbuf_pool_create(namebuf,
		NUM_MBUF,0,0,RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());
	
	if(!mpool->mbuf_pool) {
		rte_free(mpool);
		rte_exit(EXIT_FAILURE,"Mempool create failed\n");
	}

	printf("in ring create\n");	
	sprintf(namebuf,"in ring %d",id);
	mpool->in = rte_ring_create(namebuf,RING_SIZE,rte_socket_id(),RING_F_SP_ENQ | RING_F_SC_DEQ);
	if(mpool->in == NULL)
		rte_exit(EXIT_FAILURE,"in ring create failed\n");
	printf("out ring create\n");	
	sprintf(namebuf,"out ring %d",id);
	mpool->out = rte_ring_create(namebuf,RING_SIZE,rte_socket_id(),RING_F_SP_ENQ | RING_F_SC_DEQ);
	if(mpool->out == NULL)
		rte_exit(EXIT_FAILURE,"out ring create failed\n");
	printf("ring created\n");	

	if(mtable->mpool_set == NULL) {
		mtable->mpool_set = mpool;
		mtable->mpool_set->prev = mpool;
		mtable->mpool_set->next = mpool;
	} else {
		mpool->prev = mtable->mpool_set->prev;
		mpool->next = mtable->mpool_set;
		mpool->prev->next = mpool;
		mpool->next->prev = mpool;
	}

	mtable->count = mtable->count + 1;
	return mpool;
}
