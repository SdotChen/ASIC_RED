#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>

#include "port.h"
#include "list.h"

#define SIZE 32

static const struct rte_eth_conf port_conf_default = {

	.rxmode = {.max_rx_pkt_len = RTE_ETHER_MAX_LEN}

};

//void init_port(struct rte_mempool *mbuf_pool) {
//void init_port(struct mempool *mpool) {
void init_port(void *arg) {
/*
	int q;
	char *buf[SIZE];
	for(q = 0; q < RX_QUEUES; q++) {
		sprintf(buf,"mbuf pool %d",q);
		struct rte_mempool *mpool[q] = rte_pktmbuf_pool_create(buf,
			NUM_MBUF,0,0,RTE_MBUF_DEFAULT_BUF_SIZE,rte_socket_id());
	}
*/
	struct mempool *mpool = (struct mempool *)arg;
	int q ;
	uint16_t num_avail_eth = rte_eth_dev_count_avail();
	if(num_avail_eth == 0)
		rte_exit(EXIT_FAILURE,"No supported eth found\n");

	struct rte_eth_dev_info dev_info;
	rte_eth_dev_info_get(0,&dev_info);

	const int num_rx_queues = RX_QUEUES;
	const int num_tx_queues = TX_QUEUES;
	struct rte_eth_conf port_conf = port_conf_default;

	rte_eth_dev_configure(0,num_rx_queues,num_tx_queues,&port_conf);

	struct rte_eth_txconf txq_conf = dev_info.default_txconf;
	txq_conf.offloads = port_conf.rxmode.offloads;

	struct mempool *iter = mpool;
	for(q = 0; q < num_rx_queues; q++) {
		if(rte_eth_rx_queue_setup(0,q,256,
			rte_eth_dev_socket_id(0),NULL,iter->mbuf_pool) < 0) {
		
			rte_exit(EXIT_FAILURE,"RX queues setting failed\n");
		}

		if(rte_eth_tx_queue_setup(0,q,256,
			rte_eth_dev_socket_id(0),&txq_conf) < 0){
			
			rte_exit(EXIT_FAILURE,"TX queues setting failed\n");
		}

		iter = iter->next;
	}
	if(rte_eth_dev_start(0) < 0) {
		rte_exit(EXIT_FAILURE,"Start failed\n");
	}

}
