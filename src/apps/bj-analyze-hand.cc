// "bj-analyze-hand.cc"
// computes the player's edge in any situtation

#include <iostream>
#include "bj/core/blackjack.hh"
#include "bj/core/analysis.hh"
#include "util/iosfmt_saver.hh"
#include "util/value_saver.hh"
#include "util/getopt_mapped.tcc"
#include "util/string.tcc"
#include "util/tokenize.hh"

#define	ENABLE_STACKTRACE			0
#include "util/stacktrace.hh"

using std::cout;
using std::cerr;
using std::endl;
using cards::card_name;
using cards::card_index;
using cards::reveal_print_ordering;
using cards::INVALID_CARD;
using util::precision_saver;
using util::strings::string_to_num;
using util::value_saver;
using namespace blackjack;

static
basic_strategy_analyzer
basic_player_cache;

#if 0
/**
	Evaluate the player's edges for each possible decision
	for a given situation, using only basic strategy, 
	based on an infinite deck approximation.
	Number of decks doesn't matter.
 */
static
void
analyze_player_hand_basic_standard(const variation& var) {
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

/**
	Compute spread of dealer final states using infinite deck
	with modified distribution, based only on the revealed up-card.
	Cards are drawn with replacement.
 */
static
void
analyze_player_hand_dynamic_reveal_1(const variation& var, 
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
	Computes dealer's final state spread exactly.
	Decks are finite.  Cards are drawn without replacement.
 */
static
void
analyze_player_hand_exact_reveal_1(const variation& var,
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
#endif

/**
	Various options for this program.
 */
struct options {
	variation				var;
	play_map				play;
	// includes perceived_deck_state for initial deck
	player_situation_key_type		key;
	string					hand;
	card_type				upcard;
	analysis_parameters			analysis_params;
	size_t					precision;
	bool					help_option;

	options() : var(), play(var), key(), hand(), upcard(INVALID_CARD),
		analysis_params(), precision(4), help_option(false) {
		key.card_dist.initialize_num_decks(var.num_decks);
	}

	bool
	initialize_player_situation(void);

	/**
		\param c must be a valid card index
		\pre this is first time being called.
		\return true on error
	 */
	bool
	set_dealer_upcard(const card_type c) {
		if (upcard != card_type(INVALID_CARD)) {
			cerr << "Dealer's upcard may only be set once."
				<< endl;
			return true;
		}
		if (key.card_dist.remove_if_any(c)) {
			upcard = cards::card_value_map[c];
			key.dealer.set_upcard(upcard);
		} else {
			cerr << "No more " << card_name[c] <<
				" cards in deck to choose from."  << endl;
			return true;
		}
		return false;
	}

	ostream&
	dump(ostream& o) const {
		var.dump(o) << endl;
		key.dump(o, play);
		o << "evaluation method: " << analysis_params.mode << endl;
		return o;
	}

};	// end struct options

/**
	Assign player initial state, based on cards.
	\pre hand contains at least two cards.
	\return true if there's an error.
 */
bool
options::initialize_player_situation(void) {
	STACKTRACE_VERBOSE;
	play.reset();
	if (hand.length() < 2) {
		cerr << "Player's hand requires at least 2 cards." << endl;
		return true;
	}
	perceived_deck_state& deck(key.card_dist);
	const card_type p1 = card_index(hand[0]);
	const card_type p2 = card_index(hand[1]);
	if (p1 == card_type(INVALID_CARD)) {
		cerr << "Invalid card: " << hand[0] << endl;
		return true;
	}
	if (p2 == card_type(INVALID_CARD)) {
		cerr << "Invalid card: " << hand[1] << endl;
		return true;
	}
	if (!deck.remove_if_any(p1)) {
		cerr << "Unable to draw card " << hand[0]
			<< " from deck." << endl;
		return true;
	}
	if (!deck.remove_if_any(p2)) {
		cerr << "Unable to draw card " << hand[1]
			<< " from deck." << endl;
		return true;
	}
	player_hand p(play);
	STACKTRACE_INDENT_PRINT("before deal_player()" << endl);
	p.deal_player(p1, p2, true, upcard);
	STACKTRACE_INDENT_PRINT("after deal_player()" << endl);
	string::const_iterator hi(hand.begin() +2), he(hand.end());
	for ( ; hi != he; ++hi) {
		const card_type n = card_index(*hi);
		if (n == card_type(INVALID_CARD)) {
			cerr << "Invalid card: " << *hi << endl;
			return true;
		}
		if (!deck.remove_if_any(n)) {
			cerr << "Unable to draw card " << *hi
				<< " from deck." << endl;
			return true;
		}
		p.hit_player(n);
		// each hit will update player's state
		// including set of permissible actions
	}
	key.player.hand = p;
	key.player.splits.initialize(var.max_split_hands, 
		play.is_player_pair(p.state));
	return false;
}

// option handlers
static
void
__set_num_decks(options& o, const char* arg) {
	const string args(arg);
	if (args == "h" || args == "half") {
		o.var.num_decks = 1;
		o.key.card_dist.initialize_num_decks(o.var.num_decks);
		o.key.card_dist.scale_cards(1, 2);
	} else {
		// is a number
		if (string_to_num(args, o.var.num_decks)) {
			cerr << "Invalid number of decks: " << args << endl;
			throw util::getopt_exception(2);
		} else {
			// success: re-initialize deck count
			o.key.card_dist.initialize_num_decks(o.var.num_decks);
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

/**
	This should only be called after number of decks is established.
	\param arg is a string of cards to add.
 */
static
void
__add_cards(options& o, const char* arg) {
	const string args(arg);
	string::const_iterator i(args.begin()), e(args.end());
	for (; i!=e; ++i) {
		const card_type c(cards::card_index(*i));
		if (c != card_type(INVALID_CARD)) {
			o.key.card_dist.add(c, 1);
		} else {
			cerr << "Ignoring invalid card: " << *i << endl;
		}
	}
}

/**
	This should only be called after number of decks is established.
	\param arg is a string of cards to remove.
 */
static
void
__remove_cards(options& o, const char* arg) {
	const string args(arg);
	string::const_iterator i(args.begin()), e(args.end());
	for (; i!=e; ++i) {
		const card_type c(cards::card_index(*i));
		if (c != card_type(INVALID_CARD)) {
			o.key.card_dist.remove_if_any(c);
		} else {
			cerr << "Ignoring invalid card: " << *i << endl;
		}
	}
}

/**
	Sets the dealer's up-card, and removes it from the deck.
 */
static
void
__set_dealer_upcard(options& o, const char* arg) {
	const string args(arg);
	if (args.length() == 1) {
		const card_type c = card_index(args[0]);
		// check for valid card!
		if (c == card_type(INVALID_CARD)) {
			cerr << "Invalid card: " << args[0] << endl;
			throw util::getopt_exception(2);
		}
		if (o.set_dealer_upcard(c)) {
			throw util::getopt_exception(2);
		}
	} else {
		cerr << "Invalid card argument: " << args << endl;
		throw util::getopt_exception(2);
	}
}

/**
	Sets the player's hand, and removes the cards in her hand from deck.
 */
static
void
__set_player_hand(options& o, const char* arg) {
	if (o.hand.length()) {
		cerr << "Player's hand can only be set once." << endl;
		throw util::getopt_exception(2);
	}
	const string args(arg);
	if (args.length() < 2) {
		cerr << "Player's hand requires at least 2 cards." << endl;
		throw util::getopt_exception(2);
	} else {
		o.hand = arg;
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
"Blackjack player hand analyzer and edge calculator\n"
"options: \n"
//	Options (long options not available yet):
"  -h : this help\n"
"game variation:\n"
"  -H : dealer hits on soft-17\n"
"  -S : dealer stands on soft-17\n"
"  -o <option=value>: other variation option commands\n"
"\tVariation Options:\n";
	blackjack::variation::option_help(o);
o <<
"hand description (required):\n"
"  -P <cards> : player's cards (at least 2)\n"
"  -U <card> : dealer's revealed up-card\n"
"deck modification:\n"
"  -n <int> : number of decks.\n"	// " h for half-deck\n"
"  -r <cards> : remove the string of cards from deck initially (after -n)\n"
"  -a <cards> : add the string of cards to deck initially (after -n)\n"
"calculation:\n"
"  -b : use static standard (infinite) deck approximation\n"
"  -d : use static distribution after card removal\n"
"  -e : use exact distribution after each removed card\n"
"  -p <int> : display precision\n"
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
	optmap.add_option('P', &__set_player_hand);
	optmap.add_option('U', &__set_dealer_upcard);
	optmap.add_option('b', &__set_basic_calc);
	optmap.add_option('d', &__set_dynamic_calc);
	optmap.add_option('e', &__set_exact_calc);
	optmap.add_option('p', &__set_precision);
	optmap.add_option('o', &__set_variation_command);
	optmap.add_option('h', &__help);
	optmap.add_option('r', &__remove_cards);
	optmap.add_option('a', &__add_cards);
	options opt;
	const int err = optmap(argc, argv, opt);
if (opt.help_option) {
	usage(cout, argv[0]);
	return 0;
}
	if (opt.upcard == card_type(INVALID_CARD)) {
		cerr << "Dealer's upcard must be set with -U." << endl;
		return 1;
	}
	if (!opt.hand.length()) {
		cerr << "Player's hand must be set with -P." << endl;
		return 1;
	}
	if (opt.initialize_player_situation()) {
		// already have error message
		return 1;
	}
if (!err) {
	const precision_saver ps1(cout, opt.precision);
	const precision_saver ps2(cerr, opt.precision);
	opt.key.dump(cout, opt.play);		// show the complete situation
	switch (opt.analysis_params.mode) {
	case ANALYSIS_BASIC: {
		const player_situation_basic_key_type s(opt.key);
		const expectations&
			e(basic_player_cache.evaluate_player_basic(opt.play, s));
		e.dump_choice_actions(cout, -opt.var.surrender_penalty);
		break;
	}
#if 0
	case ANALYSIS_DYNAMIC:
		analyze_player_hand_dynamic_reveal_1(opt.var, D, 
			opt.analysis_params);
		break;
	case ANALYSIS_EXACT:
		analyze_player_hand_exact_reveal_1(opt.var, D, 
			opt.analysis_params);
		break;
#endif
	default:
		cout << "Unsupported calculation mode." << endl;
	}
}
	return err;
}
