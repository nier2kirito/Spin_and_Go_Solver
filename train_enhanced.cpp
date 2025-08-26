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

// Forward declarations - keep only these, remove any class definitions
class Node {
public:
    Node();
    Node(int numActions);
    Node(const std::vector<Action>& legalActions);
    std::vector<double> getStrategy();
    std::vector<double> getAverageStrategy() const;
    std::vector<Action> getActions() const;
    std::vector<double> regretSum;
    std::vector<double> strategy;
    std::vector<double> strategySum;
    std::vector<Action> actions;
    int visitCount;  // Add this field to track visits
};

// Add these global variables to cache cluster data
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> clusterCache;
bool clustersLoaded = false;

// Initialize global nodeMap
std::unordered_map<std::string, Node> nodeMap;

// Add this global variable for tracking recursion depth
static int mccfr_depth = 0;

// Node implementation
Node::Node() : regretSum(5, 0.0), strategy(5, 0.0), strategySum(5, 0.0), visitCount(0) {}

// Node::Node(int numActions) : regretSum(numActions, 0.0), 
//                            strategy(numActions, 0.0),
//                            strategySum(numActions, 0.0),
//                            visitCount(0) {}

Node::Node(const std::vector<Action>& legalActions) : 
    regretSum(legalActions.size(), 0.0), 
    strategy(legalActions.size(), 0.0),
    strategySum(legalActions.size(), 0.0),
    actions(legalActions),
    visitCount(0) {}

