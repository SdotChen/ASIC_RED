#!/bin/bash

bash /root/asicred/scripts/start.sh
bash /root/asicred/scripts/flowtable.sh
bash /root/asicred/scripts/meter.sh
bash /root/asicred/scripts/mod_macaddr.sh
bash /root/asicred/scripts/set_drop_prob_mapping.sh
