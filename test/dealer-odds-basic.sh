#!/bin/sh
bin=../src/apps
echo "*** dealer state odds, basic calculation, H17 ***"
$bin/bj-dealer-odds -n 1 -b -H
echo ""
echo "*** dealer state odds, basic calculation, S17 ***"
$bin/bj-dealer-odds -n 1 -b -S
