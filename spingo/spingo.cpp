/*
KickOff Poker in C++
This implementation follows the logic of the Python demo:
– Three players with an initial stack.
– A deck of cards is built from ranks and suits.
– Game rounds (“preflop”, “flop”, “turn”, “river”, “showdown”).
– Legal actions include folding, posting blinds, calling, betting, raising,
  checking, all‐in and dealing (a chance node).
– A simple PokerEvaluator that scores a hand by taking the best
  (maximum) sum of “card values” from any combination of 5 cards out of 7.

Compile with, e.g.:
g++ -std=c++17 -O2 kickoff_poker.cpp -o kickoff_poker
*/

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <set>
#include <stdexcept>
#include <numeric>
#include <iterator>
#include <unordered_map>
#include <map>
#include <cassert>
#include <chrono>
#include <utility>
#include <tuple>
#include <vector>
#include <fstream>
#include <functional>
#include <iomanip>  // for setprecision

using namespace std;

// Constants definitions
constexpr int NUM_PLAYERS = 3;
constexpr float INITIAL_STACK = 15.0;
static const float RAKE_PERCENTAGE = 0.07;  // 7% rake

// ----------------------------------------------------------------------------
// Card, Deck, and Helper Functions
// ----------------------------------------------------------------------------

struct Card {
    std::string rank;
    std::string suit;

    std::string toString() const {
        return rank + suit;
    }
};

const vector<string> RANKS = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
const vector<char> SUITS = {'h', 'd', 'c', 's'};

// Build a standard 52-card deck.
vector<Card> make_deck() {
    std::vector<Card> deck;
    std::vector<std::string> ranks = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
    std::vector<std::string> suits = {"h", "d", "c", "s"};
    for (const auto& rank : ranks) {
        for (const auto& suit : suits) {
            deck.push_back(Card{rank, suit});
        }
    }
    return deck;
}
// Add betting round enum
enum class BettingRound {
    PREFLOP = 0,
    FLOP = 1,
    TURN = 2,
    RIVER = 3,
    SHOWDOWN = 4
};

// ----------------------------------------------------------------------------
// PokerEvaluator
// ----------------------------------------------------------------------------

