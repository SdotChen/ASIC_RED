#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <arpa/inet.h>

#include "server.h"
#include "list.h"
#include "port.h"

struct rte_mbuf *set_mbuf(struct rte_mempool *mbuf_pool, uint16_t frame_len, uint8_t l4_proto, uint32_t sip, uint32_t dip){

	struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
	if(mbuf == NULL)
		return NULL;

	mbuf->pkt_len = frame_len;
	mbuf->data_len = frame_len;

	void *pktdata = rte_pktmbuf_mtod(mbuf, void *);

	// Ethernet
	struct rte_ether_hdr *hdr_ether = (struct rte_ether_hdr *)pktdata;

	rte_eth_macaddr_get(0,(struct rte_ether_addr *)&hdr_ether->s_addr);
	memset(&hdr_ether->d_addr.addr_bytes,1,sizeof(struct rte_ether_addr));
	hdr_ether->ether_type = htons(RTE_ETHER_TYPE_IPV4);

	// IPv4
	struct rte_ipv4_hdr *hdr_ipv4 = (struct rte_ipv4_hdr *)(hdr_ether + 1);
	
	hdr_ipv4->version_ihl = 0x45;
	hdr_ipv4->type_of_service = 0;
	hdr_ipv4->total_length = htons(frame_len - sizeof(struct rte_ether_hdr));
	hdr_ipv4->packet_id = 0;
	hdr_ipv4->fragment_offset = 0;
	hdr_ipv4->time_to_live = 64;
	hdr_ipv4->next_proto_id = l4_proto;
	hdr_ipv4->src_addr = sip;
	hdr_ipv4->dst_addr = dip;
	hdr_ipv4->hdr_checksum = 0;
	hdr_ipv4->hdr_checksum = rte_ipv4_cksum(hdr_ipv4);

	if(l4_proto == IPPROTO_TCP) {
		
		struct rte_tcp_hdr *hdr_tcp = (struct rte_tcp_hdr *)(hdr_ipv4 + 1);

		hdr_tcp->src_port = htons(33445);
		hdr_tcp->dst_port = htons(33446);
		hdr_tcp->sent_seq = 0;
		hdr_tcp->recv_ack = 0;
		hdr_tcp->data_off = 0x50;
		hdr_tcp->rx_win = htons(10000);
		hdr_tcp->tcp_urp = 0;

		uint16_t tcp_len = frame_len - sizeof(struct rte_ether_hdr) - sizeof(struct rte_ipv4_hdr);
		memset((uint8_t *)(hdr_tcp + 1),1,tcp_len - sizeof(struct rte_tcp_hdr));
		*((char *)hdr_tcp + tcp_len) = '\0';

		hdr_tcp->cksum = 0;
		hdr_tcp->cksum = rte_ipv4_udptcp_cksum(hdr_ipv4,hdr_tcp);

	} else if(l4_proto == IPPROTO_UDP) {

		struct rte_udp_hdr *hdr_udp = (struct rte_udp_hdr *)(hdr_ipv4 + 1);

		hdr_udp->src_port = htons(33445);
		hdr_udp->dst_port = htons(33446);
		uint16_t dgram_len = frame_len - sizeof(struct rte_ether_hdr) - sizeof(struct rte_ipv4_hdr);
		hdr_udp->dgram_len = htons(dgram_len);

		memset((uint8_t *)(hdr_udp + 1),1,dgram_len - sizeof(struct rte_udp_hdr));
		*((char *)hdr_udp + dgram_len) = '\0';

		hdr_udp->dgram_cksum = 0;
		hdr_udp->dgram_cksum = rte_ipv4_udptcp_cksum(hdr_ipv4,hdr_udp);

	} else {
		rte_free(mbuf);
		return NULL;
	}

	return mbuf;
}

int mbuf_produce(void *arg) {

	struct mempool *mpool = (struct mempool *)arg;
	struct mem_table *mtable = memInstance(); 

	while(1){
//		struct mempool *mpool_iter;

//		for(mpool_iter = mpool; mpool_iter->next != mpool; mpool_iter = mpool_iter->next) {	
	
			//struct rte_mbuf *mbuf = set_mbuf(mpool_iter->mbuf_pool,1500,IPPROTO_UDP,htonl(0xC0A80202),htonl(0xC0A80203));
			struct rte_mbuf *mbuf = set_mbuf(mpool->next->mbuf_pool,1500,IPPROTO_UDP,htonl(0xC0A80202),htonl(0xC0A80203));
			if(mbuf == NULL)
				continue;

			rte_ring_mp_enqueue_burst(mtable->mpool_set->out, (void **)&mbuf, 1, NULL);
//		}
	} // end while(1)

	return 0;

}
