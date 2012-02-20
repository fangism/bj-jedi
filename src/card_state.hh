// "card_state.hh"

#ifndef	__BOC2_CARD_STATE_HH__
#define	__BOC2_CARD_STATE_HH__

#include <iosfwd>
#include <string>
#include <vector>
#include "deck.hh"

namespace cards {
using std::ostream;
using std::string;

//-----------------------------------------------------------------------------
/**
	A deck is just modeled as a probability vector.  
	\invariant sum of probabilities should be 1.
 */
typedef	std::vector<probability_type>		probability_vector;

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
	typedef	std::vector<size_t>		edge_vector;
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
	typedef	std::vector<node>		state_set_type;
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
		const probability_vector&, probability_vector&) const;

	bool
	convolve(const deck_distribution&,
		const probability_vector&, probability_vector&) const;

	void
	check(void) const;

	ostream&
	dump(ostream&) const;
};	// end struct state_machine

}	// end namespace cards

#endif	// __BOC2_CARD_STATE_HH__

