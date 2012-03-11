// "grader.hh"
// could be called 'table'

#ifndef	__BOC2_GRADER_HH__
#define	__BOC2_GRADER_HH__

#include <vector>
#include <string>
#include <utility>			// for std::pair
#include "strategy.hh"
#include "blackjack.hh"
#include "deck_state.hh"
#include "statistics.hh"
#include "hand.hh"

namespace blackjack {
class variation;
struct play_options;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The user enters both the dealer and player's actions and outcomes.  
	The purposes is to characterize the odds of the game, grade the player's
	actions, and rate luck and skill.
	Input stream: history of cards seen and transactions.  
 */
class grader {
public:
	/**
		For a particular grader session, the rule variations 
		may not change.
	 */
	const variation&			var;
	/**
		These in-play options are allowed to be modified, 
		and will persist after a grader session.
	 */
	play_options&				opt;
	istream&				istr;
	ostream&				ostr;
private:
	/**
		Game state machines, as player and dealer hit.
	 */
	play_map				play;
	/**
		Static strategy calculation, based on rule variations
		only, not current card counts.  
	 */
	strategy				basic_strategy;
	/**
		Dynamic strategy computer.
		Re-evaluate on every hand, after cards have been drawn.
		Changes slightly depending on card distribution.  
	 */
	strategy				dynamic_strategy;
	/**
		Maintain the state of the deck, and cards used.  
		This is state information that can be saved/restored.
		So is bankroll.
	 */
	deck_state				C;

	// hand information is not saved, it is only temporary
	// should these be removed from here?
	/**
		Maintain a vector of hands in case of splits and re-splits.
	 */
	std::vector<hand>			player_hands;
	/**
		Dealer's hand, as it develops.  
	 */
	hand					dealer_hand;
	/**
		The dealer's revealed card.  Redundant with dealer_hand.
	 */
	size_t					dealer_reveal;
	/**
		Dealer's hidden card.
	 */
	size_t					dealer_hole;

public:
	statistics				stats;
	// random seed?
	/**
		Hand-by-hand record of trends to plot.
		e.g. true count vs. bet
	 */
	struct history {
	};	// end struct history
public:

	// other quantities for grading and statistics

public:
	explicit
	grader(const variation&, play_options&, istream&, ostream&);

	~grader();

	ostream&
	status(ostream&) const;

	bool
	deal_hand(void);

	const double&
	get_bankroll(void) const { return stats.bankroll; }

	const variation&
	get_variation(void) const { return var; }

	const play_map&
	get_play_map(void) const { return play; }

	const deck_state&
	get_deck_state(void) const { return C; }

	deck_state&
	get_deck_state(void) { return C; }

	const strategy&
	get_basic_strategy(void) const { return basic_strategy; }

	const strategy&
	get_dynamic_strategy(void) const { return dynamic_strategy; }

	void
	shuffle(void) { C.reshuffle(); }

	void
	update_dynamic_strategy(void);

	int
	main(void);

private:
	bool
	offer_insurance(const bool) const;

	size_t
	draw_up_card(void);

	size_t
	draw_hole_card(void);

	void
	reveal_hole_card(const size_t);

	bool
	already_split(void) const {
		return player_hands.size() >= 2;
	}

	void
	handle_player_action(const size_t, const player_choice);

	bool
	play_out_hand(const size_t);

	bool
	auto_shuffle(void);

	ostream&
	dump_situation(const size_t) const;

	pair<expectations, expectations>
	assess_action(const size_t, const size_t, ostream&, 
		const bool, const bool, const bool);
 
};	// end class grader

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_GRADER_HH__

