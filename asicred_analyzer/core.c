#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <rte_timer.h>

#include "core.h"
#include "list.h"
#include "port.h"

#ifndef BURST_SIZE
#define BURST_SIZE 8192
#endif

static uint64_t last_seq_ig = 0;
static uint64_t last_seq_eg = 0;

static uint32_t global_seq = 0;
static uint16_t global_qdepth = 0;

static uint64_t prev_tsc = 0, cur_tsc, diff_tsc;

FILE *f;

struct global_stat *gstatInst = NULL;

struct global_stat *gstatInstance(void) {

	if(gstatInst == NULL) {
		int i;
		char buf[32];

		gstatInst = rte_malloc("global_stat",sizeof(struct global_stat),0);
		if(gstatInst == NULL)
			return NULL;
		memset(gstatInst,0,sizeof(struct global_stat));

		for(i = 0; i < 8; i++) {
			sprintf(buf,"q_stat %d",i);

			gstatInst->q[i] = rte_malloc(buf,sizeof(struct q_stat),0);
			if(gstatInst->q[i] == NULL)
				return NULL;
			memset(gstatInst->q[i],0,sizeof(struct q_stat));
			
			sprintf(buf,"q%d indicator stat",i);
			gstatInst->q[i]->indicator = rte_malloc(buf,sizeof(struct indicator_stat),0);
			if(gstatInst->q[i]->indicator == NULL)
				return NULL;
			memset(gstatInst->q[i]->indicator,0,sizeof(struct indicator_stat));

			sprintf(buf,"q%d color stat",i);
			gstatInst->q[i]->color = rte_malloc(buf,sizeof(struct color_stat),0);
			if(gstatInst->q[i]->color == NULL)
				return NULL;
			memset(gstatInst->q[i]->color,0,sizeof(struct color_stat));

		} // end for(i = 0;i < 8...
	} // end if(gstatInst == NULL

	return gstatInst;	

}

void print_queue_stat(struct q_stat* queue, FILE *fp) {

	if(queue->num_udp + queue->num_tcp != 0) {
		printf("----------Global statistics----------\n");
		printf("recv UDP pkts:%u,bytes:%lu,GB:%.3f\n",queue->num_udp,queue->bytes_udp,(double)queue->bytes_udp * 8  / (1000 * 1000 * 1000));
		printf("recv TCP pkts:%u,bytes:%lu,GB:%.3f\n",queue->num_tcp,queue->bytes_tcp,(double)queue->bytes_tcp * 8  / (1000 * 1000 * 1000));

		printf("\n");
		printf("----------Indicator statistics----------\n");
		double aver_queue = (double)queue->indicator->total_qdepth / (queue->num_udp + queue->num_tcp);
		printf("aver qdepth:%.2f\t\t",aver_queue);
		fprintf(fp,"%.2lf\t\t",aver_queue);
//		printf("total qdepth:%lu\t",queue->indicator->total_qdepth);
		printf("max qdepth:%ld\t\t\t",queue->indicator->max_qdepth);
		fprintf(fp,"%lu\t\t",queue->indicator->max_qdepth);
		printf("min qdepth:%ld\n",queue->indicator->min_qdepth);

		double aver_qdelay = (double)queue->indicator->total_qdelta / (queue->num_udp + queue->num_tcp);
		printf("aver qdelay:%.2f\t\t",aver_qdelay);
		fprintf(fp,"%.2lf\t",aver_qdelay);
//		printf("total qdelay:%lu\t",queue->indicator->total_qdelta);
		printf("max qdelay:%u\t\t",queue->indicator->max_qdelta);
		fprintf(fp,"%u\t\t",queue->indicator->max_qdelta);
		printf("min qdelay:%u\n",queue->indicator->min_qdelta);

		printf("aver ig2eg delay:%.2f\t",(double)queue->indicator->total_time_ig2eg / (queue->num_udp + queue->num_tcp));
//		printf("total ig2eg qdelay:%lu\t",queue->indicator->total_time_ig2eg);
		printf("max ig2eg delay:%ld\t\t",queue->indicator->max_time_ig2eg);
		printf("min ig2eg delay:%ld\n",queue->indicator->min_time_ig2eg);

		printf("----------qdepth allocation----------\n");
		unsigned i;
		for(i = 0; i < 10;i++){
			printf("%u%%-%u%%:%u\n",i*10,10*(i+1),queue->qdepth_alloc[i]);
		}	
		printf("----------Color statistics----------\n");
		unsigned sum = queue->color->red + queue->color->yellow + queue->color->green;
		printf("meter color: red:%u(%.2f%%),yellow:%u(%.2f%%),green:%u(%.2f%%)\n",queue->color->red,(double)queue->color->red * 100 / sum,
			queue->color->yellow, (double)queue->color->yellow * 100 / sum, queue->color->green,(double)queue->color->green * 100 / sum);
		fprintf(fp,"%u\n",queue->color->red);
		fflush(fp);
		printf("\n");
	}
}

