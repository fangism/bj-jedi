// "grader.cc"

#include <iostream>
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
// class grader method definitions

grader::grader(const variation& v, istream& i, ostream& o) :
		var(v), istr(i), ostr(o), play(v),
		basic_strategy(play), dynamic_strategy(play), 
		C(v),
	player_hands(), 
	pick_cards(false),
	bankroll(100.0), bet(1.0) {
	player_hands.reserve(16);	// to prevent realloc
	const deck_count_type& d(C.get_card_counts());
	// each call does util::normalize()
	basic_strategy.set_card_distribution(d);
	dynamic_strategy.set_card_distribution(d);
	basic_strategy.evaluate();
	dynamic_strategy.evaluate();	// not really necessary
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
grader::~grader() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
grader::offer_insurance(const bool pbj) const {
	// if peek_ACE
	const char* prompt = pbj ?  "even-money?" : "insurance?";
	bool done = false;
	bool buy_insurance = false;
	string line;
	do {
	do {
		ostr << prompt << " [ync]: ";
		istr >> line;
	} while (line.empty() && istr);
	if (line == "n" || line == "N") {
		done = true;
	} else if (line == "y" || line == "Y") {
		buy_insurance = true;
		done = true;
	} else if (line == "c" || line == "C") {
		C.show_count(ostr);
	}
	} while (!done);
	return buy_insurance;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
grader::draw_up_card(void) {
	return C.option_draw(pick_cards, istr, ostr);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
grader::draw_hole_card(void) {
	C.option_draw_hole_card(pick_cards, istr, ostr);
	return C.peek_hole_card();
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
	player_hands.resize(1);
	if (pick_cards) {
		ostr << "choose player cards." << endl;
	}
	const size_t p1 = draw_up_card();
	const size_t p2 = draw_up_card();
	hand& pih(player_hands.front());
	pih.deal_player(play, p1, p2, true);
	if (pick_cards) {
		ostr << "choose dealer up-card." << endl;
	}
	dealer_reveal = draw_up_card();
	dealer_hand.initial_card_dealer(play, dealer_reveal);
	// TODO: first query whether hold card makes dealer blackjack
	if (pick_cards) {
		ostr << "choose dealer hole-card." << endl;
	}
	// except for European: no hole card
	const size_t hole_card = draw_hole_card();
	ostr << "dealer: " << card_name[dealer_reveal] << endl;
	// TODO: if early_surrender (rare) ...
	// prompt for insurance
	const bool pbj = pih.has_blackjack();
	if (pbj) {
		ostr << "Player has blackjack!" << endl;
	}
	// if either player or dealer has blackjack, hand is over
	bool end = pbj;
	if (dealer_reveal == ACE) {
		pih.dump_player(ostr, play) << endl;
		if (var.peek_on_Ace) {
		const bool buy_insurance = offer_insurance(pbj);;
		// determine change in bankroll
		// check for blackjack for player
		const double half_bet = bet *0.5;
		if (hole_card == TEN) {
			end = true;
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
//			o << "No dealer blackjack." << endl;
			if (buy_insurance) {
				bankroll -= half_bet;
			}
			if (pbj) {
				bankroll += var.bj_payoff *bet;
			}
			// else keep playing
		}
		}	// peek_on_Ace
	} else if (dealer_reveal == TEN) {
		pih.dump_player(ostr, play) << endl;
		if (var.peek_on_10) {
		if (hole_card == ACE) {
			end = true;
			ostr << "Dealer has blackjack." << endl;
			if (!pbj) {
				bankroll -= bet;
			} else {
				ostr << "Push." << endl;
			}
		} else {
//			o << "No dealer blackjack." << endl;
		}
		}	// peek_on_10
	} else if (pbj) {
		// pay off player's blackjack
		// end = true;
		pih.dump_player(ostr, play) << endl;
		bankroll += var.bj_payoff *bet;
	}
	// play ends if either dealer or player had blackjack (and was peeked)
if (!end) {
	size_t j = 0;
	for ( ; j<player_hands.size(); ++j) {
		play_out_hand(j);
	}
#if 1
	ostr << "Dealer plays..." << endl;
#endif
	// dealer plays after player is done
	// should dealer play when there are no live hands?
	dealer_hand.hit_dealer(play, hole_card);
	C.reveal_hole_card();
	// dealer_hand.dump_dealer(o, play);
	while (!play.is_dealer_terminal(dealer_hand.state)) {
		dealer_hand.hit_dealer(play, draw_up_card());
	}
	dealer_hand.dump_dealer(ostr, play) << endl;
	// suspense double-down?  nah
	const double bet2 = var.double_multiplier *bet;
for (j=0; j<player_hands.size(); ++j) {
	dump_situation(j);
	if (!player_hands[j].surrendered) {
		// check for surrender could/should be outside this loop
	const outcome& wlp(play_map::outcome_matrix
		[play_map::player_final_state_map(player_hands[j].state)]
		[dealer_hand.state -stop]);
	const double delta = player_hands[j].doubled_down ? bet2 : bet;
	switch (wlp) {
	case WIN: ostr << "Player wins." << endl; bankroll += delta; break;
	case LOSE: ostr << "Player loses." << endl; bankroll -= delta; break;
	case PUSH: ostr << "Push." << endl; break;
	}	// end switch
	}
}	// end for
}
	if (C.reshuffle_auto()) {
		ostr << "Reshuffling..." << endl;
	}
	status(ostr);
//	only update when requested
//	update_dynamic_strategy();
}	// end grader::deal_hand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::play_out_hand(const size_t j) {
	hand& ph(player_hands[j]);
	// caution, reference may dangle after a vector::push_back
	// player may resplit hands
	bool live = true;	// indicates whether or not hand is still active
do {
	bool prompt = false;	// when hand is finished, live or not, stop prompt
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
	const player_choice pc = prompt_player_action(istr, ostr, d, p, r);
	switch (pc) {
	case STAND: live = false; prompt = false; break;
	case DOUBLE:
		live = false;
		prompt = false;
		ph.doubled_down = true;
		// fall-through
	case HIT:
		ph.hit_player(play, draw_up_card());
		if (ph.state == player_bust) {
			ph.dump_player(ostr, play) << endl;
			live = false;
			prompt = false;
		} else if (!ph.doubled_down) {
			prompt = true;
		}
		// also don't bother prompt if hits 21
		break;
	case SPLIT: {
		const size_t split_card = ph.state - pair_offset;
		// ph.presplit(play);
		// 21 should not be considered blackjack when splitting
		ph.deal_player(play, split_card, draw_up_card(), false);
		hand nh;
		nh.deal_player(play, split_card, draw_up_card(), false);
		player_hands.push_back(nh);
		dump_situation(j);
		dump_situation(player_hands.size() -1);
		// TODO: check for special case of split-Aces
		// show other split hand for counting purposes
		prompt = true;
		break;
	}
	case SURRENDER: bankroll -= bet *var.surrender_penalty;
		// recall surrender_penalty is positive
		ph.surrendered = true;
		live = false;
		prompt = false;
		break;
	case COUNT:
		C.show_count(ostr);
		prompt = true;
		break;
	case HINT: {
		// basic:
		const strategy::expectations& be(basic_strategy
			.lookup_player_action_expectations(ph.state, dealer_reveal));
		ostr << "edges:\tbasic\tdynamic:" << endl;
		// ostr << "edges per action (basic):" << endl;
		// be.dump_choice_actions(ostr, -var.surrender_penalty);
		const pair<player_choice, player_choice> br(be.best_two(d, p, r));
		// ostr << "basic strategy recommends: " <<
		//	action_names[br.first] << endl;
		// dynamic: need to recompute given recently seen cards
		// show count?
		update_dynamic_strategy();	// evaluate when needed
		
		const strategy::expectations& de(dynamic_strategy
			.lookup_player_action_expectations(ph.state, dealer_reveal));
		// ostr << "edges per action (dynamic):" << endl;
		// de.dump_choice_actions(ostr, -var.surrender_penalty);
		const pair<player_choice, player_choice> dr(de.best_two(d, p, r));
		// ostr << "dynamic strategy recommends: " <<
		//	action_names[dr.first] << endl;
		strategy::expectations::dump_choice_actions_2(ostr,
			be, de, -var.surrender_penalty);
		ostr << "advise:\t" << action_names[br.first]
			<< '\t' << action_names[dr.first] << endl;
		prompt = true;
		break;
	}
	case OPTIM:
		prompt = true;
		ostr << "Auto-play not yet available." << endl;
		break;
	default:
		prompt = true;
		break;
	}
	} while (prompt);
} while (live);
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
		dealer_hand.dump_dealer(ostr, play);
	} else {
		ostr << "dealer: " << card_name[dealer_reveal];
	}
	ostr << ", ";
	player_hands[j].dump_player(ostr, play) << endl;
	return ostr;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
player_choice
prompt_player_action(istream& i, ostream& o, 
		const bool d, const bool p, const bool r) {
	player_choice c = NIL;
do {
	string line;
	do {
		// prompt for legal choices
		o << "action? [hs";
		if (d) o << 'd';
		if (p) o << 'p';
		if (r) o << 'r';
		o << "c?!]> ";
		i >> line;
	} while (line.empty() && i);
	switch (line[0]) {
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
	o << "bankroll: " << bankroll <<
		", bet: " << bet << endl;
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
	g.bet = nb;
	break;
}
default:
	cerr << "usage: " << name << ' ' << brief << endl;
	return CommandStatus::SYNTAX;
}
	g.ostr << "bet: " << g.bet << ", bankroll: " << br << endl;
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
int
Count::main(grader& g, const string_list&) {
	g.get_deck_state().show_count(g.ostr);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Deal, "deal", ": deal next hand")
int
Deal::main(grader& g, const string_list&) {
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
DECLARE_GRADER_COMMAND_CLASS(CardsRandom, "cards-random",
	": randomly draw cards")
int
CardsRandom::main(grader& g, const string_list&) {
	g.pick_cards = false;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(CardsPick, "cards-pick",
	": draw user-chosen cards")
int
CardsPick::main(grader& g, const string_list&) {
	g.pick_cards = true;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// EditDeck

#undef	DECLARE_GRADER_COMMAND_CLASS
}	// end namespace grader_commands
//=============================================================================
}	// end namespace blackjack

