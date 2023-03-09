#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <rte_timer.h>

#include "server.h"
#include "port.h"
#include "core.h"
#include "list.h"

#define NUM_MBUF 8191
#define BURST_SIZE 4096
#define RING_SIZE 4096

#define ENABLE_PARSE 1
#define ENABLE_SEND 0


int main(int argc, char **argv) {
	if(rte_eal_init(argc,argv) < 0) {
		rte_exit(EXIT_FAILURE,"Init failed\n");
	}

	struct mem_table *mtable = memInstance();
	

/*	struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("mbuf pool",
		NUM_MBUF,0,0,RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());

	if(!mbuf_pool)
		rte_exit(EXIT_FAILURE,"Mempool create failed\n");
*/

	int iter_q;
	for(iter_q = 0; iter_q < RX_QUEUES; iter_q++){
//		struct mempool *mpool = mempool_create(); 
		mempool_create();
	}
	printf("After mpool creation\n");
	struct mempool *i_mpool = mtable->mpool_set;
//	for(; i_mpool->next != mtable->mpool_set; ){
	for(iter_q = 0; iter_q < RX_QUEUES; iter_q++){
		printf("id = %d\t",i_mpool->id);
		printf("next = %p\n",i_mpool->next);
		i_mpool = i_mpool->next;
	}
	
	//init_port(mtable->mpool_set->mbuf_pool);
	init_port(mtable->mpool_set);
/*
	struct inout_ring *ring = ringInstance();
	if(!ring)
		rte_exit(EXIT_FAILURE,"Ring buffer init failed\n");
	if(!ring->in)
		ring->in = rte_ring_create("in ring",RING_SIZE,rte_socket_id(),RING_F_SP_ENQ | RING_F_SC_DEQ);
	if(!ring->out)
		ring->out = rte_ring_create("out ring",RING_SIZE,rte_socket_id(),RING_F_SP_ENQ | RING_F_SC_DEQ);
*/
	unsigned lcore_id = rte_lcore_id();

	lcore_id = rte_get_next_lcore(lcore_id,1,0);
#if ENABLE_PARSE
	//rte_eal_remote_launch(pkt_process,mtable->mpool_set->mbuf_pool,lcore_id);
	rte_eal_remote_launch(pkt_process,mtable->mpool_set,lcore_id);
	printf("launched pkt_process\n");
#endif

#if ENABLE_SEND	
	lcore_id = rte_get_next_lcore(lcore_id,1,0);
	rte_eal_remote_launch(mbuf_produce,mtable->mpool_set,lcore_id);
	printf("launched mbuf_produce\n");
#endif

	printf("mbuf pool num:%d\n",mtable->count);
	while(1) {
		struct mempool *mpool_iter = mtable->mpool_set;

		struct rte_mbuf *rx[BURST_SIZE];
		struct rte_mbuf *tx[BURST_SIZE];

		unsigned i;
		for(i = 0; i < RX_QUEUES; i++) {
			unsigned num_rx = rte_eth_rx_burst(0,i,rx,BURST_SIZE);
			if(num_rx > BURST_SIZE)
				rte_exit(EXIT_FAILURE, "Error recv from eth\n");
			else if(num_rx > 0) {
//				printf("recv %u pkts by %d\n",num_rx,rte_lcore_id());
				rte_ring_sp_enqueue_burst(mpool_iter->in,(void **)rx,num_rx,NULL);
			}

			unsigned num_tx = rte_ring_sc_dequeue_burst(mpool_iter->out,(void **)tx,BURST_SIZE,NULL);
			if(num_tx > 0) {
				
				rte_eth_tx_burst(0,i,tx,num_tx);
				
				unsigned j = 0;
				for(j = 0; j < num_tx; j++) {
					rte_pktmbuf_free(tx[i]);
				}

			}

			mpool_iter = mpool_iter->next;
		}
	}

	return 0;
}
