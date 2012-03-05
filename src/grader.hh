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
			If true, reshuffle after every hand.
			In this case number of decks matters less
			or not at all.
		 */
		bool				continuous_shuffle;
		/**
			If true, give user the option of hand-picking 
			every card with prompt.
			Useful for simulating scenarios.
		 */
		bool				pick_cards;
		/**
			More computationally intense.
			Re-calculates dynamic strategy optimization 
			immediately before prompting user for action.  
			Also applies to suggestions and notifications, 
			and edge calculations.  
		 */
		bool				use_dynamic_strategy;
		/**
			If all player hands are busted/surrendered, 
			don't bother playing.
			Default: true
		 */
		bool				dealer_plays_only_against_live;
		/**
			Always show count when prompted for action.
		 */
		bool				always_show_count_at_action;
		/**
			Always show count at the start of each hand.
		 */
		bool				always_show_count_at_hand;
		/**
			Always compute optimal decision and suggest the best.
		 */
		bool				always_suggest;
		/**
			Notify when decision is not optimal.
		 */
		bool				notify_when_wrong;
		/**
			Show mathematical edges with suggestions and 
			wrong-move notifications.  
		 */
		bool				show_edges;

		/// size of current bet (convert from integer)
		double				bet;

		options();			// defaults

		bool
		always_update_dynamic(void) const {
			return use_dynamic_strategy &&
				(always_suggest || notify_when_wrong);
		}

		void
		save(ostream&) const;

		void
		load(istream&);

	};	// end struct options
	options					opt;

	// should privatize/protect some members...
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
		// spread of initial hands vs. dealer-reveal (matrix)
		// initial edges -- computed on first deal (pre-peek)
		// decision edges -- updated every decision
		// optimal edges
		// edge margins
		// basic vs. dynamic
		// bet distribution (map)
		// expectation
		// lucky draws
		// bad beats

		statistics();			// initial value

		void
		initialize_bankroll(const double);

		void
		compare_bankroll(void);

		void
		register_bet(const double);

		ostream&
		dump(ostream&) const;

		void
		save(ostream&) const;

		void
		load(istream&);

	};	// end struct statistics
	statistics				stats;
	// random seed?
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
	get_bankroll(void) const { return stats.bankroll; }

	const variation&
	get_variation(void) const { return var; }

	const deck_state&
	get_deck_state(void) const { return C; }

	deck_state&
	get_deck_state(void) { return C; }

	const strategy&
	get_basic_strategy(void) const { return basic_strategy; }

	const strategy&
	get_dynamic_strategy(void) const { return dynamic_strategy; }

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

	void
	auto_shuffle(void);

	ostream&
	dump_situation(const size_t) const;

	pair<player_choice, player_choice>
	assess_action(const size_t, const size_t, ostream&, 
		const bool, const bool, const bool);
 
};	// end class grader

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_GRADER_HH__

