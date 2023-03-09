table = bfrt.asicred.pipe.Egress.map_qdepth_to_prob_t

min_ratio = 0.03
max_ratio = 0.1
max_qdepth = 24254
prob_scope = 1023

min_prob = int(min_ratio * prob_scope)
max_prob = int(max_ratio * prob_scope)

for i in range(min_prob,max_prob+1):
	table.add_with_map_qdepth_to_prob(qdepth_for_match_start=int(max_qdepth * i / prob_scope),qdepth_for_match_end=int(max_qdepth * (i+1) / prob_scope),prob=i)

table.add_with_map_qdepth_to_prob(qdepth_for_match_start=int(max_qdepth * (max_prob+1) / prob_scope),qdepth_for_match_end=65535,prob=1023)
