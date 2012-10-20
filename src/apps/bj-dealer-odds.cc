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
using util::value_saver;
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
	if (dk.card_dist.remove_if_any(d)) {
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
}

/**
	Tuple for comparison and sorting report.
 */
struct composition_dependent_key_type {
	const play_map&				play;	// for state machine
//	card_type				dealer_reveal;
	player_hand_base			player_hand;
	card_type				player_cards[2];

	composition_dependent_key_type(
		const play_map& pm,
//		const card_type d, 
		const player_hand_base& h, 
		const card_type p1, const card_type p2) :
		play(pm),
//		dealer_reveal(d), 
		player_hand(h) {
		player_cards[0] = p1;
		player_cards[1] = p2;
	}

	int
	compare(const composition_dependent_key_type& r) const {
//		int dd = int(dealer_reveal -r.dealer_reveal);
//		if (dd) return dd;
		const int pd = player_hand.compare(r.player_hand);
		if (pd) return pd;
		const int cd1 = int(player_cards[0] -r.player_cards[0]);
		if (cd1) return cd1;
		const int cd2 = int(player_cards[1] -r.player_cards[1]);
		return cd2;
	}

	bool
	operator < (const composition_dependent_key_type& r) const {
		return compare(r) < 0;
	}
};	// end struct composition_dependent_key_type

ostream&
operator << (ostream& o, const composition_dependent_key_type& c) {
	o << c.play.player_hit[c.player_hand.state].name << ':';
	if (c.player_cards[0] != c.player_cards[1] && 
		c.player_cards[1] != cards::ACE) {
		o << card_name[c.player_cards[0]] <<
			card_name[c.player_cards[1]];
	}
	return o;
}

typedef	std::map<composition_dependent_key_type, dealer_final_vector>
				composition_dependent_map_type;

static
void
compute_dealer_odds_dynamic_reveal_3(const variation& var, 
		const perceived_deck_state& pd, 
		const analysis_parameters& ap) {
	const play_map play(var);
	play_map::dealer_final_table_header(cout << "dlr/plr") << endl;
	card_type c = 0;		// TWO, ... TEN, Ace
for ( ; c < card_values; ++c) {
	const card_type d = reveal_print_ordering[c];
	dealer_situation_key_type
		dk(play_map::d_initial_card_map[d], pd);
	if (dk.card_dist.remove_if_any(d)) {
//		dk.card_dist.show_count(cout) << endl;
	// post-peek conditions only
	switch (d) {
	case cards::TEN: if (var.peek_on_10) dk.peek_no_Ace(); break;
	case cards::ACE: if (var.peek_on_Ace) dk.peek_no_10(); break;
	default: break;
	}
	composition_dependent_map_type m;	// for sorting
	card_type p1 = 0;
	for ( ; p1 < card_values; ++p1) {
	card_type p2 = p1;
	const card_type pp1 = reveal_print_ordering[p1];
	for ( ; p2 < card_values; ++p2) {
		const card_type pp2 = reveal_print_ordering[p2];
		const value_saver<perceived_deck_state> dss(dk.card_dist);
		if (dk.card_dist.remove_if_any(pp1)) {
		if (dk.card_dist.remove_if_any(pp2)) {
//		dk.card_dist.show_count_brief(cout);
		player_hand ph(play);
		ph.deal_player(pp1, pp2, true, d);
		const dealer_final_vector&
			dfv(dealer_cache.evaluate_dealer_dynamic(play, dk, ap));
		const composition_dependent_key_type k(play, ph, pp1, pp2);
		const std::pair<composition_dependent_map_type::iterator, bool>
			f(m.insert(composition_dependent_map_type::value_type(
				k, dfv)));
		assert(f.second);	// was uniquely inserted
		}
		}
	}
	}
	composition_dependent_map_type::const_iterator mi(m.begin()), me(m.end());
	for ( ; mi!=me; ++mi) {
		cout << card_name[d] << '/' << mi->first << '\t';
		dump_dealer_final_vector(cout, mi->second, false);
	}
	}
}
}


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
	if (dk.card_dist.remove_if_any(d)) {
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
}

static
void
compute_dealer_odds_exact_reveal_3(const variation& var, 
		const perceived_deck_state& pd, 
		const analysis_parameters& ap) {
	const play_map play(var);
	play_map::dealer_final_table_header(cout << "dlr/plr") << endl;
	card_type c = 0;		// TWO, ... TEN, Ace
for ( ; c < card_values; ++c) {
	const card_type d = reveal_print_ordering[c];
	dealer_situation_key_type
		dk(play_map::d_initial_card_map[d], pd);
	if (dk.card_dist.remove_if_any(d)) {
//		dk.card_dist.show_count(cout) << endl;
	// post-peek conditions only
	switch (d) {
	case cards::TEN: if (var.peek_on_10) dk.peek_no_Ace(); break;
	case cards::ACE: if (var.peek_on_Ace) dk.peek_no_10(); break;
	default: break;
	}
	composition_dependent_map_type m;	// for sorting
	card_type p1 = 0;
	for ( ; p1 < card_values; ++p1) {
	card_type p2 = p1;
	const card_type pp1 = reveal_print_ordering[p1];
	for ( ; p2 < card_values; ++p2) {
		const card_type pp2 = reveal_print_ordering[p2];
		const value_saver<perceived_deck_state> dss(dk.card_dist);
		if (dk.card_dist.remove_if_any(pp1)) {
		if (dk.card_dist.remove_if_any(pp2)) {
//		dk.card_dist.show_count_brief(cout);
		player_hand ph(play);
		ph.deal_player(pp1, pp2, true, d);
		const dealer_final_vector&
			dfv(dealer_cache.evaluate_dealer_exact(play, dk, ap));
		const composition_dependent_key_type k(play, ph, pp1, pp2);
		const std::pair<composition_dependent_map_type::iterator, bool>
			f(m.insert(composition_dependent_map_type::value_type(
				k, dfv)));
		assert(f.second);	// was uniquely inserted
		}
		}
	}
	}
	composition_dependent_map_type::const_iterator mi(m.begin()), me(m.end());
	for ( ; mi!=me; ++mi) {
		cout << card_name[d] << '/' << mi->first << '\t';
		dump_dealer_final_vector(cout, mi->second, false);
	}
	}
}
}



