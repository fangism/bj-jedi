#!/bin/sh
bin=../src/apps
echo "*** dealer state odds, dynamic calculation, S17 ***"
$bin/bj-dealer-odds -n 1 -d -S
for c in 2 3 4 5 6 7 8 9 T A
do
echo ""
echo "*** removing a card: $c ***"
$bin/bj-dealer-odds -n 1 -d -S -r $c
echo ""
echo "*** adding a card: $c ***"
$bin/bj-dealer-odds -n 1 -d -S -a $c
done
