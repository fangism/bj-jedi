// "boc2.cc"

#include <iostream>
#include "blackjack.hh"
#include "util/probability.tcc"

using std::cout;
using std::endl;
using blackjack::variation;
using blackjack::strategy;
using blackjack::edge_type;
using util::normalize;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
/**
	\param H17 true if dealer hits on soft 17
 */
static
void
test(const bool H17, const deck& d) {
	variation v;
	v.H17 = H17;
	v.dump(cout);
	strategy S(v);
	S.set_card_distribution(d);
	S.evaluate();
	S.dump(cout);
	cout << endl;
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
main(int, char*[]) {
	variation v;
	v.H17 = true;
//	v.H17 = false;
//	v.surrender_late = true;
//	v.one_card_on_split_aces = false;
	strategy S(v);
	S.set_card_distribution(standard_deck);
	S.evaluate();
	S.dump(cout);
	const edge_type base_edge = S.overall_edge();
//	cout << "Player\'s overall edge = " << base_edge << endl;
	cout << endl;

// sensitivity analysis to differential probability
	// for a 10% relative increase in frequency
	const probability_type pdel = 0.1;
	size_t i;		// which card to vary probability
	for (i=0; i<10; ++i) {
		deck d(standard_deck);
		d[i] *= 1 +pdel;
		normalize(d);
		strategy del(v);
		del.set_card_distribution(d);
		del.evaluate();
		const edge_type diff_edge = del.overall_edge();
	cout << "edge sensitivity [card=" << strategy::card_name[i] <<
		", +" << pdel << "]: " << diff_edge <<
		", de/dp=" << (diff_edge -base_edge)/pdel << endl;
	}
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

