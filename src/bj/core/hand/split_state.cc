// "bj/core/hand/split_state.cc"

#include <iostream>
#include <vector>
#include <map>				// for map<split_state>
#include <set>				// for set<split_state>
#include <sstream>
#include "bj/core/hand/split_state.hh"
#include "bj/core/num.hh"

namespace blackjack {
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::string;
using cards::state_type;
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
}	// end namespace blackjack

