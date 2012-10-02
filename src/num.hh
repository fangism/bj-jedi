// "num.hh"
// numeric types for blackjack project
// eventually support rational numbers, e.g. P/Q

#ifndef	__BOC2_NUM_HH__
#define	__BOC2_NUM_HH__

namespace cards {

typedef	double			probability_type;
typedef	probability_type	edge_type;

typedef	size_t			card_type;	// possibly enum
typedef	size_t			count_type;
typedef	size_t			state_type;

// possibly enumerate these:
typedef	state_type		player_state_type;
typedef	state_type		dealer_state_type;

}	// end namespace cards

#endif	// __BOC2_DECK_HH__