class PokerEvaluator {
    public:
        // Rank-to-value mapping
        const unordered_map<string, int> RANK_VALUES = {
            {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5},
            {"6", 6}, {"7", 7}, {"8", 8}, {"9", 9},
            {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
        };
    
        // Standard poker hand rankings (higher is better)
        enum HandRank {
            HIGH_CARD = 0,
            PAIR = 1,
            TWO_PAIR = 2,
            THREE_KIND = 3,
            STRAIGHT = 4,
            FLUSH = 5,
            FULL_HOUSE = 6,
            FOUR_KIND = 7,
            STRAIGHT_FLUSH = 8
        };
    
        // Evaluate the best hand given hole cards and community cards.
        int evaluateHand(const vector<Card>& holeCards, const vector<Card>& communityCards) {
            vector<Card> allCards = holeCards;
            allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
            
            vector<vector<Card>> combinations;
            vector<Card> currentCombo;
            generateCombinations(allCards, 5, 0, currentCombo, combinations);
            
            int bestScore = 0;
            for (const auto &combo : combinations) {
                int score = evaluateFiveCardHand(combo);
                if (score > bestScore) {
                    bestScore = score;
                }
            }
            return bestScore;
        }
    
        int evaluateFiveCardHand(const vector<Card>& hand) {
            vector<int> cardValues;
            for (const auto &c : hand)
                cardValues.push_back(RANK_VALUES.at(c.rank));
            sort(cardValues.rbegin(), cardValues.rend());
    
            map<int, int, greater<int>> freq;
            for (int v : cardValues)
                freq[v]++;
    
            bool isFlush = checkFlush(hand);
            int straightHigh = 0;
            bool isStraight = checkStraight(hand, straightHigh);
    
            if (isFlush && isStraight) return makeScore(HandRank::STRAIGHT_FLUSH, {straightHigh});
            
            for (const auto &entry : freq) {
                if (entry.second == 4) {
                    int kicker = 0;
                    for (int v : cardValues) {
                        if (v != entry.first) { kicker = v; break; }
                    }
                    return makeScore(HandRank::FOUR_KIND, {entry.first, kicker});
                }
            }
            
            int tripleRank = 0, pairRank = 0;
            for (const auto &entry : freq) {
                if (entry.second >= 3 && tripleRank == 0)
                    tripleRank = entry.first;
            }
            if (tripleRank) {
                for (const auto &entry : freq) {
                    if (entry.first != tripleRank && entry.second >= 2) {
                        pairRank = entry.first;
                        break;
                    }
                }
                if (pairRank)
                    return makeScore(HandRank::FULL_HOUSE, {tripleRank, pairRank});
            }
            
            if (isFlush) return makeScore(HandRank::FLUSH, cardValues);
            if (isStraight) return makeScore(HandRank::STRAIGHT, {straightHigh});
            
            for (const auto &entry : freq) {
                if (entry.second == 3) {
                    vector<int> kickers;
                    for (int v : cardValues) {
                        if (v != entry.first) kickers.push_back(v);
                    }
                    return makeScore(HandRank::THREE_KIND, {entry.first, kickers[0], kickers[1]});
                }
            }
            
            vector<int> pairs;
            int kicker = 0;
            for (const auto &entry : freq) {
                if (entry.second >= 2)
                    pairs.push_back(entry.first);
            }
            if (pairs.size() >= 2) {
                sort(pairs.begin(), pairs.end(), greater<int>());
                for (int v : cardValues) {
                    if (v != pairs[0] && v != pairs[1]) { kicker = v; break; }
                }
                return makeScore(HandRank::TWO_PAIR, {pairs[0], pairs[1], kicker});
            }
            
            for (const auto &entry : freq) {
                if (entry.second == 2) {
                    vector<int> kickers;
                    for (int v : cardValues) {
                        if (v != entry.first) kickers.push_back(v);
                    }
                    return makeScore(HandRank::PAIR, {entry.first, kickers[0], kickers[1], kickers[2]});
                }
            }
            
            return makeScore(HandRank::HIGH_CARD, cardValues);
        }
    
        int makeScore(int handRank, const vector<int>& vals) {
            int score = handRank * 1000000; // Base score for hand rank, scaled up
            for (int v : vals) {
                score += v; // Add the values of the cards
            }
            return score;
        }
        void generateCombinations(const vector<Card>& cards, int combinationSize, int start,
                                  vector<Card>& currentCombo, vector<vector<Card>>& allCombinations) {
            if (currentCombo.size() == combinationSize) {
                allCombinations.push_back(currentCombo);
                return;
            }
            for (int i = start; i <= (int)cards.size() - (combinationSize - currentCombo.size()); ++i) {
                currentCombo.push_back(cards[i]);
                generateCombinations(cards, combinationSize, i + 1, currentCombo, allCombinations);
                currentCombo.pop_back();
            }
        }
    
        bool checkFlush(const vector<Card>& hand) {
            string suit = hand.front().suit;
            return all_of(hand.begin(), hand.end(), [&suit](const Card &c) { return c.suit == suit; });
        }
    
        bool checkStraight(const vector<Card>& hand, int &highStraightValue) {
            vector<int> values;
            for (const auto &card : hand)
                values.push_back(RANK_VALUES.at(card.rank));
            sort(values.begin(), values.end());
            
            bool normalStraight = (values[4] - values[0] == 4);
            if (normalStraight) {
                for (int i = 0; i < 4; i++) {
                    if (values[i+1] - values[i] != 1) return false;
                }
                highStraightValue = values[4];
                return true;
            }
            
            if (values == vector<int>{2, 3, 4, 5, 14}) {
                highStraightValue = 5;
                return true;
            }
            return false;
        }
    };

// ----------------------------------------------------------------------------
// Action enumeration and action amounts mapping
// ----------------------------------------------------------------------------

// Add betting actions
enum class Action {
     FOLD = 0,
     CHECK = 1,
     CALL = 2,
//     BET_1 = 3,
     BET_2 = 4,
//     BET_3 = 5,
//     RAISE_4 = 6,
//     RAISE_5 = 7,
//     RAISE_6 = 8,
//     RAISE_7 = 9, 
//     RAISE_8 = 10,
     ALL_IN = 11,
     DEAL = 12,
     POST_SB = 13,
     POST_BB = 14,
     UNKNOWN = 15
 };


unordered_map<Action, double> ACTION_AMOUNTS = {
    // {Action::BET_1, 1.0},
    {Action::BET_2, 2.0}, 
    //{Action::BET_3, 3.0},
    //{Action::RAISE_4, 4.0},
    //{Action::RAISE_5, 5.0},
    // {Action::RAISE_6, 6.0},
    //{Action::RAISE_7, 7.0},
    //{Action::RAISE_8, 8.0},
    {Action::POST_SB, 0.5},
    {Action::POST_BB, 1.0}
};

double amount(Action a) {
    if(ACTION_AMOUNTS.find(a) != ACTION_AMOUNTS.end())
        return ACTION_AMOUNTS[a];
    return 0.0;
}

// Function prototype
// Convert Action enum to string
string action_to_string(Action action) {
    switch (action) {
        case Action::FOLD: return "FOLD";
        case Action::CHECK: return "CHECK";
        case Action::CALL: return "CALL";
        // case Action::BET_1: return "BET_1";
        case Action::BET_2: return "BET_2";
        // case Action::BET_3: return "BET_3";
        // case Action::RAISE_4: return "RAISE_4";
        // case Action::RAISE_5: return "RAISE_5";
        // case Action::RAISE_6: return "RAISE_6";
        // case Action::RAISE_7: return "RAISE_7";
        // case Action::RAISE_8: return "RAISE_8";
        case Action::ALL_IN: return "ALL_IN";
        case Action::DEAL: return "DEAL";
        case Action::POST_SB: return "POST_SB";
        case Action::POST_BB: return "POST_BB";
        case Action::UNKNOWN: return "UNKNOWN";
        default: return "INVALID_ACTION";
    }
}
// ----------------------------------------------------------------------------
// SpinGoState class
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// SpinGoState class
// ----------------------------------------------------------------------------

class SpinGoState {
public:
    // Game state variables:
    vector<Card> cards; // all hole cards (2 per player)
    vector<Action> bets; // last action for each player
    vector<double> pot; // current round contributions per player
    vector<double> players_stack; // stacks remaining per player
    bool game_over;
    int next_player;
    string round; // "preflop", "flop", "turn", "river", "showdown"
    set<int> active_players; // players not folded
    vector<Card> community_cards; // community cards
    vector<double> cumulative_pot; // total chips contributed per player over rounds
    double current_bet = 0.0;
    vector<Card> deck;             // the deck (shuffled)

