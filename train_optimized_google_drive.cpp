#include "spingo/spingo.cpp"
#include <iostream>
#include <sstream>
#include <random>
#include <ctime>
#include <numeric>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <functional>
#include <cstdlib>

// Forward declarations - keep only these, remove any class definitions
class Node {
public:
    Node();
    Node(int numActions);
    Node(const std::vector<Action>& legalActions);
    std::vector<double> getStrategy(double realizationWeight);
    std::vector<double> getAverageStrategy() const;
    std::vector<Action> getActions() const;
    std::vector<double> regretSum;
    std::vector<double> strategy;
    std::vector<double> strategySum;
    std::vector<Action> actions;
    int strategyUpdateCount;  // Rename from visitCount to strategyUpdateCount
};

// Add these global variables to cache cluster data
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> clusterCache;
bool clustersLoaded = false;

// Initialize global nodeMap
std::unordered_map<std::string, Node> nodeMap;

static int mccfr_depth = 0;
// Add mutex for nodeMap access
std::mutex nodeMapMutex;

// Node implementation
Node::Node() : regretSum(5, 0.0), strategy(5, 0.0), strategySum(5, 0.0), strategyUpdateCount(0) {}

Node::Node(int numActions) : regretSum(numActions, 0.0), 
                           strategy(numActions, 0.0),
                           strategySum(numActions, 0.0),
                           strategyUpdateCount(0) {}

Node::Node(const std::vector<Action>& legalActions) : 
    regretSum(legalActions.size(), 0.0), 
    strategy(legalActions.size(), 0.0),
    strategySum(legalActions.size(), 0.0),
    actions(legalActions),
    strategyUpdateCount(0) {}

std::vector<double> Node::getStrategy(double realizationWeight) {
    std::vector<double> result = strategy;
    double normalizingSum = 0;
    int numActions = regretSum.size();
    
    for (int a = 0; a < numActions; a++) {
        result[a] = (regretSum[a] > 0 ? regretSum[a] : 0);
        normalizingSum += result[a];
    }
    
    for (int a = 0; a < numActions; a++) {
        if (normalizingSum > 0)
            result[a] /= normalizingSum;
        else
            result[a] = 1.0 / numActions;
        
        // Update strategy sum
        strategySum[a] += realizationWeight * result[a];
    }
    
    return result;
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

std::vector<Action> Node::getActions() const {
    return actions;
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

// Function to preload all clusters from CSV files
void preloadClusters() {
    if (clustersLoaded) return;
    
    std::vector<std::string> rounds = {"flop", "turn", "river"};
    
    // Pre-allocate expected capacity
    for (const auto& round : rounds) {
        clusterCache[round].reserve(10000); // Adjust based on expected number of clusters
    }
    
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

// Updated helper function to get flop cluster from the clustering file
int getFlopCluster(const std::vector<Card>& communityCards, const std::vector<Card>& holeCards) {
    // Default cluster if not found
    int defaultCluster = 0;
    
    // Check if we have at least 3 cards for the flop and 2 hole cards
    if (communityCards.size() < 3 || holeCards.size() < 2) {
        return defaultCluster;
    }
    
    // Make sure clusters are loaded
    if (!clustersLoaded) {
        preloadClusters();
    }
    
    // Create a vector with all cards (hole cards + community cards)
    std::vector<Card> allCards = holeCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.begin() + 3);
    
    // Sort cards by rank
    std::string sortedHand = sortHandByRank(allCards);
    
    // Get rank and suit patterns
    std::string rankPattern = parse_hand(sortedHand);
    std::string suitPattern = suit_pattern(sortedHand);
    
    // Find cluster using the cache
    std::string clusterStr = findCluster("flop", rankPattern, suitPattern);
    
    // Convert cluster string to int, handling potential errors
    try {
        return std::stoi(clusterStr);
    } catch (const std::exception& e) {
        return defaultCluster;
    }
}

// Updated helper function to get turn cluster from the clustering file
int getTurnCluster(const std::vector<Card>& communityCards, const std::vector<Card>& holeCards) {
    // Default cluster if not found
    int defaultCluster = 0;
    
    // Check if we have at least 4 cards for the turn and 2 hole cards
    if (communityCards.size() < 4 || holeCards.size() < 2) {
        return defaultCluster;
    }
    
    // Make sure clusters are loaded
    if (!clustersLoaded) {
        preloadClusters();
    }
    
    // Create a vector with all cards (hole cards + community cards)
    std::vector<Card> allCards = holeCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.begin() + 4);
    
    // Sort cards by rank
    std::string sortedHand = sortHandByRank(allCards);
    
    // Get rank and suit patterns
    std::string rankPattern = parse_hand(sortedHand);
    std::string suitPattern = suit_pattern(sortedHand);
    
    // Find cluster using the cache
    std::string clusterStr = findCluster("turn", rankPattern, suitPattern);
    
    // Convert cluster string to int, handling potential errors
    try {
        return std::stoi(clusterStr);
    } catch (const std::exception& e) {
        return defaultCluster;
    }
}

// Updated helper function to get river cluster from the clustering file
int getRiverCluster(const std::vector<Card>& communityCards, const std::vector<Card>& holeCards) {
    // Default cluster if not found
    int defaultCluster = 0;
    
    // Check if we have at least 5 cards for the river and 2 hole cards
    if (communityCards.size() < 5 || holeCards.size() < 2) {
        return defaultCluster;
    }
    
    // Make sure clusters are loaded
    if (!clustersLoaded) {
        preloadClusters();
    }
    
    // Create a vector with all cards (hole cards + community cards)
    std::vector<Card> allCards = holeCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.begin() + 5);
    
    // Sort cards by rank
    std::string sortedHand = sortHandByRank(allCards);
    
    // Get rank and suit patterns
    std::string rankPattern = parse_hand(sortedHand);
    std::string suitPattern = suit_pattern(sortedHand);
    
    // Find cluster using the cache
    std::string clusterStr = findCluster("river", rankPattern, suitPattern);
    
    // Convert cluster string to int, handling potential errors
    try {
        return std::stoi(clusterStr);
    } catch (const std::exception& e) {
        return defaultCluster;
    }
}

