#!/bin/bash

# Clove Run mode: 0 Edge flowlet, 1 Clove-ECN, 2 Clove-INT

# cd ../../build/examples/load-balance
cd ../..

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../..
export NS_LOG='CongaSimulationLarge=level_all'

echo "Starting Simulation: $1"
echo "Logs enabled with: $NS_LOG"  

for seed in 21 # {21..25}
do
#    for CloveRunMode in 0  1
  #  do
        for Ld in 0.8
        do
   #        nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Clove --cloveRunMode=$CloveRunMode --cloveDisToUncongestedPath=false --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/DCTCP_CDF.txt --load=$Ld --randomSeed=$seed > /dev/null 2>&1 &
 #          nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Clove --cloveRunMode=$CloveRunMode --cloveDisToUncongestedPath=true --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=$Ld --randomSeed=$seed > /dev/null 2>&1 &
        #  nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=TLB --StartTime=0 --EndTime=3 --FlowLaunchEndTime=0.5 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --TLBRunMode=12 --TLBSmooth=true --TLBProbingEnable=true --TLBMinRTT=63 --TLBT1=66 --TLBRerouting=true --TcpPause=false --TLBProbingInterval=500 --transportProt=DcTcp  --TLBBetterPathRTT=100 --TLBHighRTT=180 --TLBS=640000 --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
           nohup ./waf --run "conga-simulation-large --ID=$1 --runMode=ECMP --StartTime=0 --EndTime=4 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed" > ./Data_mining_8x8_LD70_ECMP_small.log 2>&1 &
  #         nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Conga --StartTime=0 --EndTime=3 --FlowLaunchEndTime=0.5 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
        done
 #   done
done
exit

