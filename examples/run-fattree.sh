#! /bin/bash
cd ../../../
./waf --run "src/ns4/examples/p4-example --podnum=$2 --model=$1" >run_res/$1_$2.txt 2>&1 

echo "Running Result: $PWD/run_res/$1_$2.txt" 
grep "Running time:"  run_res/$1_$2.txt
