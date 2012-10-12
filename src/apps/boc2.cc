// "boc2.cc"

#include <iostream>
#include <iterator>
#include "bj/core/variation.hh"
#include "bj/core/blackjack.hh"
#include "bj/core/strategy.hh"
// #include "util/probability.tcc"

using std::cout;
using std::endl;
using std::ostream_iterator;
using cards::card_name;
using cards::card_values;
using cards::standard_deck_count;
using cards::deck_count_type;
using cards::probability_type;
using cards::reveal_print_ordering;
using cards::card_type;
using cards::count_type;
using blackjack::variation;
using blackjack::play_map;
using blackjack::strategy;
using blackjack::edge_type;

/**
	Define to 1 to perform distribution sensitivity analysis.
	Analysis is based on card count.
 */
#define	SENSITIVITY_ANALYSIS			1

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
main(int, char*[]) {
	variation v;
	v.H17 = true;
//	v.H17 = false;
//	v.surrender_late = true;
//	v.one_card_on_split_aces = false;
	play_map pm(v);
	strategy S(pm);
	S.set_card_distribution(standard_deck_count);
	S.evaluate();
	S.dump(cout);
	cout << endl;
#if SENSITIVITY_ANALYSIS
// sensitivity analysis to differential probability
	// for a 10% relative increase in frequency
	const edge_type base_edge = S.overall_edge();
//	const probability_type pdel = 0.10;
	const probability_type pdel = 1.0/52.0;	// +/- 1 card to deck
	cout <<
"Sensitivity analysis: relative probability increase per card = "
		<< pdel << endl;
	card_type j;		// which card to vary probability
	for (j=0; j<card_values; ++j) {
		const card_type i = reveal_print_ordering[j];
		deck_count_type pd(standard_deck_count);
		deck_count_type nd(standard_deck_count);
		pd[i] += 1;
		nd[i] -= 1;
		strategy del(pm);
		strategy ndel(pm);
		del.set_card_distribution(pd);
		ndel.set_card_distribution(nd);
		del.evaluate();
		ndel.evaluate();
		const edge_type diff_edge = del.overall_edge();
		const edge_type ndiff_edge = ndel.overall_edge();
	cout << "------------------ " << card_name[i]
		<< " ------------------" << endl;
	cout << "card odds: (A,2,...T)" << endl;
	copy(pd.begin(), pd.end(),
		ostream_iterator<count_type>(cout, "\t"));
	cout << "(+)" << endl;
	copy(nd.begin(), nd.end(),
		ostream_iterator<count_type>(cout, "\t"));
	cout << "(-)" << endl;
		
	del.dump_reveal_edges(cout << "+1: ");
	ndel.dump_reveal_edges(cout << "-1: ");
	cout << "edge sensitivity [card=" << card_name[i] <<
		", +1/52]: edge=" << diff_edge <<
		"; de/dp=" << (diff_edge -base_edge)/pdel << endl;
	cout << "edge sensitivity [card=" << card_name[i] <<
		", -1/52]: edge=" << ndiff_edge <<
		"; de/dp=" << (base_edge -ndiff_edge)/pdel << endl;
		cout << endl;
	}	// end for

#if 0
	{
	// for ACEs and TENs (common hi-lo)
	const probability_type hdel = pdel/2;
	const probability_type ddel = pdel*2;
	cout << "------------------ AT ------------------" << endl;
		deck_count_type pd(standard_deck_count);
		deck_count_type nd(standard_deck_count);
		pd[ACE] += 1;
		pd[TEN] += 1;
		nd[ACE] -= 1;
		nd[TEN] -= 1;
		strategy del(pm);
		strategy ndel(pm);
		del.set_card_distribution(pd);
		ndel.set_card_distribution(nd);
		del.evaluate();
		ndel.evaluate();
		const edge_type diff_edge = del.overall_edge();
		const edge_type ndiff_edge = ndel.overall_edge();
	cout << "card odds: (A,2,...T)" << endl;
	copy(pd.begin(), pd.end(),
		ostream_iterator<count_type>(cout, "\t"));
	cout << "(+)" << endl;
	copy(nd.begin(), nd.end(),
		ostream_iterator<count_type>(cout, "\t"));
	cout << "(-)" << endl;
		
	del.dump_reveal_edges(cout << "+1: ");
	ndel.dump_reveal_edges(cout << "-1: ");
	cout << "edge sensitivity [card=AT" <<
		", +1/52]: edge=" << diff_edge <<
		"; de/dp=" << (diff_edge -base_edge)/ddel << endl;
	cout << "edge sensitivity [card=AT" <<
		", -1/52]: edge=" << ndiff_edge <<
		"; de/dp=" << (base_edge -ndiff_edge)/ddel << endl;
		cout << endl;
	}
#endif
#endif
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

