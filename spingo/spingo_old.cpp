#ifndef AOF_POKER_H 
#define SPINGO_POKER_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <set>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <set>
#include <tuple>
#include <random>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <utility> // For std::pair
using namespace std;

struct GameParameters {
    float rake_percentage;
    float starting_stack_bb;
    float prize_pool_multiplier;  // New field for Spin & Go multiplier
    float big_blind;
};
// Constants definitions
static const float RAKE_PERCENTAGE = 0.07;  // 7% rake
static const float STARTING_STACK_BB = 15.0;  // 15 BB starting stack
static const int NUM_PLAYERS = 3;  // Spin & Go is 3-handed

const vector<string> RANKS = {"2", "3", "4", "5", "6", "7", "8", "9",
"10", "J", "Q", "K", "A"};
const vector<string> SUITS = {"h", "d", "c", "s"};

// Card structure
struct Card {
    std::string rank;
    std::string suit;

    std::string toString() const {
        return rank + suit;
    }
};

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
    RIVER = 3
};

// Add betting actions
enum class Action {
    FOLD = 0,
    CHECK = 1,
    CALL = 2,
    RAISE = 3,
    ALL_IN = 4,
    DEAL = 5  // for chance nodes
};

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
        vector<int> evaluateHand(const vector<Card>& holeCards, const vector<Card>& communityCards) {
            vector<Card> allCards = holeCards;
            allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
            
            vector<vector<Card>> combinations;
            vector<Card> currentCombo;
            generateCombinations(allCards, 5, 0, currentCombo, combinations);
            
            vector<int> bestScore;
            for (const auto &combo : combinations) {
                vector<int> score = evaluateFiveCardHand(combo);
                if(bestScore.empty() || score > bestScore)
                    bestScore = score;
            }
            return bestScore;
        }
    
        vector<int> evaluateFiveCardHand(const vector<Card>& hand) {
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
    
        vector<int> makeScore(int handRank, const vector<int>& vals) {
            vector<int> score = {handRank};
            score.insert(score.end(), vals.begin(), vals.end());
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

// Forward declaration.
class SpinGoGame;

// The SpinGoState class represents one state of the game.
class SpinGoState {
public:
    SpinGoState(SpinGoGame* game_in);
    // Add new members
    BettingRound current_round;
    float current_bet;
    vector<float> player_bets;  // Track bets in current round
    int button_pos;  // Track dealer button position
    float min_raise;
    
    SpinGoState(GameParameters params) 
    : current_round(BettingRound::PREFLOP),
      current_bet(params.big_blind),
      player_bets(NUM_PLAYERS, 0),
      button_pos(2),
      min_raise(params.big_blind),
      pot(1.5),
      _game_over(false),
      _next_player(2),
      initial_stacks(NUM_PLAYERS, params.starting_stack_bb),
      players_stack(NUM_PLAYERS, params.starting_stack_bb) {
        // Initialize blinds
        postBlinds();
        // ... rest of initialization
    }

    // Add new methods
    void postBlinds() {
        int sb_pos = (button_pos + 1) % NUM_PLAYERS;
        int bb_pos = (button_pos + 2) % NUM_PLAYERS;
        
        // Post small blind
        players_stack[sb_pos] -= sb;
        player_bets[sb_pos] = sb;
        pot += sb;
        
        // Post big blind
        players_stack[bb_pos] -= bb;
        player_bets[bb_pos] = bb;
        pot += bb;
    }

    // Modify legal actions to include all betting options
    vector<Action> legalActions() const;

    // Add method to advance betting round
    void advanceRound() {
        // Reset per-round variables
        current_bet = 0;
        fill(player_bets.begin(), player_bets.end(), 0);
        min_raise = bb;

        // Deal community cards based on round
        switch(current_round) {
            case BettingRound::PREFLOP:
                // Deal flop (3 cards)
                for(int i = 0; i < 3; i++) {
                    community_cards.push_back(deck.back());
                    deck.pop_back();
                }
                current_round = BettingRound::FLOP;
                break;
            case BettingRound::FLOP:
                // Deal turn (1 card)
                community_cards.push_back(deck.back());
                deck.pop_back();
                current_round = BettingRound::TURN;
                break;
            case BettingRound::TURN:
                // Deal river (1 card)
                community_cards.push_back(deck.back());
                deck.pop_back();
                current_round = BettingRound::RIVER;
                break;
            case BettingRound::RIVER:
                _game_over = true;
                handleGameEnd();
                break;
        }

        // First to act is first player after button
        _next_player = (button_pos + 1) % NUM_PLAYERS;
        while(folded[_next_player] || players_stack[_next_player] == 0) {
            _next_player = (_next_player + 1) % NUM_PLAYERS;
        }
    }

    // Modify applyAction to handle new betting actions
    void applyAction(Action action);

    // Add method to check if betting round is complete
    bool isRoundComplete() const {
        // Round is complete if:
        // 1. All active players have bet the same amount or are all-in
        // 2. At least one player has acted this round
        bool all_bets_equal = true;
        float target_bet = current_bet;

        for (int p = 0; p < NUM_PLAYERS; p++) {
            if (!folded[p] && players_stack[p] > 0) {
                if (player_bets[p] != target_bet) {
                    all_bets_equal = false;
                    break;
                }
            }
        }

        return all_bets_equal;
    }

    // Returns the index of the current player; if game is over, returns -1.
    int currentPlayer() const;
    float pot;
        // Initial stacks of players.
    vector<float> initial_stacks;
    // Current stack sizes.
    vector<float> players_stack;

    // Returns true if a chance node (i.e. cards have not yet been dealt).
    bool isChanceNode() const;

    // Fold status: true if player has folded.
    vector<bool> folded;

    // A set of players that are all-in.
    set<int> all_in_players;
    
    // Returns true if the game is over.
    bool isTerminal() const { return _game_over; }

    // Returns the final returns (i.e. changes in stacks) for all players.
    vector<float> returns();

    // String representation of the state.
    string toString() const;

    // Produce a full copy of the state.
    SpinGoState clone() const { return *this; }

    // Add this method to access hole cards
    const vector<Card>& getHoleCards() const {
        return cards;
    }

    void advanceToNextPlayer();

private:
    // Deal hole cards and post blinds.
    void dealCards();

    // Calculate side pots and evaluate the hand.
    void handleGameEnd();
    void calculateSidePots();

    SpinGoGame* game;
    bool _game_over;
    int _next_player;
    std::vector<Card> deck;
    std::vector<Card> cards;

    // Blinds values.
    float sb = 0.5;
    float bb = 1; 

    // The community cards.
    vector<Card> community_cards;

    // Side pots; each entry is a pair: <pot amount, list of contributor player indices>.
    vector<tuple<float, vector<int>>> side_pots;

};

// The SpinGoGame class, a factory for game states.
class SpinGoGame {
public:
    SpinGoGame(float small_blind, float big_blind, float rake_per_hand, float jackpot_fee_per_hand, float jackpot_payout_percentage, const std::vector<float>& initial_stacks_bb = std::vector<float>());
    SpinGoState newInitialState();

    float small_blind;
    float big_blind;
    float rake_per_hand;
    float jackpot_fee_per_hand;
    float jackpot_payout_percentage;
    // initial stacks in units of big blinds.
    std::vector<float> initial_stacks;
};

// SpinGoState implementations
// ... (copy all the implementations from the original file)
// ---------- SpinGoState method implementations ----------

SpinGoState::SpinGoState(SpinGoGame* game_in)
: game(game_in),
sb(game->small_blind),
bb(game->big_blind),
pot(1.5),
_game_over(false),
_next_player(2),
initial_stacks(NUM_PLAYERS),
players_stack(NUM_PLAYERS),
folded(NUM_PLAYERS, false),
cards(),
deck(make_deck()) {
    // initial_stacks: each player's chips = bigBlind * starting stack units.
    for (int p = 0; p < NUM_PLAYERS; p++) {
        initial_stacks[p] = game->initial_stacks[p] * bb;
        players_stack[p] = game->initial_stacks[p] * bb;
    }

    // empty cards
    cards.clear();

    // Prepare deck.
    deck = make_deck();
    // Shuffle deck
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    shuffle(deck.begin(), deck.end(), default_random_engine(seed));

    community_cards.clear();
    side_pots.clear();
}

int SpinGoState::currentPlayer() const {
    if (_game_over)
        return -1;
    return _next_player;
}

bool SpinGoState::isChanceNode() const {
    // In our design the chance node happens if no cards have been dealt.
    return cards.empty();
}

void SpinGoState::dealCards() {
    if (!cards.empty())
        return;
    // Deal hole cards: each player gets 2 cards.
    for (int i = 0; i < NUM_PLAYERS * 2; i++) {
        cards.push_back(deck.back());
        deck.pop_back();
    }
    // The original Python code sets _next_player to 2.
    _next_player = 2;
    // Reset folded status.
    folded.assign(NUM_PLAYERS, false);
}

vector<Action> SpinGoState::legalActions() const {
    if (isChanceNode())
        return vector<Action>{Action::DEAL};

    // If game is over or the current player has already folded, return empty list.
    int player = currentPlayer();
    if (player < 0 || folded[player])
        return vector<Action>{};

    return vector<Action>{Action::FOLD, Action::ALL_IN};
}

void SpinGoState::advanceToNextPlayer() {
    _next_player = (_next_player + 1) % NUM_PLAYERS;
    // keep advancing if the player has folded, unless the game is over.
    while (folded[_next_player] && !_game_over) {
        _next_player = (_next_player + 1) % NUM_PLAYERS;
    }
    // In Python: if all except the current player are folded, the game is over.
    int activeCount = 0;
    for (int p = 0; p < NUM_PLAYERS; p++) {
        if (!folded[p])
            activeCount++;
    }
    if (activeCount <= 1)
        _game_over = true;
}

void SpinGoState::applyAction(Action action) {
    if (isChanceNode()) {
        dealCards();
        _next_player = 2;  // Start with Player 2 after blinds
        return;
    }
    
    int player = currentPlayer();

    if (action == Action::FOLD) {
        folded[player] = true;
    } else if (action == Action::ALL_IN) {
        float amount = players_stack[player];
        pot += amount;
        all_in_players.insert(player);
        players_stack[player] = 0.0;
    }

    advanceToNextPlayer();

    // Check if all players have acted (either folded or gone all-in)
    bool all_players_acted = true;
    int active_players = 0;
    
    for (int p = 0; p < NUM_PLAYERS; p++) {
        if (!folded[p]) {
            active_players++;
            // If a player hasn't folded or gone all-in, they haven't acted
            if (all_in_players.count(p) == 0 && p >= _next_player) {
                all_players_acted = false;
            }
        }
    }

    // Only end the game if:
    // 1. All players have acted AND
    // 2. Either only one player is active OR all active players are all-in
    if (all_players_acted && 
        (active_players == 1 || 
         (active_players > 1 && active_players == all_in_players.size()))) {
        _game_over = true;
        handleGameEnd();
    }
}

void SpinGoState::handleGameEnd() {
    // Deal community cards (5 cards)
    for (int i = 0; i < 5 && !deck.empty(); i++) {
        community_cards.push_back(deck.back());
        deck.pop_back();
    }

    calculateSidePots();
    // In Python, returns() is called at the end to calculate outcome.
    // Here, returns() will be used by context as needed.
}

void SpinGoState::calculateSidePots() {
    // Clear existing side pots
    side_pots.clear();
    
    // Track total contributions from each player
    vector<float> contributions(NUM_PLAYERS, 0.0);
    
    // Add blinds
    contributions[0] = 0.5;  // Small blind
    contributions[1] = 1;  // Big blind
    
    // Add all-in amounts
    for (int p : all_in_players) {
        contributions[p] += initial_stacks[p];
    }
    
    // Sort contributions to create side pots
    vector<pair<float, int>> sorted_contribs;
    for (int p = 0; p < NUM_PLAYERS; p++) {
        if (!folded[p] && contributions[p] > 0) {
            sorted_contribs.push_back({contributions[p], p});
        }
    }
    sort(sorted_contribs.begin(), sorted_contribs.end());
    
    // Create side pots
    float prev_amount = 0.0;
    for (size_t i = 0; i < sorted_contribs.size(); i++) {
        float current_amount = sorted_contribs[i].first;
        float pot_size = 0.0;
        vector<int> contributors;
        
        // Calculate pot size and contributors
        for (size_t j = i; j < sorted_contribs.size(); j++) {
            int player = sorted_contribs[j].second;
            pot_size += (current_amount - prev_amount);
            contributors.push_back(player);
        }
        
        if (pot_size > 0) {
            side_pots.push_back({pot_size, contributors});
        }
        prev_amount = current_amount;
    }
}

vector<float> SpinGoState::returns() {
    vector<float> net_changes(NUM_PLAYERS, 0.0);
    
    // If everyone folded except one player, they win the pot
    net_changes[0] = -0.5;    // Others lose nothing
    net_changes[1] = -1;

    int active_count = 0;
    int last_active = -1;
    for (int p = 2; p < NUM_PLAYERS; p++) {
        if (!folded[p]) {
            active_count++;
            last_active = p;
        }
    }
    for (int p = 0; p < 2; p++) {
        if (!folded[p]) {
            active_count++;
            last_active = p;
        }
    }
    
    if (active_count == 1) {
        // Everyone folded case
        // Note: In many poker rooms, rake is not taken if there's no flop (no community cards)
        // But some rooms take rake even in this case. Adjust according to your rules.
        float rake_amount = game->rake_per_hand;
        float jackpot_fee = game->jackpot_fee_per_hand;
        
        // Option 1: No rake on fold (common rule)
        net_changes[last_active] += 1.5;  // Winner gets both blinds
        
        // Option 2: Apply rake even on fold (uncomment if this is your rule)
        // float adjusted_pot = 1.5 - rake_amount - jackpot_fee;
        // net_changes[last_active] += adjusted_pot;
        
        return net_changes;
    }

    // Otherwise evaluate hands for showdown
    PokerEvaluator evaluator;
    vector<int> best_score;
    vector<int> winners;
    
    for (int p = 0; p < NUM_PLAYERS; p++) {
        if (!folded[p]) {
            vector<Card> hole_cards = {cards[2*p], cards[2*p + 1]};
            vector<Card> all_cards = hole_cards;
            all_cards.insert(all_cards.end(), community_cards.begin(), community_cards.end());
            
            vector<vector<Card>> combinations;
            vector<Card> currentCombo;
            evaluator.generateCombinations(all_cards, 5, 0, currentCombo, combinations);
            
            vector<int> player_best_score;
            for (const auto& combo : combinations) {
                vector<int> score = evaluator.evaluateFiveCardHand(combo);
                if (player_best_score.empty() || score > player_best_score) {
                    player_best_score = score;
                }
            }
            
            if (best_score.empty() || player_best_score > best_score) {
                best_score = player_best_score;
                winners = {p};
            } else if (player_best_score == best_score) {
                winners.push_back(p);
            }
        }
    }

    // Apply rake and jackpot fee - this is where utility is directly affected by rake
    float original_pot = pot;
    float rake_amount = game->rake_per_hand;
    float jackpot_fee = game->jackpot_fee_per_hand;
    float jackpot_payout = original_pot * game->jackpot_payout_percentage;
    
    // Utility calculation with rake components:
    // 1. Rake directly reduces the pot (negative utility)
    // 2. Jackpot fee is another reduction (negative utility)
    // 3. Jackpot payout is a potential gain (positive utility)
    float adjusted_pot = original_pot - rake_amount - jackpot_fee + jackpot_payout;

    // In a rake-free game, sum of utilities would be zero (zero-sum game)
    // With rake, sum of utilities is negative (negative-sum game)
    // The total negative utility equals: rake_amount + jackpot_fee - jackpot_payout

    // Split pot among winners
    float share = adjusted_pot / winners.size();
    for (int winner : winners) {
        net_changes[winner] += share - initial_stacks[winner];  // Add winnings to their net change
    }

    // Apply losses for non-winners
    for (int p = 0; p < NUM_PLAYERS; p++) {
        if (find(winners.begin(), winners.end(), p) == winners.end()) {
            // Player is not a winner
            if (all_in_players.count(p) > 0) {
                // If they went all-in, they lose their entire stack
                net_changes[p] -= initial_stacks[p];
            }
        }
    }
    
    return net_changes;
}

string SpinGoState::toString() const {
    string s;
    s += "Hole Cards: ";
    for (const auto &c : cards)
        s += c.toString() + " ";
    s += "\nCommunity Cards: ";
    for (const auto &c : community_cards)
        s += c.toString() + " ";
    s += "\nPlayer Stacks: ";
    for (int p = 0; p < NUM_PLAYERS; p++) {
        s += "P" + to_string(p) + ": " + to_string(players_stack[p]) + " ";
    }
    s += "\nPot: " + to_string(pot);
    s += "\nNext Player: " + to_string(_next_player);
    s += "\nGame Over: " + string(_game_over ? "True" : "False");
    return s;
}

// Function to evaluate a five-card hand
int evaluateFive(const std::vector<Card>& hand) {
    // This is a very basic evaluation function
    // You should expand this to cover all poker hand rankings

    std::map<std::string, int> rankCount;
    std::map<std::string, int> suitCount;

    for (const auto& card : hand) {
        rankCount[card.rank]++;
        suitCount[card.suit]++;
    }

    // Check for flush
    bool isFlush = std::any_of(suitCount.begin(), suitCount.end(), [](const auto& entry) {
        return entry.second == 5;
    });

    // Check for pairs, three of a kind, etc.
    int pairs = 0, threeOfAKind = 0, fourOfAKind = 0;
    for (const auto& entry : rankCount) {
        if (entry.second == 2) pairs++;
        if (entry.second == 3) threeOfAKind++;
        if (entry.second == 4) fourOfAKind++;
    }

    // Example scoring logic
    if (isFlush) return 6; // Flush
    if (fourOfAKind) return 7; // Four of a kind
    if (threeOfAKind && pairs) return 6; // Full house
    if (threeOfAKind) return 3; // Three of a kind
    if (pairs == 2) return 2; // Two pairs
    if (pairs == 1) return 1; // One pair

    return 0; // High card
}

void runTests() {
    PokerEvaluator evaluator;

    // Test 1: High Card
    {
        vector<Card> holeCards = {Card{"2", "h"}, Card{"4", "d"}};
        vector<Card> communityCards = {Card{"6", "c"}, Card{"8", "s"}, Card{"10", "h"}, Card{"J", "d"}, Card{"K", "c"}};
        vector<int> expected = {PokerEvaluator::HIGH_CARD, 13, 11, 10, 8, 6};
        assert(evaluator.evaluateHand(holeCards, communityCards) ==  expected);
    }

    // Test 2: One Pair
    {
        vector<Card> holeCards = {Card{"2", "h"}, Card{"2", "d"}};
        vector<Card> communityCards = {Card{"6", "c"}, Card{"8", "s"}, Card{"10", "h"}, Card{"J", "d"}, Card{"K", "c"}};
        vector<int> expected = {PokerEvaluator::PAIR, 2, 13, 11, 10};
        auto result = evaluator.evaluateHand(holeCards, communityCards);
        assert(evaluator.evaluateHand(holeCards, communityCards) == expected);
    }

    // Tie-breaking scenario: Two players with High Card, different kickers
    {
        vector<Card> holeCards1 = {Card{"A", "h"}, Card{"K", "d"}};
        vector<Card> communityCards1 = {Card{"Q", "c"}, Card{"J", "s"}, Card{"9", "h"}, Card{"3", "d"}, Card{"2", "c"}};
        vector<Card> holeCards2 = {Card{"A", "s"}, Card{"K", "c"}};
        vector<Card> communityCards2 = {Card{"Q", "d"}, Card{"J", "h"}, Card{"8", "s"}, Card{"3", "c"}, Card{"2", "h"}};
        vector<int> expected1 = {PokerEvaluator::HIGH_CARD, 14, 13, 12, 11, 9};
        vector<int> expected2 = {PokerEvaluator::HIGH_CARD, 14, 13, 12, 11, 8};
        assert(evaluator.evaluateHand(holeCards1, communityCards1) == expected1);
        assert(evaluator.evaluateHand(holeCards2, communityCards2) == expected2);
    }

    // Add this test case
    {
        // Two pair: Kings and Twos vs Queens and Jacks
        vector<Card> hand1 = {Card{"K", "h"}, Card{"K", "d"}, Card{"2", "h"}, Card{"2", "d"}, Card{"A", "c"}};
        vector<Card> hand2 = {Card{"Q", "h"}, Card{"Q", "d"}, Card{"J", "h"}, Card{"J", "d"}, Card{"K", "c"}};
        vector<int> score1 = evaluator.evaluateFiveCardHand(hand1);
        vector<int> score2 = evaluator.evaluateFiveCardHand(hand2);
        assert(score1 > score2);  // Kings and Twos should beat Queens and Jacks
    }
}

// Implementation of the constructor
SpinGoGame::SpinGoGame(float small_blind, float big_blind, float rake_per_hand, float jackpot_fee_per_hand, float jackpot_payout_percentage, const std::vector<float>& initial_stacks_bb)
    : small_blind(small_blind), big_blind(big_blind), rake_per_hand(rake_per_hand), 
      jackpot_fee_per_hand(jackpot_fee_per_hand), jackpot_payout_percentage(jackpot_payout_percentage) {
    if (initial_stacks_bb.empty()) {
        initial_stacks = std::vector<float>{
            STARTING_STACK_BB - small_blind,  // P0: Starting stack - small blind
            STARTING_STACK_BB - big_blind,    // P1: Starting stack - big blind 
            STARTING_STACK_BB,                // P2: Starting stack
        };
    } else {
        initial_stacks = initial_stacks_bb;
    }
}

// Implementation of newInitialState
SpinGoState SpinGoGame::newInitialState() {
    return SpinGoState(this);
}

void printVector(const vector<float>& vec) {
    for (const auto& value : vec) {
        cout << value << " ";
    }
}

int main() {
    // Set up game parameters
    GameParameters params;
    params.rake_percentage = RAKE_PERCENTAGE;
    params.starting_stack_bb = STARTING_STACK_BB;
    params.prize_pool_multiplier = 2.0; // Example multiplier
    params.big_blind = 1.0;

    // Create a new SpinGoGame instance
    SpinGoGame game(params.big_blind / 2, params.big_blind, 0.0, 0.0, 0.0);
    SpinGoState state = game.newInitialState();

    // Print initial game state
    cout << "Initial Stacks: ";
    printVector(state.initial_stacks);
    cout << endl;
    cout << "Initial Game State:" << endl;
    cout << state.toString() << endl;

    // Simulate a simple game loop with random actions
    while (!state.isTerminal()) {
        int currentPlayer = state.currentPlayer();
        cout << "Current Player: " << currentPlayer << endl;
            
        // Get legal actions and choose one randomly
        vector<Action> legalActions = state.legalActions();
        if (!legalActions.empty()) {
            int randomIndex = rand() % legalActions.size();
            Action randomAction = legalActions[randomIndex];
            
            // Print the action taken
            cout << "Player " << currentPlayer << " takes action: ";
            switch (randomAction) {
                case Action::FOLD:
                    cout << "FOLD" << endl;
                    break;
                case Action::CHECK:
                    cout << "CHECK" << endl;
                    break;
                case Action::CALL:
                    cout << "CALL" << endl;
                    break;
                case Action::RAISE:
                    cout << "RAISE" << endl;
                    break;
                case Action::ALL_IN:
                    cout << "ALL IN" << endl;
                    break;
                case Action::DEAL:
                    cout << "DEAL" << endl;
                    break;
            }
            
            state.applyAction(randomAction);
        }

        // Advance to the next player
        state.advanceToNextPlayer();
    }

    // Print final game state
    cout << "Game Over!" << endl;
    cout << "Final Pot: " << state.pot << endl;
    cout << "Final Player Stacks: ";
    printVector(state.players_stack);
    cout << endl;

    return 0;
}

#endif // SPINGO_POKER_H