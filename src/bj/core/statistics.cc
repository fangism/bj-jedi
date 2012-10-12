// "bj/core/statistics.cc"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>		// for std::accumulate
#include "bj/core/statistics.hh"
#include "bj/core/blackjack.hh"
#include "util/iosfmt_saver.hh"

namespace blackjack {

using std::setw;
using std::endl;
using std::fill;
using std::accumulate;
using cards::card_name;
using cards::reveal_print_ordering;

//=============================================================================
// class statistics method declarations

statistics::statistics() :
	initial_bankroll(0.0),
	min_bankroll(0.0),
	max_bankroll(0.0),
	bankroll(0.0),
	initial_bets(0.0),
	total_bets(0.0),
	hands_played(0),
	decisions_made(0),
	basic_wrong_decisions(0),
	dynamic_wrong_decisions(0),
	basic_priori(),
	dynamic_priori(),
	basic_posteriori(),
	dynamic_posteriori() {
	initial_state_histogram_type::iterator
		i(initial_state_histogram.begin()),
		e(initial_state_histogram.end());
	for ( ; i!=e; ++i) {
		fill(i->begin(), i->end(), 0);
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
statistics::initialize_bankroll(const double b) {
	initial_bankroll = b;
	min_bankroll = b;
	max_bankroll = b;
	bankroll = b;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Add funds, adjust statistics accordingly.
	TODO: If tracking history, adjust that too!
 */
void
statistics::deposit(const double d) {
	initial_bankroll += d;
	min_bankroll += d;
	max_bankroll += d;
	bankroll += d;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Update bankroll watermarks.
 */
void
statistics::compare_bankroll(void) {
	if (bankroll < min_bankroll)
		min_bankroll = bankroll;
	else if (bankroll > max_bankroll)
		max_bankroll = bankroll;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Print all statistics.
 */
ostream&
statistics::dump(ostream& o, const play_map& play) const {
	o << "initial bankroll: " << initial_bankroll << endl;
	o << "min bankroll: " << min_bankroll << endl;
	o << "max bankroll: " << max_bankroll << endl;
	o << "current bankroll: " << bankroll << endl;
	o << "hands played: " << hands_played << endl;
	o << "decisions made: " << decisions_made << endl;
	o << "wrong decisions (basic): " << basic_wrong_decisions << endl;
	o << "wrong decisions (dynamic): " << dynamic_wrong_decisions << endl;
	o << "initial bets: " << initial_bets << endl;
	o << "total bets: " << total_bets << endl;
	o << "Edges before each hand dealt (a priori): {" << endl;
	o << "unweighted, uncounted expected change: " << basic_priori.edge_sum
		<< " (/hand = " << basic_priori.edge_sum/hands_played << ')' << endl;
	o << "weighted, uncounted expected change: " << basic_priori.weighted_edge_sum
		<< " (/hand = " << basic_priori.weighted_edge_sum/hands_played
		<< ')' << endl;
	o << "unweighted, counted expected change: " << dynamic_priori.edge_sum
		<< " (/hand = " << dynamic_priori.edge_sum/hands_played << ')' << endl;
	o << "weighted, counted expected change: " << dynamic_priori.weighted_edge_sum
		<< " (/hand = " << dynamic_priori.weighted_edge_sum/hands_played
		<< ')' << endl;
	o << '}' << endl;
	o << "Edges after each hand dealt (a posteriori): {" << endl;
	o << "unweighted, uncounted expected change: " << basic_posteriori.edge_sum
		<< " (/hand = " << basic_posteriori.edge_sum/hands_played << ')' << endl;
	o << "weighted, uncounted expected change: " << basic_posteriori.weighted_edge_sum
		<< " (/hand = " << basic_posteriori.weighted_edge_sum/hands_played
		<< ')' << endl;
	o << "unweighted, counted expected change: " << dynamic_posteriori.edge_sum
		<< " (/hand = " << dynamic_posteriori.edge_sum/hands_played << ')' << endl;
	o << "weighted, counted expected change: " << dynamic_posteriori.weighted_edge_sum
		<< " (/hand = " << dynamic_posteriori.weighted_edge_sum/hands_played
		<< ')' << endl;
	o << '}' << endl;
{
	dealer_reveal_histogram_type vtotals;
	fill(vtotals.begin(), vtotals.end(), 0);
	o << "initial deal histogram: {" << endl;
	o << "\tP\\D\t";
	card_type j = 0;
	for ( ; j<card_values; ++j) {
		const card_type k = reveal_print_ordering[j];
		o << "   " << card_name[k];
//		o << '(' << k << ')';
	}
	const util::width_saver ws(o, 4);
	o << "\ttotal" << endl;
	for (j=0; j<p_action_states; ++j) {
		o << '\t' << play.player_hit[j].name << '\t';
		const dealer_reveal_histogram_type&
			row(initial_state_histogram[j]);
		card_type i = 0;
		for ( ; i<card_values; ++i) {
			const card_type k = reveal_print_ordering[i];
			o << setw(4) << row[k];
			vtotals[k] += row[k];
		}
		o << '\t' << accumulate(row.begin(), row.end(), 0) << endl;
	}
	o << "\n\ttotal\t";
	for (j=0; j<card_values; ++j) {
		const card_type k = reveal_print_ordering[j];
		o << setw(4) << vtotals[k];
	}
	o << '\t' << accumulate(vtotals.begin(), vtotals.end(), 0) << endl;
	o << '}' << endl;
}
	return o;
}

// TODO: save, load

//=============================================================================
}	// end namespace blackjack

