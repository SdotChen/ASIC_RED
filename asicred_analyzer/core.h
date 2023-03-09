#ifndef __CORE_H__
#define __CORE_H__

#define STAT_REPORT_PERIOD_UNIT 1000000000ULL
#define STAT_REPORT_MAG 10
#define DEFAULT_MAX_QDEPTH 24200
struct color_stat {
	unsigned red;
	unsigned yellow;
	unsigned green;
};

struct congest_stat {
	unsigned enq_stat[4];
	unsigned app_stat[4];
};

struct indicator_stat {
	uint64_t total_time_ig2eg;
	uint64_t max_time_ig2eg;
	uint64_t min_time_ig2eg;
	uint64_t total_qdelta;
	uint32_t min_qdelta;
	uint32_t max_qdelta;
	uint64_t total_qdepth;
	uint64_t max_qdepth;
	uint64_t min_qdepth;
};

struct q_stat {
	unsigned num_udp;
	unsigned num_tcp;
	uint64_t bytes_udp;
	uint64_t bytes_tcp;
	unsigned qdepth_alloc[10];
	struct indicator_stat *indicator;
	struct color_stat *color;
};

struct global_stat {
	uint64_t seq_ig;
	uint64_t seq_eg;
	unsigned num_udp;
	unsigned num_tcp;
	uint64_t bytes_udp;
	uint64_t bytes_tcp;
	unsigned num_repeat;
	struct q_stat *q[8];
	uint32_t last_seq_recirc;
};

struct p4_hdr {
	uint32_t tstamp_ig;
	uint32_t tstamp_eg;
	uint8_t	 qid;
	uint32_t enq_qdepth;
	uint8_t  color;
	uint32_t qdelta;
	uint32_t seq_ig;
	uint32_t seq_eg;
	uint16_t egress_port;
	uint16_t  drop_prob;
	uint32_t seq_recirc;
	uint32_t aver_qdepth;
}__attribute__((__packed__));


/*
	get stat singleton
*/
struct global_stat *gstatInstance(void);

/*
	used in print_stat()
*/
void print_queue_stat(struct q_stat* queue,FILE *);

/*
	for each pkt update queue state
*/
void update_stat_q(struct q_stat *queue, uint8_t protocol, unsigned frame_len, struct p4_hdr *p4);

/*
	get q_stat by qid in p4_header
*/
struct q_stat *get_queue_from_qid(struct global_stat *gstat, uint8_t qid);

/*
	print the global stat information and all q_stat statistics.	
*/
void print_stat(struct global_stat *,FILE *);

/*
	reset the global stat information and all q_stat statistics.
*/
void clear_stat(struct global_stat *);

void fprint_qdepth(struct p4_hdr *);
/*
	protocol stack thread
*/
int pkt_process(void *);  

#endif
