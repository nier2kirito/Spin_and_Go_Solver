#include "spingo.h"
#include <iostream>
#include <sstream>
#include <random>
#include <ctime>
#include <numeric>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <set>

// Forward declarations - keep only these, remove any class definitions
class Node {
public:
    Node();
    Node(int numActions);
    std::vector<double> getStrategy(double realizationWeight);
    std::vector<double> getAverageStrategy() const;
    std::vector<double> regretSum;
    std::vector<double> strategy;
    std::vector<double> strategySum;
};

// Initialize global nodeMap
std::unordered_map<std::string, Node> nodeMap;

// Node implementation
Node::Node() : regretSum(5, 0.0), strategy(5, 0.0), strategySum(5, 0.0) {}

Node::Node(int numActions) : regretSum(numActions, 0.0), 
                           strategy(numActions, 0.0),
                           strategySum(numActions, 0.0) {}

std::vector<double> Node::getStrategy(double realizationWeight) {
    double normalizingSum = 0;
    int numActions = regretSum.size();
    for (int a = 0; a < numActions; a++) {
        strategy[a] = (regretSum[a] > 0 ? regretSum[a] : 0);
        normalizingSum += strategy[a];
    }
    for (int a = 0; a < numActions; a++) {
        if (normalizingSum > 0)
            strategy[a] /= normalizingSum;
        else
            strategy[a] = 1.0 / numActions;
        strategySum[a] += realizationWeight * strategy[a];
    }
    return strategy;
}

std::vector<double> Node::getAverageStrategy() const {
    std::vector<double> avg(strategySum.size(), 0.0);
    double sum = 0;
    for (size_t a = 0; a < strategySum.size(); a++)
        sum += strategySum[a];
    for (size_t a = 0; a < strategySum.size(); a++) {
        if (sum > 0)
            avg[a] = strategySum[a] / sum;
        else
            avg[a] = 1.0 / strategySum.size();
    }
    return avg;
}

// Helper function to determine if two cards are suited
bool areSuited(const Card& card1, const Card& card2) {
    return card1.suit == card2.suit;
}

// Helper function to get abstracted card representation
std::string getAbstractedCard(const Card& card1, const Card& card2, bool isFirst) {
    if (isFirst) {
        return card1.rank;  // First card shows only rank
    } else {
        return card2.rank + (areSuited(card1, card2) ? "s" : "o");  // Second card shows rank + suited/offsuit
    }
}

// Helper function to get flop cluster from the clustering file
int getFlopCluster(const std::vector<Card>& communityCards) {
    // Default cluster if not found
    int defaultCluster = 0;
    
    // Check if we have at least 3 cards for the flop
    if (communityCards.size() < 3) {
        return defaultCluster;
    }
    
    // Create a string representation of the flop cards
    std::string flopStr = communityCards[0].toString() + " " + 
                          communityCards[1].toString() + " " + 
                          communityCards[2].toString();
    
    // Here you would look up the cluster from the flop clustering file
    // This is a placeholder - you'll need to implement the actual lookup
    // from ./utils/l2_flop_clustered_hands.csv
    
    // For now, return a default value
    return defaultCluster;
}

// Helper function to get turn cluster from the clustering file
int getTurnCluster(const std::vector<Card>& communityCards) {
    // Default cluster if not found
    int defaultCluster = 0;
    
    // Check if we have at least 4 cards for the turn
    if (communityCards.size() < 4) {
        return defaultCluster;
    }
    
    // Create a string representation of the turn cards
    std::string turnStr = communityCards[0].toString() + " " + 
                          communityCards[1].toString() + " " + 
                          communityCards[2].toString() + " " +
                          communityCards[3].toString();
    
    // Here you would look up the cluster from the turn clustering file
    // This is a placeholder - you'll need to implement the actual lookup
    // from ./utils/le2_turn_clustered_hands.csv
    
    // For now, return a default value
    return defaultCluster;
}

// Helper function to get river cluster from the clustering file
int getRiverCluster(const std::vector<Card>& communityCards) {
    // Default cluster if not found
    int defaultCluster = 0;
    
    // Check if we have at least 5 cards for the river
    if (communityCards.size() < 5) {
        return defaultCluster;
    }
    
    // Create a string representation of the river cards
    std::string riverStr = communityCards[0].toString() + " " + 
                           communityCards[1].toString() + " " + 
                           communityCards[2].toString() + " " +
                           communityCards[3].toString() + " " +
                           communityCards[4].toString();
    
    // Here you would look up the cluster from the river clustering file
    // This is a placeholder - you'll need to implement the actual lookup
    // from ./utils/le2_river_clustered_hands.csv
    
    // For now, return a default value
    return defaultCluster;
}

