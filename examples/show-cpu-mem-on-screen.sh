#! /bin/bash

while test 1=1;do ps -aux | grep ns3.26-p4-examp | grep -v grep | awk '{print "cpu: "$3" mem:"$4}';sleep 5;done 


