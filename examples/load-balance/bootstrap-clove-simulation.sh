#!/bin/bash

# Clove Run mode: 0 Edge flowlet, 1 Clove-ECN, 2 Clove-INT

# cd ../../build/examples/load-balance
cd ../..

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../..
#export NS_LOG='CongaSimulationLarge=level_all'

echo "Starting Simulation: $1"

CDF='DCTCP'
# CDF='VL2'
seed=21
timeout=50
leafServerCapacity=10
spineLeafCapacity=10
# for seed in 21
# do
        for Ld in 0.1 0.2 0.3 0.4
        do
		tmp=${Ld:2:1}
			# CLOVE
			# taskset -c $((tmp-1)) nohup ./waf --run "conga-simulation-large --ID=$1 --runMode=Clove --cloveRunMode=1 --cloveDisToUncongestedPath=false --cloveFlowletTimeout=$timeout --StartTime=0 --EndTime=4 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8  --serverCount=16 --spineLeafCapacity=$spineLeafCapacity --leafServerCapacity=$leafServerCapacity --transportProt=DcTcp --cdfFileName=examples/load-balance/$CDF\_CDF.txt --load=$Ld  --asymCapacity=true --asymCapacityPoss=20 --randomSeed=$seed" > ./"$1"_"$CDF"_8x8_CloveECN_TO$timeout\_$Ld.log 2>&1 &
			# Letflow
			taskset -c $((tmp-1)) nohup ./waf --run "conga-simulation-large --ID=$1 --runMode=LetFlow --letFlowFlowletTimeout=$timeout --StartTime=0 --EndTime=4 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8  --serverCount=16 --spineLeafCapacity=$spineLeafCapacity --leafServerCapacity=$leafServerCapacity --transportProt=DcTcp --cdfFileName=examples/load-balance/$CDF\_CDF.txt --load=$Ld  --asymCapacity=true --asymCapacityPoss=20 --randomSeed=$seed" > ./"$1"_"$CDF"_8x8_Letflow_TO$timeout\_$Ld.log 2>&1 &
			sleep 5
			echo "Launching Load:${Ld} in CPU:$((tmp-1))"
        done
# done
exit

