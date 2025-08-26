#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <chrono>
#include <iomanip>

// Add this line to include the implementation
#include "spingo/spingo.cpp"


// Add these global variables to cache cluster data
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> clusterCache;
bool clustersLoaded = false;

// Forward declare functions
void saveInfosetsToCSV(const std::string& filename);

// Node class for MCCFR
class Node {
public:
    Node() : regretSum(3, 0.0), strategy(3, 0.0), strategySum(3, 0.0) {}
    
    Node(int numActions) : regretSum(numActions, 0.0), 
                           strategy(numActions, 0.0),
                           strategySum(numActions, 0.0) {}
    
    std::vector<double> getStrategy(double realizationWeight) {
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
    
    std::vector<double> getAverageStrategy() const {
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
    
    std::vector<double> regretSum;
    std::vector<double> strategy;
    std::vector<double> strategySum;
};

// Initialize global nodeMap
std::unordered_map<std::string, Node> nodeMap;

// Function to sort a poker hand by rank and return it as a string
std::string sortHandByRank(const std::vector<Card>& cards) {
    // Create a copy of the cards to sort
    std::vector<Card> sortedCards = cards;
    
    // Define a map for card rank values
    std::unordered_map<std::string, int> rankValue = {
        {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8},
        {"9", 9}, {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
    };
    
    // Sort cards by rank
    std::sort(sortedCards.begin(), sortedCards.end(), [&rankValue](const Card& a, const Card& b) {
        return rankValue[a.rank] < rankValue[b.rank];
    });
    
    // Combine into a string with spaces between cards but not at the end
    std::string sortedHand;
    for (size_t i = 0; i < sortedCards.size(); ++i) {
        sortedHand += sortedCards[i].toString();
        if (i < sortedCards.size() - 1) {
            sortedHand += " ";
        }
    }
    
    return sortedHand;
}

// Parse a hand string and return a formatted string of rank counts
std::string parse_hand(const std::string& hand) {
    // Split the hand into individual cards
    std::vector<std::string> cards;
    std::istringstream iss(hand);
    std::string card;
    while (iss >> card) {
        cards.push_back(card);
    }
    
    // Extract ranks from the cards
    std::vector<std::string> ranks;
    for (const auto& card : cards) {
        // Assuming last character is the suit
        ranks.push_back(card.substr(0, card.length() - 1));
    }
    
    // Count occurrences of each rank
    std::unordered_map<std::string, int> rank_counts;
    for (const auto& rank : ranks) {
        if (rank_counts.find(rank) != rank_counts.end()) {
            rank_counts[rank] += 1;
        } else {
            rank_counts[rank] = 1;
        }
    }
    
    // Convert to vector for sorting
    std::vector<std::pair<std::string, int>> ordered_counts(rank_counts.begin(), rank_counts.end());
    
    // Define rank order
    std::unordered_map<std::string, int> rankOrder = {
        {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8},
        {"9", 9}, {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
    };
    
    // Sort by rank order
    std::sort(ordered_counts.begin(), ordered_counts.end(), 
        [&rankOrder](const auto& a, const auto& b) { return rankOrder[a.first] < rankOrder[b.first]; });
    
    // Format the output string
    std::ostringstream result;
    result << "\"{\'";
    
    for (size_t i = 0; i < ordered_counts.size(); ++i) {
        result << ordered_counts[i].first << "\': " << ordered_counts[i].second;
        if (i < ordered_counts.size() - 1) {
            result << ", \'";
        }
    }
    
    result << "}\"";
    return result.str();
}

// Helper function to get rank pattern (needed for suit_pattern)
std::vector<int> parse_hand_ranks(const std::string& hand) {
    // Split the hand into individual cards
    std::vector<std::string> cards;
    std::istringstream iss(hand);
    std::string card;
    while (iss >> card) {
        cards.push_back(card);
    }
    
    // Extract ranks from the cards
    std::vector<std::string> ranks;
    for (const auto& card : cards) {
        // Assuming last character is the suit
        ranks.push_back(card.substr(0, card.length() - 1));
    }
    
    // Define rank order
    std::unordered_map<std::string, int> rankOrder = {
        {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8},
        {"9", 9}, {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
    };
    
    // Sort ranks by value
    std::sort(ranks.begin(), ranks.end(), 
        [&rankOrder](const std::string& a, const std::string& b) {
            return rankOrder[a] < rankOrder[b];
        });
    
    // Count occurrences of each rank
    std::unordered_map<std::string, int> rank_counts;
    for (const auto& rank : ranks) {
        rank_counts[rank]++;
    }
    
    // Extract the counts and sort them in descending order
    std::vector<int> pattern;
    for (const auto& pair : rank_counts) {
        pattern.push_back(pair.second);
    }
    
    std::sort(pattern.begin(), pattern.end(), std::greater<int>());
    return pattern;
}

// Get the suit pattern of a hand as a formatted string
std::string suit_pattern(const std::string& hand) {
    // Split the hand into individual cards
    std::vector<std::string> cards;
    std::istringstream iss(hand);
    std::string card;
    while (iss >> card) {
        cards.push_back(card);
    }
    
    // Extract ranks and suits
    std::vector<std::string> ranks;
    std::vector<char> suits;
    for (const auto& card : cards) {
        ranks.push_back(card.substr(0, card.length() - 1));
        suits.push_back(card.back());
    }
    
    // Define rank order
    std::unordered_map<std::string, int> rankOrder = {
        {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8},
        {"9", 9}, {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
    };
    
    // Create rank-to-group mapping
    std::unordered_map<std::string, int> rankToGroup;
    int groupIdx = 0;
    
    // Sort ranks by frequency (descending) and then by rank value (descending)
    std::unordered_map<std::string, int> rankCounts;
    for (const auto& rank : ranks) {
        rankCounts[rank]++;
    }
    
    std::vector<std::pair<std::string, int>> ranksWithCounts;
    for (const auto& pair : rankCounts) {
        ranksWithCounts.push_back(pair);
    }
    
    std::sort(ranksWithCounts.begin(), ranksWithCounts.end(), 
        [&rankOrder](const auto& a, const auto& b) {
            if (a.second != b.second) {
                return a.second > b.second; // Sort by count (descending)
            }
            return rankOrder[a.first] > rankOrder[b.first]; // Then by rank value (descending)
        });
    
    // Assign group indices
    for (const auto& pair : ranksWithCounts) {
        rankToGroup[pair.first] = groupIdx++;
    }
    
    // Create signature for each suit
    std::unordered_map<char, std::vector<int>> suitSignature;
    for (size_t i = 0; i < cards.size(); ++i) {
        char suit = suits[i];
        std::string rank = ranks[i];
        int group = rankToGroup[rank];
        
        if (suitSignature.find(suit) == suitSignature.end()) {
            suitSignature[suit] = std::vector<int>();
        }
        
        // Check if this group is already in the signature for this suit
        if (std::find(suitSignature[suit].begin(), suitSignature[suit].end(), group) 
            == suitSignature[suit].end()) {
            suitSignature[suit].push_back(group);
        }
    }
    
    // Sort each suit's signature
    for (auto& pair : suitSignature) {
        std::sort(pair.second.begin(), pair.second.end());
    }
    
    // Convert to vector and sort for consistent output
    std::vector<std::vector<int>> pattern;
    for (const auto& pair : suitSignature) {
        pattern.push_back(pair.second);
    }
    
    std::sort(pattern.begin(), pattern.end());
    
    // Format the output string
    std::ostringstream result;
    result << "\"[";
    
    for (size_t i = 0; i < pattern.size(); ++i) {
        result << "[";
        for (size_t j = 0; j < pattern[i].size(); ++j) {
            result << pattern[i][j];
            if (j < pattern[i].size() - 1) {
                result << ", ";
            }
        }
        result << "]";
        if (i < pattern.size() - 1) {
            result << ", ";
        }
    }
    
    result << "]\"";
    return result.str();
}

// Function to convert hole cards to preflop notation (e.g., "AKs", "T9o")
std::string holeCardsToPreflop(const Card& card1, const Card& card2) {
    // Define a map for card rank values
    std::unordered_map<std::string, int> rankValue = {
        {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8},
        {"9", 9}, {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
    };
    
    // Map numeric ranks to characters for compact representation
    std::unordered_map<std::string, std::string> rankChar = {
        {"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {"7", "7"}, {"8", "8"},
        {"9", "9"}, {"10", "10"}, {"J", "J"}, {"Q", "Q"}, {"K", "K"}, {"A", "A"}
    };
    
    // Determine higher and lower rank
    std::string highRank, lowRank;
    if (rankValue[card1.rank] >= rankValue[card2.rank]) {
        highRank = rankChar[card1.rank];
        lowRank = rankChar[card2.rank];
    } else {
        highRank = rankChar[card2.rank];
        lowRank = rankChar[card1.rank];
    }
    
    // Determine if suited or offsuit
    std::string suitedness = (card1.suit == card2.suit) ? "s" : "o";
    
    return lowRank + highRank + suitedness;
}

// Function to preload all clusters from CSV files
void preloadClusters() {
    if (clustersLoaded) return;
    
    std::vector<std::string> rounds = {"flop", "turn", "river"};
    
    for (const auto& round : rounds) {
        std::string filename = "./utils/repr_l2_" + round + "_equities_clustered_hands_with_avg_equity.csv";
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            continue;
        }
        
        std::string line;
        // Skip header line
        std::getline(file, line);
        
        while (std::getline(file, line)) {
            // Parse CSV line properly handling quoted fields
            std::vector<std::string> fields;
            std::string field;
            bool inQuotes = false;
            
            for (char c : line) {
                if (c == '"') {
                    inQuotes = !inQuotes;
                    field += c;
                } else if (c == ',' && !inQuotes) {
                    fields.push_back(field);
                    field.clear();
                } else {
                    field += c;
                }
            }
            fields.push_back(field); // Add the last field
            
            // Check if we have enough fields
            if (fields.size() < 8) continue;
            
            std::string cluster = fields[3];
            std::string rankPattern = fields[6];
            std::string suitPattern = fields[7];
            
            // Clean up patterns to handle whitespace differences
            rankPattern.erase(std::remove_if(rankPattern.begin(), rankPattern.end(), ::isspace), rankPattern.end());
            suitPattern.erase(std::remove_if(suitPattern.begin(), suitPattern.end(), ::isspace), suitPattern.end());
            
            // Create a composite key from rank and suit patterns
            std::string key = rankPattern + "|" + suitPattern;
            clusterCache[round][key] = cluster;
        }
        
        std::cout << "Loaded " << clusterCache[round].size() << " clusters for " << round << std::endl;
    }
    
    clustersLoaded = true;
}

// Updated function to find cluster using the cache
std::string findCluster(const std::string& round, const std::string& rankPattern, const std::string& suitPattern) {
    // Make sure clusters are loaded
    if (!clustersLoaded) {
        preloadClusters();
    }
    
    // Skip preflop as it uses a different abstraction
    if (round == "preflop") {
        return "N/A";
    }
    
    // Clean up patterns
    std::string cleanRankPattern = rankPattern;
    std::string cleanSuitPattern = suitPattern;
    cleanRankPattern.erase(std::remove_if(cleanRankPattern.begin(), cleanRankPattern.end(), ::isspace), cleanRankPattern.end());
    cleanSuitPattern.erase(std::remove_if(cleanSuitPattern.begin(), cleanSuitPattern.end(), ::isspace), cleanSuitPattern.end());
    
    // Create the key
    std::string key = cleanRankPattern + "|" + cleanSuitPattern;
    
    // Look up in cache
    if (clusterCache.find(round) != clusterCache.end() && 
        clusterCache[round].find(key) != clusterCache[round].end()) {
        return clusterCache[round][key];
    }
    
    return "Cluster not found - Rank: " + rankPattern + ", Suit: " + suitPattern;
}

// Helper function to get information set string for a player
std::string getInformationSet(const SpinGoState* state, int player) {
    std::stringstream ss;
    
    // Add round information
    ss << state->round << ":";
    
    // Add player information
    ss << "P" << player << ":";
    
    // Add player's hole cards or abstraction
    if (player * 2 + 1 < state->cards.size()) {
        const Card& card1 = state->cards[player * 2];
        const Card& card2 = state->cards[player * 2 + 1];
        
        // Use the abstraction based on the round
        if (state->round == "preflop") {
            ss << holeCardsToPreflop(card1, card2);
        } else {
            // For post-flop, use the cluster abstraction
            std::vector<Card> allCards = {card1, card2};
            allCards.insert(allCards.end(), state->community_cards.begin(), state->community_cards.end());
            
            // Convert cards to string format for pattern extraction
            std::string handString;
            for (size_t i = 0; i < allCards.size(); ++i) {
                handString += allCards[i].toString();
                if (i < allCards.size() - 1) {
                    handString += " ";
                }
            }
            
            std::string rankPatternStr = parse_hand(handString);
            std::string suitPatternStr = suit_pattern(handString);
            
            // Find cluster for this hand
            std::string cluster = findCluster(state->round, rankPatternStr, suitPatternStr);
            ss << cluster;
        }
    }
    
    // Add previous actions
    ss << ":Actions[";
    // Use bets vector instead of action_history
    for (int i = 0; i < state->bets.size(); i++) {
        if (state->bets[i] != Action::DEAL) {
            ss << action_to_string(state->bets[i]) << " ";
        }
    }
    ss << "]";
    
    // Add pot information
    double totalPot = 0.0;
    for (int p = 0; p < NUM_PLAYERS; p++) {
        totalPot += state->pot[p] + state->cumulative_pot[p];
    }
    ss << ":Pot" << totalPot;
    
    return ss.str();
}

// Sample an action based on strategy probabilities
int sampleAction(const std::vector<double>& probs) {
    static std::mt19937 rng(std::random_device{}());
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
    if (state->game_over) {
        std::vector<double> util;
        for (const auto& ret : state->returns()) {
            util.push_back(static_cast<double>(ret));
        }
        return util[player];
    }
    
    if (state->is_chance_node()) {
        state->apply_action(Action::DEAL);
        return mccfr(state, player, reachProb);
    }
    
    int currPlayer = state->current_player();
    std::string infoSet = getInformationSet(state, currPlayer);
    
    std::vector<Action> allLegal = state->legal_actions();
    
    if (currPlayer == player) {
        // Create node if it doesn't exist
        if (nodeMap.find(infoSet) == nodeMap.end())
            nodeMap.insert({infoSet, Node(allLegal.size())});
        
        Node& node = nodeMap[infoSet];
        std::vector<double> strategy = node.getStrategy(reachProb[player]);
        std::vector<double> util(allLegal.size(), 0.0);
        double nodeUtil = 0.0;
        
        for (size_t i = 0; i < allLegal.size(); i++) {
            SpinGoState nextState = *state;  // Create a copy
            nextState.apply_action(allLegal[i]);
            
            std::vector<double> nextReach = reachProb;
            nextReach[currPlayer] *= strategy[i];
            
            util[i] = mccfr(&nextState, player, nextReach);
            nodeUtil += strategy[i] * util[i];
        }
        
        // Update regrets
        for (size_t i = 0; i < allLegal.size(); i++) {
            double regret = util[i] - nodeUtil;
            node.regretSum[i] += (reachProb[player] > 0 ? regret : 0);
        }
        
        return nodeUtil;
    } else {
        std::string infoSetOpp = getInformationSet(state, currPlayer);
        if (nodeMap.find(infoSetOpp) == nodeMap.end())
            nodeMap.insert({infoSetOpp, Node(allLegal.size())});
        
        Node& oppNode = nodeMap[infoSetOpp];
        std::vector<double> strategy = oppNode.getStrategy(reachProb[currPlayer]);
        int actionIndex = sampleAction(strategy);
        std::vector<double> nextReach = reachProb;
        nextReach[currPlayer] *= strategy[actionIndex];
        
        SpinGoState nextState = *state;  // Create a copy
        nextState.apply_action(allLegal[actionIndex]);
        double value = mccfr(&nextState, player, nextReach);
        return value;
    }
}

// Train the MCCFR model
void trainMCCFR(int iterations, const std::string& outputFile) {
    std::cout << "Training MCCFR model with " << iterations << " iterations..." << std::endl;
    
    SpinGoGame game;
    std::vector<double> totalUtility(NUM_PLAYERS, 0.0);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastReportTime = startTime;
    
    for (int it = 1; it <= iterations; it++) {
        // Calculate progress percentage
        double percentage = (static_cast<double>(it) / iterations) * 100.0;
        
        // Calculate ETA
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - startTime).count();
        
        if (elapsedSeconds > 5 || it == iterations) {  // Update every 5 seconds or at the end
            double iterationsPerSecond = static_cast<double>(it) / std::max(1.0, static_cast<double>(elapsedSeconds));
            int remainingIterations = iterations - it;
            int estimatedRemainingSeconds = static_cast<int>(remainingIterations / iterationsPerSecond);
            
            int etaHours = estimatedRemainingSeconds / 3600;
            int etaMinutes = (estimatedRemainingSeconds % 3600) / 60;
            int etaSeconds = estimatedRemainingSeconds % 60;
            
            std::string etaString = "";
            if (etaHours > 0) {
                etaString += std::to_string(etaHours) + "h ";
            }
            etaString += std::to_string(etaMinutes) + "m " + std::to_string(etaSeconds) + "s";
            
            std::cout << "\rProgress: " << std::fixed << std::setprecision(1) << percentage << "% | "
                      << "Completed " << it << "/" << iterations 
                      << " | Infosets: " << nodeMap.size()
                      << " | Speed: " << std::fixed << std::setprecision(1) << iterationsPerSecond 
                      << " iter/sec | ETA: " << etaString << std::flush;
            
            lastReportTime = currentTime;
        }
        
        for (int p = 0; p < NUM_PLAYERS; p++) {
            SpinGoState state = game.new_initial_state();
            std::vector<double> reachProb(NUM_PLAYERS, 1.0);
            
            double value = mccfr(&state, p, reachProb);
            totalUtility[p] += value;
        }
    }
    
    std::cout << "\nTraining complete. Saving results..." << std::endl;
    
    // Save infosets to CSV file
    saveInfosetsToCSV(outputFile);
}

// Save infosets to CSV file
void saveInfosetsToCSV(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }
    
    // Write header
    file << "Round,Player,Abstraction,PreviousActions,Strategy,CumulatedPot\n";
    
    // Track unique infosets to avoid duplicates
    std::unordered_set<std::string> uniqueInfosetKeys;
    
    for (const auto& entry : nodeMap) {
        std::string infoSet = entry.first;
        std::vector<double> avgStrategy = entry.second.getAverageStrategy();
        
        // Parse the infoset string to extract components
        std::string round, player, abstraction, actions, pot;
        
        // Extract round (before first colon)
        size_t pos = infoSet.find(":");
        if (pos != std::string::npos) {
            round = infoSet.substr(0, pos);
            infoSet = infoSet.substr(pos + 1);
        }
        
        // Extract player (before next colon)
        pos = infoSet.find(":");
        if (pos != std::string::npos) {
            player = infoSet.substr(0, pos);
            infoSet = infoSet.substr(pos + 1);
        }
        
        // Extract abstraction (before :Actions)
        pos = infoSet.find(":Actions");
        if (pos != std::string::npos) {
            abstraction = infoSet.substr(0, pos);
            infoSet = infoSet.substr(pos);
        }
        
        // Extract actions (between [ and ])
        pos = infoSet.find("[");
        size_t endPos = infoSet.find("]");
        if (pos != std::string::npos && endPos != std::string::npos) {
            actions = infoSet.substr(pos + 1, endPos - pos - 1);
        }
        
        // Extract pot (after :Pot)
        pos = infoSet.find(":Pot");
        if (pos != std::string::npos) {
            pot = infoSet.substr(pos + 4);
        }
        
        // Create a unique key for this infoset to detect duplicates
        std::string uniqueKey = round + "|" + player + "|" + abstraction + "|" + actions;
        
        // Skip if we've already processed this infoset
        if (uniqueInfosetKeys.find(uniqueKey) != uniqueInfosetKeys.end()) {
            continue;
        }
        
        // Add to our set of processed infosets
        uniqueInfosetKeys.insert(uniqueKey);
        
        // Determine legal actions based on the round and previous actions
        std::vector<Action> legalActions;
        
        // Check if there was a bet in previous actions
        bool hasBet = actions.find("BET_1") != std::string::npos || 
                      actions.find("BET_1_5") != std::string::npos ||
                      actions.find("BET_2") != std::string::npos ||
                      actions.find("BET_3") != std::string::npos ||
                      actions.find("BET_4") != std::string::npos ||
                      actions.find("BET_5") != std::string::npos ||
                      actions.find("BET_6") != std::string::npos ||
                      actions.find("BET_7") != std::string::npos ||
                      actions.find("ALL_IN") != std::string::npos;
        
        if (round == "preflop") {
            // Preflop actions
            if (hasBet) {
                legalActions = {Action::FOLD, Action::CALL, Action::ALL_IN};
            } else {
                legalActions = {Action::FOLD, Action::CALL, Action::BET_3};
            }
        } else if (round == "flop" || round == "turn" || round == "river") {
            // Post-flop actions
            if (hasBet) {
                legalActions = {Action::FOLD, Action::CALL, Action::ALL_IN};
            } else {
                legalActions = {Action::CHECK, Action::BET_3, Action::ALL_IN};
            }
        }
        
        // Clean up the actions string to remove any "unknown" actions
        std::string cleanedActions = "";
        std::istringstream actionStream(actions);
        std::string actionToken;
        while (actionStream >> actionToken) {
            if (actionToken != "unknown" && actionToken != "UNKNOWN") {
                if (!cleanedActions.empty()) {
                    cleanedActions += " ";
                }
                cleanedActions += actionToken;
            }
        }
        
        // Format strategy as a string with action names
        std::ostringstream strategyStr;
        
        // Create a mapping from legal actions to strategy probabilities
        std::unordered_map<Action, double> actionToProb;
        
        // First, initialize all legal actions with zero probability
        for (const auto& action : legalActions) {
            actionToProb[action] = 0.0;
        }
        
        // Then, fill in the probabilities we have from avgStrategy
        // Make sure we don't go out of bounds
        int numActionsToUse = std::min(static_cast<int>(avgStrategy.size()), static_cast<int>(legalActions.size()));
        for (int i = 0; i < numActionsToUse; i++) {
            actionToProb[legalActions[i]] = avgStrategy[i];
        }
        
        // Now output all legal actions with their probabilities
        bool firstAction = true;
        for (const auto& action : legalActions) {
            std::string actionName = action_to_string(action);
            
            // Skip unknown actions
            if (actionName == "unknown" || actionName == "UNKNOWN") {
                continue;
            }
            
            if (!firstAction) {
                strategyStr << ",";
            }
            strategyStr << actionName << ":" << std::fixed << std::setprecision(6) << actionToProb[action];
            firstAction = false;
        }
        
        // Write to CSV
        file << round << ","
             << player.substr(1) << "," // Remove 'P' from player
             << abstraction << ","
             << "\"" << cleanedActions << "\","
             << "\"" << strategyStr.str() << "\","
             << pot << "\n";
    }
    
    file.close();
    std::cout << "Saved " << uniqueInfosetKeys.size() << " unique infosets to " << filename << std::endl;
}

// Generate scenarios and track infosets
void generateScenarios(int maxSamples, const std::string& outputFile) {
    // Preload all clusters at the start
    preloadClusters();
    
    SpinGoGame game;
    std::mt19937 rng(std::random_device{}());
    std::ofstream outFile(outputFile, std::ios::out | std::ios::trunc);
    
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file " << outputFile << std::endl;
        return;
    }
    
    // Write header with added CumulatedPot column
    outFile << "Round,Player,Abstraction,PreviousActions,CumulatedPot\n";
    
    // Track unique infosets and total count
    std::unordered_map<std::string, int> infosetCounts;
    int totalInfosets = 0;
    
    // Track progress with timing
    auto startTime = std::chrono::high_resolution_clock::now();
    auto globalStartTime = startTime;
    int lastReportedSample = 0;
    
    for (int sample = 0; sample < maxSamples; ++sample) {
        SpinGoState state = game.new_initial_state();
        
        // Track previous actions for infoset
        std::vector<std::string> previousActions;
        
        while (!state.game_over) {
            // Apply DEAL action if it's a chance node
            if (state.is_chance_node()) {
                state.apply_action(Action::DEAL);
                continue;
            }
            
            int currentPlayer = state.current_player();
            if (currentPlayer < 0) break; // Game over
            
            // Get hole cards for current player
            std::vector<Card> holeCards;
            if (currentPlayer * 2 + 1 < state.cards.size()) {
                holeCards.push_back(state.cards[currentPlayer * 2]);
                holeCards.push_back(state.cards[currentPlayer * 2 + 1]);
            }
            
            // Skip if we don't have hole cards yet
            if (holeCards.size() < 2) {
                // Choose a random action
                std::vector<Action> actions = state.legal_actions();
                std::uniform_int_distribution<int> dist(0, actions.size() - 1);
                Action randomAction = actions[dist(rng)];
                state.apply_action(randomAction);
                
                if (randomAction != Action::DEAL) {
                    // Only add valid action strings, not "unknown"
                    std::string actionStr = action_to_string(randomAction);
                        previousActions.push_back(actionStr);
                }
                continue;
            }
            
            // Create abstraction based on the round
            std::string abstraction;
            std::string rankPatternStr = "";
            std::string suitPatternStr = "";
            
            if (state.round == "preflop") {
                // Preflop abstraction: use standard notation (AKs, T9o, etc.)
                abstraction = holeCardsToPreflop(holeCards[0], holeCards[1]);
            } else {
                // Post-flop abstraction: use rank pattern and suit pattern
                std::vector<Card> allCards = holeCards;
                allCards.insert(allCards.end(), state.community_cards.begin(), state.community_cards.end());
                
                // Sort cards by rank
                std::unordered_map<std::string, int> rankValue = {
                    {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8},
                    {"9", 9}, {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
                };
                std::sort(allCards.begin(), allCards.end(), [&rankValue](const Card& a, const Card& b) {
                    return rankValue[a.rank] < rankValue[b.rank];
                });
                
                // Convert cards to string format
                std::string handString;
                for (size_t i = 0; i < allCards.size(); ++i) {
                    handString += allCards[i].toString();
                    if (i < allCards.size() - 1) {
                        handString += " ";
                    }
                }
                
                rankPatternStr = parse_hand(handString);
                suitPatternStr = suit_pattern(handString);
                
                // Find cluster for this hand (now uses cache)
                abstraction = findCluster(state.round, rankPatternStr, suitPatternStr);
            }
            
            // Format previous actions with spaces between them
            std::string prevActionsStr;
            for (size_t i = 0; i < previousActions.size(); ++i) {
                prevActionsStr += previousActions[i];
                if (i < previousActions.size() - 1) {
                    prevActionsStr += " ";
                }
            }
            
            // Calculate total pot size by summing all players' current and cumulative contributions
            double totalPot = 0.0;
            if (state.game_over) {
                // Sum all contributions when game is over
                for (int p = 0; p < state.cumulative_pot.size(); p++) {
                    totalPot += state.cumulative_pot[p];
                }
            } else {
                // For ongoing games, sum current pot and cumulative pot
                for (int p = 0; p < state.pot.size(); p++) {
                    totalPot += state.pot[p];  // Current round contributions
                    totalPot += state.cumulative_pot[p];  // Previous rounds contributions
                }
            }
            
            // Create a unique key for this infoset
            std::string infosetKey = state.round + "|" + abstraction + "|" + prevActionsStr;
            
            // Track all infosets, not just unique ones
            infosetCounts[infosetKey]++;
            totalInfosets++;
            
            // Only write to file if we haven't seen this infoset before
            if (infosetCounts[infosetKey] == 1) {
                // Write infoset to file with pot size
                outFile << state.round << ","
                       << currentPlayer << ","
                       << abstraction << ","
                       << prevActionsStr << ","
                       << totalPot << "\n";
            }
            
            // Choose a random action
            std::vector<Action> legalActions = state.legal_actions();
            std::uniform_int_distribution<int> dist(0, legalActions.size() - 1);
            Action randomAction = legalActions[dist(rng)];

            // Store the current round before applying the action
            std::string currentRound = state.round;

            state.apply_action(randomAction);

            // Check if the round has changed after applying the action
            if (currentRound != state.round) {
                // Clear previous actions when moving to a new round
                previousActions.clear();
            }

            if (randomAction != Action::DEAL) {
                // Only add valid action strings, not "unknown"
                std::string actionStr = action_to_string(randomAction);
                if (actionStr != "unknown") {
                    previousActions.push_back(actionStr);
                }
            }
        }
        
        // Progress indicator with timing information
        if (sample % 10 == 0 || sample == maxSamples - 1) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - startTime).count();
            
            auto totalElapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                currentTime - globalStartTime).count();
            
            if (elapsedSeconds > 0 || sample == maxSamples - 1) {
                int samplesProcessed = sample - lastReportedSample;
                double samplesPerSecond = static_cast<double>(samplesProcessed) / std::max(1.0, static_cast<double>(elapsedSeconds));
                
                // Calculate ETA
                std::string etaString = "N/A";
                if (samplesPerSecond > 0 && sample < maxSamples - 1) {
                    int remainingSamples = maxSamples - sample - 1;
                    int estimatedRemainingSeconds = static_cast<int>(remainingSamples / samplesPerSecond);
                    
                    int etaHours = estimatedRemainingSeconds / 3600;
                    int etaMinutes = (estimatedRemainingSeconds % 3600) / 60;
                    int etaSeconds = estimatedRemainingSeconds % 60;
                    
                    std::ostringstream etaStream;
                    if (etaHours > 0) {
                        etaStream << etaHours << "h ";
                    }
                    etaStream << etaMinutes << "m " << etaSeconds << "s";
                    etaString = etaStream.str();
                }
                
                // Calculate progress percentage
                double progressPercent = (static_cast<double>(sample + 1) / maxSamples) * 100.0;
                
                // Clear the line and print updated progress
                std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear line
                std::cout << "Progress: " << std::fixed << std::setprecision(1) << progressPercent << "% | "
                          << "Completed " << sample + 1 << "/" << maxSamples 
                          << " | Unique infosets: " << infosetCounts.size()
                          << " | Speed: " << std::fixed << std::setprecision(1) << samplesPerSecond 
                          << " samples/sec | ETA: " << etaString << std::flush;
                
                startTime = currentTime;
                lastReportedSample = sample;
            }
        }
    }
    
    // Calculate and print duplicate statistics
    int duplicates = totalInfosets - infosetCounts.size();
    double duplicatePercentage = (static_cast<double>(duplicates) / totalInfosets) * 100.0;
    
    std::cout << "\nGenerated " << infosetCounts.size() << " unique infosets from " 
              << maxSamples << " scenarios to " << outputFile << std::endl;
    std::cout << "Total infosets encountered: " << totalInfosets << std::endl;
    std::cout << "Number of duplicates: " << duplicates << " (" 
              << std::fixed << std::setprecision(5) << duplicatePercentage << "%)" << std::endl;
    
    outFile.close();
}

// Update main function to focus only on MCCFR training
int main(int argc, char* argv[]) {
    // Default parameters
    int mccfrIterations = 1000;  // Default to 1000 iterations
    std::string mccfrOutputFile = "mccfr_infosets.csv";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--mccfr" && i + 1 < argc) {
            mccfrIterations = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            mccfrOutputFile = argv[++i];
        }
    }
    
    // Preload clusters
    preloadClusters();
    
    // Run MCCFR training
    trainMCCFR(mccfrIterations, mccfrOutputFile);
    
    return 0;
}