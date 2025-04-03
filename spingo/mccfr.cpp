#include "spingo.h"
#include <iostream>
#include <sstream>
#include <random>
#include <ctime>
#include <numeric>
#include <iomanip>
#include <fstream>
#include <chrono>
// Standalone Node class (not inside AoFState)
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
Node::Node() : regretSum(3, 0.0), strategy(3, 0.0), strategySum(3, 0.0) {}

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

// Helper function implementation
std::string getInformationSet(const AoFState* state, int player) {
    std::stringstream ss;
    ss << "P" << player << ":";
    
    // Add status of all other players when player is 0 or 1
    if (player <= 1) {
        for (int p = 0; p < 4; p++) {
            if (p != player) {
                ss << "[P" << p;
                if (state->folded[p]) {
                    ss << ":F";  // F for Folded
                } else if (state->all_in_players.find(p) != state->all_in_players.end()) {
                    ss << ":A";  // A for All-in
                } else {
                    ss << ":P";  // P for Playing/Pending
                }
                ss << "]";
            }
        }
    } else {
        // For players 2 and 3, only show players before them
        for (int p = 0; p < player; p++) {
            ss << "[P" << p;
            if (state->folded[p]) {
                ss << ":F";
            } else if (state->all_in_players.find(p) != state->all_in_players.end()) {
                ss << ":A";
            } else {
                ss << ":P";
            }
            ss << "]";
        }
    }
    
    // Add current player's hole cards in abstracted form
    const auto& cards = state->getHoleCards();
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
    
    ss << "Pot:" << state->pot;
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
double mccfr(AoFState* state, int player, std::vector<double>& reachProb) {
    if (state->isTerminal()) {
        std::vector<float> util = state->returns();
        double retVal = util[player];
        return retVal;  // Don't delete state here
    }

    if (state->isChanceNode()) {
        state->applyAction(Action::DEAL);
        double value = mccfr(state, player, reachProb);  // Use the same state
        return value;
    }

    int currPlayer = state->currentPlayer();
    std::string infoSet = getInformationSet(state, currPlayer);

    std::vector<Action> allLegal = state->legalActions();
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
            AoFState nextState = state->clone();  // Create a copy
            nextState.applyAction(allLegal[i]);
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
        
        state->applyAction(allLegal[actionIndex]);  // Modify the existing state
        double value = mccfr(state, player, nextReach);
        return value;
    }
}
void trainMCCFR(AoFGame& game, int iterations) {
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
            AoFState state = game.newInitialState();  // Create state on stack
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

