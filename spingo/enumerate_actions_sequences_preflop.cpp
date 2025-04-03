/*
Program to enumerate all possible action sequences in the flop round
of Spin & Go and calculate their total number.

Compile with:
g++ -std=c++17 -O2 enumerate_actions_sequences.cpp -o enumerate_actions_sequences
*/

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <random>
#include <queue>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

// Constants from spingo.cpp
constexpr int NUM_PLAYERS = 3;
constexpr float INITIAL_STACK = 15.0;

// Card representation
struct Card {
    std::string rank;
    std::string suit;

    std::string toString() const {
        return rank + suit;
    }
};

// Action enum - simplified version of what's in spingo.cpp
enum class Action {
    FOLD = 0,
    CHECK = 1,
    CALL = 2,
    BET_2 = 4,
    ALL_IN = 11,
    DEAL = 12,
    POST_SB = 13,
    POST_BB = 14,
    UNKNOWN = 15
};

// Helper function to convert Action to string
string action_to_string(Action action) {
    switch (action) {
        case Action::FOLD: return "FOLD";
        case Action::CHECK: return "CHECK";
        case Action::CALL: return "CALL";
        case Action::BET_2: return "BET_2";
        case Action::ALL_IN: return "ALL_IN";
        case Action::DEAL: return "DEAL";
        case Action::POST_SB: return "POST_SB";
        case Action::POST_BB: return "POST_BB";
        case Action::UNKNOWN: return "UNKNOWN";
        default: return "INVALID_ACTION";
    }
}

// Action amounts
unordered_map<Action, double> ACTION_AMOUNTS = {
    {Action::BET_2, 2.0},
    {Action::POST_SB, 0.5},
    {Action::POST_BB, 1.0}
};

double amount(Action a) {
    if(ACTION_AMOUNTS.find(a) != ACTION_AMOUNTS.end())
        return ACTION_AMOUNTS[a];
    return 0.0;
}
std::string split_and_remove_last(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> words;
    std::string word;

    // Split the string by spaces
    while (iss >> word) {
        words.push_back(word);
    }

    // Remove the last element if the vector is not empty
    if (!words.empty()) {
        words.pop_back();
    }

    // Create a string object to output the result
    std::string result;
    for (const auto& w : words) {
        result += w + " "; // Append each word followed by a space
    }

    // Trim the trailing space
    if (!result.empty()) {
        result.pop_back();
    }

    return result; // Return the resulting string
}

// Simplified version of SpinGoState focused on the flop round
class SimplifiedSpinGoState {
public:
    vector<Card> cards; // all hole cards (2 per player)
    vector<Action> bets; // last action for each player
    vector<double> pot; // current round contributions per player
    vector<double> players_stack; // stacks remaining per player
    bool game_over;
    int next_player;
    string round; // only "flop" in this simplified version
    set<int> active_players; // players not folded
    double current_bet = 0.0;
    vector<Action> action_history; // To keep track of the action sequence

    SimplifiedSpinGoState() {
        bets = vector<Action>(NUM_PLAYERS, Action::UNKNOWN);
        pot = vector<double>(NUM_PLAYERS, 0.0);
        players_stack = vector<double>(NUM_PLAYERS, INITIAL_STACK);
        game_over = false;
        next_player = 0;
        round = "flop";
        for (int p = 0; p < NUM_PLAYERS; p++)
            active_players.insert(p);
    }

    // Clone function for BFS
    SimplifiedSpinGoState clone() const {
        SimplifiedSpinGoState copy;
        copy.bets = this->bets;
        copy.pot = this->pot;
        copy.players_stack = this->players_stack;
        copy.game_over = this->game_over;
        copy.next_player = this->next_player;
        copy.round = this->round;
        copy.active_players = this->active_players;
        copy.current_bet = this->current_bet;
        copy.action_history = this->action_history;
        return copy;
    }

    // Returns the index of the current player (or -1 if game is over)
    int current_player() const {
        if (game_over || round != "preflop")
            return -1;
        return next_player;
    }

