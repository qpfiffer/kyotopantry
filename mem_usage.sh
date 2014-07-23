#!/usr/bin/env bash

function get_usage() {
    J=0
    for I in $(cat /proc/$1/smaps | grep '^Size:' | awk '{print $2}'); 
    do
        ((J=J+$I))
    done
    echo -n $J' kB'
}

echo $(get_usage $(pgrep kyotopantry))
