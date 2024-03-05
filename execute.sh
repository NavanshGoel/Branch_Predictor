#!/bin/bash
clear
make clean
make
printf "\nStarting Predictions...\n"

for file in $(ls ./traces)
do
printf "\n################# Testing on $file ########################\n"
printf "\nGShare 13 bits global history:\n"
bunzip2 -kc ./traces/$file |./predictor --gshare:13 #| grep "Misprediction Rate:"
printf "\nTournament :9 bits global history, 10 bits local history, 10 PC bit\n"
bunzip2 -kc ./traces/$file |./predictor --tournament:9:10:10 #| grep "Misprediction Rate:"
printf "\nPerceptron Predictor\n"
bunzip2 -kc ./traces/$file |./predictor --custom #| grep "Misprediction Rate:"
printf "\n#############################################################\n"
done
