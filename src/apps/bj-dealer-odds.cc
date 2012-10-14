// "bj-dealer-odds.cc"
// computes the spread of the dealer's final states using different methods

#include <iostream>
#include "bj/core/blackjack.hh"
#include "bj/core/analysis.hh"
#include "util/iosfmt_saver.hh"
#include "util/value_saver.hh"
#include "util/getopt_mapped.tcc"
#include "util/string.tcc"
#include "util/tokenize.hh"

using std::cout;
using std::cerr;
using std::endl;
using cards::card_name;
using cards::reveal_print_ordering;
using util::precision_saver;
using util::strings::string_to_num;
using namespace blackjack;

static
dealer_outcome_cache_set
dealer_cache;

// number of decks doesn't matter
static
void
compute_dealer_odds_basic_standard(const variation& var) {
	const play_map play(var);
	play_map::dealer_final_table_header(cout << "dealer") << endl;
	// peek vs. no-peek doesn't matter b/c infinite deck assumption
	card_type c = 0;		// TWO, ... TEN, Ace
	for ( ; c < card_values; ++c) {
		const card_type d = reveal_print_ordering[c];
		dealer_hand_base dealer(play_map::d_initial_card_map[d]);
		switch (d) {
		case cards::TEN: if (var.peek_on_10) dealer.peek_no_Ace(); break;
		case cards::ACE: if (var.peek_on_Ace) dealer.peek_no_10(); break;
		default: break;
		}
		const dealer_final_vector&
			dfv(dealer_cache.evaluate_dealer_basic(play, dealer));
		cout << card_name[d] << '\t';
		dump_dealer_final_vector(cout, dfv, false);
	}
}

// number of decks matter here, use post-peek conditions
// only based on dealer's reveal card
static
void
compute_dealer_odds_dynamic_reveal_1(const variation& var, 
		const perceived_deck_state& pd, 
		const analysis_parameters& ap) {
	const play_map play(var);
	play_map::dealer_final_table_header(cout << "dealer") << endl;
	card_type c = 0;		// TWO, ... TEN, Ace
	for ( ; c < card_values; ++c) {
		const card_type d = reveal_print_ordering[c];
		dealer_situation_key_type
			dk(play_map::d_initial_card_map[d], pd);
		dk.card_dist.remove(d);
//		dk.card_dist.show_count(cout) << endl;
		// post-peek conditions only
		switch (d) {
		case cards::TEN: if (var.peek_on_10) dk.peek_no_Ace(); break;
		case cards::ACE: if (var.peek_on_Ace) dk.peek_no_10(); break;
		default: break;
		}
		const dealer_final_vector&
			dfv(dealer_cache.evaluate_dealer_dynamic(play, dk, ap));
		cout << card_name[d] << '\t';
		dump_dealer_final_vector(cout, dfv, false);
	}
}

#if 0
// based on dealer's reveal card and player's cards
static
void
compute_dealer_odds_dynamic_reveal_3(const variation& var) {
}
#endif

// number of decks matter here
static
void
compute_dealer_odds_exact_reveal_1(const variation& var,
		const perceived_deck_state& pd, 
		const analysis_parameters& ap) {
	const play_map play(var);
	play_map::dealer_final_table_header(cout << "dealer") << endl;
	card_type c = 0;		// TWO, ... TEN, Ace
	for ( ; c < card_values; ++c) {
		const card_type d = reveal_print_ordering[c];
		dealer_situation_key_type
			dk(play_map::d_initial_card_map[d], pd);
		dk.card_dist.remove(d);
//		dk.card_dist.show_count(cout) << endl;
		// post-peek conditions only
		switch (d) {
		case cards::TEN: if (var.peek_on_10) dk.peek_no_Ace(); break;
		case cards::ACE: if (var.peek_on_Ace) dk.peek_no_10(); break;
		default: break;
		}
		const dealer_final_vector&
			dfv(dealer_cache.evaluate_dealer_exact(play, dk, ap));
		cout << card_name[d] << '\t';
		dump_dealer_final_vector(cout, dfv, false);
	}
}

