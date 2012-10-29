// "bj/core/hand.cc"

#include <iostream>
#include <map>				// for map<split_state>
#include <set>				// for set<split_state>
#include <sstream>
#include "bj/core/hand.hh"
#include "bj/core/blackjack.hh"

#define	DEBUG_HAND				0

namespace blackjack {
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::set;
using cards::ACE;
using cards::TEN;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::card_index;
using cards::card_symbols;
using cards::card_value_map;
using cards::state_machine;
using cards::count_type;

//=============================================================================
// class split_state method definitions

/**
	Initializes split-state.
	\param available number of splits (form variation)
	\param p true if initial hand is a splittable pair.
 */
void
split_state::initialize(const ssize_t n, const bool p) {
	splits_remaining = n;
	if (p) {
		paired_hands = 1;
		unpaired_hands = 0;
	} else {
		paired_hands = 0;
		unpaired_hands = 1;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
split_state::dump_code(ostream& o) const {
	const count_type Ps = std::min(splits_remaining, paired_hands);
	const count_type Pn = paired_hands -Ps;
#if 0
	o << Ps << ',' << Pn << ',' << unpaired_hands;	// debug
#else
{
	count_type i=0;
	for ( ; i<Ps; ++i) {
		o << 'P';
	}
}{
	count_type i=0;
	for ( ; i<Pn; ++i) {
		o << 'X';
	}
}{
	ssize_t i=0;
	for ( ; i<unpaired_hands; ++i) {
		o << 'Y';
	}
}
#endif
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Construct the 3 resulting possible states after splitting 
	one (of several) hands.
	If hand is not splittable, just copy the this hand 3 ways.
	\param s1 the case of split forms two new paired hands.
	\param s2 the case where split makes one paired, one unpaired hand.
	\param s2 case where split results in two unpaired hands.  
 */
void
split_state::post_split_states(
		split_state& s1, split_state& s2, split_state& s3) const {
	s1 = *this;
	s2 = *this;
	s3 = *this;
	if (is_splittable()) {
		--s1.splits_remaining;
		--s2.splits_remaining;
		--s3.splits_remaining;
		++s1.paired_hands;
		++s2.unpaired_hands;
		--s3.paired_hands;
		s3.unpaired_hands += 2;
#if 0
		s1.dump_code(cout << "split 1: ") << endl;
		s2.dump_code(cout << "split 2: ") << endl;
		s3.dump_code(cout << "split 3: ") << endl;
#endif
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class split_graph {
	typedef	map<split_state, state_type>		index_map_type;
	typedef	vector<index_map_type::iterator>	edges_type;
	struct node {
		edges_type				out_edges;
		state_type				path_count;
		node() : out_edges(), path_count(1) { }
	};
	index_map_type				index_map;
	vector<node>				nodes;

public:
	explicit
	split_graph(const split_state& root) : index_map(), nodes() {
		add(root);
	}

	ostream&
	dump_dot_digraph(ostream&) const;

private:
	index_map_type::iterator
	add(const split_state& r);
};	// end class split_graph

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
split_graph::index_map_type::iterator
split_graph::add(const split_state& r) {
	const std::pair<index_map_type::iterator, bool>
		p(index_map.insert(std::make_pair(r, nodes.size())));
	const size_t ind = nodes.size();
if (p.second) {
	nodes.push_back(node());
	if (r.is_splittable()) {
		split_state s[3];
		r.post_split_states(s[0], s[1], s[2]);
		nodes.back().out_edges.reserve(3);
		// each call to add() invalidates reference b(nodes.back())
		index_map_type::iterator i[3];
		i[0] = add(s[0]);
		i[1] = add(s[1]);
		i[2] = add(s[2]);
		node& b(nodes[ind]);
		b.out_edges.push_back(i[0]);
		b.out_edges.push_back(i[1]);
		b.out_edges.push_back(i[2]);
#if 0
		cout << "sub indices: " <<
			b.out_edges[0]->second << ' ' <<
			b.out_edges[1]->second << ' ' <<
			b.out_edges[2]->second << endl;
#endif
		b.path_count = 
			nodes[b.out_edges[0]->second].path_count
			+nodes[b.out_edges[1]->second].path_count 
			+nodes[b.out_edges[2]->second].path_count;
	}	// already existed
}
	return p.first;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Construct edge-weighted, directed acyclic dependency graph of states.
	Output formate: dot (graphviz, digraph)
 */
ostream&
split_graph::dump_dot_digraph(ostream& o) const {
	index_map_type::const_iterator
		i(index_map.begin()), e(index_map.end());
	for ( ; i!=e; ++i) {
		const split_state& current(i->first);
		const node& info(nodes[i->second]);
		std::ostringstream oss;
		current.dump_code(oss);
		const string& cs(oss.str());
		o << cs << "\t[label=\"" << current.splits_remaining << ':'
			<< cs << "\\n(" << info.path_count << ")\"];" << endl;
		if (current.is_splittable()) {
			info.out_edges[0]->first.dump_code(o << cs << " -> ")
				<< "\t[label=\"pp\", color=red];" << endl;
			info.out_edges[1]->first.dump_code(o << cs << " -> ")
				<< "\t[label=\"2pq\", color=green];" << endl;
			info.out_edges[2]->first.dump_code(o << cs << " -> ")
				<< "\t[label=\"qq\", color=blue];" << endl;
		}
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
split_state::generate_graph(ostream& o) const {
	split_graph G(*this);
	return G.dump_dot_digraph(o);
}

//=============================================================================
// class player_hand_base method definitions

/**
	This constructor automatically sets the action mask depending
	on whether or not the state is splittable.
 */
player_hand_base::player_hand_base(const player_state_type ps,
		const play_map& play) :
		state(ps),
		player_options(
			play.is_player_pair(ps) ?
			action_mask::all : action_mask::all_but_split) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand_base::hit(const play_map& play, const card_type c) {
	assert(c < card_symbols);
	state = play.hit_player(state, card_value_map[c]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand_base::split(const play_map& play) {
	state = play.p_initial_card_map[pair_card()];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand_base::split(const play_map& play, const card_type c) {
	assert(c < card_symbols);
	state = play.split_player(state, card_value_map[c]);
	// assume re-splittable
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
player_hand_base::dump_state_only(ostream& o, const state_machine& sm) const {
	return o << sm[state].name;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
player_hand_base::dump(ostream& o, const state_machine& sm) const {
	o << sm[state].name << ", ";
//	return player_options.dump_verbose(o);
	return player_options.dump_debug(o);	// more compact
}

//=============================================================================
// class dealer_hand_base method definitions

void
dealer_hand_base::set_upcard(const card_type c) {
	assert(c < card_symbols);
	const card_type d = card_value_map[c];
	state = play_map::d_initial_card_map[d];
	first_card = true;
	// peek_state untouched
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
dealer_hand_base::dump(ostream& o, const state_machine& sm) const {
	o << sm[state].name << ' ';
	switch (peek_state) {
	case PEEKED_NO_10: o << "peek!10"; break;
	case PEEKED_NO_ACE: o << "peek!A "; break;
	case NO_PEEK:
	default: o << "no-peek"; break;
	}
	return o;
}

//=============================================================================
// class player_hand method definitions

void
player_hand::initial_card_player(const card_type p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_symbols);
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_player(card_value_map[p1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param p1 reveal card can be A,2-9,T-K, will be translated to value
 */
void
dealer_hand::initial_card_dealer(const card_type p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_symbols);
	const card_type reveal_value = card_value_map[p1];
	reveal = p1;
//	reveal = card_value_map[p1];
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_dealer(reveal_value);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Appends card and updates allowable actions.
 */
void
player_hand::hit_player(const card_type p2) {
	player_hand_base::hit(*play, p2);
	cards.push_back(card_name[p2]);
	player_options &= play->post_hit_actions;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
dealer_hand::hit_dealer(const card_type p2) {
	assert(p2 < card_symbols);
	reveal_hole_card();
	cards.push_back(card_name[p2]);
	state = play->hit_dealer(state, card_value_map[p2]);
	if ((cards.size() == 2) && (state == goal)) {
		state = dealer_blackjack;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Assigns initial state based on player's first two cards.
	\param p1,p2 first two player's cards
	\param nat true if 21 should be considered natural blackjack
	\param dr dealer's reveal card
 */
void
player_hand::deal_player(const card_type p1, const card_type p2, const bool nat, 
		const card_type dr) {
	cards.clear();
	cards.push_back(card_name[p1]);
	cards.push_back(card_name[p2]);
	const card_type dv = card_value_map[dr];
	state = play->deal_player(card_value_map[p1], card_value_map[p2], nat);
#if DEBUG_HAND
	play->initial_actions_per_state[state].dump_debug(cout << "state : ") << endl;
	play->initial_actions_given_dealer[dv].dump_debug(cout << "dealer: ") << endl;
#endif
	player_options = play->initial_actions_per_state[state]
		& play->initial_actions_given_dealer[dv];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand::double_down(const card_type p2) {
	hit_player(p2);
	action = DOUBLED_DOWN;
	player_options &= play->post_double_down_actions;
	// yes, in rare cases, surrender is allowed after double-down
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: support splitting un-paired hands!
	TODO: split-21s-are-naturals?
	Splits player's hand into two, using initial cards.  
	\param s1 second card for first hand
	\param s2 second card for second hand
	\param dr dealer's reveal card
 */
void
player_hand::split(player_hand& nh,
		const card_type s1, const card_type s2, const card_type dr) {
	const card_type split_card = pair_card();
	// 21 should not be considered blackjack when splitting (variation)?
	deal_player(split_card, s1, false, dr);
	player_options &= play->post_split_actions;
	nh.deal_player(split_card, s2, false, dr);
	nh.player_options &= play->post_split_actions;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
bool
player_hand::splittable(void) const {
	// TODO: allow variants that split nonmatching cards!
	return (cards.size() == 2) && (cards[0] == cards[1]);
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
player_hand::dump_player(ostream& o) const {
	o << "player: " << cards << " (" << play->player_hit[state].name << ")";
	switch (action) {
	case DOUBLED_DOWN: o << " doubled-down"; break;
	case SURRENDERED: o << " surrendered"; break;
	default: break;
	}
	// TODO: show player_options.dump()
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
dealer_hand::dump_dealer(ostream& o) const {
	o << "dealer: " << cards << " (" << play->dealer_hit[state].name << ")";
	return o;
}

//=============================================================================
}	// end namespace blackjack

