#!/bin/sh
bin=../src/apps
echo "*** dealer state odds, dynamic calculation, H17 ***"
$bin/bj-dealer-odds -n 1 -d -H
for c in 2 3 4 5 6 7 8 9 T A
do
echo ""
echo "*** removing a card: $c ***"
$bin/bj-dealer-odds -n 1 -d -H -r $c
echo ""
echo "*** adding a card: $c ***"
$bin/bj-dealer-odds -n 1 -d -H -a $c
done
