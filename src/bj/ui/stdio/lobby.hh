// "bj/ui/stdio/lobby.hh"

#ifndef	__BJ_UI_STDIO_LOBBY_HH__
#define	__BJ_UI_STDIO_LOBBY_HH__

#include "bj/core/variation.hh"
#include "bj/ui/stdio/play_options.hh"

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

#endif	// __BJ_UI_STDIO_LOBBY_HH__
