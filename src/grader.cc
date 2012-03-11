// "grader.cc"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <numeric>		// for std::accumulate
#include <cstdio>
#include <cstdlib>
#include "grader.hh"
#include "play_options.hh"
#include "util/string.tcc"	// for string_to_num
#include "util/command.tcc"
#include "util/value_saver.hh"

namespace blackjack {
typedef	util::Command<grader>			GraderCommand;
}
namespace util {
template class command_registry<blackjack::GraderCommand>;
}
namespace blackjack {
typedef	util::command_registry<GraderCommand>		grader_command_registry;

using std::cerr;
using std::endl;
using std::ostringstream;
using std::ostream_iterator;
using std::fill;
using std::setw;
using std::accumulate;
using cards::ACE;
using cards::TEN;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::card_index;
using cards::reveal_print_ordering;

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
		player_hands(), dealer_hand(play), 
		stats() {
	player_hands.reserve(16);	// to prevent realloc
	const deck_count_type& d(C.get_card_counts());
	// each call does util::normalize()
	basic_strategy.set_card_distribution(d);
	dynamic_strategy.set_card_distribution(d);
	basic_strategy.evaluate();
	dynamic_strategy.evaluate();	// not really necessary
	stats.initialize_bankroll(100.0);	// default bankroll
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
grader::~grader() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: automatic dynamic strategy decision, to account for count.
	Should be based on fraction of 10s vs. insurance payoff.
 */
bool
grader::offer_insurance(const bool pbj) const {
	// if peek_ACE
	const char* prompt = pbj ?  "even-money?" : "insurance?";
	bool done = false;
	bool buy_insurance = false;
	const double p = C.draw_ten_probability();
	const double o = (1.0 -p)/p;
	const bool advise = var.insurance > o;
	string line;
	do {
	do {
		ostr << prompt << " [ync?!]: ";
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
		C.show_count(ostr);
		ostr << "odds: " << o << " : 1" << endl;
	} else if (line == "?" || line == "!") {
		ostr << "odds: " << o << " : 1" << endl;
		ostr << "pay : " << var.insurance << " : 1" << endl;
		ostr << "advise: " << (advise ? "yes" : "no") << endl;
		if (line == "!") {
			buy_insurance = advise;
			ostr << (buy_insurance ? "Accepted" : "Declined")
				<< " insurance." << endl;
			done = true;
		}
	}
	} while (!done);
	return buy_insurance;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
grader::draw_up_card(void) {
	return C.option_draw(opt.pick_cards, istr, ostr);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
grader::draw_hole_card(void) {
	C.option_draw_hole_card(opt.pick_cards, istr, ostr);
	return C.peek_hole_card();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::reveal_hole_card(const size_t hole_card) {
	dealer_hand.hit_dealer(hole_card);
	C.reveal_hole_card();	// only now, count the hole card
	// optional:
	dealer_hand.dump_dealer(ostr) << endl;
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
	ostr << "---------------- new hand ----------------" << endl;
	player_hands.clear();
	if (opt.always_show_count_at_hand) {
		C.show_count(ostr);
	}
	const double& bet(opt.bet);
	double& bankroll(stats.bankroll);
	stats.initial_bets += bet;
	stats.total_bets += bet;
	const bool pick_cards = opt.pick_cards;
	if (pick_cards) {
		ostr << "choose player cards." << endl;
	}
	hand iph(play);
	player_hands.push_back(iph);
	const size_t p1 = draw_up_card();
	const size_t p2 = draw_up_card();
	hand& pih(player_hands.front());
	pih.deal_player(p1, p2, true);
	if (pick_cards) {
		ostr << "choose dealer up-card." << endl;
	}
	dealer_reveal = draw_up_card();
	dealer_hand.initial_card_dealer(dealer_reveal);
	// TODO: first query whether hold card makes dealer blackjack
	// so prompt for this after checking, if peeking for blackjack
	if (pick_cards) {
		ostr << "choose dealer hole-card." << endl;
	}
	++stats.initial_state_histogram[pih.state][dealer_hand.state];
	// except for European: no hole card
	// TODO: option-draw hole card 2-stage prompt
	// if peek, ask if dealer has blackjack
	// if not, then draw among remaining non-blackjack cards
	const size_t hole_card = draw_hole_card();
	ostr << "dealer: " << card_name[dealer_reveal] << endl;
	// TODO: if early_surrender (rare) ...
	// prompt for insurance
	const bool pbj = pih.has_blackjack();
	if (pbj) {
		ostr << "Player has blackjack!" << endl;
	}
	// if either player or dealer has blackjack, hand is over
	// TODO: show odds pre-peak and post-peek
	bool end = pbj;
	if (dealer_reveal == ACE) {
		pih.dump_player(ostr) << endl;
		if (var.peek_on_Ace) {
		const bool buy_insurance = offer_insurance(pbj);;
		// determine change in bankroll
		// check for blackjack for player
		const double half_bet = bet *0.5;
		if (hole_card == TEN) {
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
			if (buy_insurance) {
				bankroll -= half_bet;
			}
			if (pbj) {
				bankroll += var.bj_payoff *bet;
			}
			// else keep playing
			// hole card is known to be NOT a TEN
		}
		}	// peek_on_Ace
	} else if (dealer_reveal == TEN) {
		pih.dump_player(ostr) << endl;
		if (var.peek_on_10) {
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
		}
		}	// peek_on_10
	} else if (pbj) {
		// pay off player's blackjack
		// end = true;
		pih.dump_player(ostr) << endl;
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
	while (!play.is_dealer_terminal(dealer_hand.state)) {
		dealer_hand.hit_dealer(draw_up_card());
		dealer_took_card = true;
	}
	if (dealer_took_card) {
		dealer_hand.dump_dealer(ostr) << endl;
	}	// otherwise dealer stands on first two cards
} else {
	// hole card is never revealed and thus not counted
	// treat as if hole card is replaced into shoe
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
		// check for surrender could/should be outside this loop
	const outcome& wlp(play_map::outcome_matrix
		[play_map::player_final_state_map(player_hands[j].state)]
		[dealer_hand.state -stop]);
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
	const bool ret = auto_shuffle();
	if (opt.always_show_status) {
		status(ostr);
	}
//	only update_dynamic_strategy() when needed or requested
	// update bet min/max watermark
	return ret;
}	// end grader::deal_hand


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
grader::auto_shuffle(void) {
	bool b = false;
	if (opt.continuous_shuffle) {
		C.reshuffle();
		b = true;
	} else {
		b = C.reshuffle_auto();
	}
	if (b) {
		ostr << "**************** Reshuffling... ****************"
			<< endl;
	}
	return b;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Handles player single action.
 */
void
grader::handle_player_action(const size_t j, const player_choice pc) {
	hand& ph(player_hands[j]);
switch (pc) {
	case STAND:
		ph.stand();
		break;
	case DOUBLE:
		ph.double_down(draw_up_card());
		break;
	case HIT:
		ph.hit_player(draw_up_card());
		// don't bother prompting if hits 21 (auto-stand)
		break;
	case SPLIT: {
		const size_t split_card = ph.state - pair_offset;
		// ph.presplit();
		// 21 should not be considered blackjack when splitting
		ph.deal_player(split_card, draw_up_card(), false);
		hand nh(play);	// new hand
		nh.deal_player(split_card, draw_up_card(), false);
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
	hand& ph(player_hands[j]);
	// caution, reference may dangle after a vector::push_back
	// player may resplit hands
	player_choice pc = NIL;
do {
	do {
	// these predicates can be further refined by
	// variation/rules etc...
	const bool d = ph.doubleable();
	// TODO: check for double_after_split and other limitations
	const bool p = ph.splittable() &&
		(player_hands.size() < player_hands.capacity());
	// TODO: check for resplit limit
	const bool r = ph.surrenderable() && !already_split();
	// only first hand is surrenderrable, normally
	dump_situation(j);
	if (opt.always_show_count_at_action) {
		C.show_count(ostr);
	}
	ostringstream oss;
	const pair<expectations, expectations>
		ace(assess_action(ph.state, dealer_reveal, oss, d, p, r));
	const pair<player_choice, player_choice>
		acp(ace.first.best(d, p, r), ace.second.best(d, p, r));
	const player_choice ac =
		opt.use_dynamic_strategy ? acp.second : acp.first;
	const player_choice ac2 =
		opt.use_dynamic_strategy ? acp.first : acp.second;
	if (opt.always_suggest) {
		ostr << oss.str() << endl;
	}
	pc = prompt_player_action(istr, ostr, d, p, r, opt.auto_play);
	switch (pc) {
		// all fall-through to handle_player_action()
	case STAND:
	case DOUBLE:
	case HIT:
	case SPLIT: 
	case SURRENDER:
		handle_player_action(j, pc);
		if (opt.notify_when_wrong) {
		if (pc != ac) {
			ostr << "optimal choice was: " <<
				action_names[ac] << endl;
			if (pc == ac2) {
				ostr << "but " << action_names[ac2]
					<< " is also acceptable." << endl;
			}
			// TODO: show edges
		}
		}
		break;
	case COUNT:
		C.show_count(ostr);
		break;
	case HINT:
		ostr << oss.str() << endl;
		break;
	case OPTIM: {
		ostr << oss.str() << endl;	// optional?
		ostr << "auto: " << action_names[ac] << endl;
		handle_player_action(j, ac);
		break;
	}
	default: break;
	}	// end switch
	} while (ph.player_prompt());
} while (ph.player_prompt());
	if (ph.player_busted()) {
		// just in case player doubles-down and busts!
		stats.bankroll -= ph.doubled_down() ? opt.bet *2.0 : opt.bet;
		ph.dump_player(ostr) << endl;	// show final state
	}
	return ph.player_live();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Re-computes card-count-based strategy.
	\param ps the player's state index
	\param dlr the dealer's reveal card
	\param d, p, r, whether or not double,split,surrender are allowed
	\return best choice based on basic (first)
		and dynamic strategy (second)
 */
pair<expectations, expectations>
grader::assess_action(const size_t ps, const size_t dlr, ostream& o,
		const bool d, const bool p, const bool r) {
	// basic:
	const expectations& be(basic_strategy
		.lookup_player_action_expectations(ps, dlr));
	// ostr << "edges per action (basic):" << endl;
	// be.dump_choice_actions(ostr, -var.surrender_penalty);
	const pair<player_choice, player_choice> br(be.best_two(d, p, r));
	// ostr << "basic strategy recommends: " <<
	//	action_names[br.first] << endl;
	// dynamic: need to recompute given recently seen cards
	// show count?
	update_dynamic_strategy();	// evaluate when needed
	
	const expectations& de(dynamic_strategy
		.lookup_player_action_expectations(ps, dlr));
	// ostr << "edges per action (dynamic):" << endl;
	// de.dump_choice_actions(ostr, -var.surrender_penalty);
	const pair<player_choice, player_choice> dr(de.best_two(d, p, r));
	// ostr << "dynamic strategy recommends: " <<
	//	action_names[dr.first] << endl;
if (opt.show_edges) {
	o << "edges:\tbasic\tdynamic" << endl;
	expectations::dump_choice_actions_2(o,
		be, de, -var.surrender_penalty, d, p, r);
}
	o << "advise:\t" << action_names[br.first]
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
void
grader::update_dynamic_strategy(void) {
	// only now do we need to update probabilities
	// TODO: track whether or not already up-to-date
	// if so, don't re-evaluate
#if DECK_PROBABILITIES
	C.update_probabilities();
	dynamic_strategy.set_card_distribution(C.get_card_probabilities());
#else
	dynamic_strategy.set_card_distribution(C.get_card_counts());
#endif
	dynamic_strategy.evaluate();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::dump_situation(const size_t j) const {
	assert(j < player_hands.size());
	ostr << '[' << j+1 << '/' << player_hands.size() << "] ";
	if (dealer_hand.cards.size() > 1) {
		dealer_hand.dump_dealer(ostr);
	} else {
		ostr << "dealer: " << card_name[dealer_reveal];
	}
	ostr << ", ";
	player_hands[j].dump_player(ostr) << endl;
	return ostr;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param d if double-down is permitted.
	\param p is splitting is permitted.
	\param r if surrendering is permitted.
	\param a if should just automatically play (OPTIM)
 */
player_choice
prompt_player_action(istream& i, ostream& o, 
		const bool d, const bool p, const bool r, const bool a) {
	player_choice c = NIL;
do {
//	int ch;
	string line;
	do {
		// prompt for legal choices
		o << "action? [hs";
		if (d) o << 'd';
		if (p) o << 'p';
		if (r) o << 'r';
		o << "c?!]> ";
		if (!a) {
//		ch = getchar();
		// TODO: ncurses getch();
		// i >> line;
		getline(i, line);
		}
	} while (line.empty() && i && !a);
	if (a) {
		o << '!' << endl;
		return OPTIM;
	}
	switch (line[0])
//	switch (ch)
	{
	case 'h':
	case 'H':
		c = HIT; break;
	case 'd':
	case 'D':
		if (d) { c = DOUBLE; } break;
	case 'p':
	case 'P':
		if (p) { c = SPLIT; } break;
	case 's':
	case 'S':
		c = STAND; break;
	case 'r':
	case 'R':
		if (r) { c = SURRENDER; } break;
	case 'c':
	case 'C':
		c = COUNT; break;	// show count
	case '?':
		c = HINT; break;
		// caller should provide detailed hint with edges
		// for both basic and dynamic strategies
	case '!':
		c = OPTIM; break;
	default: break;
	}
} while (c == NIL && i);
	return c;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::status(ostream& o) const {
	o << "bankroll: " << stats.bankroll <<
		", bet: " << opt.bet << endl;
	// deck remaning (%)
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
grader::main(void) {
	ostr <<
"You sit down at a blackjack table.\n"
"Type 'help' or '?' for a list of table commands." << endl;
	const value_saver<string>
		tmp1(grader_command_registry::prompt, "table> ");
	const value_saver<util::completion_function_ptr>
		tmp(rl_attempted_completion_function,
			&grader_command_registry::completion);
	grader_command_registry::interpret(*this);
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace grader_commands {
#define	DECLARE_GRADER_COMMAND_CLASS(class_name, _cmd, _brief)		\
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(grader, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Help, "help", ": list all table commands")
int
Help::main(grader& g, const string_list&) {
	grader_command_registry::list_commands(g.ostr);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Help2, "?", ": list all table commands")
int
Help2::main(grader& g, const string_list&) {
	grader_command_registry::list_commands(g.ostr);
	return CommandStatus::NORMAL;
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
	g.ostr << "bet: " << bet << ", bankroll: " << br << endl;
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
	g.get_deck_state().show_count(g.ostr);
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
			g.ostr << "Out of cash after " << i+1 << " hands!"
				<< endl;
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
	while (!g.deal_hand()) { }
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
case 1: b.dump_optimal_actions(g.ostr); break;
case 2: {
	const string& t(args.back());
	if (t == "-edges") {
		b.dump_action_expectations(g.ostr);
	} else if (t == "-verbose") {
		b.dump(g.ostr);
	} else {
		cerr << "Error: invalid option." << endl;
		return CommandStatus::BADARG;
	}
	break;
}
default:
	cerr << "Error: command expects at most 1 option." << endl;
	return CommandStatus::SYNTAX;
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
case 1: b.dump_optimal_actions(g.ostr); break;
case 2: {
	const string& t(args.back());
	if (t == "-edges") {
		b.dump_action_expectations(g.ostr);
	} else if (t == "-verbose") {
		b.dump(g.ostr);
	} else {
		cerr << "Error: invalid option." << endl;
		return CommandStatus::BADARG;
	}
	break;
}
default:
	cerr << "Error: command expects at most 1 option." << endl;
	return CommandStatus::SYNTAX;
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
		ushort sd[3] = {0, 0, 0};
		sd[0] = tm & ushort(-1);
		sd[1] = (tm >> 16) & ushort(-1);
		// if (sizeof(time_t) > 4) {
		// sd[2] = (tm >> 32) & ushort(-1);
		sd[2] = (tm >> 8) & ushort(-1);
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
// TODO: group these into options
// rename: random mode? draw-mode? replay-mode?
DECLARE_GRADER_COMMAND_CLASS(CardsRandom, "cards-random",
	": randomly draw cards")
int
CardsRandom::main(grader& g, const string_list&) {
	g.opt.pick_cards = false;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(CardsPick, "cards-pick",
	": draw user-chosen cards")
int
CardsPick::main(grader& g, const string_list&) {
	g.opt.pick_cards = true;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(EditDeck, "edit-deck",
	"[all | card [+|-] N] : alter card distribution")
int
EditDeck::main(grader& g, const string_list& args) {
	deck_state& C(g.get_deck_state());
switch (args.size()) {
case 1: Count::main(g, args);	// just show count
	g.ostr << "usage: " << name << " " << brief << endl;
	break;
case 2: {
	// interactively edit all quantities
	Count::main(g, args);		// first show all
	if (args.back() == "all") {
		C.edit_deck_all(g.istr, g.ostr);	// exit status?
		Count::main(g, args);	// show all once again
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
		break;
	}
}
default: cerr << "usage: " << name << " " << brief << endl;
	return CommandStatus::SYNTAX;
}	// end switch
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: auto-play a whole hand, or N hands, or until some condition...
// TODO: auto-bet amount based on some criteria

#undef	DECLARE_GRADER_COMMAND_CLASS
}	// end namespace grader_commands
//=============================================================================
}	// end namespace blackjack

