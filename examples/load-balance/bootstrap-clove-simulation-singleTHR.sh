#!/bin/bash

# Clove Run mode: 0 Edge flowlet, 1 Clove-ECN, 2 Clove-INT

# cd ../../build/examples/load-balance
cd ../..

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../..
#export NS_LOG='CongaSimulationLarge=level_all'

echo "Starting Simulation: $1"

# CDF='DCTCP'
CDF='VL2'
seed=21
timeout=150
leafServerCapacity=1
spineLeafCapacity=1
# for seed in 21
# do
        for Ld in 0.9
        do
            nohup ./waf --run "conga-simulation-large --ID=$1 --runMode=Clove --cloveRunMode=1 --cloveDisToUncongestedPath=false --cloveFlowletTimeout=$timeout --StartTime=0 --EndTime=4 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8  --serverCount=16 --spineLeafCapacity=$spineLeafCapacity --leafServerCapacity=$leafServerCapacity --transportProt=DcTcp --cdfFileName=examples/load-balance/$CDF\_CDF.txt --load=$Ld  --asymCapacity=true --asymCapacityPoss=20 --randomSeed=$seed" > ./"$1"_"$CDF"_8x8_CloveECN_TO$timeout\_$Ld.log 2>&1 &

			sleep 5
			echo "Launching Load:${Ld}"
        done
# done
exit

