// "play_options.hh"

#ifndef	__BJ_UI_STDIO_PLAY_OPTIONS_HH__
#define	__BJ_UI_STDIO_PLAY_OPTIONS_HH__

#include <iosfwd>
#include "util/tokenize.hh"

namespace blackjack {
using std::ostream;
using std::istream;
using util::string_list;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Collection of general game-play options, 
	feedback and display settings.  
	Options that affect the mathematics should belong in
	the variation struct.
	Any options that don't affect the mathematical outcomes
	may go in here.  
 */
struct play_options {
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
		Even more computationally intense.
		If true, then compute a dynamic strategy
		based on non-replacement drawing from the deck.
		Each card drawn changes the distribution!
		Computation follows a complete optimal decision tree.
	 */
	bool				use_exact_strategy;
	/**
		If all player hands are busted/surrendered, 
		don't bother playing.  Hole card is not revealed.
		Default: true
	 */
	bool				dealer_plays_only_against_live;
	/**
		If true, show bankroll and current bet after
		every hand.
	 */
	bool				always_show_status;
	/**
		Show the player's mathematical edge between each hand.
	 */
	bool				always_show_dynamic_edge;
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
		Notify when dynamic strategy differs and is better.
	 */
	bool				notify_dynamic;
	/**
		Notify when exact strategy differs from either
		basic or dynamic strategy.
	 */
	bool				notify_exact;
	/**
		Show detailed count when notifying.
	 */
	bool				notify_with_count;
	/**
		Show mathematical edges with suggestions and 
		wrong-move notifications.  
	 */
	bool				show_edges;
	/**
		Let computer automatically play optimally.
	 */
	bool				auto_play;
	/**
		If true, show face cards by symbol, not always just 'T' for 10.
		Adds a little bit more realism.
	 */
	bool				face_cards;
	/**
		If true, show separate counts for face cards and 10s.
		Not particularly useful.
	 */
	bool				count_face_cards;
	/**
		If true, show separate counts for used up cards.
		Information is redundant.
	 */
	bool				count_used_cards;
	/**
		If true, automatically bookmark situations
		where user's action was not optimal.
	 */
	bool				bookmark_wrong;
	/**
		If true, bookmark all situations when
		dynamic strategy differed from basic strategy.
	 */
	bool				bookmark_dynamic;
	/**
		If true, bookmark all situations where exact strategy
		differs from basic or dynamic strategy.
	 */
	bool				bookmark_exact;
	/**
		If true, prompt for count before reshuffling.
	 */
	bool				quiz_count_before_shuffle;
#if 0
	/**
		If true, split hands are played in order, and
		remaining split hands do not get a second card until
		earlier hands have finished.  
		When counting cards, this affects the
		counts because cards are removed.
		For analysis purposes, it is easier to withhold the 
		second card on split hands until earlier hands are finished.
	 */
	bool				split_play_before_second_card;
#endif
	/**
		Quiz count after every N hands.
	 */
	size_t				quiz_count_frequency;
	/// size of current bet (convert from integer)
	double				bet;

	play_options();			// defaults

	bool
	always_update_dynamic(void) const {
		return use_dynamic_strategy &&
			(always_suggest || notify_when_wrong);
	}

	ostream&
	dump(ostream&) const;

	void
	configure(void);

	int
	command(const string_list&);

	void
	save(ostream&) const;

	void
	load(istream&);

};	// end struct play_options

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_UI_STDIO_PLAY_OPTIONS_HH__

