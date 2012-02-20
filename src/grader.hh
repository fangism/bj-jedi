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
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The user enters both the dealer and player's actions and outcomes.  
	The purposes is to characterize the odds of the game, grade the player's
	actions, and rate luck and skill.
	Input stream: history of cards seen and transactions.  
 */
class grader {
	variation				var;
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
	 */
	deck_state				C;

#if 0
	/**
		A single player hand state.
	 */
	struct hand {
		/**
			A,2-9,T.
		 */
		typedef	string				player_cards;
		player_cards				cards;
		/**
			Enumerated state, from the state machine.
		 */
		size_t					state;
		/**
			Whether or not this hand was doubled-down.
		 */
		bool					doubled_down;
		/**
			Whether or not this hand was surrendered.
		 */
		bool					surrendered;

		hand() : cards(), state(0),
			doubled_down(false), surrendered(false) { }

		// initial deal
		void
		initial_card_player(const play_map&, const size_t);

		void
		initial_card_dealer(const play_map&, const size_t);

		void
		deal_player(const play_map&, const size_t, const size_t);

		void
		deal_dealer(const play_map&, const size_t, const size_t);

		// hit state transition -- use this for double-down too
		void
		hit_player(const play_map&, const size_t);

		void
		hit_dealer(const play_map&, const size_t);

		void
		presplit(const play_map&);

		void
		split(const play_map&, const size_t);

		bool
		splittable(void) const;

		bool
		doubleable(void) const {
			return cards.size() == 2;
		}
		bool
		surrenderable(void) const {
			return cards.size() == 2;
		}

		bool
		has_blackjack(void) const {
			return state == player_blackjack;
		}

		ostream&
		dump_dealer(ostream&, const play_map&) const;

		ostream&
		dump_player(ostream&, const play_map&) const;

	};	// end struct hand
#endif

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
// options:
	/**
		If true, give user the option of hand-picking every card with prompt.
	 */
	bool					pick_cards;
	/**
		More computationally intense.
		Re-calculates dynamic strategy optimization immediately
		before prompting user for action.  
	 */
	bool					use_dynamic_strategy;

private:
// state:
	/// current amount of money
	double					bankroll;
public:
	/// size of current bet (convert from integer)
	double					bet;

	// other quantities for grading and statistics

public:
	explicit
	grader(const variation&);

	~grader();

	ostream&
	status(ostream&) const;

	void
	deal_hand(istream&, ostream&);

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
	offer_insurance(istream&, ostream&, const bool) const;

	bool
	already_split(void) const {
		return player_hands.size() >= 2;
	}

	ostream&
	dump_situation(ostream& o, const size_t) const;

};	// end class grader

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_GRADER_HH__

