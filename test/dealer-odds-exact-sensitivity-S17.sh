#!/bin/sh
bin=../src/apps
echo "*** dealer state odds, exact calculation, S17 ***"
$bin/bj-dealer-odds -n 1 -e -S
for c in 2 3 4 5 6 7 8 9 T A
do
echo ""
echo "*** removing a card: $c ***"
$bin/bj-dealer-odds -n 1 -e -S -r $c
echo ""
echo "*** adding a card: $c ***"
$bin/bj-dealer-odds -n 1 -e -S -a $c
done