// Updated getInformationSet function to properly include cumulative pot information

// Now define getInformationSet after SpinGoState is fully defined
std::string getInformationSet(const SpinGoState* state) {
    if (state->is_chance_node()) {
        return "";
    }
    
    // Pre-allocate a reasonably sized string to avoid reallocations
    std::string result;
    result.reserve(256);
    
    int player = state->current_player();
    // Use string append instead of stringstream for better performance
    result.append("P");
    result.append(std::to_string(player));
    result.append(": Round:");
    result.append(state->round);
    result.append(" ");
    
    // Add current player's hole cards in abstracted form
    const auto& cards = state->cards;
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
    if (state->round == "preflop"){
    // Determine which card should be shown first based on rank
    if (rankValues[card1] <= rankValues[card2]) {
        result.append(getAbstractedCard(cards[firstCardIdx], cards[secondCardIdx], true));
        result.append(" ");
        result.append(getAbstractedCard(cards[firstCardIdx], cards[secondCardIdx], false));
        result.append(" ");
    } else {
        result.append(getAbstractedCard(cards[secondCardIdx], cards[firstCardIdx], true));
        result.append(" ");
        result.append(getAbstractedCard(cards[secondCardIdx], cards[firstCardIdx], false));
        result.append(" ");
    }
    }
    // Add community cards information based on the round
    if (state->round == "flop" || state->round == "turn" || state->round == "river" || state->round == "showdown") {
        // For flop, turn, and river, add cluster information
        if (state->round == "flop") {
            int flopCluster = getFlopCluster(state->community_cards, {cards[player*2], cards[player*2+1]});
            result.append(" FlopCluster:");
            result.append(std::to_string(flopCluster));
        } else if (state->round == "turn") {
            int turnCluster = getTurnCluster(state->community_cards, {cards[player*2], cards[player*2+1]});
            result.append(" TurnCluster:");
            result.append(std::to_string(turnCluster));
        } else if (state->round == "river" || state->round == "showdown") {
            int riverCluster = getRiverCluster(state->community_cards, {cards[player*2], cards[player*2+1]});
            result.append(" RiverCluster:");
            result.append(std::to_string(riverCluster));
        }
    }
    
    // Add action sequence with more detailed information
    result.append(" Actions:");
    
    // Include detailed bet amounts for each player
    const auto& bets = state->bets;
    const auto& pot = state->pot;
    const auto& cumulative_pot = state->cumulative_pot;

    auto hist_it = state->round_action_history.find(state->round);
    if (hist_it != state->round_action_history.end()) {
        const auto& current_round_actions = hist_it->second;
        std::stringstream ss;  // Declare the stringstream here
        for (const auto& action_pair : current_round_actions) {
            ss << "[P" << action_pair.first << ":" << action_to_string(action_pair.second) << "]";
        }
        result.append(ss.str());
    }
    
    // Add pot information with individual contributions
    result.append(" Pot:");
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (cumulative_pot[0] + cumulative_pot[1] + cumulative_pot[2] + pot[0] + pot[1] + pot[2]);
        result.append(oss.str());
    }
    
    // Add current bet information
    result.append(" CurrentBet:");
    result.append(std::to_string(state->current_bet));
    
    // Add active players count
    result.append(" ActivePlayers:");
    result.append(std::to_string(state->active_players.size()));
    
    // Add current player
    result.append(" CurrentPlayer:");
    result.append(std::to_string(state->current_player()));
    
    return result;
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
    mccfr_depth++;
    
    if (state->game_over) {
        double result = state->returns()[player];
        if (mccfr_depth > 0) mccfr_depth--;
        return result;
    }

    if (state->is_chance_node()) {
        state->apply_action(Action::DEAL);
        double result = mccfr(state, player, reachProb);
        if (mccfr_depth > 0) mccfr_depth--;
        return result;
    }

    int currPlayer = state->current_player();
    std::string infoSet = getInformationSet(state);

    std::vector<Action> legalActions = state->legal_actions();
    
    if (legalActions.empty()) {
        std::cerr << "Error: No legal actions available for player " << currPlayer << std::endl;
        if (mccfr_depth > 0) mccfr_depth--;
        return 0.0;
    }

    // Create a local copy of the node to work with
    Node localNode;
    bool nodeExists = false;
    
    {
        std::lock_guard<std::mutex> lock(nodeMapMutex);
        auto nodeIt = nodeMap.find(infoSet);
        if (nodeIt != nodeMap.end() && nodeIt->second.strategy.size() == legalActions.size()) {
            localNode = nodeIt->second;  // Make a copy
            nodeExists = true;
        } else {
            localNode = Node(legalActions);
            nodeMap[infoSet] = localNode;  // Store a copy in the map
        }
    }
    
    if (currPlayer == player) {
        // Work with the local copy
        localNode.strategyUpdateCount++;
        std::vector<double> strategy = localNode.getStrategy(reachProb[player]);
        
        // Calculate opponent reach probability for regret scaling
        double opponent_reach_prod = 1.0;
        for (int i = 0; i < NUM_PLAYERS; ++i) {
            if (i != currPlayer) {
                opponent_reach_prod *= reachProb[i];
            }
        }
        
        // Update strategy sum weighted by opponent reach probability
        if (legalActions.size() > 1 && opponent_reach_prod > 0) {
            for (size_t i = 0; i < strategy.size(); i++) {
                localNode.strategySum[i] += opponent_reach_prod * strategy[i];
            }
        }
        
        double nodeUtil = 0.0;
        std::vector<double> actionUtils(legalActions.size());
        
        // Create a single state copy to reuse
        SpinGoState stateCopy = *state;
        
        for (size_t i = 0; i < legalActions.size(); i++) {
            SpinGoState& nextState = (i == 0) ? stateCopy : (stateCopy = *state);
            nextState.apply_action(legalActions[i]);
            
            double originalReachProb = reachProb[player];
            reachProb[player] *= strategy[i];
            actionUtils[i] = mccfr(&nextState, player, reachProb);
            reachProb[player] = originalReachProb;
            
            nodeUtil += strategy[i] * actionUtils[i];
        }
        
        // Update regrets in the local copy, scaled by opponent reach probability
        for (size_t i = 0; i < legalActions.size(); i++) {
            double regret = actionUtils[i] - nodeUtil;
            localNode.regretSum[i] += opponent_reach_prod * regret;
        }
        
        // Update the global node with our changes
        {
            std::lock_guard<std::mutex> lock(nodeMapMutex);
            auto& globalNode = nodeMap[infoSet];
            
            // Update regret sums
            for (size_t i = 0; i < localNode.regretSum.size() && i < globalNode.regretSum.size(); i++) {
                globalNode.regretSum[i] = localNode.regretSum[i];
            }
            
            // Update strategy sums and strategyUpdateCount
            bool strategyActuallyUpdated = false;
            for (size_t i = 0; i < localNode.strategySum.size() && i < globalNode.strategySum.size(); i++) {
                if (globalNode.strategySum[i] != localNode.strategySum[i]) { // Check if value changes
                    globalNode.strategySum[i] = localNode.strategySum[i];
                    strategyActuallyUpdated = true;
                }
            }
            if (strategyActuallyUpdated) {
                globalNode.strategyUpdateCount++;
            }
        }
        
        if (mccfr_depth > 0) mccfr_depth--;
        return nodeUtil;
    } else {
        // For opponent nodes, we just need the strategy
        localNode.strategyUpdateCount++;
        std::vector<double> strategy = localNode.getStrategy(reachProb[currPlayer]);
        
        // Update the strategy sums and strategyUpdateCount in the global node
        {
            std::lock_guard<std::mutex> lock(nodeMapMutex);
            auto& globalNode = nodeMap[infoSet];
            
            // Update strategy sums and strategyUpdateCount
            bool strategyActuallyUpdated = false;
            double opponent_reach_prod = 1.0;
            for (int i = 0; i < NUM_PLAYERS; ++i) {
                if (i != currPlayer) {
                    opponent_reach_prod *= reachProb[i];
                }
            }
            
            if (legalActions.size() > 1 && opponent_reach_prod > 0) {
                for (size_t i = 0; i < strategy.size() && i < globalNode.strategySum.size(); i++) {
                    if (opponent_reach_prod * strategy[i] != 0.0) { // If the additive term is non-zero, it's an update
                        globalNode.strategySum[i] += opponent_reach_prod * strategy[i];
                        strategyActuallyUpdated = true;
                    }
                }
            }
            if (strategyActuallyUpdated) {
                globalNode.strategyUpdateCount++;
            }
        }
        
        int actionIndex = sampleAction(strategy);
        if (actionIndex < 0 || actionIndex >= static_cast<int>(legalActions.size())) {
            actionIndex = 0;
        }
        
        double originalReachProb = reachProb[currPlayer];
        reachProb[currPlayer] *= strategy[actionIndex];
        
        state->apply_action(legalActions[actionIndex]);
        double result = mccfr(state, player, reachProb);
        
        reachProb[currPlayer] = originalReachProb;
        
        if (mccfr_depth > 0) mccfr_depth--;
        return result;
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

// Updated saveInfoSetsToFile function to correctly extract pot information
void saveInfoSetsToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return;
    }

    // Write CSV header with added StrategyUpdateCount column
    file << "Round,Player,Abstraction,PreviousActions,Strategy,CumulatedPot,StrategyUpdateCount\n";

    for (const auto& entry : nodeMap) {
        const std::string& infoSet = entry.first;
        const Node& node = entry.second;
        std::vector<double> avgStrat = node.getAverageStrategy();
        std::vector<Action> actions = node.getActions();
        
        // Parse the infoSet string to extract the required information
        std::string round, player, abstraction, pot;
        
        // Extract player
        size_t playerPos = infoSet.find("P");
        size_t playerEndPos = infoSet.find(":", playerPos);
        if (playerPos != std::string::npos && playerEndPos != std::string::npos) {
            player = infoSet.substr(playerPos + 1, playerEndPos - playerPos - 1);
        }
        
        // Extract round
        size_t roundPos = infoSet.find("Round:");
        size_t roundEndPos = infoSet.find(" ", roundPos + 6);
        if (roundPos != std::string::npos && roundEndPos != std::string::npos) {
            round = infoSet.substr(roundPos + 6, roundEndPos - roundPos - 6);
        }
        
        // Extract abstraction (preflop cards or cluster)
        if (round == "preflop") {
            // For preflop, abstraction is the two cards
            size_t cardsPos = roundEndPos + 1;
            size_t cardsEndPos = infoSet.find(" Actions:", cardsPos);
            if (cardsPos != std::string::npos && cardsEndPos != std::string::npos) {
                abstraction = infoSet.substr(cardsPos, cardsEndPos - cardsPos);
                // Clean up any extra spaces
                abstraction.erase(std::remove(abstraction.begin(), abstraction.end(), ' '), abstraction.end());
            }
        } else {
            // For other rounds, abstraction is the cluster
            std::string clusterPrefix;
            if (round == "flop") clusterPrefix = "FlopCluster:";
            else if (round == "turn") clusterPrefix = "TurnCluster:";
            else if (round == "river" || round == "showdown") clusterPrefix = "RiverCluster:";
            
            size_t clusterPos = infoSet.find(clusterPrefix);
            if (clusterPos != std::string::npos) {
                size_t clusterEndPos = infoSet.find(" ", clusterPos + clusterPrefix.length());
                if (clusterEndPos != std::string::npos) {
                    abstraction = infoSet.substr(clusterPos + clusterPrefix.length(), 
                                                clusterEndPos - clusterPos - clusterPrefix.length());
                }
            }
        }
        
        // Extract previous actions and reformat them to be in chronological order
        std::string previousActions;
        size_t actionsPos = infoSet.find("Actions:");
        size_t actionsEndPos = infoSet.find(" Pot:", actionsPos);
        if (actionsPos != std::string::npos && actionsEndPos != std::string::npos) {
            std::string rawActions = infoSet.substr(actionsPos + 8, actionsEndPos - actionsPos - 8);
            
            // Parse the actions to extract them in order
            std::vector<std::string> actionsList;
            size_t pos = 0;
            while (pos < rawActions.length()) {
                size_t startPos = rawActions.find("[", pos);
                if (startPos == std::string::npos) break;
                
                size_t endPos = rawActions.find("]", startPos);
                if (endPos == std::string::npos) break;
                
                std::string action = rawActions.substr(startPos + 1, endPos - startPos - 1);
                actionsList.push_back(action);
                pos = endPos + 1;
            }
            
            // Format actions in chronological order
            for (size_t i = 0; i < actionsList.size(); ++i) {
                previousActions += actionsList[i];
                if (i < actionsList.size() - 1) {
                    previousActions += "|";
                }
            }
        }
        
        // Extract pot - now using the cumulative pot value
        size_t potPos = infoSet.find("Pot:");
        size_t potEndPos = infoSet.find(" CurrentBet:", potPos);
        if (potPos != std::string::npos && potEndPos != std::string::npos) {
            pot = infoSet.substr(potPos + 4, potEndPos - potPos - 4);
            // Trim any whitespace
            pot.erase(0, pot.find_first_not_of(" \t\n\r\f\v"));
            pot.erase(pot.find_last_not_of(" \t\n\r\f\v") + 1);
        }
        
        // Format strategy as a string with action labels
        std::stringstream strategyStr;
        for (size_t i = 0; i < avgStrat.size(); ++i) {
            // Use the stored action if available, otherwise use a default label
            std::string actionLabel;
            if (i < actions.size()) {
                actionLabel = action_to_string(actions[i]);
            } else {
                actionLabel = "ACTION_" + std::to_string(i);
            }
            
            strategyStr << actionLabel << ":" << std::fixed << std::setprecision(6) << avgStrat[i];
            
            if (i < avgStrat.size() - 1) {
                strategyStr << "|";
            }
        }
        
        // Write the CSV row with strategy update count
        file << round << "," 
             << player << "," 
             << abstraction << "," 
             << previousActions << "," 
             << strategyStr.str() << "," 
             << pot << "," 
             << node.strategyUpdateCount << "\n";  // Add strategy update count to output
    }

    file.close();
    std::cout << "InfoSets saved to " << filename << std::endl;
}