// Now define getInformationSet after SpinGoState is fully defined
std::string getInformationSet(const SpinGoState* state, int player) {
    // Don't create infosets for chance nodes
    if (state->is_chance_node()) {
        return "";
    }
    
    std::stringstream ss;
    ss << "P" << player << ":";
    
    
    // Add round information
    ss << " Round:" << state->round << " ";
    
    // Add current player's hole cards in abstracted form
    const auto& cards = state->cards;  // Access member variable directly
    int firstCardIdx = player * 2;
    int secondCardIdx = player * 2 + 1;
    
    // Get both cards to compare ranks
    std::string card1 = cards[firstCardIdx].rank;
    std::string card2 = cards[secondCardIdx].rank;
    
    // Convert ranks to numeric values for comparison
    std::map<std::string, int> rankValues = {
        {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8},
        {"9", 9}, {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
    };
    
    // Determine which card should be shown first based on rank
    if (rankValues[card1] <= rankValues[card2]) {
        ss << getAbstractedCard(cards[firstCardIdx], cards[secondCardIdx], true) << " ";
        ss << getAbstractedCard(cards[firstCardIdx], cards[secondCardIdx], false) << " ";
    } else {
        ss << getAbstractedCard(cards[secondCardIdx], cards[firstCardIdx], true) << " ";
        ss << getAbstractedCard(cards[secondCardIdx], cards[firstCardIdx], false) << " ";
    }
    
    // Add community cards information based on the round
    if (state->round == "flop" || state->round == "turn" || state->round == "river" || state->round == "showdown") {
        // For flop, turn, and river, add cluster information
        if (state->round == "flop") {
            // Get flop cluster from the flop clustering file
            int flopCluster = getFlopCluster(state->community_cards);
            ss << " FlopCluster:" << flopCluster;
        } else if (state->round == "turn") {
            // Get turn cluster from the turn clustering file
            int turnCluster = getTurnCluster(state->community_cards);
            ss << " TurnCluster:" << turnCluster;
        } else if (state->round == "river" || state->round == "showdown") {
            // Get river cluster from the river clustering file
            int riverCluster = getRiverCluster(state->community_cards);
            ss << " RiverCluster:" << riverCluster;
        }
    }
    
    // Add action sequence from the beginning
    ss << " Actions:";
    const auto& bets = state->bets;    // Access member variable directly
    for (int p = 0; p < NUM_PLAYERS; p++) {
        if (bets[p] != Action::UNKNOWN) {
            ss << "[P" << p << ":" << action_to_string(bets[p]) << "]";
        }
    }
    
    // Add pot information
    const auto& pot = state->pot;      // Access member variable directly
    ss << " Pot:" << (pot[0] + pot[1] + pot[2]);
    
    // Add current bet information
    ss << " CurrentBet:" << state->current_bet;
    
    return ss.str();
}

int sampleAction(const std::vector<double>& probs) {
    static std::mt19937 rng((unsigned)std::time(0));
    double r = std::uniform_real_distribution<>(0, 1)(rng);
    double cumulative = 0.0;
    for (size_t i = 0; i < probs.size(); i++) {
        cumulative += probs[i];
        if (r < cumulative)
            return i;
    }
    return probs.size() - 1;
}

// MCCFR implementation
double mccfr(SpinGoState* state, int player, std::vector<double>& reachProb) {
    if (state->game_over()) {
        std::vector<double> util = state->returns();
        return util[player];
    }

    if (state->is_chance_node()) {
        state->apply_action(Action::DEAL);
        return mccfr(state, player, reachProb);
    }

    int currPlayer = state->current_player();
    std::string infoSet = getInformationSet(state, currPlayer);

    std::vector<Action> allLegal = state->legal_actions();
    std::vector<int> legalActs(allLegal.size());
    std::iota(legalActs.begin(), legalActs.end(), 0);

    if (currPlayer == player) {
        if (nodeMap.find(infoSet) == nodeMap.end())
            nodeMap.insert({infoSet, Node(legalActs.size())});
        
        Node& node = nodeMap[infoSet];
        std::vector<double> strategy = node.getStrategy(reachProb[player]);
        std::vector<double> util(legalActs.size(), 0.0);
        double nodeUtil = 0.0;
        
        for (size_t i = 0; i < legalActs.size(); i++) {
            SpinGoState nextState = *state; // Create a copy of the state
            nextState.apply_action(allLegal[i]);
            std::vector<double> nextReach = reachProb;
            nextReach[player] *= strategy[i];
            util[i] = mccfr(&nextState, player, nextReach);
            nodeUtil += strategy[i] * util[i];
        }
        
        for (size_t i = 0; i < legalActs.size(); i++) {
            double regret = util[i] - nodeUtil;
            node.regretSum[i] += (reachProb[player] > 0 ? regret : 0);
        }
        
        return nodeUtil;
    } else {
        std::string infoSetOpp = getInformationSet(state, currPlayer);
        if (nodeMap.find(infoSetOpp) == nodeMap.end())
            nodeMap.insert({infoSetOpp, Node(legalActs.size())});
        
        Node& oppNode = nodeMap[infoSetOpp];
        std::vector<double> strategy = oppNode.getStrategy(reachProb[currPlayer]);
        int actionIndex = sampleAction(strategy);
        std::vector<double> nextReach = reachProb;
        nextReach[currPlayer] *= strategy[actionIndex];
        
        state->apply_action(allLegal[actionIndex]);
        return mccfr(state, player, nextReach);
    }
}

void trainMCCFR(SpinGoGame& game, int iterations) {
    std::cout << "Training progress:\n";
    
    std::vector<double> totalUtility(NUM_PLAYERS, 0.0);
    std::vector<double> finalResults(NUM_PLAYERS, 0.0);
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (int it = 1; it <= iterations; it++) {
        double percentage = (static_cast<double>(it) / iterations) * 100;
        
        // Calculate remaining iterations and time per iteration
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
        double seconds_per_iter = elapsed.count() / static_cast<double>(it);
        int remaining_iters = iterations - it;
        int remaining_seconds = static_cast<int>(seconds_per_iter * remaining_iters);
        
        // Convert to hours, minutes, seconds
        int hours = remaining_seconds / 3600;
        int minutes = (remaining_seconds % 3600) / 60;
        int seconds = remaining_seconds % 60;
        
        std::cout << "\rIteration " << it << " (" << std::fixed << std::setprecision(4) 
                  << percentage << "% completed, ETA: " << hours << "h " 
                  << minutes << "m " << seconds << "s)" << std::flush;
        
        for (int p = 0; p < NUM_PLAYERS; p++) {
            SpinGoState state = game.new_initial_state();  // Create state on stack
            std::vector<double> reachProb(NUM_PLAYERS, 1.0);
            
            double value = mccfr(&state, p, reachProb);
            totalUtility[p] += value;
            finalResults[p] = totalUtility[p] / it;
        }
    }

    // Save final results
    std::ofstream resultFile("final_results.txt");
    if (resultFile.is_open()) {
        for (int p = 0; p < NUM_PLAYERS; p++) {
            resultFile << "Player " << p << " final average utility: " << finalResults[p] << "\n";
        }
        resultFile.close();
    } else {
        std::cerr << "Unable to open file to save results.\n";
    }
}

void printAverageStrategies() {
    std::cout << "\nLearned Strategies:" << std::endl;
    for (const auto& entry : nodeMap) {
        std::cout << "InfoSet: " << entry.first << "\n";
        std::vector<double> avgStrat = entry.second.getAverageStrategy();
        std::cout << " ";
        for (double s : avgStrat)
            std::cout << s << " ";
        std::cout << "\n";
    }
}

// Add this implementation with other functions
void saveInfoSetsToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }

    for (const auto& entry : nodeMap) {
        file << "InfoSet: " << entry.first << "\n";
        std::vector<double> avgStrat = entry.second.getAverageStrategy();
        file << "Strategy: ";
        for (double s : avgStrat) {
            file << std::fixed << std::setprecision(6) << s << " ";
        }
        file << "\n\n";
    }

    file.close();
    std::cout << "InfoSets saved to " << filename << std::endl;
}

int main() {
    // Create a new SpinGoGame instance
    SpinGoGame game;
    
    // Set the number of training iterations
    int iterations = 10000;  // You can adjust this number
    
    // Run the MCCFR training
    trainMCCFR(game, iterations);
    
    // Print the learned strategies
    printAverageStrategies();
    
    // Save the learned strategies to a file
    saveInfoSetsToFile("spingo_strategies.txt");
    
    return 0;
}