#if 0
static
void
compute_dealer_odds_exact_reveal_3(const variation& var) {
}
#endif

struct options {
	variation				var;
	analysis_parameters			analysis_params;
	bool					player_composition;

	options() : var(), analysis_params(), player_composition(false) { }
};	// end struct options

static
void
__set_num_decks(options& o, const char* arg) {
	const string args(arg);
	if (args == "h" || args == "half") {
		cerr << "TODO: support half-deck blackjack." << endl;
		throw util::getopt_exception(2);
	} else {
		// is a number
		if (string_to_num(args, o.var.num_decks)) {
			cerr << "Invalid number of decks: " << args << endl;
			throw util::getopt_exception(2);
		}
	}
}

static
void
__set_H17(options& o) { o.var.H17 = true; }

static
void
__set_S17(options& o) { o.var.H17 = false; }

static
void
__set_basic_calc(options& o) { o.analysis_params.mode = ANALYSIS_BASIC; }

static
void
__set_dynamic_calc(options& o) { o.analysis_params.mode = ANALYSIS_DYNAMIC; }

static
void
__set_exact_calc(options& o) { o.analysis_params.mode = ANALYSIS_EXACT; }

static
void
__set_player_composition(options& o) { o.player_composition = true; }

static
void
__set_variation_command(options& o, const char* arg) {
	util::string_list toks;
	util::tokenize_char(arg, toks, '=');
	if (o.var.command(toks)) {
		cerr << "Malformed variation command: " << arg << endl;
		throw util::getopt_exception(2);
	}
}

typedef	util::getopt_map<options>	getopt_map_type;

/**
	Options (long options not available yet):
	-n, --num-decks <int> : number of decks.  h for half-deck
	-H, --H17 : dealer hits on soft-17
	-S, --S17 : dealer stands on soft-17
	-p, --player-composition : include effect of player hand composition
	-o <option=value>: other variation option command
		can access options like peek-10, peek-ace
	-b, --basic : use static standard (infinite) deck approximation
	-d, --dynamic : use static distribution after card removal
	-e, --exact : use exact distribution after each removed card
	-r, --remove : remove the string of cards from deck initially
	-a, --add : add the string of cards to deck initially
	-h : help
	-v : version
TODO: control accuracy and precision
 */
int
main(int argc, char* argv[]) {
	getopt_map_type optmap;
	optmap.add_option('n', &__set_num_decks);
	optmap.add_option('H', &__set_H17);
	optmap.add_option('S', &__set_S17);
	optmap.add_option('b', &__set_basic_calc);
	optmap.add_option('d', &__set_dynamic_calc);
	optmap.add_option('e', &__set_exact_calc);
	optmap.add_option('p', &__set_player_composition);
	optmap.add_option('o', &__set_variation_command);
	options opt;
	const int err = optmap(argc, argv, opt);
if (!err) {
	const precision_saver ps1(cout, 4);
	const precision_saver ps2(cerr, 4);
	opt.var.dump(cout) << endl;
	cout << "evaluation method: " << opt.analysis_params.mode << endl;
	perceived_deck_state D(opt.var.num_decks);
//	D.show_count(cout) << endl;
	switch (opt.analysis_params.mode) {
	case ANALYSIS_BASIC:
		compute_dealer_odds_basic_standard(opt.var);
		break;
	case ANALYSIS_DYNAMIC:
		compute_dealer_odds_dynamic_reveal_1(opt.var, D, 
			opt.analysis_params);
// TODO: player_composition
		break;
	case ANALYSIS_EXACT:
		compute_dealer_odds_exact_reveal_1(opt.var, D, 
			opt.analysis_params);
// TODO: player_composition
		break;
	default:
		cout << "Unsupported calculation mode." << endl;
	}
}
	return err;
}
