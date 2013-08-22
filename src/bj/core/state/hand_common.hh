// "bj/core/state/hand_common.hh"

#ifndef	__BJ_CORE_STATE_HAND_COMMON_HH__
#define	__BJ_CORE_STATE_HAND_COMMON_HH__

#include <string>

namespace blackjack {
class play_map;
using std::string;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct hand_common {
	// reference to game variation and state transition maps
	const play_map*				play;
	/**
		A,2-9,T.
	 */
	typedef	string				card_array_type;
	card_array_type				cards;

	explicit
	hand_common(const play_map& p) : play(&p), cards() { }

};	// end struct hand_common

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_STATE_HAND_COMMON_HH__

