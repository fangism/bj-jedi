// "card_state.hh"

#ifndef	__BOC2_CARD_STATE_HH__
#define	__BOC2_CARD_STATE_HH__

#include <string>
#include <vector>
#include <iosfwd>
#include "util/array.hh"

// namesspace?

using std::ostream;
using std::vector;
using std::string;

typedef	double		probability_type;

//-----------------------------------------------------------------------------
enum {
	ACE = 0,		// index of Ace (card_odds)
	TEN = 9,		// index of 10 (T) (card_odds)
	card_values = 10	// number of unique card values (TJQK = 10)
};
extern	const char		card_name[];	// use util::array?
extern	size_t		card_index(const char);	// reverse-map of card_name

/**
	A deck is just modeled as a probability vector.  
	\invariant sum of probabilities should be 1.
 */
typedef	vector<probability_type>		probability_vector;
typedef	util::array<probability_type, card_values>	deck_distribution;
typedef	util::array<size_t, card_values>	deck_count_type;

// 13 card values, 2 through A
extern	const deck_count_type			standard_deck_count_reduced;
extern	const deck_count_type			standard_deck_count;
extern	const deck_distribution			standard_deck_distribution;

//-----------------------------------------------------------------------------
/**
	Graph must be acyclic, and terminal!  Fortunately, blackjack is acyclic.
	Technically, this should be done with matrix computations, 
	taking the limit of a square state matrix multiplied unto itself
	or convergent power matrices.  
	Instead, we use an iterative, numerical, flow-based approach.  
 */
class state_machine {
public:
	typedef	vector<size_t>		edge_vector;
	struct node {
		string			name;	///< e.g. "busted" or "21"
		/**
			Outgoing edges.
			If there are no edges, then this is terminal.
			Consider it a lone self-edge.
		 */
		edge_vector		out_edges;
		// don't need incident edges

		bool
		is_terminal(void) const { return out_edges.empty(); }

		size_t
		size(void) const { return out_edges.size(); }

		const size_t&
		operator [] (const size_t i) const { return out_edges[i]; }

		// verify that all entries are non-zero
		void
		check(void) const;

		ostream&
		dump_edges(ostream&) const;

		ostream&
		dump(ostream&) const;
	};
private:
	typedef	vector<node>		state_set_type;
	/**
		States are enumerated and indexed.  
		It is up to the caller/user to interpret the indices. 
		The 0 state is should never be reached from any other state.
	 */
	state_set_type			states;

public:
	// parameter is number of nodes
	state_machine();

	const state_set_type&
	get_states(void) const { return states; }

	void
	resize(const size_t);

	const node&
	operator [] (const size_t i) const { return states[i]; }

	node&
	operator [] (const size_t i) { return states[i]; }

	void
	name_state(const size_t, const string&);

	void
	add_edge(const size_t, const size_t, const size_t, const size_t);

	void
	copy_edge_set(const size_t, const size_t);

	void
	copy_edge_set(const state_machine&, const size_t, const size_t);

	/// compute terminal probabilities (solve)
	void
	solve(const deck_distribution&,
		const probability_vector&, probability_vector&);

	bool
	convolve(const deck_distribution&,
		const probability_vector&, probability_vector&);

	void
	check(void) const;

	ostream&
	dump(ostream&) const;
};	// end struct state_machine

#endif	// __BOC2_CARD_STATE_HH__

