#!/bin/sh
bin=../src/apps
echo "*** dealer state odds, dynamic calculation, H17 ***"
$bin/bj-dealer-odds -n 1 -d -H
echo ""
echo "*** dealer state odds, dynamic calculation, S17 ***"
$bin/bj-dealer-odds -n 1 -d -S