void print_stat(struct global_stat *gstat,FILE *fp) {
	unsigned i;
	char buf[32];

	printf("recv UDP pkts:%u,bytes:%lu,GB:%.3f\n",gstat->num_udp,gstat->bytes_udp,(double)gstat->bytes_udp * 8 / (1000 * 1000 * 1000));
	printf("recv TCP pkts:%u,bytes:%lu,GB:%.3f\n",gstat->num_tcp,gstat->bytes_tcp,(double)gstat->bytes_tcp * 8 / (1000 * 1000 * 1000));
	printf("recirc pkt: %u\n",global_seq - gstat->last_seq_recirc);
	gstat->last_seq_recirc = global_seq;
	printf("While(1) repeat times: %u\n\n",gstat->num_repeat);

	if(gstat->num_udp + gstat->num_tcp != 0) {
        printf("seq_ig - seq_eg = %lu, pkt loss:%.3f%%\n\n", gstat->seq_ig - last_seq_ig - gstat->seq_eg + last_seq_eg ,
            ( (double)(gstat->seq_ig - last_seq_ig - gstat->seq_eg + last_seq_eg) * 100 / (gstat->seq_ig - last_seq_ig) ));
        last_seq_ig = gstat->seq_ig;
        last_seq_eg = gstat->seq_eg;
    } else {
        printf("no pkt recved\n\n");
	}

	for(i = 0; i < 8; i++) {
		if(gstat->q[i]->num_udp + gstat->q[i]->num_tcp != 0) {
			printf("-----------------\n");
			sprintf(buf,"-----queue %d-----",i);
			puts(buf);
			printf("-----------------\n");
			print_queue_stat(gstat->q[i],fp);
			printf("\n");
		}
	}

	printf("\n\n");
}

void clear_stat(struct global_stat *gstat) {
	unsigned i;

	gstat->num_udp = 0;
	gstat->bytes_udp = 0;
	gstat->num_tcp = 0;
	gstat->bytes_tcp = 0;
	gstat->num_repeat = 0;

	for(i = 0; i < 8; i++){

		gstat->q[i]->num_udp = 0;
		gstat->q[i]->bytes_udp = 0;
		gstat->q[i]->num_tcp = 0;
		gstat->q[i]->bytes_tcp = 0;
		memset(gstat->q[i]->indicator,0,sizeof(struct indicator_stat));
		memset(gstat->q[i]->qdepth_alloc,0,sizeof(unsigned) * 10);
		memset(gstat->q[i]->color,0,sizeof(struct color_stat));

	}// end for(i = 0; i < 8...	(q num)

}

static void stat_timer_cb(__attribute__((unused))struct rte_timer *tim,__attribute__((unused)) void *arg) {
	printf("period: %lu ns\n",diff_tsc);
	struct global_stat *gstat = gstatInstance();
	printf("recirc_seq:%u\n",global_seq);	
	print_stat(gstat,f);
	clear_stat(gstat);

	return ;
}

struct q_stat *get_queue_from_qid(struct global_stat *gstat, uint8_t qid) {

	return gstat->q[qid % 8];
}