struct options {
	variation				var;
	perceived_deck_state			initial_deck;
	analysis_parameters			analysis_params;
	bool					player_composition;
	size_t					precision;
	bool					help_option;

	options() : var(), initial_deck(var.num_decks), analysis_params(),
		player_composition(false), precision(4), help_option(false) { }
};	// end struct options

static
void
__set_num_decks(options& o, const char* arg) {
	const string args(arg);
	if (args == "h" || args == "half") {
		o.var.num_decks = 1;
		o.initial_deck.initialize_num_decks(o.var.num_decks);
		o.initial_deck.scale_cards(1, 2);
	} else {
		// is a number
		if (string_to_num(args, o.var.num_decks)) {
			cerr << "Invalid number of decks: " << args << endl;
			throw util::getopt_exception(2);
		} else {
			// success: re-initialize deck count
			o.initial_deck.initialize_num_decks(o.var.num_decks);
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
__set_precision(options& o, const char* args) {
	if (string_to_num(args, o.precision)) {
		cerr << "Invalid precision: " << args << endl;
		throw util::getopt_exception(2);
	}
}

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

static
void
__help(options& o) {
	o.help_option = true;
}

static
void
usage(ostream& o, const char* prog) {
o << prog << " [options]\n"
"probability calculator for dealer's final states, given up-card in blackjack\n"
"options: \n"
//	Options (long options not available yet):
"  -h : this help\n"
"game variation:\n"
"  -n <int> : number of decks.\n"	// " h for half-deck\n"
"  -H : dealer hits on soft-17\n"
"  -S : dealer stands on soft-17\n"
"  -o <option=value>: other variation option commands\n"
"	e.g. peek-10=0, peek-ace=0\n"
"calculation:\n"
"  -b : use static standard (infinite) deck approximation\n"
"  -d : use static distribution after card removal\n"
"  -e : use exact distribution after each removed card\n"
"  -c : include effect of player hand composition\n"
"  -p <int> : display precision\n"
// "  -r, --remove : remove the string of cards from deck initially\n"
// "  -a, --add : add the string of cards to deck initially\n"
// "  -v : version\n"
// TODO: control accuracy (via tolerance)
	<< endl;
}

typedef	util::getopt_map<options>	getopt_map_type;

int
main(int argc, char* argv[]) {
	getopt_map_type optmap;
	optmap.add_option('n', &__set_num_decks);
	optmap.add_option('H', &__set_H17);
	optmap.add_option('S', &__set_S17);
	optmap.add_option('b', &__set_basic_calc);
	optmap.add_option('d', &__set_dynamic_calc);
	optmap.add_option('e', &__set_exact_calc);
	optmap.add_option('c', &__set_player_composition);
	optmap.add_option('p', &__set_precision);
	optmap.add_option('o', &__set_variation_command);
	optmap.add_option('h', &__help);
	options opt;
	const int err = optmap(argc, argv, opt);
if (opt.help_option) {
	usage(cout, argv[0]);
	return 0;
}
if (!err) {
	const precision_saver ps1(cout, opt.precision);
	const precision_saver ps2(cerr, opt.precision);
	opt.var.dump(cout) << endl;
	const perceived_deck_state& D(opt.initial_deck);
	D.show_count(cout) << endl;
	cout << "evaluation method: " << opt.analysis_params.mode << endl;
	switch (opt.analysis_params.mode) {
	case ANALYSIS_BASIC:
		compute_dealer_odds_basic_standard(opt.var);
		break;
	case ANALYSIS_DYNAMIC:
		if (opt.player_composition) {
			compute_dealer_odds_dynamic_reveal_3(opt.var, D, 
				opt.analysis_params);
		} else {
			compute_dealer_odds_dynamic_reveal_1(opt.var, D, 
				opt.analysis_params);
		}
		break;
	case ANALYSIS_EXACT:
		if (opt.player_composition) {
			compute_dealer_odds_exact_reveal_3(opt.var, D, 
				opt.analysis_params);
		} else {
			compute_dealer_odds_exact_reveal_1(opt.var, D, 
				opt.analysis_params);
		}
		break;
	default:
		cout << "Unsupported calculation mode." << endl;
	}
}
	return err;
}
