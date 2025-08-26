#ifndef SPINGO_POKER_H
#define SPINGO_POKER_H

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

// Betting round enum
enum class BettingRound {
    PREFLOP = 0,
    FLOP = 1,
    TURN = 2,
    RIVER = 3,
    SHOWDOWN = 4
};

// Action definitions
enum class Action {
    FOLD = 0,
    CHECK = 1,
    CALL = 2,
    BET_1 = 3,
    BET_1_5 = 4,
    BET_2 = 5,
    BET_3 = 6,
    BET_4 = 7,
    BET_5 = 8,
    BET_6 = 9,
    BET_7 = 10, 
    BET_8 = 11,
    ALL_IN = 12,
    DEAL = 13,
    POST_SB = 14,
    POST_BB = 15,
    UNKNOWN = 16
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
    // Rank-to-value mapping
    const std::unordered_map<std::string, int> RANK_VALUES = {
        {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5},
        {"6", 6}, {"7", 7}, {"8", 8}, {"9", 9},
        {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
    };
    
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

    int evaluateHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards);
    int evaluateFiveCardHand(const std::vector<Card>& hand);
    int makeScore(int handRank, const std::vector<int>& vals);
    void generateCombinations(const std::vector<Card>& cards, int combinationSize, int start,
                            std::vector<Card>& currentCombo, std::vector<std::vector<Card>>& allCombinations);
    bool checkFlush(const std::vector<Card>& hand);
    bool checkStraight(const std::vector<Card>& hand, int &highStraightValue);
};

// Forward declaration
class SpinGoGame;

class SpinGoState {
public:
    SpinGoState();

    std::string to_string() const;
    bool game_over() const { return _game_over; }
    bool is_chance_node() const;
    int current_player() const;
    std::vector<Action> legal_actions() const;
    void apply_action(Action action);
    SpinGoState clone() const { return *this; }
    std::vector<double> returns();
    void write_infosets(const std::string& filename);
    double get_current_player_stack();

    std::vector<Card> cards; // all hole cards (2 per player)
    std::vector<Action> bets; // last action for each player
    std::vector<double> pot; // current round contributions per player
    std::vector<double> players_stack; // stacks remaining per player
    bool _game_over;
    int next_player;
    std::string round; // "preflop", "flop", "turn", "river", "showdown"
    std::set<int> active_players; // players not folded
    std::vector<Card> community_cards; // community cards
    std::vector<double> cumulative_pot; // total chips contributed per player over rounds
    double current_bet;
    std::vector<Card> deck;

private:
    void deal_cards();
    bool betting_round_complete();
    void advance_to_next_player();
    void advance_round();
    bool should_end_game();
};

class SpinGoGame {
public:
    SpinGoGame();
    SpinGoState new_initial_state();
};

// Function declarations
std::string action_to_string(Action action);
double amount(Action a);

#endif // SPINGO_POKER_H 