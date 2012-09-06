// "grader.cc"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <numeric>		// for std::accumulate
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "grader.hh"
#include "player_action.hh"
#include "play_options.hh"
#include "util/string.tcc"	// for string_to_num
#include "util/command.tcc"
#include "util/value_saver.hh"
#include "util/iosfmt_saver.hh"
#include "util/configure_option.hh"

namespace blackjack {
typedef	util::Command<grader>			GraderCommand;
}
namespace util {
template class command_registry<blackjack::GraderCommand>;
}
namespace blackjack {
typedef	util::command_registry<GraderCommand>		grader_command_registry;

using std::vector;
using std::cerr;
using std::endl;
using std::ostringstream;
using std::ostream_iterator;
using std::fill;
using std::accumulate;
using cards::ACE;
using cards::TEN;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::card_index;
using cards::reveal_print_ordering;
using cards::card_value_map;

using util::yn;
using util::value_saver;
using util::Command;
using util::CommandStatus;
using util::string_list;
using util::strings::string_to_num;

//=============================================================================
// class grader method definitions

grader::grader(const variation& v, play_options& p, istream& i, ostream& o) :
		var(v), opt(p), istr(i), ostr(o), play(v),
		basic_strategy(play), dynamic_strategy(play), 
		C(v),
		P(),
		hi_lo_counter("hi-lo", counter(cards::hi_lo_signature)),
		player_hands(), dealer(play), 
		stats(), 
		bookmarks() {
	player_hands.reserve(16);	// to prevent realloc
	const extended_deck_count_type& d(C.get_card_counts());
	P.initialize(d);
	// each call does util::normalize()
	basic_strategy.set_card_distribution(d);
	dynamic_strategy.set_card_distribution(d);
	basic_strategy.evaluate();
	dynamic_strategy.evaluate();	// not really necessary
	stats.initialize_bankroll(100.0);	// default bankroll
	bookmarks.reserve(256);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
grader::~grader() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Should be based on fraction of 10s vs. insurance payoff.
	TODO: notify when wrong, based on count!
	TODO: support bookmarking insurance?
	Is non-const only because of updating statistics.
	TODO: even-money offer is != insurance when payoff != 2:1
		Most 6:5 games will not offer even-money.
 */
bool
grader::offer_insurance(const bool pbj) {
	// if peek_ACE
	const char* prompt = pbj ?  "even-money" : "insurance";
	bool done = false;
	bool buy_insurance = false;
	const double p = C.draw_ten_probability();
	const double o = (1.0 -p)/p;
	const bool advise = var.insurance > o;
	string line;
	do {
	do {
		ostr << prompt << "? [ync?!]: ";
		// istr >> line;
		if (opt.auto_play) {
			ostr << '!' << endl;
			line = "!";
		} else {
			getline(istr, line);
		}
	} while (line.empty() && istr);
	if (line == "n" || line == "N") {
		done = true;
	} else if (line == "y" || line == "Y") {
		buy_insurance = true;
		done = true;
	} else if (line == "c" || line == "C") {
		show_count();
		ostr << "odds: " << o << " : 1" << endl;
	} else if (line == "?" || line == "!") {
		ostr << "odds: " << o << " : 1" << endl;
		ostr << "pay : " << var.insurance << " : 1" << endl;
		ostr << "advise: " << yn(advise) << endl;
		if (line == "!") {
			buy_insurance = advise;
			ostr << (buy_insurance ? "Accepted" : "Declined")
				<< " " << prompt << "." << endl;
			done = true;
		}
	}
	} while (!done);
	// TODO: calculate decision edges
	++stats.decisions_made;
	if (buy_insurance != advise) {
		++stats.dynamic_wrong_decisions;
		if (buy_insurance) {
			++stats.basic_wrong_decisions;
		}
	}
	return buy_insurance;
}	// end grader::offer_insurance

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::show_count(void) const {
	P.show_count(ostr);
	C.show_count(ostr, opt.count_face_cards, opt.count_used_cards);
	// TODO: support generalized counting schemes
	hi_lo_counter.second.dump(ostr,
		hi_lo_counter.first.c_str(), C.get_cards_remaining());
	ostr << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: take into account peek_state_enum, and adjust odds.
	See text section on Peeking.
 */
size_t
grader::draw_up_card(const char* prompt) {
	if (opt.pick_cards && prompt) {
		ostr << prompt << endl;
	}
	const size_t ret = C.option_draw(opt.pick_cards, istr, ostr);
	P.remove(ret);
	hi_lo_counter.second.incremental_count_card(ret);
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
grader::draw_hole_card(const char* prompt) {
	if (opt.pick_cards && prompt) {
		ostr << prompt << endl;
	}
	C.option_draw_hole_card(opt.pick_cards, istr, ostr);
	return C.peek_hole_card();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::reveal_hole_card(const size_t hole_card) {
	dealer.hit_dealer(hole_card);
	// also calls dealer_hand::reveal_hole_card()
	C.reveal_hole_card();	// only now, count the hole card
	switch (dealer.reveal) {
	case ACE:
		if (var.peek_on_Ace) {
			P.reveal_peek_10(hole_card);
		} else {
			P.remove(hole_card);
		}
		break;
	case TEN:
		if (var.peek_on_10) {
			P.reveal_peek_Ace(hole_card);
		} else {
			P.remove(hole_card);
		}
		break;
	default:
		P.remove(hole_card);
	}
	hi_lo_counter.second.incremental_count_card(hole_card);
	// optional:
	dealer.dump_dealer(ostr) << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Hole card was not used.
	If hole card was peeked, leave the peeked-count incremented;
	do not decrement it.  
 */
void
grader::replace_hole_card(void) {
	C.replace_hole_card();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Play a hand of blackjack.
	Player hands, dealer state, hole cards, etc...
	could all just be local variables?
	\return true if reshuffled.
 */
bool
grader::deal_hand(void) {
	if (stats.broke()) {
		ostr << "Out of cash!  Deposit more money or restart." << endl;
		return false;
	}
	ostr << "---------------- new hand ----------------" << endl;
	player_hands.clear();
	if (opt.always_show_count_at_hand) {
		show_count();
	}
	const double& bet(opt.bet);
	double& bankroll(stats.bankroll);

{	// accumulate some statistics
	// TODO: edge stats should be optional, since updating dynamic edge
	// is compute-intensive
	stats.initial_bets += bet;
	stats.total_bets += bet;
	update_dynamic_strategy();
	const edge_type dyn_edge = dynamic_strategy.overall_edge();
	const edge_type basic_edge = basic_strategy.overall_edge();
	const edge_type wdyn_edge = bet *dyn_edge;	// bet-weighted
	const edge_type wbasic_edge = bet *basic_edge;
	// option to print these
	stats.basic_priori.edge_sum += basic_edge;
	stats.basic_priori.weighted_edge_sum += wbasic_edge;
	stats.dynamic_priori.edge_sum += dyn_edge;
	stats.dynamic_priori.weighted_edge_sum += wdyn_edge;
}
	player_hand iph(play);
	player_hands.push_back(iph);
	const size_t p1 = draw_up_card("choose player's first card.");
	const size_t p2 = draw_up_card("choose player's second card.");
	player_hand& pih(player_hands.front());
	const size_t dealer_reveal = draw_up_card("choose dealer's up-card.");
	const size_t dealer_reveal_value = card_value_map[dealer_reveal];
	pih.deal_player(p1, p2, true, dealer_reveal_value);
	dealer.initial_card_dealer(dealer_reveal);
	// TODO: first query whether hold card makes dealer blackjack
	// so prompt for this after checking, if peeking for blackjack
{	// accumulate more statistics after initial deal
	++stats.initial_state_histogram[pih.state][dealer_reveal_value];
	const edge_type post_basic_edge =
		basic_strategy.lookup_pre_peek_initial_edge(
			pih.state, dealer_reveal_value);
	const edge_type post_dynamic_edge =
		dynamic_strategy.lookup_pre_peek_initial_edge(
			pih.state, dealer_reveal_value);
	const edge_type wpost_basic_edge = post_basic_edge *bet;
	const edge_type wpost_dynamic_edge = post_dynamic_edge *bet;
	// option to print these
	stats.basic_posteriori.edge_sum += post_basic_edge;
	stats.basic_posteriori.weighted_edge_sum += wpost_basic_edge;
	stats.dynamic_posteriori.edge_sum += post_dynamic_edge;
	stats.dynamic_posteriori.weighted_edge_sum += wpost_dynamic_edge;
}
	// except for European: no hole card
	// TODO: option-draw hole card 2-stage prompt
	// if peek, ask if dealer has blackjack
	// if not, then draw among remaining non-blackjack cards
	const size_t hole_card = draw_hole_card("choose dealer's hole-card.");
	const size_t hole_card_value = card_value_map[hole_card];
	ostr << "dealer: " << card_name[dealer_reveal] << endl;
	// TODO: if early_surrender (rare) ...
	// prompt for insurance
	const bool pbj = pih.has_blackjack();
	if (pbj) {
		ostr << "Player has blackjack!" << endl;
	}
	// if either player or dealer has blackjack, hand is over
	// TODO: show odds pre-peak and post-peek
	// TODO: consult outcome matrix for bj vs. bj (variations)
	bool end = pbj;
	if (dealer_reveal == ACE) {
		pih.dump_player(ostr) << endl;
		if (var.peek_on_Ace) {
			P.peek_not_10();
		const bool buy_insurance = offer_insurance(pbj);;
		// TODO: handle even-money bet for !2:1 payoffs
		// determine change in bankroll
		// check for blackjack for player
		const double half_bet = bet *0.5;
		if (hole_card_value == TEN) {
			end = true;
			reveal_hole_card(hole_card);
			ostr << "Dealer has blackjack." << endl;
			if (buy_insurance) {
				bankroll += var.insurance *half_bet;
			}
			if (!pbj) {
				bankroll -= bet;
			} else {
				ostr << "Push." << endl;
			}
		} else {
			ostr << "No dealer blackjack." << endl;
			// hole card is known to be NOT a 10
			dealer.peek_no_10();
			if (buy_insurance) {
				bankroll -= half_bet;
			}
			if (pbj) {
				reveal_hole_card(hole_card);
				bankroll += var.bj_payoff *bet;
			}
			// else keep playing
			// hole card is known to be NOT a TEN
		}
		}	// peek_on_Ace
	} else if (dealer_reveal_value == TEN) {
		pih.dump_player(ostr) << endl;
		if (var.peek_on_10) {
			P.peek_not_Ace();
		if (hole_card == ACE) {
			end = true;
			reveal_hole_card(hole_card);
			ostr << "Dealer has blackjack." << endl;
			if (!pbj) {
				bankroll -= bet;
			} else {
				ostr << "Push." << endl;
			}
		} else {
			ostr << "No dealer blackjack." << endl;
			// hole card is known to be NOT an ACE
			dealer.peek_no_Ace();
			if (pbj) {
				reveal_hole_card(hole_card);
				bankroll += var.bj_payoff *bet;
			}
		}
		}	// peek_on_10
	} else if (pbj) {
		// pay off player's blackjack
		pih.dump_player(ostr) << endl;
		reveal_hole_card(hole_card);	// calls dump_dealer
		bankroll += var.bj_payoff *bet;
	}
	// play ends if either dealer or player had blackjack (and was peeked)
if (!end) {
	size_t j = 0;
	bool live = false;
	for ( ; j<player_hands.size(); ++j) {
		if (play_out_hand(j)) {
			live = true;
		}
	}
if (live || !opt.dealer_plays_only_against_live) {
	ostr << "Dealer plays..." << endl;
	bool dealer_took_card = false;
	// dealer plays after player is done
	// should dealer play when there are no live hands?
	reveal_hole_card(hole_card);	// calls dump_dealer
	while (!play.is_dealer_terminal(dealer.state)) {
		dealer.hit_dealer(draw_up_card("choose dealer's next card."));
		dealer_took_card = true;
	}
	if (dealer_took_card) {
		dealer.dump_dealer(ostr) << endl;
	}	// otherwise dealer stands on first two cards
} else {
	// hole card is never revealed and thus not counted
	// treat as if hole card is replaced into shoe
	// this could happen if player surrendered, or busted.
	replace_hole_card();
	// FIXME: this is not truly mathematically accurate, 
	// since peeking for blackjack gives some partial information
	// about that hole card, and it is being replaced into the deck!
	// Cannot just 'discard' it from the count either,
	// for the same reason.
}
	// suspense double-down?  nah
	const double bet2 = var.double_multiplier *bet;	// winning
	stats.total_bets -= bet;
for (j=0; j<player_hands.size(); ++j) {
	dump_situation(j);
	stats.total_bets += bet;
	if (player_hands[j].player_live()) {
	if (player_hands[j].doubled_down()) {
		stats.total_bets += bet;
	}
	const outcome&
		wlp(play.lookup_outcome(player_hands[j].state, dealer.state));
	const double delta = player_hands[j].doubled_down() ? bet2 : bet;
	switch (wlp) {
	case WIN: ostr << "Player wins." << endl; bankroll += delta; break;
	case LOSE: ostr << "Player loses." << endl; bankroll -= delta; break;
	case PUSH: ostr << "Push." << endl; break;
	}	// end switch
	}	// already paid surrender penalty
}	// end for
}
	++stats.hands_played;
	stats.compare_bankroll();
	if (opt.quiz_count_frequency &&	// don't divide by 0
			!(stats.hands_played % opt.quiz_count_frequency)) {
		quiz_count();
	}
	const bool ret = auto_shuffle();
	if (opt.always_show_status) {
		status(ostr);
	}
	if (opt.always_show_dynamic_edge && !ret) {
		// don't bother if just shuffled
		show_dynamic_edge();
	}
	// update bet min/max watermark
	return ret;
}	// end grader::deal_hand


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	After actual deck_state is modified (non-incrementally)
	need to reset counts.  
 */
void
grader::reset_counts(void) {
	P.initialize(C.get_card_counts());
	hi_lo_counter.second.initialize(P.get_counts());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
grader::auto_shuffle(void) {
	bool b = false;
	if (opt.continuous_shuffle) {
		C.reshuffle();
		b = true;
	} else {
		b = C.needs_shuffle();
		if (b && opt.quiz_count_before_shuffle) {
			quiz_count();
		}
		C.reshuffle_auto();
	}
	if (b) {
		ostr << "**************** Reshuffling... ****************"
			<< endl;
		reset_counts();
		if (opt.always_show_dynamic_edge && !opt.continuous_shuffle) {
			// don't bother if shuffling is continuous
			show_dynamic_edge();
		}
	}
	return b;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::quiz_count(void) const {
if (!opt.auto_play) {
	// TODO: quiz different counting schemes
	string line;
	ostr << "Quiz: " << hi_lo_counter.first << " running count? ";
	getline(istr, line);
	int ans;
	if (string_to_num(line, ans)) {
		// for any other response, just show count
//		C.show_count(ostr, false, false);
		P.show_count(ostr);
	} else {
		if (ans == hi_lo_counter.second.get_running_count()) {
			ostr << "Correct." << endl;
		} else {
			ostr << "Incorrect." << endl;
//			C.show_count(ostr, false, false);
			P.show_count(ostr);
		}
	}
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::show_dynamic_edge(void) {
	update_dynamic_strategy();
	ostr << "player's edge (dynamic): " <<
		dynamic_strategy.overall_edge() << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Handles player single action.
 */
void
grader::handle_player_action(const size_t j, const player_choice pc) {
	++stats.decisions_made;
	player_hand& ph(player_hands[j]);
switch (pc) {
	case STAND:
		ph.stand();
		break;
	case DOUBLE:
		ph.double_down(draw_up_card("choose player's double-down card."));
		break;
	case HIT:
		// TODO: prompt if pick_cards
		ph.hit_player(draw_up_card("choose player's next card."));
		// don't bother prompting if hits 21 (auto-stand)
		break;
	case SPLIT: {
		player_hand nh(play);	// new hand
		// avoid sequence-point
		const size_t s1 = draw_up_card("choose player's second card.");
		const size_t s2 = draw_up_card("choose player's second card.");
		ph.split(nh, s1, s2, card_value_map[dealer.reveal]);
		player_hands.push_back(nh);
		// show new hands and other split hand for counting purposes
		dump_situation(j);
		dump_situation(player_hands.size() -1);
		// TODO: check for special case of split-Aces
		// auto-stand both hands
		ostr << "back to player hand " << j+1 << endl;
		break;
	}
	case SURRENDER:
		// recall surrender_penalty is positive
		stats.bankroll -= opt.bet *var.surrender_penalty;
		ph.surrender();
		break;
	default: break;
}	// end switch
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Plays out player hand until finished.
	\param j player hand index (in case of splits)
	\return true if player's hand is still live
		(for showdown against dealer).
 */
bool
grader::play_out_hand(const size_t j) {
	player_hand& ph(player_hands[j]);
	// caution, reference may dangle after a vector::push_back
	// player may resplit hands
	player_choice pc = NIL;
do {
	do {
	// TODO: check for resplit limit from variation
	action_mask& m(ph.player_options);
	if (player_hands.size() >= player_hands.capacity()) {
		m -= SPLIT;
	}
	// only first hand is surrenderrable, normally
	dump_situation(j);
	if (opt.always_show_count_at_action) {
		show_count();
	}
	// snapshot of current state, in case of bookmark
	const bookmark bm(dealer.reveal, ph, P);
	ostringstream oss;
	oss.flags(ostr.flags());
	oss.precision(ostr.precision());
	const pair<expectations, expectations>
		ace(assess_action(ph.state, card_value_map[dealer.reveal],
			oss, m));
	const pair<player_choice, player_choice>
		acp(ace.first.best(m), ace.second.best(m));
	const player_choice ac =
		opt.use_dynamic_strategy ? acp.second : acp.first;
	const player_choice ac2 =	// the other one
		opt.use_dynamic_strategy ? acp.first : acp.second;
	if (opt.always_suggest) {
		ostr << oss.str() << endl;
	}
	pc = ph.player_options.prompt(istr, ostr, opt.auto_play);
	bool bookmarked = false;
	bool notified = false;
	bool detailed = false;
	if (acp.second != acp.first) {
	// basic != dynamic strategy (and is thus, interesting)
		if (opt.notify_dynamic) {
			ostr << "*** dynamic strategy beats basic ***" << endl;
			notified = true;
		}
		if (opt.bookmark_dynamic && !bookmarked) {
			bookmarks.push_back(bm);
			bookmarked = true;
		}
	}
	if (notified || (pc == HINT)) {
		// count needs to be shown before card is drawn
		// same with auto bookmarks
		if (opt.notify_with_count) {
			show_count();
		}
		ostr << oss.str() << endl;
		detailed = true;
	}
	switch (pc) {
		// all fall-through to handle_player_action()
	case STAND:
	case DOUBLE:
	case HIT:
	case SPLIT: 
	case SURRENDER: {
		// if matches dynamic optimum, don't count against basic
		if (pc != acp.second) {
			++stats.dynamic_wrong_decisions;
		if (pc != acp.first) {
			++stats.basic_wrong_decisions;
		}
		}
		if (pc != ac) {
		// player's choice was wrong, sub-optimal
		if (opt.notify_when_wrong) {
			ostr << "*** optimal choice was: " <<
				action_names[ac] << endl;
			if (pc == ac2) {
				ostr << "*** but " << action_names[ac2]
					<< " is also acceptable from " <<
					(opt.use_dynamic_strategy ? "basic" : "dynamic")
					<< " strategy." << endl;
			}
			if (!detailed) {
				if (opt.notify_with_count) {
					show_count();
				}
				ostr << oss.str() << endl;
				detailed = true;
			}
		}
		if (opt.bookmark_wrong && !bookmarked) {
			bookmarks.push_back(bm);
			bookmarked = true;
		}
		}
		handle_player_action(j, pc);
		break;
	}
	case BOOKMARK:
		if (!bookmarked) {
			bookmarks.push_back(bm);
			bookmarked = true;
		}
		break;
	case COUNT:
		show_count();
		break;
#if 0
	// handled above
	case HINT:
		ostr << oss.str() << endl;
		break;
#endif
	case OPTIM: {
		if (opt.show_edges && !detailed) {
			ostr << oss.str() << endl;
		}
		ostr << "auto: " << action_names[ac] << endl;
		handle_player_action(j, ac);
		break;
	}
	default: break;
	}	// end switch(pc)
	if (bookmarked) {
		ostr << "Bookmark saved." << endl;
	}
	} while (ph.player_prompt());
} while (ph.player_prompt());
	if (ph.player_busted()) {
		// just in case player doubles-down and busts!
		stats.bankroll -= ph.doubled_down() ? opt.bet *2.0 : opt.bet;
		ph.dump_player(ostr) << endl;	// show final state
	}
	return ph.player_live();
}	// end grader::play_out_hand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Re-computes card-count-based strategy.
	\param ps the player's state index
	\param dlr the dealer's reveal card
	\param d, p, r, whether or not double,split,surrender are allowed
	\return best choice based on basic (first)
		and dynamic strategy (second)
	TODO: compute exact strategy here
 */
pair<expectations, expectations>
grader::assess_action(const size_t ps, const size_t dlr, ostream& o,
		const action_mask& m) {
	// basic:
	const expectations& be(basic_strategy
		.lookup_player_action_expectations(ps, dlr));
	// ostr << "edges per action (basic):" << endl;
	// be.dump_choice_actions(ostr, -var.surrender_penalty);
	const pair<player_choice, player_choice>
		br(be.best_two(m));
	// ostr << "basic strategy recommends: " <<
	//	action_names[br.first] << endl;
	// dynamic: need to recompute given recently seen cards
	// show count?
	update_dynamic_strategy();	// evaluate when needed
	
	const expectations& de(dynamic_strategy
		.lookup_player_action_expectations(ps, dlr));
	// ostr << "edges per action (dynamic):" << endl;
	// de.dump_choice_actions(ostr, -var.surrender_penalty);
	const pair<player_choice, player_choice>
		dr(de.best_two(m));
	// ostr << "dynamic strategy recommends: " <<
	//	action_names[dr.first] << endl;
#if 0
	// if computing exact strategy
	expectations ee;
	if (opt.notify_exact) {
		// TODO: compute exact edge
		card_state cc(C);	// copy
		compute_exact_strategy(ps, dlr, d, p, r, cc, ee);
	}
#endif
if (opt.show_edges) {
	o << "\tedges:\tbasic\tdynamic" << endl;
	// TODO: _3
	expectations::dump_choice_actions_2(o,
		be, de, -var.surrender_penalty, m, "\t");
}
	o << "\tadvise:\t" << action_names[br.first]
		<< '\t' << action_names[dr.first];
	if (br.first != dr.first) {
		// alert when dynamic strategy is different from basic
		o << "\t(different! using " <<
			(opt.use_dynamic_strategy ? "dynamic" : "basic")
			<< ")";
	}
//	o << endl;
	return std::make_pair(be, de);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: for evaluating a situation only update calculation
	for the given delear reveal card.
 */
void
grader::update_dynamic_strategy(void) {
	// only now do we need to update probabilities
	dynamic_strategy.set_card_distribution(C.get_card_counts());
	dynamic_strategy.evaluate();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::dump_situation(const size_t j) const {
	assert(j < player_hands.size());
	ostr << '[' << j+1 << '/' << player_hands.size() << "] ";
	if (dealer.cards.size() > 1) {
		dealer.dump_dealer(ostr);
	} else {
		ostr << "dealer: " << card_name[dealer.reveal];
	}
	ostr << ", ";
	player_hands[j].dump_player(ostr) << endl;
	return ostr;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::status(ostream& o) const {
	const util::precision_saver p(o, 2);
	o << "bankroll: " << stats.bankroll << ", bet: " << opt.bet << endl;
	// deck remaning (%)
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::list_bookmarks(void) const {
	ostr << "-------- bookmarks --------" << endl;
	vector<bookmark>::const_iterator
		i(bookmarks.begin()), e(bookmarks.end());
	for ( ; i!=e; ++i) {
//		i->dump(ostr) << endl;
		i->dump(ostr, hi_lo_counter.first.c_str(), 
			hi_lo_counter.second) << endl;
	}
	ostr << "------ end bookmarks ------" << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::clear_bookmarks(void) {
	bookmarks.clear();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
grader::main(void) {
	ostr <<
"You sit down at a blackjack table.\n"
"Type 'help' or '?' for a list of table commands." << endl;
	const value_saver<string>
		tmp1(grader_command_registry::prompt, "table> ");
	grader_command_registry::interpret(*this);
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace grader_commands {
#define	DECLARE_GRADER_COMMAND_CLASS(class_name, _cmd, _brief)		\
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(grader, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Help, "help",
	"[cmd] : list table command(s)")
int
Help::main(grader& g, const string_list& args) {
	ostream& cout(g.ostr);
switch (args.size()) {
case 1:
	grader_command_registry::list_commands(cout);
	break;
default: {
	string_list::const_iterator i(++args.begin()), e(args.end());
	for ( ; i!=e; ++i) {
		if (!grader_command_registry::help_command(cout, *i)) {
			cout << "Command not found: " << *i << endl;
		} else {
			cout << endl;
		}
	}
}
}
	return CommandStatus::NORMAL;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Help2, "?",
	"[cmd] : list table command(s)")
int
Help2::main(grader& v, const string_list& args) {
	return Help::main(v, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Quit, "quit", ": leave table, return to lobby")
int
Quit::main(grader& g, const string_list&) {
	const double b = g.get_bankroll();
	if (!g.stats.broke()) {
		g.ostr << "You collect your remaining chips from the table ("
			<< b << ") and return to the lobby." << endl;
	} else {
		g.ostr << "Better luck next time!" << endl;
	}
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Exit, "exit", ": leave table, return to lobby")
int
Exit::main(grader& g, const string_list& args) {
	return Quit::main(g, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Echo, "echo", ": print arguments")
int
Echo::main(grader& g, const string_list& args) {
	copy(++args.begin(), args.end(), ostream_iterator<string>(g.ostr, " "));
	g.ostr << endl;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Variation, "variation", ": show rule variations")
int
Variation::main(grader& g, const string_list& args) {
if (args.size() > 1) {
	g.ostr << "error: cannot change rules at the table." << endl;
	return CommandStatus::BADARG;
} else {
	// can't modify/configure here
	g.var.dump(g.ostr);
	return CommandStatus::NORMAL;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(PlayOptions, "options",
	": (menu) show/set game play options")
int
PlayOptions::main(grader& g, const string_list& args) {
if (args.size() <= 1) {
	g.opt.configure();	// enter menu
	return CommandStatus::NORMAL;
} else {
	// execute a single play_option command
	const string_list rem(++args.begin(), args.end());
	return g.opt.command(rem);
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Bet, "bet", "[amt] : change/show bet amount")
DECLARE_GRADER_COMMAND_CLASS(Bet2, "Bet", "[amt] : change/show bet amount")
int
Bet2::main(grader& g, const string_list& args) {
	return Bet::main(g, args);
}
int
Bet::main(grader& g, const string_list& args) {
	const double br = g.get_bankroll();
	double& bet(g.opt.bet);
switch (args.size()) {
case 1: break;
case 2: {
	const string& amt(*++args.begin());
	double nb;
	// or less than min
	if (string_to_num(amt, nb)) {
		cerr << "Error: invalid quantity." << endl;
		return CommandStatus::BADARG;
	}
	if (nb < 0.0) {
		cerr << "Error: bet must be positive." << endl;
		return CommandStatus::BADARG;
	}
	if (nb > br) {
		cerr << "Error: bet cannot exceed bankroll." << endl;
		return CommandStatus::BADARG;
	}
	// TODO: enforce table min/max
	bet = nb;
	break;
}
default:
	cerr << "usage: " << name << ' ' << brief << endl;
	return CommandStatus::SYNTAX;
}
	g.status(g.ostr);
	return CommandStatus::NORMAL;
}
// can't configure here

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Deposit, "deposit",
		"[amount] : add money to bankroll")
int
Deposit::main(grader& g, const string_list& args) {
switch (args.size()) {
case 2: {
	double diff;
	if (string_to_num(args.back(), diff)) {
		g.ostr << "Error: invalid real value." << endl;
		return CommandStatus::BADARG;
	}
	if (diff < 0.0) {
		cerr << "Error: value must be >= 0.0" << endl;
		return CommandStatus::BADARG;
	}
	g.stats.deposit(diff);
	break;
}
default:
	g.ostr << "usage: " << name << ' ' << brief << endl;
	return CommandStatus::SYNTAX;
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Status, "status", ": show bankroll and bet amount")
int
Status::main(grader& g, const string_list&) {
	g.status(g.ostr);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Count, "count", ": show card counts")
DECLARE_GRADER_COMMAND_CLASS(Count2, "Count", ": show card counts")
int
Count::main(grader& g, const string_list&) {
	g.show_count();
	return CommandStatus::NORMAL;
}
int
Count2::main(grader& g, const string_list& a) {
	return Count::main(g, a);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Shuffle, "shuffle", ": shuffle deck")
int
Shuffle::main(grader& g, const string_list&) {
	// optional quiz count here?
	g.shuffle();
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Also begin with 'D' to more easily disambiguate from other 
	d-commands.
 */
DECLARE_GRADER_COMMAND_CLASS(Deal, "deal", "[N]: deal next hand(s)")
DECLARE_GRADER_COMMAND_CLASS(Deal2, "Deal", "[N]: deal next hand(s)")
int
Deal::main(grader& g, const string_list& args) {
	size_t N = 1;
if (args.size() > 1) {
	if (string_to_num(args.back(), N)) {
		g.ostr << "Error: invalid count." << endl;
		return CommandStatus::BADARG;
	}
}
	size_t i = 0;
	for ( ; i<N; ++i) {
		g.deal_hand();
		// if broke, exit early
		if (g.stats.broke()) {
			g.ostr << "Out of cash";
			if (N > 1) {
				g.ostr << " after " << i+1 << " hands";
			}
			g.ostr << '!' << endl;
		}
	}
	return CommandStatus::NORMAL;
}

int
Deal2::main(grader& g, const string_list& args) {
	return Deal::main(g, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(DealUntilShuffle, "deal-until-shuffle",
	": keep dealing hands until deck is reshuffled")
int
DealUntilShuffle::main(grader& g, const string_list&) {
	while (!g.deal_hand() && !g.stats.broke()) { }
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(AutoPlay, "auto-play", 
	"[N] : automatically play optimally N hands")
int
AutoPlay::main(grader& g, const string_list& args) {
	const value_saver<bool> __tmp__(g.opt.auto_play, true);
	return Deal::main(g, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(AutoPlayUntilShuffle, "auto-play-until-shuffle", 
	"[N] : automatically play until next reshuffle")
int
AutoPlayUntilShuffle::main(grader& g, const string_list& args) {
	const value_saver<bool> __tmp__(g.opt.auto_play, true);
	return DealUntilShuffle::main(g, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: introduce a bunch of in-strategy commands
DECLARE_GRADER_COMMAND_CLASS(Basic, "basic-strategy",
	"[-edge|-verbose]: show basic strategy")
int
Basic::main(grader& g, const string_list& args) {
	const strategy& b(g.get_basic_strategy());
switch (args.size()) {
case 1: return b.main("basic> "); break;
default:
	const string_list a(++args.begin(), args.end());
	return b.command(a);
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Dynamic, "dynamic-strategy",
	"[-edge|-verbose]: show dynamic strategy")
int
Dynamic::main(grader& g, const string_list& args) {
	g.update_dynamic_strategy();
	const strategy& b(g.get_dynamic_strategy());
switch (args.size()) {
case 1: return b.main("dynamic> "); break;
default:
	const string_list a(++args.begin(), args.end());
	return b.command(a);
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This generic command should be available from several contexts.
 */
DECLARE_GRADER_COMMAND_CLASS(Seed, "seed",
	"[time|int int int] : show/set random seed")
int
Seed::main(grader& g, const string_list& args) {
	typedef	unsigned short		ushort;
const size_t sz = args.size();
switch (sz) {
case 1: {
	ushort sd[3] = {0, 0, 0};
	// grab seed (destructive)
	const ushort* tmp = seed48(sd);
	sd[0] = tmp[0];
	sd[1] = tmp[1];
	sd[2] = tmp[2];
	g.ostr << "seed48 = " << sd[0] << ' ' << sd[1] << ' ' << sd[2] << endl;
	// restore it
	seed48(sd);
	break;
}
case 2: {
	if (args.back() == "time") {
		const time_t tm = std::time(NULL);	// Epoch time
		const ushort mask(-1);
		ushort sd[3] = {0, 0, 0};
		sd[0] = tm & mask;
		sd[1] = (tm >> 16) & mask;
		// if (sizeof(time_t) > 4) {
		// sd[2] = (tm >> 32) & mask;
		sd[2] = (tm >> 8) & mask;
		// }
		g.ostr << "seed48 = " << sd[0] << ' ' << sd[1] << ' ' << sd[2] << endl;
		seed48(sd);
	} else {
		cerr << "usage: " << name << " " << brief << endl;
		return CommandStatus::BADARG;
	}
	break;
}
case 4: {
	ushort sd[3];
	string_list::const_iterator i(args.begin());
	// pre-increment to skip first command token
	if (string_to_num(*++i, sd[0]) ||
		string_to_num(*++i, sd[1]) ||
		string_to_num(*++i, sd[2])) {
		cerr << "usage: " << name << " " << brief << endl;
		return CommandStatus::BADARG;
	}
	seed48(sd);
	break;
}
default:
	cerr << "usage: " << name << " " << brief << endl;
	return CommandStatus::SYNTAX;
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Statistics, "stats",
	": show game play statistics")
int
Statistics::main(grader& g, const string_list&) {
	g.stats.dump(g.ostr, g.get_play_map());
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(EditDeck, "edit-deck",
	"[all | card [+|-] N] : alter card distribution")
int
EditDeck::main(grader& g, const string_list& args) {
	deck_state& C(g.get_deck_state());
	perceived_deck_state& P(g.get_perceived_deck_state());
switch (args.size()) {
case 1: Count::main(g, args);	// just show count
	g.ostr << "usage: " << name << " " << brief << endl;
	break;
case 2: {
	// interactively edit all quantities
	Count::main(g, args);		// first show all
	if (args.back() == "all") {
		C.edit_deck_all(g.istr, g.ostr);	// exit status?
		g.reset_counts();
		return Count::main(g, args);	// show all once again
	} else {
		g.ostr << "usage: " << name << " " << brief << endl;
		return CommandStatus::BADARG;
	}
	break;
}
case 3: {
	// set number
	string_list::const_iterator i(++args.begin());
	const string& cn(*i++);		// card name [2-9AT]
	const string& qs(*i);		// quantity
	const size_t ci = card_index(cn[0]);
	int q;
	if (string_to_num(qs, q)) {
		g.ostr << "error: invalid quantity" << endl;
		return CommandStatus::BADARG;
	}
	if (C.edit_deck(ci, q)) {
		return CommandStatus::BADARG;
	}
}
case 4: {
	// add or subtract
	string_list::const_iterator i(++args.begin());
	const string& cn(*i++);		// card name [2-9AT]
	const string& sn(*i++);		// [+-=]
	const string& qs(*i);		// quantity
	const size_t ci = card_index(cn[0]);
	int q;
	if (string_to_num(qs, q)) {
		g.ostr << "error: invalid quantity" << endl;
		return CommandStatus::BADARG;
	}
	switch (sn[0]) {
	case '+':
		if (C.edit_deck_add(ci, q)) {
			return CommandStatus::BADARG;
		}
		break;
	case '-':
		if (C.edit_deck_sub(ci, q)) {
			return CommandStatus::BADARG;
		}
		break;
	case '=':
		if (C.edit_deck(ci, q)) {
			return CommandStatus::BADARG;
		}
		break;
	default:
		cerr << "error: invalid operator, expecting [+-=]" << endl;
		return CommandStatus::BADARG;
	}
	break;
}
default: cerr << "usage: " << name << " " << brief << endl;
	return CommandStatus::SYNTAX;
}	// end switch
	g.reset_counts();
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(BookmarksClear, "bookmarks-clear",
	": erase all saved bookmarks")
int
BookmarksClear::main(grader& g, const string_list& args) {
	g.clear_bookmarks();
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(BookmarksList, "bookmarks-list",
	": print all saved bookmarks")
int
BookmarksList::main(grader& g, const string_list& args) {
	g.list_bookmarks();
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// bookmarks-export (save)
// bookmarks-import (load)
// bookmarks-review (quiz)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: auto-play a whole hand, or N hands, or until some condition...
// TODO: auto-bet amount based on some criteria

#undef	DECLARE_GRADER_COMMAND_CLASS
}	// end namespace grader_commands
//=============================================================================
}	// end namespace blackjack