// Add a simple utility to aggregate strategies from multiple files
void aggregateStrategies(const std::vector<std::string>& inputFiles, const std::string& outputFile) {
    // Structure to hold aggregated strategy data
    struct StrategyEntry {
        std::string round;
        std::string player;
        std::string abstraction;
        std::string previousActions;
        std::string pot;
        std::map<std::string, double> actionProbs;  // Action -> probability
        int totalStrategyUpdates; // Changed from totalVisits
    };
    
    // Map from infoset key to aggregated strategy
    std::map<std::string, StrategyEntry> aggregatedStrategies;
    
    // Process each input file
    for (const auto& filename : inputFiles) {
        std::cout << "Processing file: " << filename << std::endl;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Unable to open file: " << filename << std::endl;
            continue;
        }
        
        std::string line;
        // Skip header
        std::getline(file, line);
        
        // Process each line
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string round, player, abstraction, previousActions, strategyStr, pot, strategyUpdateCountStr;
            
            std::getline(ss, round, ',');
            std::getline(ss, player, ',');
            std::getline(ss, abstraction, ',');
            std::getline(ss, previousActions, ',');
            std::getline(ss, strategyStr, ',');
            std::getline(ss, pot, ',');
            std::getline(ss, strategyUpdateCountStr, ',');
            
            // Adjust strategy update count to account for the initial count of 1
            int strategyUpdates = std::stoi(strategyUpdateCountStr);
            // Subtract 1 from the strategy update count to account for initialization
            // but ensure it doesn't go below 0
            strategyUpdates = std::max(0, strategyUpdates - 1);
            
            // Create a key for this infoset
            std::string key = round + "|" + player + "|" + abstraction + "|" + previousActions + "|" + pot;
            
            // Parse strategy string
            std::map<std::string, double> actionProbs;
            std::stringstream stratSS(strategyStr);
            std::string actionProb;
            
            while (std::getline(stratSS, actionProb, '|')) {
                size_t colonPos = actionProb.find(':');
                if (colonPos != std::string::npos) {
                    std::string action = actionProb.substr(0, colonPos);
                    double prob = std::stod(actionProb.substr(colonPos + 1));
                    actionProbs[action] = prob;
                }
            }
            
            // If this is the first time seeing this infoset, initialize it
            if (aggregatedStrategies.find(key) == aggregatedStrategies.end()) {
                StrategyEntry entry;
                entry.round = round;
                entry.player = player;
                entry.abstraction = abstraction;
                entry.previousActions = previousActions;
                entry.pot = pot;
                entry.actionProbs = actionProbs;
                entry.totalStrategyUpdates = strategyUpdates;
                aggregatedStrategies[key] = entry;
            } else {
                // We've seen this infoset before, update with weighted average
                auto& entry = aggregatedStrategies[key];
                
                // For each action in the current strategy
                for (const auto& [action, prob] : actionProbs) {
                    // If we haven't seen this action before, initialize it
                    if (entry.actionProbs.find(action) == entry.actionProbs.end()) {
                        entry.actionProbs[action] = 0.0;
                    }
                    
                    // Update with weighted average
                    double oldWeight = static_cast<double>(entry.totalStrategyUpdates);
                    double newWeight = static_cast<double>(strategyUpdates);
                    double totalWeight = oldWeight + newWeight;
                    
                    if (totalWeight > 0) {
                        entry.actionProbs[action] = (entry.actionProbs[action] * oldWeight + prob * newWeight) / totalWeight;
                    } else if (newWeight > 0) {
                        entry.actionProbs[action] = prob;
                    }
                }
                
                // Update total strategy updates
                entry.totalStrategyUpdates += strategyUpdates;
            }
        }
        
        file.close();
    }
    
    // Write aggregated strategies to output file
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        std::cerr << "Unable to open output file: " << outputFile << std::endl;
        return;
    }
    
    // Write CSV header
    outFile << "Round,Player,Abstraction,PreviousActions,Strategy,CumulatedPot,StrategyUpdateCount\n";
    
    // Write each aggregated strategy
    for (const auto& [key, entry] : aggregatedStrategies) {
        // Format strategy string
        std::stringstream strategyStr;
        bool first = true;
        for (const auto& [action, prob] : entry.actionProbs) {
            if (!first) strategyStr << "|";
            strategyStr << action << ":" << std::fixed << std::setprecision(6) << prob;
            first = false;
        }
        
        // Add 1 back to the strategy update count when writing to file
        // to maintain consistency with the original format
        int outputStrategyUpdateCount = entry.totalStrategyUpdates + 1;
        
        // Write CSV row
        outFile << entry.round << "," 
                << entry.player << "," 
                << entry.abstraction << "," 
                << entry.previousActions << "," 
                << strategyStr.str() << "," 
                << entry.pot << "," 
                << outputStrategyUpdateCount << "\n";
    }
    
    outFile.close();
    std::cout << "Aggregated " << aggregatedStrategies.size() << " strategies to " << outputFile << std::endl;
}

