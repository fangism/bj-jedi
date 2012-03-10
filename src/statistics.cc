// "statistics.cc"

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <numeric>		// for std::accumulate
#include "statistics.hh"
#include "blackjack.hh"
#if 0
#include "util/string.tcc"	// for string_to_num
#include "util/command.tcc"
#include "util/value_saver.hh"
#endif

namespace blackjack {
#if 0
typedef	util::Command<statistics>			GraderCommand;
}
namespace util {
template class command_registry<blackjack::GraderCommand>;
}
namespace blackjack {
typedef	util::command_registry<GraderCommand>		stats_command_registry;
#endif

using std::endl;
using std::fill;
using std::setw;
using std::accumulate;
using cards::card_name;
using cards::reveal_print_ordering;

#if 0
using util::value_saver;
using util::Command;
using util::CommandStatus;
using util::string_list;
using util::strings::string_to_num;
#endif

//=============================================================================
// clas statistics method declarations

statistics::statistics() :
	initial_bankroll(0.0),
	min_bankroll(0.0),
	max_bankroll(0.0),
	bankroll(0.0),
	initial_bets(0.0),
	total_bets(0.0),
	hands_played(0) {
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
	o << "initial bets: " << initial_bets << endl;
	o << "total bets: " << total_bets << endl;
{
	dealer_reveal_histogram_type vtotals;
	fill(vtotals.begin(), vtotals.end(), 0);
	o << "initial deal histogram: {" << endl;
	o << "\tP\\D\t";
	size_t j = 0;
	for ( ; j<card_values; ++j) {
		const size_t k = reveal_print_ordering[j];
		o << "   " << card_name[k];
//		o << '(' << k << ')';
	}
	o << "\ttotal" << endl;
	for (j=0; j<p_action_states; ++j) {
		o << '\t' << play.player_hit[j].name << '\t';
		const dealer_reveal_histogram_type&
			row(initial_state_histogram[j]);
		size_t i = 0;
		for ( ; i<card_values; ++i) {
			const size_t k = reveal_print_ordering[i];
			o << setw(4) << row[k];
			vtotals[i] += row[k];
		}
		o << '\t' << accumulate(row.begin(), row.end(), 0) << endl;
	}
	o << "\n\ttotal\t";
	for (j=0; j<card_values; ++j) {
		const size_t k = reveal_print_ordering[j];
		o << setw(4) << vtotals[j];
	}
	o << '\t' << accumulate(vtotals.begin(), vtotals.end(), 0) << endl;
	o << '}' << endl;
}
	return o;
}

// TODO: save, load

//=============================================================================
}	// end namespace blackjack

