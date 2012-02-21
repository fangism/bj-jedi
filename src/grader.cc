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

using std::cin;
using std::cout;
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

grader::grader(const variation& v) :
		var(v), play(v), basic_strategy(play), dynamic_strategy(play), 
		C(v),
	player_hands(), 
	pick_cards(false),
	bankroll(100.0), bet(1.0) {
	player_hands.reserve(16);
	basic_strategy.set_card_distribution(standard_deck_distribution);
	dynamic_strategy.set_card_distribution(standard_deck_distribution);
	basic_strategy.evaluate();
	dynamic_strategy.evaluate();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
grader::~grader() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
grader::offer_insurance(istream& i, ostream& o, const bool pbj) const {
	// if peek_ACE
	const char* prompt = pbj ?  "even-money?" : "insurance?";
	bool done = false;
	bool buy_insurance = false;
	string line;
	do {
	do {
		o << prompt << " [ync]: ";
		i >> line;
	} while (line.empty() && i);
	if (line == "n" || line == "N") {
		done = true;
	} else if (line == "y" || line == "Y") {
		buy_insurance = true;
		done = true;
	} else if (line == "c" || line == "C") {
		C.show_count(o);
	}
	} while (!done);
	return buy_insurance;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Play a hand of blackjack.
	Player hands, dealer state, hole cards, etc...
	could all just be local variables?
 */
void
grader::deal_hand(istream& i, ostream& o) {
	player_hands.clear();
	player_hands.resize(1);
	if (pick_cards) {
		o << "choose player cards." << endl;
	}
	const size_t p1 = C.option_draw(pick_cards, i, o);
	const size_t p2 = C.option_draw(pick_cards, i, o);
	hand& pih(player_hands.front());
	pih.deal_player(play, p1, p2);
#if 0
	pih.dump_player(o, play) << endl;
#endif
	if (pick_cards) {
		o << "choose dealer up-card." << endl;
	}
	dealer_reveal = C.option_draw(pick_cards, i, o);
	dealer_hand.initial_card_dealer(play, dealer_reveal);
	if (pick_cards) {
		o << "choose dealer hole-card." << endl;
	}
	C.option_draw_hole_card(pick_cards, i, o);
	const size_t hole_card = C.peek_hole_card();
	o << "dealer: " << card_name[dealer_reveal] << endl;
	// TODO: if early_surrender (rare) ...
	// prompt for insurance
	const bool pbj = pih.has_blackjack();
	if (pbj) {
		o << "Player has blackjack!" << endl;
	}
	bool end = pbj;
	if (dealer_reveal == ACE) {
		pih.dump_player(o, play) << endl;
		if (var.peek_on_Ace) {
		const bool buy_insurance = offer_insurance(i, o, pbj);;
		// determine change in bankroll
		// check for blackjack for player
		const double half_bet = bet / 2.0;
		if (hole_card == TEN) {
			end = true;
			o << "Dealer has blackjack." << endl;
			if (buy_insurance) {
				bankroll += var.insurance *half_bet;
			}
			if (!pbj) {
				bankroll -= bet;
			} else {
				o << "Push." << endl;
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
		pih.dump_player(o, play) << endl;
		if (var.peek_on_10) {
		if (hole_card == ACE) {
			end = true;
			o << "Dealer has blackjack." << endl;
			if (!pbj) {
				bankroll -= bet;
			} else {
				o << "Push." << endl;
			}
		} else {
//			o << "No dealer blackjack." << endl;
		}
		}	// peek_on_10
	}
	// play ends if either dealer or player had blackjack (and was peeked)
if (!end) {
	size_t j = 0;
	for ( ; j<player_hands.size(); ++j) {
		hand& ph(player_hands[j]);
		// caution, reference may dangle after a vector::push_back
		// player may resplit hands
		bool live = true;
		do {
			dump_situation(o, j);
			// these predicates can be further refined by
			// variation/rules etc...
			const bool d = ph.doubleable();
			// check for double_after_split and other limitations
			const bool p = ph.splittable() &&
				(player_hands.size() < player_hands.capacity());
			// check for resplit limit
			const bool r = ph.surrenderable() && !already_split();
			// only first hand is surrenderrable, normally
			bool prompt = false;
			do {
			const player_choice pc =
				prompt_player_action(i, o, d, p, r);
			switch (pc) {
			case STAND: live = false; break;
			case DOUBLE:
				live = false;
				ph.doubled_down = true;
				// fall-through
			case HIT:
				ph.hit_player(play, 
					C.option_draw(pick_cards, i, o));
				break;
			case SPLIT:
				ph.presplit(play);
				player_hands.push_back(ph);
				ph.hit_player(play, 
					C.option_draw(pick_cards, i, o));
				player_hands.back().hit_player(play, 
					C.option_draw(pick_cards, i, o));
				// check for special case of split-Aces
				// show other split hand for counting purposes
				break;
			case SURRENDER: bankroll -= bet *var.surrender_penalty;
				// recall surrender_penalty is positive
				ph.surrendered = true;
				live = false; break;
			case HINT:
			case OPTIM:
				prompt = true;
				o << "Hints not yet available." << endl;
				break;
			default:
				prompt = true;
				break;
			}
			} while (prompt);
			if (ph.state == player_bust) {
				ph.dump_player(o, play) << endl;
				// o << "Player busts." << endl;
				live = false;
			}
		} while (live);
		dump_situation(o, j);
	}
#if 1
	o << "Dealer plays..." << endl;
#endif
	// dealer plays after player is done
	// should dealer play when there are no live hands?
	dealer_hand.hit_dealer(play, hole_card);
	C.reveal_hole_card();
	// dealer_hand.dump_dealer(o, play);
	while (!play.is_dealer_terminal(dealer_hand.state)) {
		dealer_hand.hit_dealer(play, C.option_draw(pick_cards, i, o));
	}
	dealer_hand.dump_dealer(o, play) << endl;
	// suspense double-down?  nah
for (j=0; j<player_hands.size(); ++j) {
	dump_situation(o, j);
	if (!player_hands[j].surrendered) {
		// check for surrender could/should be outside this loop
	const outcome& wlp(play_map::outcome_matrix
		[play_map::player_final_state_map(player_hands[j].state)]
		[dealer_hand.state -stop]);
	const double delta = player_hands[j].doubled_down ?
		var.double_multiplier * bet : bet;
	switch (wlp) {
	case WIN: o << "Player wins." << endl; bankroll += delta; break;
	case LOSE: o << "Player loses." << endl; bankroll -= delta; break;
	case PUSH: o << "Push." << endl; break;
	}	// end switch
	}
}	// end for
}
	if (C.reshuffle_auto()) {
		o << "Reshuffling..." << endl;
	}
	status(o);
	C.update_probabilities();
	dynamic_strategy.set_card_distribution(C.get_card_probabilities());
	dynamic_strategy.evaluate();	// slow?
}	// end grader::deal_hand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::dump_situation(ostream& o, const size_t j) const {
	assert(j < player_hands.size());
	o << '[' << j+1 << '/' << player_hands.size() << "] ";
	if (dealer_hand.cards.size() > 1) {
		dealer_hand.dump_dealer(o, play);
	} else {
		o << "dealer: " << card_name[dealer_reveal];
	}
	o << ", ";
	player_hands[j].dump_player(o, play) << endl;
	return o;
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
		o << "?!] ";
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
	cout <<
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
Help::main(grader&, const string_list&) {
	grader_command_registry::list_commands(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Help2, "?", ": list all table commands")
int
Help2::main(grader&, const string_list&) {
	grader_command_registry::list_commands(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Quit, "quit", ": leave table, return to lobby")
int
Quit::main(grader& g, const string_list&) {
	const double b = g.get_bankroll();
	if (b > 0.0) {
		cout << "You collect your remaining chips from the table ("
			<< b << ") and return to the lobby." << endl;
	} else {
		cout << "Better luck next time!" << endl;
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
	g.get_variation().dump(cout);
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
	cout << "bet: " << g.bet << ", bankroll: " << br << endl;
	return CommandStatus::NORMAL;
}
// can't configure here

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Status, "status", ": show bankroll and bet amount")
int
Status::main(grader& g, const string_list&) {
#if 0
	const double br = g.get_bankroll();
	cout << "bankroll: " << br << ", bet: " << g.bet << endl;
#else
	g.status(cout);
#endif
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Count, "count", ": show card counts")
int
Count::main(grader& g, const string_list&) {
	g.get_deck_state().show_count(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Deal, "deal", ": deal next hand")
int
Deal::main(grader& g, const string_list&) {
	g.deal_hand(cin, cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_GRADER_COMMAND_CLASS(Basic, "basic-strategy",
	"[-edge|-verbose]: show basic strategy")
int
Basic::main(grader& g, const string_list& args) {
	const strategy& b(g.get_basic_strategy());
switch (args.size()) {
case 1: b.dump_optimal_actions(cout); break;
case 2: {
	const string& t(args.back());
	if (t == "-edges") {
		b.dump_action_expectations(cout);
	} else if (t == "-verbose") {
		b.dump(cout);
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
case 1: b.dump_optimal_actions(cout); break;
case 2: {
	const string& t(args.back());
	if (t == "-edges") {
		b.dump_action_expectations(cout);
	} else if (t == "-verbose") {
		b.dump(cout);
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

