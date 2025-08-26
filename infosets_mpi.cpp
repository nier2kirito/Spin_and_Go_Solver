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

// Generate scenarios and track infosets
void generateScenarios(int maxSamples, const std::string& outputFile) {
    // Preload all clusters at the start
    preloadClusters();
    
    // Declare infosetCounts as a local variable
    std::unordered_map<std::string, int> infosetCounts;

    // Load existing infosets from the CSV file
    std::ifstream existingFile(outputFile);
    if (existingFile.is_open()) {
        std::string line;
        // Skip header line
        std::getline(existingFile, line);
        while (std::getline(existingFile, line)) {
            // Assuming the format is: Round,Player,Abstraction,PreviousActions,CumulatedPot
            std::istringstream iss(line);
            std::string round, player, abstraction, previousActions;
            double cumulatedPot;
            std::getline(iss, round, ',');
            std::getline(iss, player, ',');
            std::getline(iss, abstraction, ',');
            std::getline(iss, previousActions, ',');
            iss >> cumulatedPot;

            // Create a unique key for this infoset
            std::string infosetKey = round + "|" + abstraction + "|" + previousActions;
            infosetCounts[infosetKey]++;
        }
        existingFile.close();
    }
    
    SpinGoGame game;
    std::mt19937 rng(std::random_device{}());
    std::ofstream outFile(outputFile, std::ios::out | std::ios::app);
    
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file " << outputFile << std::endl;
        return;
    }
    
    // Write header with added CumulatedPot column if the file is empty
    if (outFile.tellp() == 0) {
        outFile << "Round,Player,Abstraction,PreviousActions,CumulatedPot\n";
    }
    
    // Track unique infosets and total count
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
                    previousActions.push_back(action_to_string(randomAction));
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
                previousActions.push_back(action_to_string(randomAction));
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

int main(int argc, char* argv[]) {
    // Default number of samples
    int numSamples = 10;
    // Default output file name
    std::string outputFile = "poker_infosets.csv";
    
    // Parse command line arguments
    if (argc > 1) {
        try {
            numSamples = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing number of samples. Using default: " << numSamples << std::endl;
        }
    }
    
    // Check if a second argument for output file name is provided
    if (argc > 2) {
        outputFile = argv[2]; // Set output file name from command line argument
    }
    
    // Generate scenarios and track infosets
    generateScenarios(numSamples, outputFile);
    
    return 0;
}