    // Return vector of legal actions for the current player
    vector<Action> legal_actions() const {
        vector<Action> actions;
        if (game_over || round != "preflop")
            return actions;
        
        int player = current_player();
        
        // FOLD is always allowed.
        actions.push_back(Action::FOLD);
        
        // If no bet has been made or player has already matched current bet, CHECK is legal.
        if (current_bet == 0 || current_bet == pot[player]) {
            actions.push_back(Action::CHECK);
            actions.erase(remove(actions.begin(), actions.end(), Action::FOLD), actions.end());
        }
        // Otherwise, allow CALL if the player has enough chips.
        else {
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
        
        // Raise options (only if there is already a bet).
        if (current_bet > 0) {
            // Add BET_2 as a raise option
            if (players_stack[player] > amount(Action::BET_2) && current_bet < amount(Action::BET_2)) {
                actions.push_back(Action::BET_2);
            }
        }
        
        // All-In is allowed if the player has any chips.
        if (players_stack[player] > 0)
            actions.push_back(Action::ALL_IN);
        
        return actions;
    }

    // Check whether the betting round is complete
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
        double first_pot = pot[*active_players.begin()];
        for (int p : active_players) {
            if (pot[p] != first_pot) {
                return false;
            }
        }

        // Special case for preflop: Check if player 1 has posted the big blind and others have acted
        if (round == "preflop") {
            // If we haven't even posted blinds yet
            if (pot[0] < amount(Action::POST_SB) || pot[1] < amount(Action::POST_BB)) {
                return false;
            }
            
            // Check if all players have acted after the big blind
            bool allPlayersActed = true;
            for (int p : active_players) {
                if (bets[p] == Action::UNKNOWN || bets[p] == Action::POST_SB || bets[p] == Action::POST_BB) {
                    if (!(p == 0 && bets[p] == Action::POST_SB) && !(p == 1 && bets[p] == Action::POST_BB)) {
                        allPlayersActed = false;
                        break;
                    }
                }
            }
            
            return allPlayersActed;
        }

        return true;
    }

    // Advance to the next player
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

    // Advance the round
    void advance_round() {
        if (round == "flop")
            round = "flop";
        game_over = true; // We're only concerned with flop in this simulation
    }

    // Apply the given action
    void apply_action(Action action) {
        int player = current_player();
        vector<Action> legal = legal_actions();
        if (find(legal.begin(), legal.end(), action) == legal.end()) {
            throw runtime_error("Illegal action chosen for player " + to_string(player));
        }
        
        bets[player] = action;
        action_history.push_back(action); // Record the action
        
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
        } else if (action == Action::BET_2) {
            double bet_amt = amount(action);
            current_bet = bet_amt;
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
        }

        // After the action, check if betting round is complete.
        if (betting_round_complete()) {
            advance_round();
        } else {
            advance_to_next_player();
        }
    }

    // String representation of action history
    string action_history_string() const {
        string result;
        for (const auto& action : action_history) {
            result += action_to_string(action) + " ";
        }
        return result;
    }
};

int main() {
    // Use BFS to enumerate all possible action sequences in the preflop round
    queue<SimplifiedSpinGoState> queue;
    SimplifiedSpinGoState initial_state;
    initial_state.round = "preflop"; // Ensure the round is set to preflop
    queue.push(initial_state);
    int sequence_count = 0;
    map<string, int> unique_sequences;

    ofstream output_file("preflop_infosets.txt", ios::trunc); // Ensure the file is cleared before writing

    while (!queue.empty()) {
        SimplifiedSpinGoState current_state = queue.front();
        queue.pop();
        
        // If the game is over or we're past preflop, count this sequence
        if (current_state.game_over || current_state.round != "preflop") {
            string input = current_state.action_history_string();
            string sequence = split_and_remove_last(input);

            if (unique_sequences.find(sequence) == unique_sequences.end()) {
                unique_sequences[sequence] = 1;
                sequence_count++;
                output_file << sequence << endl; // Write the unique sequence to the file
            }
            continue;
        }
        
        // Get all legal actions for the current state
        vector<Action> legal_actions = current_state.legal_actions();
    

        // Create a new state for each legal action and add it to the queue
        for (const auto& action : legal_actions) {
            SimplifiedSpinGoState new_state = current_state.clone();
            new_state.apply_action(action);
            queue.push(new_state);
        }
    }

    cout << "Total number of unique preflop action sequences: " << sequence_count << endl;

    // Close the output file
    output_file.close();
    
    return 0;
}