// Add this forward declaration before main()
void trainMCCFRParallel(SpinGoGame& game, int iterations);

// Function to read environment variables from .env file
std::string getEnvVar(const std::string& key) {
    // First try to get from environment
    const char* val = std::getenv(key.c_str());
    if (val != nullptr) {
        return val;
    }
    
    // If not found, try to read from .env file
    std::ifstream envFile(".env");
    if (envFile.is_open()) {
        std::string line;
        while (std::getline(envFile, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Parse KEY=VALUE format
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string envKey = line.substr(0, pos);
                std::string envVal = line.substr(pos + 1);
                
                // Remove quotes if present
                if (envVal.size() >= 2 && 
                    ((envVal.front() == '"' && envVal.back() == '"') || 
                     (envVal.front() == '\'' && envVal.back() == '\''))) {
                    envVal = envVal.substr(1, envVal.size() - 2);
                }
                
                if (envKey == key) {
                    return envVal;
                }
            }
        }
        envFile.close();
    }
    
    return "";
}

// Function to get a fresh access token using the refresh token
std::string getFreshAccessToken() {
    std::string refreshToken = getEnvVar("GOOGLE_DRIVE_REFRESH_TOKEN");
    std::string clientId = getEnvVar("GOOGLE_DRIVE_CLIENT_ID");
    std::string clientSecret = getEnvVar("GOOGLE_DRIVE_CLIENT_SECRET");
    
    if (refreshToken.empty() || clientId.empty() || clientSecret.empty()) {
        std::cerr << "Error: Missing OAuth credentials in environment or .env file" << std::endl;
        return "";
    }
    
    // Create a temporary file to store the response
    std::string tempFile = "token_response.txt";
    
    // Build the curl command to refresh the token
    std::string command = "curl -s -X POST "
                         "-d \"client_id=" + clientId + "\" "
                         "-d \"client_secret=" + clientSecret + "\" "
                         "-d \"refresh_token=" + refreshToken + "\" "
                         "-d \"grant_type=refresh_token\" "
                         "\"https://oauth2.googleapis.com/token\" > " + tempFile;
    
    // Execute the command
    int result = system(command.c_str());
    
    if (result != 0) {
        std::cerr << "Error refreshing access token" << std::endl;
        return "";
    }
    
    // Read the response
    std::ifstream file(tempFile);
    if (!file.is_open()) {
        std::cerr << "Error opening token response file" << std::endl;
        return "";
    }
    
    std::string response((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Delete the temporary file
    remove(tempFile.c_str());
    
    // Parse the JSON response to extract the access token
    // This is a simple parser that works for this specific case
    std::string accessToken;
    size_t pos = response.find("\"access_token\"");
    if (pos != std::string::npos) {
        pos = response.find(":", pos);
        if (pos != std::string::npos) {
            pos = response.find("\"", pos);
            if (pos != std::string::npos) {
                size_t endPos = response.find("\"", pos + 1);
                if (endPos != std::string::npos) {
                    accessToken = response.substr(pos + 1, endPos - pos - 1);
                }
            }
        }
    }
    
    return accessToken;
}

// Function to upload file to Google Drive with retry logic
void uploadToGoogleDrive(const std::string& filename) {
    std::cout << "Uploading " << filename << " to Google Drive..." << std::endl;
    
    // Maximum number of retry attempts
    const int maxRetries = 3;
    bool uploadSuccess = false;
    
    for (int attempt = 1; attempt <= maxRetries && !uploadSuccess; attempt++) {
        if (attempt > 1) {
            std::cout << "Retry attempt " << attempt << " of " << maxRetries << "..." << std::endl;
        }
        
        // Get a fresh access token for each attempt
        std::string accessToken = getFreshAccessToken();
        if (accessToken.empty()) {
            std::cerr << "Error: Failed to obtain access token" << std::endl;
            // Wait before retrying
            if (attempt < maxRetries) {
                std::cout << "Waiting 5 seconds before retrying..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            continue;
        }
        
        // Build the curl command
        std::string command = "curl -s -X POST "
                             "-H \"Authorization: Bearer " + accessToken + "\" "
                             "-F \"metadata={name:'" + filename + "',parents:['root']};type=application/json;charset=UTF-8\" "
                             "-F \"file=@" + filename + "\" "
                             "\"https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart\" > upload_response.txt";
        
        // Execute the command
        std::cout << "Executing upload command..." << std::endl;
        int result = system(command.c_str());
        
        // Check if the command executed successfully
        if (result == 0) {
            // Check the response for errors
            std::ifstream responseFile("upload_response.txt");
            std::string response;
            if (responseFile.is_open()) {
                response = std::string((std::istreambuf_iterator<char>(responseFile)), 
                                       std::istreambuf_iterator<char>());
                responseFile.close();
                remove("upload_response.txt");
            }
            
            // Check if the response contains an error
            if (response.find("\"error\"") != std::string::npos) {
                std::cerr << "Upload failed with API error: " << response << std::endl;
                
                // Check if it's an authorization error
                if (response.find("\"code\": 401") != std::string::npos) {
                    std::cerr << "Authorization error detected. Will retry with fresh token." << std::endl;
                    // Wait before retrying
                    if (attempt < maxRetries) {
                        std::cout << "Waiting 5 seconds before retrying..." << std::endl;
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    }
                    continue;
                }
            } else {
                // Success!
                uploadSuccess = true;
                std::cout << "Upload successful." << std::endl;
            }
        } else {
            std::cerr << "Upload command failed with error code: " << result << std::endl;
            // Wait before retrying
            if (attempt < maxRetries) {
                std::cout << "Waiting 5 seconds before retrying..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    
    if (uploadSuccess) {
        std::cout << "Deleting local file..." << std::endl;
        // Only delete the local file if upload was successful
        if (remove(filename.c_str()) != 0) {
            std::cerr << "Error deleting file: " << filename << std::endl;
        } else {
            std::cout << "Local file deleted successfully." << std::endl;
        }
    } else {
        std::cerr << "All upload attempts failed. Keeping local file: " << filename << std::endl;
    }
}

// Then the main function can call it
int main(int argc, char* argv[]) {
    // Check if we're in aggregation mode
    if (argc > 1 && std::string(argv[1]) == "--aggregate") {
        if (argc < 4) {
            std::cerr << "Usage for aggregation: " << argv[0] << " --aggregate output.csv input1.csv input2.csv [input3.csv ...]" << std::endl;
            return 1;
        }
        
        std::string outputFile = argv[2];
        std::vector<std::string> inputFiles;
        
        for (int i = 3; i < argc; i++) {
            inputFiles.push_back(argv[i]);
        }
        
        aggregateStrategies(inputFiles, outputFile);
        return 0;
    }
    
    // Original training code
    SpinGoGame game;
    
    std::cout << "Preloading clusters..." << std::endl;
    preloadClusters();
    std::cout << "Clusters loaded successfully." << std::endl;
    
    int iterations = 100000;  // Default value
    std::string outputFilename = "spingo_strategies_optimized.csv";  // Default output filename
    bool useParallel = true;  // Default to parallel mode
    bool uploadToDrive = true;  // Default to uploading to Google Drive
    
    // Parse command line arguments as positional arguments
    if (argc >= 2) {
        try {
            iterations = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid iterations value. Using default: " << iterations << std::endl;
        }
    }
    
    if (argc >= 3) {
        outputFilename = argv[2];
    }
    
    // Check for optional flags in any position
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--sequential") {
            useParallel = false;
        } else if (std::string(argv[i]) == "--no-upload") {
            uploadToDrive = false;
        }
    }
    
    std::cout << "Starting training with " << iterations << " iterations." << std::endl;
    std::cout << "Results will be saved to: " << outputFilename << std::endl;
    std::cout << "Mode: " << (useParallel ? "Parallel" : "Sequential") << std::endl;
    std::cout << "Upload to Drive: " << (uploadToDrive ? "Yes" : "No") << std::endl;
    
    // Run the MCCFR training
    if (useParallel) {
        trainMCCFRParallel(game, iterations);
    } else {
        trainMCCFR(game, iterations);
    }
    
    // Save the learned strategies to the specified CSV file
    saveInfoSetsToFile(outputFilename);
    
    // Upload to Google Drive if requested
    if (uploadToDrive) {
        uploadToGoogleDrive(outputFilename);
    }
    
    return 0;
}

void analyzeDuplicateInfoSets() {
    // Map to track potentially ambiguous infosets
    std::map<std::string, std::vector<std::string>> ambiguousInfoSets;
    
    for (const auto& entry : nodeMap) {
        const std::string& infoSet = entry.first;
        
        // Parse the infoSet string to extract components
        std::string round, player, abstraction, actions;
        
        // Extract player
        size_t playerPos = infoSet.find("P");
        size_t playerEndPos = infoSet.find(":", playerPos);
        if (playerPos != std::string::npos && playerEndPos != std::string::npos) {
            player = infoSet.substr(playerPos + 1, playerEndPos - playerPos - 1);
        }
        
        // Extract round
        size_t roundPos = infoSet.find("Round:");
        size_t roundEndPos = infoSet.find(" ", roundPos + 6);
        if (roundPos != std::string::npos && roundEndPos != std::string::npos) {
            round = infoSet.substr(roundPos + 6, roundEndPos - roundPos - 6);
        }
        
        // Extract abstraction (cluster)
        std::string clusterPrefix;
        if (round == "flop") clusterPrefix = "FlopCluster:";
        else if (round == "turn") clusterPrefix = "TurnCluster:";
        else if (round == "river" || round == "showdown") clusterPrefix = "RiverCluster:";
        
        if (!clusterPrefix.empty()) {
            size_t clusterPos = infoSet.find(clusterPrefix);
            if (clusterPos != std::string::npos) {
                size_t clusterEndPos = infoSet.find(" ", clusterPos + clusterPrefix.length());
                if (clusterEndPos != std::string::npos) {
                    abstraction = infoSet.substr(clusterPos + clusterPrefix.length(), 
                                                clusterEndPos - clusterPos - clusterPrefix.length());
                }
            }
        }
        
        // Extract actions
        size_t actionsPos = infoSet.find("Actions:");
        size_t actionsEndPos = infoSet.find(" Pot:", actionsPos);
        if (actionsPos != std::string::npos && actionsEndPos != std::string::npos) {
            actions = infoSet.substr(actionsPos + 8, actionsEndPos - actionsPos - 8);
        }
        
        // Create a simplified key to check for potential ambiguity
        std::string simplifiedKey = round + "|" + player + "|" + abstraction + "|" + actions;
        ambiguousInfoSets[simplifiedKey].push_back(infoSet);
    }
    
    // Print detailed information about duplicates
    std::cout << "\nAnalyzing duplicate InfoSets:" << std::endl;
    int totalDuplicates = 0;
    
    for (const auto& pair : ambiguousInfoSets) {
        if (pair.second.size() > 1) {
            totalDuplicates++;
            std::cout << "\nDuplicate key: " << pair.first << std::endl;
            std::cout << "Full InfoSets:" << std::endl;
            for (const auto& infoSet : pair.second) {
                std::cout << "  " << infoSet << std::endl;
            }
        }
    }
    
    std::cout << "Total duplicate keys: " << totalDuplicates << std::endl;
}

// Add a thread pool for parallel training
class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// Parallel version of trainMCCFR
void trainMCCFRParallel(SpinGoGame& game, int iterations) {
    std::cout << "Training in parallel mode\n";
    
    std::vector<double> totalUtility(NUM_PLAYERS, 0.0);
    std::vector<double> finalResults(NUM_PLAYERS, 0.0);
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Determine number of threads (leave one core free for system)
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2; // Default if detection fails
    if (numThreads > 1) numThreads--; // Leave one core free
    
    std::cout << "Using " << numThreads << " threads\n";
    
    // Create the thread pool as a pointer so we can explicitly control its lifetime
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(numThreads);
    
    std::atomic<int> completedIterations(0);
    std::mutex resultsMutex;
    
    // Create batches of work
    int batchSize = std::max(100, static_cast<int>(iterations / (numThreads * 10)));
    int numBatches = (iterations + batchSize - 1) / batchSize;
    
    for (int batch = 0; batch < numBatches; ++batch) {
        int startIter = batch * batchSize;
        int endIter = std::min(startIter + batchSize, iterations);
        int batchIterations = endIter - startIter;
        
        pool->enqueue([&game, &completedIterations, &resultsMutex, &totalUtility, 
                      batchIterations, startIter, &start_time, iterations]() {
            std::vector<double> batchUtility(NUM_PLAYERS, 0.0);
            
            for (int i = 0; i < batchIterations; ++i) {
                for (int p = 0; p < NUM_PLAYERS; p++) {
                    SpinGoState state = game.new_initial_state();
                    std::vector<double> reachProb(NUM_PLAYERS, 1.0);
                    
                    double value = mccfr(&state, p, reachProb);
                    batchUtility[p] += value;
                }
                
                // Update progress counter
                int completed = ++completedIterations;
                if (completed % 100 == 0) {
                    double percentage = (static_cast<double>(completed) / iterations) * 100;
                    
                    auto current_time = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
                    double seconds_per_iter = elapsed.count() / static_cast<double>(completed);
                    int remaining_seconds = static_cast<int>(seconds_per_iter * (iterations - completed));
                    
                    int hours = remaining_seconds / 3600;
                    int minutes = (remaining_seconds % 3600) / 60;
                    int seconds = remaining_seconds % 60;
                    
                    std::cout << "\rIteration " << completed << " (" << std::fixed << std::setprecision(4) 
                              << percentage << "% completed, ETA: " << hours << "h " 
                              << minutes << "m " << seconds << "s)" << std::flush;
                }
            }
            
            // Update global results with batch results
            {
                std::lock_guard<std::mutex> lock(resultsMutex);
                for (int p = 0; p < NUM_PLAYERS; p++) {
                    totalUtility[p] += batchUtility[p];
                }
            }
        });
    }
    
    // Wait for all threads to complete by explicitly destroying the pool
    // This ensures all tasks are completed before we continue
    pool.reset();
    
    std::cout << "\nTraining completed. Calculating final results..." << std::endl;
    
    // Calculate final results
    for (int p = 0; p < NUM_PLAYERS; p++) {
        finalResults[p] = totalUtility[p] / iterations;
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