    // Random engine for shuffling.
    mt19937 rng;

    SpinGoState() {
        bets = vector<Action>(NUM_PLAYERS, Action::UNKNOWN);
        pot = vector<double>(NUM_PLAYERS, 0.0);
        players_stack = vector<double>(NUM_PLAYERS, INITIAL_STACK);
        cumulative_pot = vector<double>(NUM_PLAYERS, 0.0);
        game_over = false;
        next_player = 0;
        round = "preflop";
        for (int p = 0; p < NUM_PLAYERS; p++)
            active_players.insert(p);
        deck = make_deck();
        // initialize random engine
        rng.seed(random_device{}());
        shuffle(deck.begin(), deck.end(), rng);
    }

    // Returns the index of the current player (or -1 if game is over)
    int current_player() const {
        if (game_over)
            return -1;
        return next_player;
    }

    // Is this a chance node? (cards still to be dealt)
    bool is_chance_node() const {
        if (round == "preflop" && cards.size() < NUM_PLAYERS * 2)
            return true;
        else if (round == "flop" && community_cards.size() < 3)
            return true;
        else if (round == "turn" && community_cards.size() < 4)
            return true;
        else if (round == "river" && community_cards.size() < 5)
            return true;
        return false;
    }

    // Deal cards according to the current round.
    void deal_cards() {
        if (round == "preflop" && cards.size() < NUM_PLAYERS * 2) {
            // Deal two hole cards per player.
            for (int i = 0; i < NUM_PLAYERS * 2; i++) {
                cards.push_back(deck.back());
                deck.pop_back();
            }
        } else if (round == "flop" && community_cards.size() < 3) {
            for (int i = 0; i < 3; i++) {
                community_cards.push_back(deck.back());
                deck.pop_back();
            }
            // Commenting out the print statement for dealt flop cards
            // cout << "Dealt Flop: ";
            // for (const auto& card : community_cards) {
            //     cout << card.toString() << " ";
            // }
            // cout << endl; // Print the dealt flop cards
        } else if (round == "turn" && community_cards.size() == 3) {
            community_cards.push_back(deck.back());
            deck.pop_back();
        } else if (round == "river" && community_cards.size() == 4) {
            community_cards.push_back(deck.back());
            deck.pop_back();
        }
    }

