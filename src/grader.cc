// "grader.cc"

#include <iostream>
#include <cstdio>
#include "grader.hh"
#include "util/string.tcc"	// for string_to_num
#include "util/command.tcc"
#include "util/value_saver.hh"

namespace blackjack {
typedef	util::Command<grader>		GraderCommand;
}
namespace util {
template class command_registry<blackjack::GraderCommand>;
}
namespace blackjack {
typedef	util::command_registry<GraderCommand>		grader_command_registry;

using std::cerr;
using std::endl;
using cards::ACE;
using cards::TEN;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::card_index;

using util::value_saver;
using util::Command;
using util::CommandStatus;
using util::string_list;
using util::strings::string_to_num;

//=============================================================================
// class grader::options method definitions

grader::options::options() :
	continuous_shuffle(false),
	pick_cards(false),
	use_dynamic_strategy(true), 
	dealer_plays_only_against_live(true),
	always_show_count_at_action(false),
	always_show_count_at_hand(false),
	always_suggest(false),
	notify_when_wrong(false),
	show_edges(true),
	bet(1.0) {
}

// TODO: dump, save, load

//=============================================================================
// clas grader::statistics method declarations

grader::statistics::statistics() :
	initial_bankroll(0.0),
	min_bankroll(0.0),
	max_bankroll(0.0),
	bankroll(0.0),
	initial_bets(0.0),
	total_bets(0.0),
	hands_played(0) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::statistics::initialize_bankroll(const double b) {
	initial_bankroll = b;
	min_bankroll = b;
	max_bankroll = b;
	bankroll = b;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Update bankroll watermarks.
 */
void
grader::statistics::compare_bankroll(void) {
	if (bankroll < min_bankroll)
		min_bankroll = bankroll;
	else if (bankroll > max_bankroll)
		max_bankroll = bankroll;
}

// TODO: dump, save, load

//=============================================================================
// class grader method definitions

grader::grader(const variation& v, istream& i, ostream& o) :
		var(v), istr(i), ostr(o), play(v),
		basic_strategy(play), dynamic_strategy(play), 
		C(v),
		player_hands(), dealer_hand(play), 
		opt(), stats() {
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
		istr >> line;
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
 */
void
grader::deal_hand(void) {
	player_hands.clear();
	if (opt.always_show_count_at_hand) {
		C.show_count(ostr);
	}
	const double& bet(opt.bet);
	double& bankroll(stats.bankroll);
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
	const double bet2 = var.double_multiplier *bet;
for (j=0; j<player_hands.size(); ++j) {
	dump_situation(j);
	if (player_hands[j].player_live()) {
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
	auto_shuffle();
	status(ostr);
//	only update_dynamic_strategy() when needed or requested
	// update bet min/max watermark
	// update bankroll min/max watermark
}	// end grader::deal_hand


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::auto_shuffle(void) {
	bool b = false;
	if (opt.continuous_shuffle) {
		C.reshuffle();
		b = true;
	} else if (C.reshuffle_auto()) {
		b = true;
	}
	if (b) {
		ostr << "*** Reshuffling... ***" << endl;
	}
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
	pc = prompt_player_action(istr, ostr, d, p, r);
	switch (pc) {
		// all fall-through to handle_player_action()
	case STAND:
	case DOUBLE:
	case HIT:
	case SPLIT: 
	case SURRENDER:
		handle_player_action(j, pc);
		break;
	case COUNT:
		C.show_count(ostr);
		break;
	case HINT:
		assess_action(ph.state, dealer_reveal, d, p, r);
		break;
	case OPTIM: {
		const pair<player_choice, player_choice>
			acp(assess_action(ph.state, dealer_reveal, d, p, r));
		const player_choice& ac = 
			opt.use_dynamic_strategy ? acp.second : acp.first;
		ostr << "auto: " << action_names[ac] << endl;
		handle_player_action(j, ac);
		break;
	}
	default: break;
	}	// end switch
	} while (ph.player_prompt());
} while (ph.player_prompt());
	if (ph.player_busted()) {
		// show final state
		stats.bankroll -= opt.bet;
		ph.dump_player(ostr) << endl;
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
pair<player_choice, player_choice>
grader::assess_action(const size_t ps, const size_t dlr, 
		const bool d, const bool p, const bool r) {
	// basic:
	const strategy::expectations& be(basic_strategy
		.lookup_player_action_expectations(ps, dlr));
	// ostr << "edges per action (basic):" << endl;
	// be.dump_choice_actions(ostr, -var.surrender_penalty);
	const pair<player_choice, player_choice> br(be.best_two(d, p, r));
	// ostr << "basic strategy recommends: " <<
	//	action_names[br.first] << endl;
	// dynamic: need to recompute given recently seen cards
	// show count?
	update_dynamic_strategy();	// evaluate when needed
	
	const strategy::expectations& de(dynamic_strategy
		.lookup_player_action_expectations(ps, dlr));
	// ostr << "edges per action (dynamic):" << endl;
	// de.dump_choice_actions(ostr, -var.surrender_penalty);
	const pair<player_choice, player_choice> dr(de.best_two(d, p, r));
	// ostr << "dynamic strategy recommends: " <<
	//	action_names[dr.first] << endl;
if (opt.show_edges) {
	ostr << "edges:\tbasic\tdynamic" << endl;
	strategy::expectations::dump_choice_actions_2(ostr,
		be, de, -var.surrender_penalty, d, p, r);
}
	ostr << "advise:\t" << action_names[br.first]
		<< '\t' << action_names[dr.first];
	if (br.first != dr.first) {
		// alert when dynamic strategy is different from basic
		ostr << "\t(different! using " <<
			(opt.use_dynamic_strategy ? "dynamic" : "basic")
			<< ")";
	}
	ostr << endl;
	return std::make_pair(br.first, dr.first);
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
player_choice
prompt_player_action(istream& i, ostream& o, 
		const bool d, const bool p, const bool r) {
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
//		ch = getchar();
		// TODO: ncurses getch();
		i >> line;
	} while (line.empty() && i);
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
	if (b > 0.0) {
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
DECLARE_GRADER_COMMAND_CLASS(Rules, "rules", ": show rule variations")
int
Rules::main(grader& g, const string_list&) {
	g.get_variation().dump(g.ostr);
	return CommandStatus::NORMAL;
}
// can't configure here

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
DECLARE_GRADER_COMMAND_CLASS(Status, "status", ": show bankroll and bet amount")
int
Status::main(grader& g, const string_list&) {
#if 0
	const double br = g.get_bankroll();
	ostr << "bankroll: " << br << ", bet: " << g.bet << endl;
#else
	g.status(g.ostr);
#endif
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Count, "count", ": show card counts")
DECLARE_GRADER_COMMAND_CLASS(Count2, "C", ": show card counts")
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
/**
	Also begin with 'D' to more easily disambiguate from other 
	d-commands.
 */
DECLARE_GRADER_COMMAND_CLASS(Deal, "deal", ": deal next hand")
DECLARE_GRADER_COMMAND_CLASS(Deal2, "Deal", ": deal next hand")
int
Deal::main(grader& g, const string_list&) {
	g.deal_hand();
	return CommandStatus::NORMAL;
}

int
Deal2::main(grader& g, const string_list&) {
	g.deal_hand();
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

#undef	DECLARE_GRADER_COMMAND_CLASS
}	// end namespace grader_commands
//=============================================================================
}	// end namespace blackjack

