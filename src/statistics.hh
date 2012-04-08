// "statistics.hh"

#ifndef	__BOC2_STATISTICS_HH__
#define	__BOC2_STATISTICS_HH__

#include <iosfwd>
#include "enums.hh"
#include "util/array.hh"
#include "deck.hh"

namespace blackjack {
using std::istream;
using std::ostream;
using util::array;
using cards::edge_type;
class play_map;

/**
	Unweighted and bet-weighted edges.
 */
struct edge_group {
	/// expected outcome with only unit-bets
	edge_type			edge_sum;
	/// expected outcome, weighted by bets
	edge_type			weighted_edge_sum;

	edge_group() : edge_sum(0.0), weighted_edge_sum(0.0) { }

};	// end struct edge group


/**
	Tracks cumulative statics about gameplay.
 */
struct statistics {
	double				initial_bankroll;
	double				min_bankroll;
	double				max_bankroll;
	double				bankroll;
	/// total amount of money risked on initial hands
	double				initial_bets;
	/// total amount of money risked, including splits/doubles
	double				total_bets;
	/// total count of initial deals (not counting splits)
	size_t				hands_played;
	/// total number of prompted decisions
	size_t				decisions_made;
	size_t				basic_wrong_decisions;
	size_t				dynamic_wrong_decisions;

	// uncounted, prior to dealing hand
	edge_group			basic_priori;
	// counted, prior to dealing hand
	edge_group			dynamic_priori;
	// uncounted, after to dealing hand
	edge_group			basic_posteriori;
	// counted, after to dealing hand
	edge_group			dynamic_posteriori;

	// won hands, lost hands, won doubles, lost doubles

	// spread of initial hands vs. dealer-reveal (matrix)
	// initial edges -- computed on first deal (pre-peek)
	typedef array<size_t, card_values>	
					dealer_reveal_histogram_type;
	typedef	array<dealer_reveal_histogram_type, p_action_states>
					initial_state_histogram_type;
	initial_state_histogram_type	initial_state_histogram;
	// TODO: histogram of won/lost/pushed hands, situations?

	// decision edges -- updated every decision
	// optimal edges
	// edge margins
	// basic vs. dynamic
	// bet distribution (map)
	// expectation
	// lucky draws
	// bad beats
	// dealer initial->final stats (including bust, BJ)
	// player initial->final stats (including bust)
	// min true count, max true count

	statistics();			// initial value

	void
	initialize_bankroll(const double);

	void
	compare_bankroll(void);

	bool
	broke(void) const {
		return bankroll <= 0.0;
	}

	void
	deposit(const double);

	void
	register_bet(const double);

	ostream&
	dump(ostream&, const play_map&) const;

	void
	save(ostream&) const;

	void
	load(istream&);

};	// end struct statistics

}	// end namespace blackjack

#endif	// __BOC2_STATISTICS_HH__

