#ifndef AOF_POKER_H
#define AOF_POKER_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <tuple>

// Constants definitions
static const float STARTING_STACK_BB = 15.0;
static const int NUM_PLAYERS = 3;

const std::vector<std::string> RANKS = {"2", "3", "4", "5", "6", "7", "8", "9",
"10", "J", "Q", "K", "A"};
const std::vector<std::string> SUITS = {"h", "d", "c", "s"};

// Card structure
struct Card {
    std::string rank;
    std::string suit;
    std::string toString() const;
};

// Build a standard 52-card deck
std::vector<Card> make_deck();

// Action definitions
enum class Action {
    FOLD = 0,
    ALL_IN = 1,
    DEAL = 2
};

// GameParameters structure
struct GameParameters {
    float rake_per_hand;
    float jackpot_fee_per_hand;
    float jackpot_payout_percentage;
};

// Function to get game parameters based on stakes
GameParameters getGameParameters(const std::pair<float, float>& stakes);

class PokerEvaluator {
public:
    const std::unordered_map<std::string, int> RANK_VALUES;
    
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

    std::vector<int> evaluateHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards);
    std::vector<int> evaluateFiveCardHand(const std::vector<Card>& hand);
    std::vector<int> makeScore(int handRank, const std::vector<int>& vals);
    void generateCombinations(const std::vector<Card>& cards, int combinationSize, int start,
                            std::vector<Card>& currentCombo, std::vector<std::vector<Card>>& allCombinations);
    bool checkFlush(const std::vector<Card>& hand);
    bool checkStraight(const std::vector<Card>& hand, int &highStraightValue);
};

// Forward declaration
class AoFGame;

class AoFState {
public:
    AoFState(AoFGame* game);

    std::string toString() const;
    bool isTerminal() const { return _game_over; }
    bool isChanceNode() const;
    int currentPlayer() const;
    std::vector<Action> legalActions() const;
    void applyAction(Action action);
    AoFState clone() const { return *this; }
    std::vector<float> returns();
    const std::vector<Card>& getHoleCards() const { return cards; }
    const std::vector<bool>& getFolded() const { return folded; }
    const std::set<int>& getAllInPlayers() const { return all_in_players; }
    float getPot() const { return pot; }

    float pot = 1.5;
    std::vector<float> initial_stacks;
    std::vector<float> players_stack;
    std::vector<bool> folded;
    std::set<int> all_in_players;

private:
    void dealCards();
    void advanceToNextPlayer();
    void handleGameEnd();
    void calculateSidePots();

    AoFGame* game;
    bool _game_over;
    int _next_player;
    std::vector<Card> deck;
    std::vector<Card> cards;
    float sb;
    float bb;
    std::vector<Card> community_cards;
    std::vector<std::tuple<float, std::vector<int>>> side_pots;
};

class AoFGame {
public:
    AoFGame(float small_blind, float big_blind, float rake_per_hand = 0.0, float jackpot_fee_per_hand = 0.0, float jackpot_payout_percentage = 0.0, const std::vector<float>& initial_stacks_bb = std::vector<float>());
    AoFState newInitialState();

    float small_blind;
    float big_blind;
    float rake_per_hand;
    float jackpot_fee_per_hand;
    float jackpot_payout_percentage;
    std::vector<float> initial_stacks;
};

// Function declarations
int evaluateFive(const std::vector<Card>& hand);
void runTests();

#endif // AOF_POKER_H 