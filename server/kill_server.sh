#!/bin/bash

proc=`adb shell su -c "ps" | grep cachegrab_server`
pid=`echo $proc | cut -d " " -f2`

if [[ ! -z $pid ]]; then
    echo "Killing $pid"
    adb shell su -c "kill $pid"
fi

