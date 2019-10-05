#!/usr/bin/env bash

#functions
fn_stoponerror () {
if [ $1 -ne 0 ]; then
        lNo=$(expr $2 - 1)
        echo "  Error at line No $lNo"
        exit
else
       printf "."
fi
}
#end functions