std::vector<double> Node::getStrategy() {
    double normalizingSum = 0;
    int numActions = regretSum.size();
    
    // If there's only one legal action, return a deterministic strategy
    if (numActions == 1) {
        strategy[0] = 1.0;
        return strategy;
    }
    
    for (int a = 0; a < numActions; a++) {
        strategy[a] = (regretSum[a] > 0 ? regretSum[a] : 0);
        normalizingSum += strategy[a];
    }
    for (int a = 0; a < numActions; a++) {
        if (normalizingSum > 0)
            strategy[a] /= normalizingSum;
        else
            strategy[a] = 1.0 / numActions;
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
        ss << getAbstractedCard(cards[firstCardIdx], cards[secondCardIdx], true) << " ";
        ss << getAbstractedCard(cards[firstCardIdx], cards[secondCardIdx], false) << " ";
    } else {
        ss << getAbstractedCard(cards[secondCardIdx], cards[firstCardIdx], true) << " ";
        ss << getAbstractedCard(cards[secondCardIdx], cards[firstCardIdx], false) << " ";
    }
    }
    // Add community cards information based on the round
    if (state->round == "flop" || state->round == "turn" || state->round == "river" || state->round == "showdown") {
        // For flop, turn, and river, add cluster information
        if (state->round == "flop") {
            int flopCluster = getFlopCluster(state->community_cards, {cards[player*2], cards[player*2+1]});
            ss << " FlopCluster:" << flopCluster;
        } else if (state->round == "turn") {
            int turnCluster = getTurnCluster(state->community_cards, {cards[player*2], cards[player*2+1]});
            ss << " TurnCluster:" << turnCluster;
        } else if (state->round == "river" || state->round == "showdown") {
            int riverCluster = getRiverCluster(state->community_cards, {cards[player*2], cards[player*2+1]});
            ss << " RiverCluster:" << riverCluster;
        }
    }
    
    // Add action sequence with more detailed information
    ss << " Actions:";
    
    // Include detailed bet amounts for each player
    const auto& bets = state->bets;
    const auto& pot = state->cumulative_pot;

    auto hist_it = state->round_action_history.find(state->round);
    if (hist_it != state->round_action_history.end()) {
        const auto& current_round_actions = hist_it->second;
        for (const auto& action_pair : current_round_actions) {
            ss << "[P" << action_pair.first << ":" << action_to_string(action_pair.second) << "]";
        }
    }
    
    // Add pot information with individual contributions
    ss << " Pot:" << (pot[0] + pot[1] + pot[2]);
    
    // Add current bet information
    ss << " CurrentBet:" << state->current_bet;
    
    // Add active players count
    ss << " ActivePlayers:" << state->active_players.size();
    
    // Add current player
    ss << " CurrentPlayer:" << state->current_player();
    
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
    mccfr_depth++;
    
    if (state->game_over) {
        std::vector<double> util = state->returns();
        if (mccfr_depth > 0) mccfr_depth--;
        return util[player];
    }

    if (state->is_chance_node()) {
        state->apply_action(Action::DEAL);
        double result = mccfr(state, player, reachProb);
        if (mccfr_depth > 0) mccfr_depth--;
        return result;
    }

    int currPlayer = state->current_player();
    std::string infoSet = getInformationSet(state, currPlayer);

    std::vector<Action> legalActions = state->legal_actions();
    
    // Fix: Make sure we have legal actions to choose from
    if (legalActions.empty()) {
        std::cerr << "Error: No legal actions available for player " << currPlayer << std::endl;
        return 0.0;
    }

    // Define should_print_debug here
    bool print_depth_debug = false;  // Renamed to avoid shadowing
    
    if (print_depth_debug) {
        std::cout << "Depth: " << mccfr_depth << ", ReachProb: ";
        for (int p = 0; p < reachProb.size(); p++) {
            std::cout << reachProb[p] << " ";
        }
        std::cout << std::endl;
    }

    if (currPlayer == player) {
        //Always create a new node or update existing node to match legal actions size
        if (nodeMap.find(infoSet) == nodeMap.end()) {
            nodeMap.insert({infoSet, Node(legalActions)});
        }
        Node& node = nodeMap[infoSet];
        node.visitCount++;  // Increment visit count for this node

        // Debugging: Print infoSet and initial regretSum
        static int debug_mccfr_player_count = 0;
        bool print_player_turn_debug = false; // Set to false to disable debugging

        if (print_player_turn_debug) {
            std::cout << "\n--- MCCFR (Player " << player << ") Debug Start ---" << std::endl;
            std::cout << "InfoSet: " << infoSet << std::endl;
            std::cout << "Visit Count: " << node.visitCount << std::endl;
            std::cout << "Initial RegretSum: ";
            for (double r : node.regretSum) std::cout << r << " ";
            std::cout << std::endl;
        }

        std::vector<double> strategy = node.getStrategy();

        // Update strategySum only if there are multiple legal actions
        // Also compute opponent_reach_prod here as it's needed for regret updates too
        double opponent_reach_prod = 1.0;
        if (legalActions.size() > 1) {
            for (int i = 0; i < NUM_PLAYERS; ++i) {
                if (i != currPlayer) {
                    opponent_reach_prod *= reachProb[i];
                }
            }

            if (opponent_reach_prod > 0) { // Only update if opponents could reach this state
                for (size_t i = 0; i < strategy.size(); ++i) {
                    node.strategySum[i] += opponent_reach_prod * strategy[i];
                }
            }
        } else if (NUM_PLAYERS > 1) { // Still need opponent_reach_prod if only one action but multiple players
             for (int i = 0; i < NUM_PLAYERS; ++i) {
                if (i != currPlayer) {
                    opponent_reach_prod *= reachProb[i];
                }
            }
        }


        std::vector<double> util(legalActions.size(), 0.0);
        double nodeUtil = 0.0;
        
        for (size_t i = 0; i < legalActions.size(); i++) {
            SpinGoState nextState = *state; // Create a copy of the state
            nextState.apply_action(legalActions[i]);
            std::vector<double> nextReach = reachProb;
            nextReach[player] *= strategy[i];
            util[i] = mccfr(&nextState, player, nextReach);
            nodeUtil += strategy[i] * util[i];
        }

        if (print_player_turn_debug) {
            std::cout << "Strategy: ";
            for (double s : strategy) std::cout << s << " ";
            std::cout << std::endl;
        }

        if (print_player_turn_debug) {
            std::cout << "Action Utils: ";
            for (double u : util) std::cout << u << " ";
            std::cout << std::endl;
            std::cout << "NodeUtil: " << nodeUtil << std::endl;
        }
        
        for (size_t i = 0; i < legalActions.size(); i++) {
            double regret = util[i] - nodeUtil;
            node.regretSum[i] += opponent_reach_prod * regret; // Scale regret by opponent reach probability
            if (print_player_turn_debug) {
                std::cout << "Action " << i << " (" << action_to_string(legalActions[i]) << "): Util = " << util[i] 
                          << ", Regret = " << regret << ", OpponentReachProd = " << opponent_reach_prod 
                          << ", Updated RegretSum[" << i << "] = " << node.regretSum[i] << std::endl;
            }
        }

        if (print_player_turn_debug) {
            std::cout << "Final RegretSum: ";
            for (double r : node.regretSum) std::cout << r << " ";
            std::cout << std::endl;
            std::cout << "StrategySum after update: ";
            for (double ss : node.strategySum) std::cout << ss << " ";
            std::cout << std::endl;
            std::cout << "ReachProb for player " << player << ": " << reachProb[player] << std::endl;
            std::cout << "--- MCCFR (Player " << player << ") Debug End ---" << std::endl;
            debug_mccfr_player_count++;
        }
        
        if (mccfr_depth > 0) mccfr_depth--;
        return nodeUtil;
    } else {
        std::string infoSetOpp = getInformationSet(state, currPlayer);
        if (nodeMap.find(infoSetOpp) == nodeMap.end()){
            nodeMap.insert({infoSetOpp, Node(legalActions)});
        }
        Node& oppNode = nodeMap[infoSetOpp];
        oppNode.visitCount++;  // Increment visit count

        std::vector<double> strategy = oppNode.getStrategy();
        
        // Sample an action according to the strategy
        int actionIndex = sampleAction(strategy);
        
        // Validate the action index
        if (actionIndex < 0 || actionIndex >= static_cast<int>(legalActions.size())) {
            std::cerr << "Error: Invalid action index " << actionIndex 
                      << " (legal actions size: " << legalActions.size() << ")" << std::endl;
            actionIndex = 0; // Default to first legal action
        }
        
        std::vector<double> nextReach = reachProb;
        nextReach[currPlayer] *= strategy[actionIndex];
        
        SpinGoState nextState = *state; // Create a copy of the state
        nextState.apply_action(legalActions[actionIndex]); // Apply action to the copy
        if (mccfr_depth > 0) mccfr_depth--;
        return mccfr(&nextState, player, nextReach); // Recurse with the copy
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

    // Write CSV header with added VisitCount column
    file << "Round,Player,Abstraction,PreviousActions,Strategy,CumulatedPot,VisitCount\n";

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
        
        // Extract cumulative pot instead of pot
        size_t potPos = infoSet.find("Pot:");
        size_t potEndPos = infoSet.find(" CurrentBet:", potPos);
        if (potPos != std::string::npos && potEndPos != std::string::npos) {
            pot = infoSet.substr(potPos + 4, potEndPos - potPos - 4);
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
        
        // Write the CSV row with visit count
        file << round << "," 
             << player << "," 
             << abstraction << "," 
             << previousActions << "," 
             << strategyStr.str() << "," 
             << pot << "," 
             << node.visitCount << "\n";  // Add visit count to output
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
        int totalVisits;
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
            std::string round, player, abstraction, previousActions, strategyStr, pot, visitCountStr;
            
            std::getline(ss, round, ',');
            std::getline(ss, player, ',');
            std::getline(ss, abstraction, ',');
            std::getline(ss, previousActions, ',');
            std::getline(ss, strategyStr, ',');
            std::getline(ss, pot, ',');
            std::getline(ss, visitCountStr, ',');
            
            int visitCount = std::stoi(visitCountStr);
            
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
                entry.totalVisits = visitCount;
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
                    double oldWeight = static_cast<double>(entry.totalVisits);
                    double newWeight = static_cast<double>(visitCount);
                    double totalWeight = oldWeight + newWeight;
                    
                    entry.actionProbs[action] = (entry.actionProbs[action] * oldWeight + prob * newWeight) / totalWeight;
                }
                
                // Update total visits
                entry.totalVisits += visitCount;
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
    outFile << "Round,Player,Abstraction,PreviousActions,Strategy,CumulatedPot,VisitCount\n";
    
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
        
        // Write CSV row
        outFile << entry.round << "," 
                << entry.player << "," 
                << entry.abstraction << "," 
                << entry.previousActions << "," 
                << strategyStr.str() << "," 
                << entry.pot << "," 
                << entry.totalVisits << "\n";
    }
    
    outFile.close();
    std::cout << "Aggregated " << aggregatedStrategies.size() << " strategies to " << outputFile << std::endl;
}

// Add this to main function to handle aggregation command
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
    // Create a new SpinGoGame instance
    SpinGoGame game;
    
    // Preload clusters before starting training
    std::cout << "Preloading clusters..." << std::endl;
    preloadClusters();
    std::cout << "Clusters loaded successfully." << std::endl;
    
    // Set the number of training iterations (default or from command line)
    int iterations = 1e6;  // Default value
    std::string outputFilename = "spingo_strategies.csv";  // Default output filename
    
    // Check if iterations were provided as a command line argument
    if (argc > 1) {
        try {
            iterations = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing iterations argument: " << e.what() << std::endl;
            std::cerr << "Using default value of " << iterations << std::endl;
        }
    }
    
    // Check if output filename was provided as a command line argument
    if (argc > 2) {
        outputFilename = argv[2];
    }
    
    std::cout << "Starting training with " << iterations << " iterations." << std::endl;
    std::cout << "Results will be saved to: " << outputFilename << std::endl;
    
    // Run the MCCFR training
    trainMCCFR(game, iterations);
    
    // Save the learned strategies to the specified CSV file
    saveInfoSetsToFile(outputFilename);
    
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