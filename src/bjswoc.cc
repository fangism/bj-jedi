// "bjswoc.cc"
// Blackjack Switch variation odds calculator
// copy-modified from "boc2.cc"

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

/**
	Define to 1 to perform distribution sensitivity analysis.
 */
#define	SENSITIVITY_ANALYSIS			0

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
main(int, char*[]) {
	variation v;
	v.push22 = true;		// dealer pushes on 22
	v.bj_payoff = 1.0;		// payoff 1:1
	v.surrender_penalty = -1.0;	// no surrender
// everything else same
	v.H17 = true;
//	v.H17 = false;
//	v.surrender_late = true;
//	v.one_card_on_split_aces = false;
	strategy S(v);
	S.set_card_distribution(standard_deck);
	S.evaluate();
	S.dump(cout);
	cout << endl;
#if SENSITIVITY_ANALYSIS
// sensitivity analysis to differential probability
	// for a 10% relative increase in frequency
	const edge_type base_edge = S.overall_edge();
	const probability_type pdel = 0.10;
	cout <<
"Sensitivitiy analysis: relative probability increase per card = "
		<< pdel << endl;
	size_t i;		// which card to vary probability
	for (i=0; i<10; ++i) {
		deck pd(standard_deck);
		deck nd(standard_deck);
		pd[i] *= 1 +pdel;
		nd[i] /= 1 +pdel;
		normalize(pd);
		normalize(nd);
		strategy del(v);
		strategy ndel(v);
		del.set_card_distribution(pd);
		ndel.set_card_distribution(nd);
		del.evaluate();
		ndel.evaluate();
		const edge_type diff_edge = del.overall_edge();
		const edge_type ndiff_edge = ndel.overall_edge();
	cout << "------------------ " << strategy::card_name[i]
		<< " ------------------" << endl;
	cout << "card odds: (A,2,...T)" << endl;
	copy(pd.begin(), pd.end(),
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
	}	// end for

	{
	// for ACEs and TENs (common hi-lo)
	const probability_type hdel = pdel/2;
	cout << "------------------ AT ------------------" << endl;
		deck pd(standard_deck);
		deck nd(standard_deck);
		pd[strategy::ACE] *= 1 +hdel;
		pd[strategy::TEN] *= 1 +hdel;
		nd[strategy::ACE] /= 1 +hdel;
		nd[strategy::TEN] /= 1 +hdel;
		normalize(pd);
		normalize(nd);
		strategy del(v);
		strategy ndel(v);
		del.set_card_distribution(pd);
		ndel.set_card_distribution(nd);
		del.evaluate();
		ndel.evaluate();
		const edge_type diff_edge = del.overall_edge();
		const edge_type ndiff_edge = ndel.overall_edge();
	cout << "card odds: (A,2,...T)" << endl;
	copy(pd.begin(), pd.end(),
		ostream_iterator<probability_type>(cout, "\t"));
	cout << "(+)" << endl;
	copy(nd.begin(), nd.end(),
		ostream_iterator<probability_type>(cout, "\t"));
	cout << "(-)" << endl;
		
	del.dump_reveal_edges(cout << "+del%: ");
	ndel.dump_reveal_edges(cout << "-del%: ");
	cout << "edge sensitivity [card=AT" <<
		", +" << hdel << "]: edge=" << diff_edge <<
		"; de/dp=" << (diff_edge -base_edge)/hdel << endl;
	cout << "edge sensitivity [card=AT" <<
		", -" << hdel << "]: edge=" << ndiff_edge <<
		"; de/dp=" << (base_edge -ndiff_edge)/hdel << endl;
		cout << endl;
	}
#endif
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

