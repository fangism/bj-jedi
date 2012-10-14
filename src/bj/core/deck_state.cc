// "bj/core/deck_state.cc"

#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>			// for copy, fill
#include <functional>			// for std::multiplies
#include <numeric>			// for accumulate

#include "bj/core/deck_state.hh"
#include "bj/core/deck.hh"		// for standard_deck_count
#include "bj/core/variation.hh"

#include "util/array.tcc"
#include "util/probability.tcc"
#include "util/value_saver.hh"
#include "util/string.tcc"		// for string_to_num
#include "util/tokenize.hh"
#include "util/iosfmt_saver.hh"

#define	ENABLE_STACKTRACE			0
#include "util/stacktrace.hh"

namespace blackjack {
using std::string;
using std::cerr;
using std::endl;
using std::setw;
using std::fill;
using std::copy;
using std::multiplies;
using std::transform;
using util::string_list;
using util::tokenize;
using util::strings::string_to_num;
using util::value_saver;
using cards::ACE;
using cards::TEN;
using cards::JACK;
using cards::QUEEN;
using cards::KING;
using cards::card_symbols;
using cards::extended_reveal_print_ordering;
using cards::deck_count_type;
using cards::card_values;
using cards::card_name;
using cards::card_value_map;
using cards::standard_deck_count;
using cards::standard_deck_distribution;
using cards::reveal_print_ordering;
using cards::card_index;

/**
	Power function, as iterative integer multiply.
	Beware of overflow?
	Complexity: O(lg p)
	TODO: efficient implementation using bitfield-powers?
 */
static
inline
size_t
zpow(const size_t b, size_t p) {
#if 0
	if (p) {
		size_t ret = b;
		for (--p; p; --p) {
			ret *= b;
		}
		return ret;
	} else {
		return 1;
	}
#else
	// composite function algorithm
	size_t ret = 1;		// multiplicative identity
	size_t factor = b;
	for ( ; p; p >>= 1, factor *= factor) {
		if (p & 1) {
			ret *= factor;
		}
	}
	return ret;
#endif
}

//=============================================================================
// class perceived_deck_state method definitions

perceived_deck_state::perceived_deck_state() : remaining(count_type(0)),
		peeked_not_10s(0), peeked_not_Aces(0), remaining_total(0) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
perceived_deck_state::perceived_deck_state(const size_t n) : remaining(), 
		peeked_not_10s(0), peeked_not_Aces(0), remaining_total(n *52) {
	transform(standard_deck_count.begin(), standard_deck_count.end(), 
		remaining.begin(), std::bind2nd(multiplies<count_type>(), n));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
perceived_deck_state::perceived_deck_state(const perceived_deck_state& p,
		const card_type r) :
		remaining(p.remaining),
		peeked_not_10s(p.peeked_not_10s),
		peeked_not_Aces(p.peeked_not_Aces),
		remaining_total(p.remaining_total) {
	remove(r);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
perceived_deck_state::initialize(const extended_deck_count_type& d) {
	cards::simplify_deck_count(d, remaining);
	peeked_not_10s = 0;
	peeked_not_Aces = 0;
	remaining_total =
		std::accumulate(remaining.begin(), remaining.end(), 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Decrements count of card.
	Only known cards are counted.
 */
void
perceived_deck_state::remove(const card_type c) {
	card_type& r(remaining[card_value_map[c]]);
	assert(r);
	--r;
	assert(remaining_total);
	--remaining_total;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Decrements count of card.
	Only known cards are counted.
 */
void
perceived_deck_state::remove_if_any(const card_type c) {
	card_type& r(remaining[card_value_map[c]]);
if (r) {
	--r;
	assert(remaining_total);
	--remaining_total;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Decrements count of card.
	Only known cards are counted.
 */
void
perceived_deck_state::remove_all(const card_type c) {
	card_type& r(remaining[card_value_map[c]]);
	assert(remaining_total);
	remaining_total -= r;
	r = 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
perceived_deck_state::reveal_peek_10(const card_type c) {
	--peeked_not_10s;
//	const card_type d = card_value_map[c];
	remove(c);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
perceived_deck_state::reveal_peek_Ace(const card_type c) {
	--peeked_not_Aces;
//	const card_type d = card_value_map[c];
	remove(c);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param p the previous peek state (which should be updated after reveal)
 */
void
perceived_deck_state::reveal(const peek_state_enum p, const card_type c) {
switch (p) {
case PEEKED_NO_ACE:
	--peeked_not_Aces;
	break;
case PEEKED_NO_10:
	--peeked_not_10s;
	break;
default: break;
}
	remove(c);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Takes an extended deck count and re-weights the count
	according to the perceived distribution.  
	See math paper in text/.
	TODO: compute four power terms more efficiently w/ valarray,
	vector operations, or the like.
 */
void
perceived_deck_state::distribution_weight_adjustment(deck_count_type& d) const {
	STACKTRACE_BRIEF;
	// computing integer power, just multiply
	// unlikely to have high powers
	const count_type p_nA = remaining_total -remaining[ACE];
	const count_type p_n10 = remaining_total -remaining[TEN];
	const count_type p_nA_1 = p_nA -1;
	const count_type p_n10_1 = p_n10 -1;
	const count_type w_10 = zpow(p_n10, peeked_not_10s);
	const count_type w_10_1 = zpow(p_n10_1, peeked_not_10s);
	const count_type w_A = zpow(p_nA, peeked_not_10s);
	const count_type w_A_1 = zpow(p_nA_1, peeked_not_10s);
	const count_type f_x = w_A_1 *w_10_1;
	const count_type f_10 = w_A_1 *w_10;
	const count_type f_A = w_A *w_10_1;
	// populate with weights
	// loop bounds depend on card enum in deck.hh
	d[ACE] = f_A;
	fill(&d[ACE+1], &d[TEN], f_x);
	d[TEN] = f_10;
	// element multiply
	transform(d.begin(), d.end(), remaining.begin(), d.begin(), 
		multiplies<count_type>());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Copy the 10 weight to J,Q,K.
 */
void
perceived_deck_state::distribution_weight_adjustment(
		extended_deck_count_type& d) const {
	deck_count_type t;
	distribution_weight_adjustment(t);
	// this assumes a particular enumeration ordering
	copy(t.begin(), t.end(), d.begin());
	fill(&d[JACK], &d[KING+1], d[TEN]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Comparison for sorting and ordering.
 */
int
perceived_deck_state::compare(const perceived_deck_state& r) const {
	if (remaining_total < r.remaining_total)
		return -1;
	if (r.remaining_total < remaining_total)
		return 1;
	const int c = remaining.compare(r.remaining);
	if (c) return c;
	if (peeked_not_10s < r.peeked_not_10s)
		return -1;
	if (r.peeked_not_10s < peeked_not_10s)
		return 1;
	if (peeked_not_Aces < r.peeked_not_Aces)
		return -1;
	if (r.peeked_not_Aces < peeked_not_Aces)
		return 1;
	// then all members are equal
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
perceived_deck_state::operator < (const perceived_deck_state& r) const {
	return compare(r) < 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Print one-liner
 */
ostream&
perceived_deck_state::show_count_brief(ostream& o) const {
	card_type i = 0;
	for (; i<card_values; ++i) {
		const card_type j = reveal_print_ordering[i];
		o << ' ' << card_name[j] << ':' << remaining[j];
	}
	o << " !10:" << peeked_not_10s;
	o << " !A:" << peeked_not_Aces;
	o << " tot:" << actual_remaining() << endl;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
perceived_deck_state::show_count(ostream& o) const {
	o << "perceived count:" << endl;
	card_type i = 0;
	o << "card:\t";
	for (; i<card_values; ++i) {
		const card_type j = reveal_print_ordering[i];
		o << "   " << card_name[j];
	}
	o << "   !10  !A\ttotal\n";
	o << "rem:\t";
	for (i=0; i<card_values; ++i) {
		const card_type j = reveal_print_ordering[i];
		o << setw(4) << remaining[j];
	}
	o << "  " << setw(4) << peeked_not_10s;
	o << setw(4) << peeked_not_Aces;
	o << '\t' << actual_remaining() << endl;
	return o;
}

//=============================================================================
// class deck_state method definitions

/**
	Initializes to default.
 */
deck_state::deck_state(const variation& v) : 
		num_decks(v.num_decks),
		hole_reserved(false) {
	reshuffle();		// does most of the initializing
	// default penetration before reshuffling: 75%
	maximum_penetration = (1.0 -v.maximum_penetration) * num_decks *52;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::reshuffle(void) {
	// ACE is at index 0
	cards_remaining = num_decks*52;
	cards_spent = 0;
	const count_type f = num_decks*4;
	std::fill(used_cards.begin(), used_cards.end(), 0);
	std::fill(cards.begin(), cards.end(), f);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
deck_state::needs_shuffle(void) const {
	return (cards_remaining < maximum_penetration);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return true if deck/shoe was shuffled.
 */
bool
deck_state::reshuffle_auto(void) {
	if (needs_shuffle()) {
		reshuffle();
		return true;
	}
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double
deck_state::draw_ten_probability(void) const {
	return double(cards[TEN] +cards[JACK] +cards[QUEEN] +cards[KING])
		/ double(cards_remaining);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Draws the user-determined card, useful for setting up
	and querying specific scenarios.
	Counts specified card as drawn from deck.
 */
void
deck_state::magic_draw(const card_type r) {
	// TODO: safe-guard against drawing non-existent
	if (!cards[r]) {
		show_count(cerr, true, true);
		cerr << "FATAL: attempt to draw card " << card_name[r] << endl;
	}
	assert(cards[r]);
	++used_cards[r];
	--cards[r];
	++cards_spent;
	--cards_remaining;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Removes one card from remaining deck at random.
	Does NOT re-compute probabilities.
 */
card_type
deck_state::quick_draw(void) {
	const card_type ret = quick_draw_uncounted();
	magic_draw(ret);
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Peeks at one card from remaining deck at random.
	Does NOT re-compute real-valued probabilities; just uses integers.
	Accounts for the fact that a hole card was drawn.
 */
card_type
deck_state::quick_draw_uncounted(void) {
if (hole_reserved) {
	count_type& count(cards[hole_card]);
	const value_saver<count_type> __tmp(count);	// save value
	assert(count);
	--count;		// the actual count (unknown to player)
	return util::random_draw_from_integer_pdf(cards);
} else {
	return util::random_draw_from_integer_pdf(cards);
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Zeros out the card indexed i, for the purposes of 
	random drawing, knowing that the card cannot be 
	of a particular value.  
	This is useful after peeking for blackjack.  
	\return index of a random card.
 */
card_type
deck_state::quick_draw_uncounted_except(const card_type i) {
	const value_saver<count_type> __tmp(cards[i], 0);
	return quick_draw_uncounted();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Gives user to choose her own card.
	Useful for specific scenario testing.
	\param m if true, prompt user to choose a specific card.
	\param s if true, show result of random draw.
 */
card_type
deck_state::option_draw_uncounted(const bool m, istream& i, ostream& o,
		const bool s) {
if (m) {
	string line;
	do {
		o << "card? ";
		i >> line;
	} while (line.empty() && i);
	const card_type c = card_index(line[0]);
	if (c != card_type(-1)) {
		assert(c < cards.size());
		assert(cards[c]);		// count check
		return c;
	} else {
		const card_type ret = quick_draw_uncounted();
		o << "drawing randomly... ";
		if (s) {
			o << card_name[ret];
		}
		o << endl;
		return ret;
	}
} else {
	return quick_draw_uncounted();
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
card_type
deck_state::option_draw(const bool m, istream& i, ostream& o) {
	const card_type ret = option_draw_uncounted(m, i, o, true);
	magic_draw(ret);
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Removes one card from remaining deck at random.
 */
card_type
deck_state::draw(void) {
	const card_type ret = quick_draw();
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::draw_hole_card(void) {
	assert(!hole_reserved);
	hole_card = quick_draw_uncounted();
	hole_reserved = true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::option_draw_hole_card(const bool m, istream& i, ostream& o) {
	assert(!hole_reserved);
	hole_card = option_draw_uncounted(m, i, o, false);
	hole_reserved = true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::replace_hole_card(void) {
	assert(hole_reserved);
	hole_reserved = false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This should only be called once per hole card draw.
 */
card_type
deck_state::reveal_hole_card(void) {
	assert(hole_reserved);
	hole_reserved = false;
	magic_draw(hole_card);
	return hole_card;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param ex if extended detailed card count (TJQK) is desired.
		Most of the time, user only cares about aggregating 10s.
	\param used to show cards that have been used (redundant info).
 */
ostream&
deck_state::show_count(ostream& o, const bool ex, const bool used) const {
	o << "actual count:" << endl;
	const card_type N = ex ? card_symbols : card_values;
	const util::precision_saver p(o, 2);
	const card_type* const po = ex ? extended_reveal_print_ordering
		: reveal_print_ordering;
	deck_count_type s_cards_rem(&cards[0]);
	deck_count_type s_cards_used(&used_cards[0]);
	if (!ex) {
		// use cards::simplify_deck_count
		s_cards_rem[TEN] += cards[JACK] +cards[QUEEN] +cards[KING];
		s_cards_used[TEN] += used_cards[JACK]
			+used_cards[QUEEN] +used_cards[KING];
	}
	const count_type* const _cards_rem =
		ex ? &cards[0] : &s_cards_rem[0];
	const count_type* const _cards_used =
		ex ? &used_cards[0] : &s_cards_used[0];
{
	const util::width_saver ws(o, 4);
	card_type i = 0;
	o << "card:\t";
	for (i=0 ; i<N; ++i) {
		const card_type j = po[i];
		o << "   " << card_name[j];
	}
	o << "\ttotal\t%" << endl;
if (used) {
	o << "used:\t";
	for (i=0 ; i<N; ++i) {
		const card_type j = po[i];
		o << setw(4) << _cards_used[j];
	}
	o << "\t" << cards_spent << "\t(" <<
		double(cards_spent) *100.0 / (num_decks *52) << "%)\n";
}
	o << "rem:\t";
	for (i=0; i<N; ++i) {
		const card_type j = po[i];
		o << setw(4) << _cards_rem[j];
	}
	// note: altered decks will have nonsense values
	o << "\t" << cards_remaining << "\t(" <<
		double(cards_remaining) *100.0 / (num_decks *52) << "%)\n";
}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
deck_state::__check_edit_deck_args(const card_type c, const int n) {
	const card_type N = card_symbols;
	if (c >= N) {
		cerr << "error: invalid card index " << c << endl;
		return true;
	}
	if (n < 0) {
		cerr << "error: resulting quantity must be non-negative"
			<< endl;
		return true;
	}
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param c the card index
	\param n the number of those cards to keep in deck
 */
bool
deck_state::edit_deck(const card_type c, const int n) {
	if (__check_edit_deck_args(c, n)) {
		return true;
	}
	const int d = n -int(cards[c]);
	cards[c] = n;
	cards_remaining += d;
	// leave used_cards and cards_spent alone
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
deck_state::edit_deck_add(const card_type c, const int n) {
	const card_type N = cards.size();
	if (c >= N) {
		cerr << "error: invalid card index " << c << endl;
		return true;
	}
	return edit_deck(c, cards[c] +n);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
deck_state::edit_deck_sub(const card_type c, const int n) {
	const card_type N = cards.size();
	if (c >= N) {
		cerr << "error: invalid card index " << c << endl;
		return true;
	}
	return edit_deck(c, cards[c] -n);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Interactively edit deck distribution for each card.
	\return true if there is an error.
 */
bool
deck_state::edit_deck_all(istream& i, ostream& o) {
	const card_type N = cards.size();
	o << "Enter quantity of each card.\n"
"\t. for unchanged\n"
"\t[+|-] value : for relative count change" << endl;
	card_type j = 0;
for ( ; j<N; ++j) {
	const card_type k = extended_reveal_print_ordering[j];
	bool accept = true;
	do {
	o << "card: " << card_name[k] << " (" << cards[k] << "): ";
	string line;
	getline(i, line);
	string_list args;
	tokenize(line, args);
	switch (args.size()) {
	case 1: {
		const string& qs(args.front());
		int q;
		if (qs == ".") {
			// accepted, unchanged
		} else if (string_to_num(qs, q)) {
			o << "error: invalid number" << endl;
			accept = false;
		} else if (edit_deck(k, q)) {
			// there was error, prompt again
			accept = false;
		}
		break;
	}
	case 2: {
		int q;
		const string& sn(args.front());
		const string& qs(args.back());
		if (string_to_num(qs, q)) {
			o << "error: invalid number" << endl;
			accept = false;
		}
		switch (sn[0]) {
		case '+':
			accept = !edit_deck_add(k, q);
			break;
		case '-':
			accept = !edit_deck_sub(k, q);
			break;
		default:
			accept = false;
			break;
		}	// end switch
		break;
	}
	default:
		accept = false;
		break;
	}	// end switch
	} while (i && !accept);
}	// end for all cards
	return !i;
}	// end edit_deck_all

//=============================================================================
}	// end namespace blackjack

