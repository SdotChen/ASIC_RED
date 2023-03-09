#ifndef __SERVER_H__
#define __SERVER_H__


struct rte_mbuf *set_mbuf(struct rte_mempool *mbuf_pool, uint16_t frame_len, uint8_t l4_proto, uint32_t sip, uint32_t dip);

int mbuf_produce(void *);

#endif
