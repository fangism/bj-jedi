// "variation.hh"

#ifndef	__BOC2_VARIATION_HH__
#define	__BOC2_VARIATION_HH__

#include <iosfwd>

/**
	Define to 1 to implement Blackjack Switch's push-on-22 rule.
	Goal: 1
	Status: drafted, basically tested
#define	PUSH22				1
 */

namespace blackjack {
using std::istream;
using std::ostream;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Options for game variations.
	popularity comments on boolean variations:
	rare - almost never
	common - both variations are common
	usual - more often than not
	standard - almost always
	TODO: organize variations into presets
	TODO: spanish 21
	TODO: european (no hole, no peek)
 */
struct variation {
	/// if true, dealer must hit on soft 17, else must stand (common)
	bool			H17;
	/// option to surrender before checking for blackjack (rare)
	bool			surrender_early;
	/// option to surrender after checking for blackjack (common)
	bool			surrender_late;
#if 0
	/// option to surrender at any point
	bool			surrender_any_time;
#endif
	/// whether or not dealer checks for A hole card (usual)
	bool			peek_on_10;
	/// whether or not dealer checks for 10 hole card (standard)
	bool			peek_on_Ace;
#if 1
	// TODO: analyze this (mutually exclusive options)
	/// dealer wins ties (disasterous for player!) (rare)
	bool			ties_lose;
	/// player wins ties (rare)
	bool			ties_win;
#endif
	/// whether or not player may double on hard 9 (usual)
	bool			double_H9;
	/// whether or not player may double on hard 10 (standard)
	bool			double_H10;
	/// whether or not player may double on hard 11 (standard)
	bool			double_H11;
#if 0
	// TODO:
	/// whether or not player may double down on Ace,X (usual)
	bool			double_AceX;
#endif
	/// whether or not player may double on other values (usual)
	bool			double_other;
#if 0
	/// automatic win on 5th card that does not bust
	bool			five_card_charlie;
#endif
	/// double-after-split? (common)
	bool			double_after_split;
#if 0
	/// interesting variation: to allow doubling down at any time
	bool			double_any_time;
#endif
	/// surrender-after-split? (never heard of it)
// TODO: options or resplitting: 2, 4, 8, INF
	/// allow splitting (standard)
	bool			split;
	/// allow resplitting, split-after-split (common)
	bool			resplit;

	/// most casinos allow only 1 card on each split ace (common)
	bool			one_card_on_split_aces;
	/// dealer pushes on 22 against any non-blackjack hand (switch)
	bool			push22;
	/// player's blackjack payoff (usually 1.5)
	double			bj_payoff;
// TODO: don't actually compute this, though it is trivial
	/// player's insurance payoff (usually 2.0)
	double			insurance;
	/// surrender loss, less than 0 (usualy -0.5)
	double			surrender_penalty;
	/// doubled-down multipler (other than 2.0 is rare)
	double			double_multiplier;
#if 0
	bool			player_bj_always_wins;
	bool			dealer_bj_always_loses;
#endif

// play-time options:
	/// affects play only, strategy uses infinite deck appx.
	size_t			num_decks;
	/// largest fraction of shoe/deck that can be played
	double			maximum_penetration;

	// known unsupported options:
	// 5-card charlie -- would need expanded state table

	/// default ctor and options
	variation() : 
		H17(true), 
		surrender_early(false),
		surrender_late(true),
		peek_on_10(true),
		peek_on_Ace(true),
		ties_lose(false),
		ties_win(false),
		double_H9(true),
		double_H10(true),
		double_H11(true),
		double_other(true),
		double_after_split(true),
		split(true),
		resplit(false),
		one_card_on_split_aces(true),
		push22(false),
		bj_payoff(1.5), 
		insurance(2.0),
		surrender_penalty(0.5),
		double_multiplier(2.0),
		num_decks(6),
		maximum_penetration(0.75) { }

	/// \return true if some form of doubling-down is allowed
	bool
	some_double(void) const {
		return double_H9 || double_H10 || double_H11 || double_other;
	}

	ostream&
	dump(ostream&) const;

	void
	configure(void);

};	// end struct variation

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_VARIATION_HH__

