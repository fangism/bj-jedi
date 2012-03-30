// "play_options.hh"

#ifndef	__BOC2_PLAY_OPTIONS_HH__
#define	__BOC2_PLAY_OPTIONS_HH__

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
		If all player hands are busted/surrendered, 
		don't bother playing.
		Default: true
	 */
	bool				dealer_plays_only_against_live;
	/**
		If true, show bankroll and current bet after
		every hand.
	 */
	bool				always_show_status;
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

#endif	// __BOC2_PLAY_OPTIONS_HH__

