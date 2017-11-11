#! /bin/bash
cd /home/kphf1995cm/ns-allinone-3.26/ns-3.26
./waf --run "src/ns4/examples/p4-example --podnum=$2 --p4=$1" >run_res/$1_$2.txt 2>&1 

echo "Running Result: $PWD/run_res/$1_$2.txt" 
grep "Running time:"  run_res/$1_$2.txt
