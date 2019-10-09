#!/usr/bin/env bash

#configs
##config.sh and functions.sh must be included in caller script
#end configs

stage="sparrow"
echo "start of stage: $stage"
fn_dirEnsure "$dirbuild"
cmdcompiler="gcc -std=c89 -Wall -Wc++-compat -O3 main.c  -lm -o $dirbuild/$exename"
cd $dirsrc
#
echo $cmdcompiler
$cmdcompiler 
fn_stoponerror "$?" $LINENO
#
cd ..
printf "\n end of stage: $stage \n"