void update_stat_q(struct q_stat *queue, uint8_t protocol, unsigned frame_len, struct p4_hdr *p4) {
	// queue global stat update
	if(protocol == IPPROTO_TCP) {
		queue->num_tcp++;
		queue->bytes_tcp += frame_len;
	} else if(protocol == IPPROTO_UDP) {
		queue->num_udp++;
		queue->bytes_udp += frame_len;
	}

	// queue color stat update
	if(p4->color == 0)
		queue->color->green++;
	else if(p4->color == 1)
		queue->color->yellow++;
	else if(p4->color == 3)
		queue->color->red++;

	// queue indicator stat update
	uint32_t time_ig2eg = ntohl(p4->tstamp_eg) - ntohl(p4->tstamp_ig);

	
	// qdepth allocation stat
	uint32_t qdep = ntohl(p4->enq_qdepth);
	if(qdep < DEFAULT_MAX_QDEPTH * 0.1)
		queue->qdepth_alloc[0]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.2)
		queue->qdepth_alloc[1]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.3)
		queue->qdepth_alloc[2]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.4)
		queue->qdepth_alloc[3]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.5)
		queue->qdepth_alloc[4]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.6)
		queue->qdepth_alloc[5]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.7)
		queue->qdepth_alloc[6]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.8)
		queue->qdepth_alloc[7]++;
	else if(qdep < DEFAULT_MAX_QDEPTH * 0.9)
		queue->qdepth_alloc[8]++;
	else if(qdep < 25000)
		queue->qdepth_alloc[9]++;

	if( (double)time_ig2eg / ntohl(p4->qdelta) > 2 && time_ig2eg > 262144)
         p4->qdelta = htonl(ntohl(p4->qdelta) + 262144 * (time_ig2eg / 262144));

	queue->indicator->total_qdepth += ntohl(p4->enq_qdepth);
	queue->indicator->total_qdelta += ntohl(p4->qdelta);
	queue->indicator->total_time_ig2eg += time_ig2eg;

	if(queue->indicator->min_qdepth == 0) {
		queue->indicator->min_qdepth = ntohl(p4->enq_qdepth);
		queue->indicator->max_qdepth = ntohl(p4->enq_qdepth);
	}

	if(ntohl(p4->enq_qdepth) < queue->indicator->min_qdepth)
		queue->indicator->min_qdepth = ntohl(p4->enq_qdepth);

	if(ntohl(p4->enq_qdepth) > queue->indicator->max_qdepth)
		queue->indicator->max_qdepth = ntohl(p4->enq_qdepth);

	if(queue->indicator->min_qdelta == 0) {
		queue->indicator->min_qdelta = ntohl(p4->qdelta);
		queue->indicator->max_qdelta = ntohl(p4->qdelta);
	}

	if(ntohl(p4->qdelta) < queue->indicator->min_qdelta)
		queue->indicator->min_qdelta = ntohl(p4->qdelta);

	if(ntohl(p4->qdelta) > queue->indicator->max_qdelta)
		queue->indicator->max_qdelta = ntohl(p4->qdelta);

	if(queue->indicator->min_time_ig2eg == 0){
		queue->indicator->min_time_ig2eg = time_ig2eg;
		queue->indicator->max_time_ig2eg = time_ig2eg;
	}

	if(time_ig2eg < queue->indicator->min_time_ig2eg)
		queue->indicator->min_time_ig2eg = time_ig2eg;

	if(time_ig2eg > queue->indicator->max_time_ig2eg)
		queue->indicator->max_time_ig2eg = time_ig2eg;
		
}
#if 1
void fprint_qdepth(struct p4_hdr *p4){
	uint32_t seq = ntohl(p4->seq_recirc);
//	printf("%u %u\n",seq,global_seq);
	if(seq > global_seq) {
//		fprintf(fp,"%u %u\n",seq,ntohs(p4->aver_qdepth));
		global_seq = seq;
		global_qdepth = ntohs(p4->aver_qdepth);
	}
}
#endif
int pkt_process(void *arg) {
#if 1
//	FILE *fp;
	printf("%p\n",f);
	f = fopen("./log_qdepth","w+");
	printf("%p\n",f);
	fprintf(f,"aver qdepth\tmax qdepth\taver qdelay\tmax qdelay\tred num\n");
	fflush(f);
	if(f == NULL)
		rte_exit(EXIT_FAILURE,"fopen failure\n");
#endif
	//struct rte_mempool *mbuf_pool = (struct rte_mempool *)arg;
	//struct inout_ring *ring = ringInstance();
	struct mempool *mpool = (struct mempool *)arg;

	struct global_stat *gstat = gstatInstance();

	rte_timer_subsystem_init();
	struct rte_timer stat_timer;
	rte_timer_init(&stat_timer);

	uint64_t hz = rte_get_timer_hz();
	unsigned lcore_id = rte_lcore_id();
	
	rte_timer_reset(&stat_timer,hz,PERIODICAL,lcore_id,stat_timer_cb,NULL);

	printf("begin pkt_process from lcore %d\n",rte_lcore_id());
	
	unsigned q;
	while(1) {
		gstat->num_repeat++;
		struct mempool *mpool_iter = mpool;

		for(q = 0; q < RX_QUEUES; q++) {
			struct rte_mbuf *mbufs[BURST_SIZE];

			unsigned num_recvd = rte_ring_mc_dequeue_burst(mpool_iter->in, (void **)mbufs, BURST_SIZE, NULL);
		
			unsigned i;
			for(i = 0; i < num_recvd; i++) {

				struct rte_ether_hdr *hdr_ether = rte_pktmbuf_mtod(mbufs[i], struct rte_ether_hdr *);
				if(hdr_ether->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
				
					struct rte_ipv4_hdr *hdr_ipv4 = (struct rte_ipv4_hdr *)(hdr_ether + 1);

					unsigned frame_len = rte_pktmbuf_pkt_len(mbufs[i]);
					if(hdr_ipv4->next_proto_id == IPPROTO_UDP) {
						struct rte_udp_hdr *hdr_udp = (struct rte_udp_hdr *)(hdr_ipv4 + 1);
					
						gstat->num_udp++;
						gstat->bytes_udp += frame_len;

						struct p4_hdr *hdr_p4 = (struct p4_hdr *)(hdr_udp + 1);

						struct q_stat *q = get_queue_from_qid(gstat,hdr_p4->qid);
						if(q == NULL)
							continue;
						fprint_qdepth(hdr_p4);

						update_stat_q(q,IPPROTO_UDP,frame_len,hdr_p4);

						if(ntohl(hdr_p4->seq_ig) == 0)
                            gstat->seq_ig = (gstat->seq_ig & 0xffffffff00000000) + 0x0000000100000000;
                        else 
                            gstat->seq_ig = (gstat->seq_ig & 0xffffffff00000000) + (uint64_t)(ntohl(hdr_p4->seq_ig));
    
                        if(ntohl(hdr_p4->seq_eg) == 0)
                            gstat->seq_eg = (gstat->seq_eg & 0xffffffff00000000) + 0x0000000100000000;
                        else 
                            gstat->seq_eg = (gstat->seq_eg & 0xffffffff00000000) + (uint64_t)(ntohl(hdr_p4->seq_eg));
					
					}

					else if(hdr_ipv4->next_proto_id == IPPROTO_TCP) {
						struct rte_tcp_hdr *hdr_tcp = (struct rte_tcp_hdr *)(hdr_ipv4 + 1);

						gstat->num_tcp++;	
						gstat->bytes_tcp += frame_len;

						struct p4_hdr *hdr_p4 = (struct p4_hdr *)(hdr_tcp + 1);

						struct q_stat *q = get_queue_from_qid(gstat,hdr_p4->qid);
						if(q == NULL)
							continue;

						fprint_qdepth(hdr_p4);
						update_stat_q(q,IPPROTO_TCP,frame_len,hdr_p4);

						if(ntohl(hdr_p4->seq_ig) == 0)
                            gstat->seq_ig = (gstat->seq_ig & 0xffffffff00000000) + 0x0000000100000000;
                        else 
                            gstat->seq_ig = (gstat->seq_ig & 0xffffffff00000000) + (uint64_t)(ntohl(hdr_p4->seq_ig));
    
                        if(ntohl(hdr_p4->seq_eg) == 0)
                            gstat->seq_eg = (gstat->seq_eg & 0xffffffff00000000) + 0x0000000100000000;
                        else 
                            gstat->seq_eg = (gstat->seq_eg & 0xffffffff00000000) + (uint64_t)(ntohl(hdr_p4->seq_eg));
					}
				}

			rte_pktmbuf_free(mbufs[i]);
			
		} // end for(i = 0...)
		mpool_iter = mpool_iter->next;
	}	

		cur_tsc = rte_rdtsc();
		diff_tsc = cur_tsc - prev_tsc;

		if(diff_tsc > STAT_REPORT_PERIOD_UNIT) {
			rte_timer_manage();
			prev_tsc = cur_tsc;
		}

	} // end while(1)

	fclose(f);
	return 0;
}