    // Return vector of legal actions for the current player.
    vector<Action> legal_actions() const {
        vector<Action> actions;
        if (is_chance_node()) {
            actions.push_back(Action::DEAL);
            return actions;
        }
        if (game_over)
            return actions;
        int player = current_player();
        
        // Special preflop blind actions - these override all other actions
        if (round == "preflop") {
            double total_pot = 0.0;
            for (double x : pot)
                total_pot += x;
            if (player == 0 && total_pot == 0.0)
                return vector<Action>{Action::POST_SB};
            if (player == 1 && total_pot == amount(Action::POST_SB))
                return vector<Action>{Action::POST_BB};
        }
        
        // FOLD is always allowed unless check is free
        if (current_bet > pot[player]) {
            actions.push_back(Action::FOLD);
        }
        
        // If no bet has been made or player has already matched current bet, CHECK is legal.
        if (current_bet == 0 || current_bet == pot[player]) {
            actions.push_back(Action::CHECK);
        }
        
        // CALL is legal if there's a bet to call
        if (current_bet > pot[player]) {
            actions.push_back(Action::CALL);
        }
        
        // In preflop, allow BET_2 as a raise option
        if (round == "preflop" && current_bet > 0) {
            if (players_stack[player] > amount(Action::BET_2) && current_bet < amount(Action::BET_2)) {
                actions.push_back(Action::BET_2);  // This effectively works as a raise in preflop
            }
        } else if (round != "preflop") {
            // In non-preflop rounds, BET actions are available when no bet exists
            if (current_bet == 0) {
                for (int i = 0; i < static_cast<int>(Action::UNKNOWN) + 1; i++) {
                    Action action = static_cast<Action>(i);
                    string action_name = action_to_string(action);
                    if (action_name.substr(0, 4) == "BET_") {
                        if (players_stack[player] > amount(action)) {
                            actions.push_back(action);
                        }
                    }
                }
            }
        }
        
        // All-In is allowed if the player has any chips.
        if (players_stack[player] > 0)
            actions.push_back(Action::ALL_IN);
        
        return actions;
    }

    // Check whether the betting round is complete.
    bool betting_round_complete() {
        // If all active players have 0 chips, then round is complete.
        bool allZero = true;
        for (int p : active_players) {
            if (players_stack[p] != 0) {
                allZero = false;
                break;
            }
        }
        if (allZero)
            return true;

        // Check if all active players have equal contributions
        double first_pot = pot[*active_players.begin()]; // Get the pot of the first active player
        for (int p : active_players) {
            if (pot[p] != first_pot) {
                return false; // If any player's pot is not equal to the first, return false
            }
        }

        // Special case for preflop: Check if player 1 has posted the big blind
        if (round == "preflop") {
            // Assuming player 1 is the one who posts the big blind
            if (pot[1] == amount(Action::POST_BB) || bets[next_player] == Action::CHECK) {
                return true; // If player 1 has posted the big blind or last action was check, the round is complete
            }
        }

        return true; // If all pots are equal, return true
    }

