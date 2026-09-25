#!/bin/bash
echo ""
echo "Auto grader for EECS3221 CPU Scheduling Project"   
echo ""
echo "Make Clean"
make clean 

echo ""
echo "--------------- FCFS ---------------------"
echo "Make FCFS"
make fcfs
echo "Running FCFS"
fcfs schedule.txt > student-fcfs-output.txt
echo "Grading FCFS"

DIFF=$(diff -wB correct-fcfs-output.txt student-fcfs-output.txt) 
if [ "$DIFF" != "" ] 
then
    echo "FCFS: Something is wrong; please use the evaluation rubric to deduct marks."
else
    echo "FCFS: Good Job -- 4 points."
fi


echo ""
echo "--------------- SJF ---------------------"
echo "Make SJF"
make sjf
echo "Running SJF"
sjf schedule.txt > student-sjf-output.txt
echo "Grading SJF"

DIFF=$(diff -wB correct-sjf-output.txt student-sjf-output.txt) 
if [ "$DIFF" != "" ] 
then
    echo "SJF: Something is wrong; please use the evaluation rubric to deduct marks."
else
    echo "SJF: Good Job -- 4 points."
fi


echo ""
echo "--------------- Round Robin ---------------------"
echo "Make RR"
make rr
echo "Running RR"
rr schedule.txt > student-rr-output.txt
echo "Grading RR"

DIFF=$(diff -wB correct-rr-output.txt student-rr-output.txt) 
if [ "$DIFF" != "" ] 
then
    echo "RR: Something is wrong; please use the evaluation rubric to deduct marks."
else
    echo "RR: Good Job -- 4 points."
fi


echo ""
echo "--------------- PRIORITY ---------------------"
echo "Make PRIORITY"
make priority
echo "Running PRIORITY"
priority schedule.txt > student-priority-output.txt
echo "Grading PRIORITY"

DIFF=$(diff -wB correct-priority-output.txt student-priority-output.txt) 
if [ "$DIFF" != "" ] 
then
    echo "PRIORITY: Something is wrong; please use the evaluation rubric to deduct marks."
else
    echo "PRIORITY: Good Job -- 4 points."
fi


echo ""
echo "--------------- PRIORITY_RR ---------------------"
echo "Make PRIORITY_RR"
make priority_rr
echo "Running PRIORITY_RR"
priority_rr schedule.txt > student-priority_rr-output.txt
echo "Grading PRIORITY_RR"

DIFF=$(diff -wB correct-priority_rr-output.txt student-priority_rr-output.txt) 
if [ "$DIFF" != "" ] 
then
    echo "PRIORITY_RR: Something is wrong; please use the evaluation rubric to deduct marks."
else
    echo "PRIORITY_RR: Good Job -- 4 points."
fi
echo "----- END -----"
echo ""