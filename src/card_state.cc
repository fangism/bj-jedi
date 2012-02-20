// "card_state.cc"

#include "card_state.hh"
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <numeric>		// for accumulate
#include <iostream>
#include <sstream>
#include <iterator>
// #include "util/array.tcc"

#define	DEBUG_SOLVE		0

namespace cards {

using std::ostringstream;
using std::cout;
using std::endl;

//-----------------------------------------------------------------------------
/**
	Assert that no edges point to the 0 state.  
	Is allowed to have no edges.  
 */
void
state_machine::node::check(void) const {
	const edge_vector::const_iterator e(out_edges.end());
	assert(find(out_edges.begin(), e, size_t(0)) == e);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
state_machine::node::dump_edges(ostream& o) const {
	std::ostream_iterator<size_t> osi(o, ",");
	copy(out_edges.begin(), out_edges.end(), osi);
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
state_machine::node::dump(ostream& o) const {
	return dump_edges(o << '\"' << name << "\":\t");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
state_machine::state_machine() :
	states() {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Allocate n states to be encoded.  
 */
void
state_machine::resize(const size_t n)  {
	states.resize(n);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Change the name of the state. 
 */
void
state_machine::name_state(const size_t i, const string& n) {
	assert(i < states.size());
	states[i].name = n;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Add an edge from state i to state j, using probability[d] from deck.
	\param i from state
	\param j to state
	\param d edge index
	\param n size of edge array, for resizing
 */
void
state_machine::add_edge(const size_t i, const size_t j, 
		const size_t d, const size_t n) {
	node& nd(states[i]);
	nd.out_edges.resize(n, 0);	// default
	assert(d < n);		// allow offset state indices
	nd.out_edges[d] = j;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Copy an entire edge set from state i to state j.
 */
void
state_machine::copy_edge_set(const size_t i, const size_t j) {
	states[j].out_edges = states[i].out_edges;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
state_machine::copy_edge_set(const state_machine& s, const size_t i, const size_t j) {
	states[j].out_edges = s.states[i].out_edges;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	One iteration of solving, advancing state machine one step
	and seeing the resulting distribution.  
	\return true if there is more work to do.
 */
bool
state_machine::convolve(const deck_distribution& cards, 
		const probability_vector& init, 
		probability_vector& final) const {
#if DEBUG_SOLVE
cout << "::convolve()" << endl;
#endif
	assert(init.size() == states.size());
	final.resize(init.size());
#if DEBUG_SOLVE
	copy(init.begin(), init.end(),
		std::ostream_iterator<probability_type>(cout, ", "));
	cout << '(' << std::accumulate(init.begin(), init.end(), 0.0)
		<< ')' << endl;
#endif
	std::fill(final.begin(), final.end(), 0.0);
	bool more = false;		// whether or not solution converged
	state_set_type::const_iterator i(states.begin()), e(states.end());
	size_t j = 0;
	for ( ; i!=e; ++i, ++j) {
	if (i->is_terminal()) {
		final[j] += init[j];
	} else if (init[j] > 0.0) {
		more = true;
		const edge_vector::const_iterator
			ob(i->out_edges.begin()), oe(i->out_edges.end());
		edge_vector::const_iterator oi(ob);
		// push-distribute probability
		for ( ; oi!=oe; ++oi) {
			final[*oi] += init[j] *cards[oi-ob];
		}
	}
	}
	return more;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	After convergence, probabilities should be aggregated in the terminal
	states.  
	\param deck the array of out-going edge probabilities.
	\param init the probability distribution of initial states.  
		Must be the same size as number of states.
	\param final the probability distrubtion of final states.  
		Will be resized to the number of states.  
 */
void
state_machine::solve(const deck_distribution& cards, 
		const probability_vector& init, 
		probability_vector& final) const {
#if DEBUG_SOLVE
cout << "::solve()" << endl;
#endif
	assert(init.size() == states.size());
	probability_vector temp(init);
	while (convolve(cards, temp, final)) {
		final.swap(temp);
	}
	// converged, result will end up in final
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
state_machine::check(void) const {
	std::for_each(states.begin(), states.end(), 
		std::mem_fun_ref(&node::check));
}

ostream&
state_machine::dump(ostream& o) const {
	size_t i;
	for (i=0; i<states.size(); ++i) {
		states[i].dump(o << i << '\t') << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace cards

