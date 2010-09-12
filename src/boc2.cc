// "boc2.cc"

#include <iostream>
#include <iterator>
#include "blackjack.hh"
#include "util/probability.tcc"

using std::cout;
using std::endl;
using std::ostream_iterator;
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
	const probability_type pdel = 0.10;
	cout << "Sensitivitiy analysis: relative probability increase per card = " << pdel << endl;
	size_t i;		// which card to vary probability
	for (i=0; i<10; ++i) {
		deck d(standard_deck);
		deck nd(standard_deck);
		d[i] *= 1 +pdel;
		nd[i] /= 1 +pdel;
		normalize(d);
		normalize(nd);
		strategy del(v);
		strategy ndel(v);
		del.set_card_distribution(d);
		ndel.set_card_distribution(nd);
		del.evaluate();
		ndel.evaluate();
		const edge_type diff_edge = del.overall_edge();
		const edge_type ndiff_edge = ndel.overall_edge();
	cout << "------------------ " << strategy::card_name[i]
		<< " ------------------" << endl;
	cout << "card odds: (A,2,...T)" << endl;
	copy(d.begin(), d.end(),
		ostream_iterator<probability_type>(cout, "\t"));
	cout << "(+)" << endl;
	copy(nd.begin(), nd.end(),
		ostream_iterator<probability_type>(cout, "\t"));
	cout << "(-)" << endl;
		
	del.dump_reveal_edges(cout << "+del%: ");
	ndel.dump_reveal_edges(cout << "-del%: ");
	cout << "edge sensitivity [card=" << strategy::card_name[i] <<
		", +" << pdel << "]: edge=" << diff_edge <<
		"; de/dp=" << (diff_edge -base_edge)/pdel << endl;
	cout << "edge sensitivity [card=" << strategy::card_name[i] <<
		", -" << pdel << "]: edge=" << ndiff_edge <<
		"; de/dp=" << (base_edge -ndiff_edge)/pdel << endl;
		cout << endl;
	}
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

