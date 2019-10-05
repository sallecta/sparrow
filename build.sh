#!/usr/bin/env bash

source "$PWD/build_scripts/_.config.sh"
source "$PWD/build_scripts/_.functions.sh"

if [ "$1" = "--linux" ] || [ "$1" = "linux" ]; then 
    source "$PWD/build_scripts/linux.sh"
elif [ "$1" = "--mingw" ] || [ "$1" = "mingw" ]; then 
    source "$PWD/build_scripts/mingw.sh"
elif [ "$1" = "--test" ] || [ "$1" = "test" ]; then 
    $dirbuild/$exename $dirtests/01.py
elif [ "$1" = "--clean" ] || [ "$1" = "clean" ]; then 
    source "$PWD/build_scripts/clean.sh"
else
    echo "Wrong argument."
fi