    // Advance _next_player to the next active player.
    void advance_to_next_player() {
        if (active_players.empty()) {
            game_over = true;
            return;
        }
        // Find next player in increasing order.
        vector<int> act(active_players.begin(), active_players.end());
        sort(act.begin(), act.end());
        if (active_players.find(next_player) == active_players.end())
            next_player = act.front();
        else {
            auto it = find(act.begin(), act.end(), next_player);
            if (it != act.end()) {
                ++it;
                if(it == act.end())
                    next_player = act.front();
                else
                    next_player = *it;
            }
        }
    }

    // Advance the round and move this round's contributions to cumulative.
    void advance_round() {
        for (int p = 0; p < NUM_PLAYERS; p++) {
            cumulative_pot[p] += pot[p];
            pot[p] = 0.0;
            bets[p] = Action::UNKNOWN;
        }
        current_bet = 0.0;
        if (round == "preflop")
            round = "flop";
        else if (round == "flop")
            round = "turn";
        else if (round == "turn")
            round = "river";
        else if (round == "river")
            round = "showdown";
    }

    // Check whether the game should end.
    bool should_end_game() {
        if (active_players.size() == 1 || round == "showdown")
            return true;
        // If all active players are all in.
        bool allIn = true;
        for (int p : active_players) {
            if (players_stack[p] > 0)
                allIn = false;
        }
        if (allIn)
            return true;
        return false;
    }

    // Apply the given action.
    void apply_action(Action action) {
        // If chance node then deal cards.
        if (is_chance_node()) {
            deal_cards();
            return;
        }
        int player = current_player();
        vector<Action> legal = legal_actions();
        if (find(legal.begin(), legal.end(), action) == legal.end()) {
            throw runtime_error("Illegal action chosen for player " + std::to_string(player));
        }
        bets[player] = action;
        
        // Handle actions
        if (action == Action::FOLD) {
            active_players.erase(player);
            if (active_players.size() <= 1) {
                game_over = true;
                return;  // Exit early to prevent further processing
            }
        } else if (action == Action::POST_SB) {
            current_bet = amount(Action::POST_SB);
            pot[player] = current_bet;
            players_stack[player] -= current_bet;
            next_player = 1;  // Set next player to BB
            return;  // Return early to prevent advance_to_next_player
        } else if (action == Action::POST_BB) {
            current_bet = amount(Action::POST_BB);
            pot[player] = current_bet;
            players_stack[player] -= current_bet;
            next_player = (player + 1) % NUM_PLAYERS;  // Set next player to the first player who will act
            return;  // Return early to prevent advance_to_next_player
        } else if (action == Action::CALL) {
            double call_amt = current_bet - pot[player];
            pot[player] = current_bet;
            players_stack[player] -= call_amt;
        } else if (action_to_string(action).substr(0, 4) == "BET_") {
            double bet_amt = amount(action);
            current_bet = bet_amt;
            double call_amt = current_bet - pot[player];
            pot[player] = current_bet;
            players_stack[player] -= call_amt;
        } else if (action_to_string(action).substr(0, 6) == "RAISE_") {
            double raise_amt = amount(action);
            current_bet = raise_amt;
            double call_amt = current_bet - pot[player];
            pot[player] = current_bet;
            players_stack[player] -= call_amt;
        } else if (action == Action::ALL_IN) {
            double all_in_amt = pot[player] + players_stack[player];
            current_bet = max(current_bet, all_in_amt);
            pot[player] = all_in_amt;
            players_stack[player] = 0;
        } else if (action == Action::CHECK) {
            // Do nothing.
        } else if (action == Action::DEAL) {
            deal_cards();
        }

        // After the action, check if betting round is complete.
        if (betting_round_complete()) {
            advance_round();
            if (should_end_game()) {
                // if game should end but some cards missing, complete the deal.
                while (round != "showdown") {
                    deal_cards();
                    advance_round();
                }
                game_over = true;
            } else {
                if (!active_players.empty())
                    next_player = *active_players.begin();  // Set next player to the first active player
            }
        } else {
            advance_to_next_player();
        }
    }

