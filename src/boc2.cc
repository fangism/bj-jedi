// "boc2.cc"

#include <iostream>
#include "blackjack.hh"

using std::cout;
using std::endl;
using blackjack::variation;
using blackjack::strategy;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param H17 true if dealer hits on soft 17
 */
static
void
test(const bool H17) {
	variation v;
	v.H17 = H17;
//	v.surrender_late = true;
	v.dump(cout);
	strategy S(v);
	S.evaluate();
//	S.dump(cout);
	cout << "Player\'s overall edge = " << S.overall_edge() << endl;
	cout << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
main(int, char*[]) {
//	test(true);
	test(false);
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

