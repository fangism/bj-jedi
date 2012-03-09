// "lobby.hh"

#ifndef	__BOC2_LOBBY_HH__
#define	__BOC2_LOBBY_HH__

#include "variation.hh"
#include "play_options.hh"

namespace blackjack {
struct lobby {
	// only in the lobby is the variation/rules allowed to change
	variation			var;
	// the play-time options may change in the grader sesssion
	play_options			opt;

	// load saved users and accounts?
	// maybe keep track of bankroll here?

	int
	main(void);

};	// end struct lobby
}	// end namespace blackjack

#endif	// __BOC2_LOBBY_HH__