    // Compute and return the rewards (for each player) at game end.
    vector<double> returns() {
        vector<double> rewards(NUM_PLAYERS, 0.0);
        if (!game_over)
            return rewards;
        double total_pot = 0.0;
        for (double x : cumulative_pot)
            total_pot += x;
        if (active_players.size() == 1) {  // one remaining winner.
            int winner = *active_players.begin();
            for (int p = 0; p < NUM_PLAYERS; p++)
                rewards[p] = -cumulative_pot[p];
            rewards[winner] = total_pot;
        } else {
            PokerEvaluator evaluator;
            unordered_map<int,int> best_hands;
            for (int p : active_players) {
                // Each player's hole cards: cards[ p*2, p*2+1 ]
                vector<Card> hole_cards;
                if (p*2+1 < cards.size()) {
                    hole_cards.push_back(cards[p*2]);
                    hole_cards.push_back(cards[p*2+1]);
                }
                int hand_value = evaluator.evaluateHand(hole_cards, community_cards);
                best_hands[p] = hand_value;
            }
            int max_value = 0;
            for (auto &entry : best_hands) {
                if (entry.second > max_value)
                    max_value = entry.second;
            }
            vector<int> winners;
            for (auto &entry : best_hands) {
                if (entry.second == max_value)
                    winners.push_back(entry.first);
            }
            double reward_share = winners.empty() ? 0.0 : total_pot / winners.size();
            for (int p = 0; p < NUM_PLAYERS; p++) {
                if (find(winners.begin(), winners.end(), p) != winners.end())
                    rewards[p] = reward_share - INITIAL_STACK;
                else
                    rewards[p] = -cumulative_pot[p];
            }
        }
        return rewards;
    }

    // A string representation of the state.
    string to_string() {
        ostringstream oss;
        oss << "Cards: ";
        for (const Card &c : cards)
            oss << c.toString() << " ";
        oss << "\nCommunity: ";
        for (const Card &c : community_cards)
            oss << c.toString() << " ";
        oss << "\nBets: ";
        for (int p = 0; p < NUM_PLAYERS; p++)
            oss << "P" << p << ": " << static_cast<float>(players_stack[p]) << "  ";
        oss << "\nPot (this round): ";
        for (int p = 0; p < NUM_PLAYERS; p++)
            oss << "P" << p << ": " << pot[p] << "  ";
        oss << "\nCurrent Bet: " << current_bet;
        oss << "\nNext Player: " << next_player;
        oss << "\nRound: " << round;
        oss << "\nStatus: " << (game_over ? "Game Over" : "In Progress");
        return oss.str();
    }

    // Returns the current player's stack
    double get_current_player_stack() {
        if (game_over) {
            throw runtime_error("Game is over, no current player.");
        }
        return players_stack[next_player];
    }

