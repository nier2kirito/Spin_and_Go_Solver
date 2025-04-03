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
            int score = handRank;
            for (int v : vals)
                score = score * 100 + v;
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
// Function prototypes
// ----------------------------------------------------------------------------

tuple<int, int, int, int> abstract_suit_pattern(const vector<Card>& cards);
map<string, bool> detect_draws(const PokerEvaluator& evaluator, const vector<Card>& hole_cards, const vector<Card>& board_cards);
tuple<string, int> abstract_high_cards(const vector<Card>& cards);

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

    PokerEvaluator evaluator;

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
        
        // FOLD is always allowed.
        actions.push_back(Action::FOLD);
        
        // If no bet has been made or player has already matched current bet, CHECK is legal.
        if (current_bet == 0 || current_bet == pot[player]) {
            actions.push_back(Action::CHECK);
            actions.erase(remove(actions.begin(), actions.end(), Action::FOLD), actions.end());
        }
        // No code needed - just removing FOLD from actions
        else {
            // Otherwise, allow CALL if the player has enough chips.
            double call_amt = current_bet - pot[player];
            if (players_stack[player] >= call_amt)
                actions.push_back(Action::CALL);
        }
        
        // Special preflop blind actions.
        if (round == "preflop") {
            double total_pot = 0.0;
            for (double x : pot)
                total_pot += x;
            if (player == 0 && total_pot == 0.0)
                return vector<Action>{Action::POST_SB};
            if (player == 1 && total_pot == amount(Action::POST_SB))
                return vector<Action>{Action::POST_BB};
        }
        
        // Betting options in rounds other than preflop.
        if (round != "preflop") {
            // Find all actions that start with "BET_"
            for (int i = 0; i < static_cast<int>(Action::UNKNOWN) + 1; i++) {
                Action action = static_cast<Action>(i);
                string action_name = action_to_string(action);
                if (action_name.substr(0, 4) == "BET_") {
                    if (players_stack[player] > amount(action) && current_bet < amount(action)) {
                        actions.push_back(action);
                    }
                }
            }
        }

        // Raise options (only if there is already a bet).
        if (current_bet > 0) {
            // Find all actions that start with "RAISE_"
            for (int i = 0; i < static_cast<int>(Action::UNKNOWN) + 1; i++) {
                Action action = static_cast<Action>(i);
                string action_name = action_to_string(action);
                if (action_name.substr(0, 6) == "RAISE_") {
                    if (players_stack[player] > amount(action) && current_bet < amount(action)) {
                        actions.push_back(action);
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
            if (active_players.size() == 1)
                game_over = true;
        } else if (action == Action::POST_SB) {
            current_bet = amount(Action::POST_SB);
            pot[player] = current_bet;
            players_stack[player] -= current_bet;
        } else if (action == Action::POST_BB) {
            current_bet = amount(Action::POST_BB);
            pot[player] = current_bet;
            players_stack[player] -= current_bet;
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
                    next_player = *active_players.begin();
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

        // Log active players
        file << "Active Players: ";
        for (int player : active_players) {
            file << "Player " << player << " ";
        }
        file << "\n";

        // Log previous actions as tuples (player index, Action)
        file << "Previous Actions: ";
        for (int p = 0; p < NUM_PLAYERS; ++p) {
            if (bets[p] != Action::UNKNOWN) {
                file << "(Player " << p << ", " << action_to_string(bets[p]) << ") ";
            }
        }
        file << "\n";

        // Log abstracted hole cards for the current player
        for (int p = 0; p < NUM_PLAYERS; ++p) {
            if (p == next_player) { // Only include current player's cards
                string lower_rank = min(cards[p * 2].rank, cards[p * 2 + 1].rank);
                string higher_rank = max(cards[p * 2].rank, cards[p * 2 + 1].rank);
                char suit_indicator = (cards[p * 2].suit == cards[p * 2 + 1].suit) ? 's' : 'o'; // 's' for suited, 'o' for offsuit
                
                file << "Abstracted Hole Cards: " << lower_rank << higher_rank << suit_indicator << "\n"; // e.g., "A2s" or "KJo"
            }
        }
        // Log draw potentials
        auto draw_flags = detect_draws(evaluator, cards, community_cards); // Pass the evaluator instance
        file << "Flush Draw: " << (draw_flags["flush_draw"] ? "true" : "false") << "\n";
        file << "Straight Draw: " << (draw_flags["straight_draw"] ? "true" : "false") << "\n";

        // Log high cards
        auto high_cards = abstract_high_cards(cards); // Check both hole cards
        file << "High Cards: " << get<0>(high_cards) << "\n";

        // Log the overall pot
        double total_pot = accumulate(pot.begin(), pot.end(), 0.0);
        file << "Overall Pot: " << total_pot << "\n\n"; // Add space after each infoset

        // Log the abstracted board information
        auto [diamonds, spades, clubs, hearts, ranks] = abstract_board_info();
        file << "Suit Counts: (" << diamonds << ", " << spades << ", " << clubs << ", " << hearts << ")\n";
        file.close();
    }

    // New method to abstract board cards
    tuple<int, int, int, int, set<string>> abstract_board_info() {
        int diamonds = 0, spades = 0, clubs = 0, hearts = 0;
        set<string> ranks;

        for (const auto& card : community_cards) {
            if (card.suit == "d") diamonds++;
            else if (card.suit == "s") spades++;
            else if (card.suit == "c") clubs++;
            else if (card.suit == "h") hearts++;

            ranks.insert(card.rank);
        }

        return make_tuple(diamonds, spades, clubs, hearts, ranks);
    }

    // New method to check if the hand is weak
    bool is_hand_weak() const {
        // Define a list of weak hands (for simplicity, we will use a few examples)
        vector<string> weak_hands = {"72o", "83o", "94o", "95o", "62o", "63o", "64o", "65o", "54o", "53o", "52o"};
        
        // Get the hole cards
        string hole_card_1 = cards[0].rank + cards[0].suit;
        string hole_card_2 = cards[1].rank + cards[1].suit;

        // Check if the hand is in the weak hands list
        if (find(weak_hands.begin(), weak_hands.end(), hole_card_1 + hole_card_2) != weak_hands.end() ||
            find(weak_hands.begin(), weak_hands.end(), hole_card_2 + hole_card_1) != weak_hands.end()) {
            return true; // Hand is weak
        }

        // Additional logic to determine if the hand is in the lowest 20% can be added here
        // For now, we will return false if not found in weak hands
        return false;
    }

    // Example usage of the new method
    bool eligible_for_action() const {
        return !is_hand_weak(); // Return false if the hand is weak
    }
};

// ----------------------------------------------------------------------------
// Function Definitions
// ----------------------------------------------------------------------------

tuple<int, int, int, int> abstract_suit_pattern(const vector<Card>& cards) {
    int diamonds = 0, spades = 0, clubs = 0, hearts = 0;
    for (const auto& card : cards) {
        if (card.suit == "d") diamonds++;
        else if (card.suit == "s") spades++;
        else if (card.suit == "c") clubs++;
        else if (card.suit == "h") hearts++;
    }
    return make_tuple(diamonds, spades, clubs, hearts);
}

map<string, bool> detect_draws(const PokerEvaluator& evaluator, const vector<Card>& hole_cards, const vector<Card>& board_cards) {
    vector<Card> all_cards = hole_cards;
    all_cards.insert(all_cards.end(), board_cards.begin(), board_cards.end());
    map<string, bool> flags = { {"flush_draw", false}, {"straight_draw", false} };

    auto suit_counts = abstract_suit_pattern(all_cards);
    if (get<0>(suit_counts) >= 3 || get<1>(suit_counts) >= 3 || get<2>(suit_counts) >= 3 || get<3>(suit_counts) >= 3) {
        flags["flush_draw"] = true;
    }

    // Use the evaluator instance to access RANK_VALUES
    vector<int> ranks;
    for (const auto& card : all_cards) {
        ranks.push_back(evaluator.RANK_VALUES.at(card.rank)); // Accessing RANK_VALUES through the evaluator
    }
    sort(ranks.begin(), ranks.end());
    // Check for open-ended straight draw logic here...

    return flags;
}

tuple<string, int> abstract_high_cards(const vector<Card>& cards) {
    set<string> high_cards;
    for (const auto& card : cards) {
        if (card.rank == "A" || card.rank == "K" || card.rank == "Q" || card.rank == "J") {
            high_cards.insert(card.rank);
        }
    }
    return make_tuple(high_cards.empty() ? "" : *high_cards.begin(), high_cards.size());
}

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
int main() {
    const int NUM_GAMES = 1e5; // Number of games to simulate
    ofstream ofs("infosets.txt", ofstream::trunc); // Clear previous content
    ofs.close(); // Close the file after truncating
    auto start_time = chrono::steady_clock::now(); // Start time for ETA calculation
    int count = 0;
    for (int game_number = 0; game_number < NUM_GAMES; ++game_number) {
        SpinGoGame game;
        SpinGoState state = game.new_initial_state();
        mt19937 rng((unsigned)chrono::system_clock::now().time_since_epoch().count());

        while (!state.game_over) {
            vector<Action> legal = state.legal_actions();
            uniform_int_distribution<int> dist(0, legal.size() - 1);
            Action chosen = legal[dist(rng)];
            try {
                state.apply_action(chosen);
                if (!state.is_chance_node() && state.round != "showdown" && state.eligible_for_action()) {
                    count ++;
                    // state.write_infosets("infosets.txt");
                }
            } catch (const exception &e) {
                cerr << "Error applying action: " << e.what() << "\n";
                break;
            }
        }

        // Update ETA calculation
        auto elapsed_time = chrono::steady_clock::now() - start_time;
        double seconds_per_game = chrono::duration_cast<chrono::seconds>(elapsed_time).count() / (game_number + 1.0);
        int remaining_seconds = static_cast<int>(seconds_per_game * (NUM_GAMES - game_number - 1));

        int eta_hours = remaining_seconds / 3600;
        int eta_minutes = (remaining_seconds % 3600) / 60;
        int eta_seconds = remaining_seconds % 60;

        cout << "\rProgress: " << game_number + 1 << "/" << NUM_GAMES
             << " | ETA: " << eta_hours << "h " << eta_minutes << "m " << eta_seconds << "s   " 
             << flush;
    }

    cout << "\nSimulation complete!\n";
    cout << "Number of infosets  : " << count << "\n" ;
    return 0;
}