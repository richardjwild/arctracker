#!/bin/sh

while true; do
    for FILE in $HOME/modfiles/*.musx; do
        echo "Testing: $FILE ($(date))" >> $HOME/modfiles/output.txt
        builddir-asan/arctracker-console --output=$HOME/modfiles/tmp.wav $FILE >> $HOME/modfiles/output.txt 2>&1
        sleep 10
    done
done
