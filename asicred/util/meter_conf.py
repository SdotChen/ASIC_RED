table = bfrt.asicred.pipe.Ingress.aqm_meter

port = 24

C_RATIO = 0
P_RATIO = 1


#the below means 'item' = 40G * ratio

cir = 40 * 1000 * 1000 * C_RATIO
cbs = 40 * 1000 * 1000 * C_RATIO

pir = 40 * 1000 * 1000 * P_RATIO
pbs = 40 * 1000 * 1000 * P_RATIO

table.mod(METER_INDEX=port,METER_SPEC_CIR_KBPS=cir,METER_SPEC_CBS_KBITS=cbs,METER_SPEC_PIR_KBPS=pir,METER_SPEC_PBS_KBITS=pbs)
