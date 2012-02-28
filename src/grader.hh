// "grader.hh"
// could be called 'table'

#ifndef	__BOC2_GRADER_HH__
#define	__BOC2_GRADER_HH__

#include <vector>
#include <string>
#include "strategy.hh"
#include "blackjack.hh"
#include "deck_state.hh"
#include "hand.hh"

namespace blackjack {
class variation;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The user enters both the dealer and player's actions and outcomes.  
	The purposes is to characterize the odds of the game, grade the player's
	actions, and rate luck and skill.
	Input stream: history of cards seen and transactions.  
 */
class grader {
public:
	const variation&			var;
	istream&				istr;
	ostream&				ostr;
private:
	/**
		Game state machines.
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
	struct options {
		/**
			If true, give user the option of hand-picking 
			every card with prompt.
			Useful for simulating scenarios.
		 */
		bool					pick_cards;
		/**
			More computationally intense.
			Re-calculates dynamic strategy optimization 
			immediately before prompting user for action.  
		 */
		bool					use_dynamic_strategy;
		/**
			If all player hands are busted/surrendered, 
			don't bother playing.
			Default: true
		 */
		bool				dealer_plays_only_against_live;
		/// size of current bet (convert from integer)
		double					bet;

		options();			// defaults
	};	// end struct options
	options					opt;

	struct statistics {
		double				initial_bankroll;
		double				min_bankroll;
		double				max_bankroll;
		double				final_bankroll;
		// spread of initial hands vs. dealer-reveal
		// initial edges
		// decision edges
		// optimal edges
		// edge margins
		// basic vs. dynamic
		// bet distribution
		// expectation
		// lucky draws
		// bad beats
	};	// end struct statistics
private:
// state:
	/// current amount of money
	double					bankroll;
public:

	// other quantities for grading and statistics

public:
	explicit
	grader(const variation&, istream&, ostream&);

	~grader();

	ostream&
	status(ostream&) const;

	void
	deal_hand(void);

	const double&
	get_bankroll(void) const { return bankroll; }

	const variation&
	get_variation(void) const { return var; }

	const deck_state&
	get_deck_state(void) const { return C; }

	const strategy&
	get_basic_strategy(void) const { return basic_strategy; }

	const strategy&
	get_dynamic_strategy(void) const { return dynamic_strategy; }

	int
	main(void);

private:
	bool
	offer_insurance(const bool) const;

	size_t
	draw_up_card(void);

	size_t
	draw_hole_card(void);

	bool
	already_split(void) const {
		return player_hands.size() >= 2;
	}

	void
	handle_player_action(const size_t, const player_choice);

	bool
	play_out_hand(const size_t);

	void
	update_dynamic_strategy(void);

	ostream&
	dump_situation(const size_t) const;

	player_choice
	assess_action(const size_t, const size_t, 
		const bool, const bool, const bool);
 
};	// end class grader

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_GRADER_HH__

