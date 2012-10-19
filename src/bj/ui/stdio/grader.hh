// "grader.hh"
// could be called 'table'

#ifndef	__BJ_UI_STDIO_GRADER_HH__
#define	__BJ_UI_STDIO_GRADER_HH__

#include <vector>
#include <string>
#include <utility>			// for std::pair
#include "bj/core/strategy.hh"
#include "bj/core/blackjack.hh"
#include "bj/core/deck_state.hh"
#include "bj/core/statistics.hh"
#include "bj/core/hand.hh"
#include "bj/core/counter.hh"
#include "bj/ui/stdio/bookmark.hh"

namespace blackjack {
struct variation;
struct play_options;
class action_mask;

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
	/**
		The perceived state only counts cards of known value
		and tracks peeked discarded cards separately.
		This is information from the player's perspective
		instead of the program's perspective.
	 */
	perceived_deck_state			P;

	/// TODO: support extendable vector of various counters
	cards::named_counter			hi_lo_counter;

	// hand information is not saved, it is only temporary
	// should these be removed from here?
	/**
		Maintain a vector of hands in case of splits and re-splits.
	 */
	std::vector<player_hand>			player_hands;
	/**
		Dealer's hand, as it develops.  
	 */
	dealer_hand					dealer;
	/**
		Dealer's hidden card.
	 */
	card_type					dealer_hole;
public:
	/**
		Collection of statistics of play.
		These are cumulative numbers, constant memory.
	 */
	statistics				stats;
	/**
		Collection of saved situations for 
		analysis.
	 */
	std::vector<bookmark>			bookmarks;
	// random seed?
	/**
		Hand-by-hand record of trends to plot.
		e.g. true count vs. bet
		These arrays grow with every hand played.
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

	// really only intended for edit_deck
	deck_state&
	get_deck_state(void) { return C; }

	const perceived_deck_state&
	get_perceived_deck_state(void) const { return P; }

	// really only intended for edit_deck
	perceived_deck_state&
	get_perceived_deck_state(void) { return P; }

	const strategy&
	get_basic_strategy(void) const { return basic_strategy; }

	const strategy&
	get_dynamic_strategy(void) const { return dynamic_strategy; }

	void
	reset_counts(void);

	void
	shuffle(void) {
		C.reshuffle();
		reset_counts();
	}

	void
	update_dynamic_strategy(void);

	void
	show_count(void) const;

	void
	list_bookmarks(void) const;

	void
	clear_bookmarks(void);

	int
	main(void);

private:
	bool
	offer_insurance(const bool);

	card_type
	draw_up_card(const char*);

	card_type
	draw_hole_card(const char*);

	void
	reveal_hole_card(const card_type);

	void
	replace_hole_card(void);

	bool
	already_split(void) const {
		return player_hands.size() >= 2;
	}

	void
	quiz_count(void) const;

	void
	handle_player_action(const size_t, const player_choice);

	bool
	play_out_hand(const size_t);

	bool
	auto_shuffle(void);

	void
	show_dynamic_edge(void);

	ostream&
	dump_situation(const size_t) const;

	pair<expectations, expectations>
	assess_action(const player_state_type, const card_type, ostream&, 
		const action_mask&);
 
};	// end class grader

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_UI_STDIO_GRADER_HH__