    // Modify the write_infosets method to append to the file
    void write_infosets(const string& filename) {
        // Check if the current state is a chance node
        if (is_chance_node()) {
            return; // Do not write infosets if it's a chance node
        }

        ofstream file(filename, ofstream::app); // Open file in append mode
        if (!file.is_open()) {
            throw runtime_error("Could not open file for writing: " + filename);
        }

        // Log the current round and current player
        file << "Round: " << round << "\n";
        file << "Current Player: Player " << next_player << "\n";

        // Log previous actions with round information
        file << "Previous Actions (Round: " << round << "): ";
        for (int p = 0; p < NUM_PLAYERS; ++p) {
            if (bets[p] != Action::UNKNOWN) {
                file << "(Player " << p << ", " << action_to_string(bets[p]) << ") ";
            }
        }
        file << "\n";

        // Log hole cards for all players with round information
        for (int p = 0; p < NUM_PLAYERS; ++p) {
            file << "Player " << p << " Hole Cards (Round: " << round << "): " 
                 << cards[p * 2].toString() << " " << cards[p * 2 + 1].toString() << "\n";
        }

        // Log the ranks of the community cards
        file << "Community Cards (Round: " << round << "): ";
        for (const auto& card : community_cards) {
            file << card.toString() << " "; // Log each community card
        }
        file << "\n\n";

        // Log the overall pot
        double total_pot = accumulate(pot.begin(), pot.end(), 0.0);
        file << "Overall Pot: " << total_pot << "\n\n"; // Add space after each infoset

        file.close();
    }

};

// ----------------------------------------------------------------------------
// KickOffGame class
// ----------------------------------------------------------------------------

class SpinGoGame {
public:
    SpinGoGame() {}
    SpinGoState new_initial_state() {
        return SpinGoState();
    }
};

// ----------------------------------------------------------------------------
// Example Usage (main function)
// ----------------------------------------------------------------------------

// Add this before main()
ostream& operator<<(ostream& os, const vector<Action>& actions) {
    os << "[";
    for (size_t i = 0; i < actions.size(); ++i) {
        os << action_to_string(actions[i]);
        if (i < actions.size() - 1) os << ", ";
    }
    os << "]";
    return os;
}

int main() {
    SpinGoGame game;
    mt19937 rng(random_device{}());
    const int num_samples = 1e3;
    SpinGoState state = game.new_initial_state(); 
    // state.apply_action(Action::DEAL);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::POST_SB);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::POST_BB);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::BET_2);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::CALL);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::CALL);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::DEAL);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::BET_2);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::CALL);
    // cout << state.legal_actions() << "\n";
    // state.apply_action(Action::CALL);
    // cout << state.legal_actions() << "\n";

    // Open file for writing and check if it is open
    ofstream file("infosets.txt");
    if (!file) {
        cerr << "Could not open file for writing: infosets.txt" << endl;
        return 1;
    }

    set<string> logged_sequences; // Set to track logged action sequences
    auto start_time = chrono::high_resolution_clock::now();

    for (int sample = 0; sample < num_samples; ++sample) {
        SpinGoState state = game.new_initial_state(); // Reinitialize the game
        string action_sequence;
        string previous_round = state.round; // Initialize previous_round

        // Calculate and print ETA
        if (sample % 100 == 0 && sample > 0) {
            auto current_time = chrono::high_resolution_clock::now();
            chrono::duration<double> elapsed = current_time - start_time;
            double eta = (elapsed.count() / sample) * (num_samples - sample);
            cout << "\rETA: " << fixed << setprecision(2) << eta << " seconds remaining" << flush;
        }

        while (!state.game_over) {
            vector<Action> actions = state.legal_actions();
            if (actions.empty()) break; // No legal actions available

            // Randomly select an action
            uniform_int_distribution<int> dist(0, actions.size() - 1);
            Action random_action = actions[dist(rng)];

            // Apply the selected action
            state.apply_action(random_action);

            // Append the action to the sequence if it's not DEAL
            if (random_action != Action::DEAL) {
                action_sequence += action_to_string(random_action) + " ";
            }

            // Add a separator when the round changes
            if (state.round != previous_round) { // Check if the round has changed
                action_sequence += "| "; // Add separator
                previous_round = state.round; // Update previous_round to current round
            }
        }
        action_sequence += "Pot: " + to_string(state.cumulative_pot[0] + state.cumulative_pot[1] + state.cumulative_pot[2]);
        if (!action_sequence.empty()) {
            string log_entry = action_sequence;
            if (logged_sequences.find(log_entry) == logged_sequences.end()) {
                file << log_entry << "\n";
                logged_sequences.insert(log_entry); // Add to the set of logged sequences
            }
        }
    }

    file.close(); // Close the file
    return 0;
}