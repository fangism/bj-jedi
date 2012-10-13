// "bj-dealer-odds.cc"
// computes the spread of the dealer's final states using different methods

#include <iostream>
#include "bj/core/blackjack.hh"
#include "bj/core/analysis.hh"
#include "util/iosfmt_saver.hh"

using std::cout;
using std::endl;
using cards::card_name;
using cards::reveal_print_ordering;
using namespace blackjack;
using util::precision_saver;

static
dealer_outcome_cache_set
dealer_cache;

// number of decks doesn't matter
static
void
compute_dealer_odds_basic_standard(const variation& var) {
	const precision_saver ps(cout, 4);
	const play_map play(var);
	play_map::dealer_final_table_header(cout << "dealer") << endl;
	// peek vs. no-peek doesn't matter b/c infinite deck assumption
	card_type c = 0;		// TWO, ... TEN, Ace
	for ( ; c < card_values; ++c) {
		const card_type d = reveal_print_ordering[c];
		dealer_hand_base dealer(play_map::d_initial_card_map[d]);
		const dealer_final_vector&
			dfv(dealer_cache.evaluate_dealer_basic(play, dealer));
		cout << card_name[d] << '\t';
		dump_dealer_final_vector(cout, dfv, false);
	}
}

#if 0
// number of decks matter here, use post-peek conditions
// only based on dealer's reveal card
static
void
compute_dealer_odds_dynamic_reveal_1(const variation& var) {
}

// based on dealer's reveal card and player's cards
static
void
compute_dealer_odds_dynamic_reveal_3(const variation& var) {
}

// number of decks matter here
static
void
compute_dealer_odds_exact_reveal_1(const variation& var) {
}

static
void
compute_dealer_odds_exact_reveal_3(const variation& var) {
}
#endif

/**
	Options:
	--num-decks, -n <int>
	--H17
	--S17
	--basic
	--dynamic
	--exact
 */
int
main(int, char*[]) {
	variation var;
	compute_dealer_odds_basic_standard(var);
	return 0;
}
