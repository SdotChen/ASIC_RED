mgid = bfrt.pre.mgid
node = bfrt.pre.node

node.add(MULTICAST_NODE_ID=24,DEV_PORT=[24,68])
node.add(MULTICAST_NODE_ID=0,DEV_PORT=[8,60])

mgid.add(MGID=24,MULTICAST_NODE_ID=[24],MULTICAST_NODE_L1_XID=[0],MULTICAST_NODE_L1_XID_VALID=[False])

