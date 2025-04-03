#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <random>
#include <unordered_set>
#include <iostream>
#include "poker_evaluator.cpp" // Evaluates a 7-card hand (hero: 2 hole + 5 board)
#include "Kmeans.cpp"          // Contains the Kmeans class and ClusterL2 method
#include <memory>              // Requires C++17 or later
#include <chrono> // Include for time tracking
#include <iomanip> // For setprecision
#include <functional>
using namespace std;

// Global constants here (We should take care of all the combinations of (2+5) cards)
const int NUM_CLUSTERS_OPP = 30;        //clusters for opponent starting-hand clustering
const int N_RANDOM_RIVER_SAMPLES = 20e3; // Number of river configurations to sample
const int N_CLUSTER_RUNS = 1;          // Number of runs for k-means clustering for rivers (Let's try 1 for now and see how much time it takes)

// Function prototype for parseCanonicalRiver
vector<Card> parseCanonicalRiver(const string &riverID, 
                                const vector<string> &RANKS, 
                                const vector<string> &SUITS);

// Add this before generateUniqueCanonicalRivers
void generateCanonicalSuitAssignments(
    vector<pair<string, double>> &riverEquities, 
    int &count, 
    int maxSamples,
    const vector<string> &RANKS, 
    const vector<string> &canonicalSuits,
    int r1, int r2, int b1, int b2, int b3, int b4, int b5,
    const vector<int> &rankCounts,
    const chrono::steady_clock::time_point &startTime);

bool cardInHand(const Card &card, const vector<Card> &hand) {
    for (const auto &c : hand) {
        if (c.rank == card.rank && c.suit == card.suit)
            return true;
    }
    return false;
}

string cardToStr(const Card &c) { return c.rank + c.suit; }

vector<Card> generateDeck() {
    vector<Card> deck;
    vector<string> SUITS = {"h", "d", "c", "s"};
    vector<string> RANKS = {"A", "K", "Q", "J", "10", "9", "8", "7", "6", "5", "4", "3", "2"};
    for (const string &suit : SUITS) {
        for (const string &rank : RANKS) {
            deck.push_back(Card{rank, suit});
        }
    }
    return deck;
}

string canonicalRiver(const vector<Card> &river) {
    vector<Card> hole(river.begin(), river.begin() + 2);
    vector<Card> board(river.begin() + 2, river.end());

    // Sort hole and board cards by rank first, then suit for canonical representation
    auto cmpCard = [](const Card &a, const Card &b) {
        if (a.rank == b.rank) return a.suit < b.suit; // Sort by suit if ranks are equal
        static const unordered_map<string, int> rankValue = {
            {"A", 14}, {"K", 13}, {"Q", 12}, {"J", 11}, {"10", 10},
            {"9", 9}, {"8", 8}, {"7", 7}, {"6", 6}, {"5", 5}, {"4", 4}, {"3", 3}, {"2", 2}
        };
        return rankValue.at(a.rank) > rankValue.at(b.rank); // Sort high to low
    };
    
    // We don't sort the cards anymore - we keep them in their original order
    // to preserve the suit ordering constraint
    
    // Create a mapping of original suits to canonical suits
    // We need to preserve the original suit ordering
    unordered_map<string, string> suitMap;
    vector<string> canonicalSuits = {"c", "d", "h", "s"}; // Order for canonical suits
    
    // Track which suits we've seen and in what order
    vector<string> seenSuits;
    for (const auto &card : river) {
        if (find(seenSuits.begin(), seenSuits.end(), card.suit) == seenSuits.end()) {
            seenSuits.push_back(card.suit);
        }
    }
    
    // Map each suit to its canonical representation
    for (size_t i = 0; i < seenSuits.size(); i++) {
        suitMap[seenSuits[i]] = canonicalSuits[i];
    }

    // Generate canonical representation
    string holeStr = "";
    for (const auto &c : hole) {
        holeStr += c.rank + suitMap[c.suit] + " ";
    }
    string boardStr = "";
    for (const auto &c : board) {
        boardStr += c.rank + suitMap[c.suit] + " ";
    }
    return "H:" + holeStr + "|B:" + boardStr;
}

// New helper function to efficiently check for available opponent hands
bool isAvailableOppHand(const Card &c1, const Card &c2, const vector<Card> &heroRiver) {
    return !cardInHand(c1, heroRiver) && !cardInHand(c2, heroRiver);
}

// Optimized function to handle all starting hand types
bool assignRepresentativeOpponentHand(int i, int j, const vector<Card> &heroRiver,
                                     const vector<string> &RANKS, const vector<string> &SUITS,
                                     vector<Card> &oppHand) {
    oppHand.clear();
    // Get canonical rank indices (make sure i >= j for easier processing)
    if (i < j) swap(i, j);
    
    // Pocket pair
    if (i == j) {
        // Try to find any available suited combination
        for (size_t a = 0; a < SUITS.size(); a++) {
            for (size_t b = a + 1; b < SUITS.size(); b++) {
                Card c1{RANKS[i], SUITS[a]};
                Card c2{RANKS[i], SUITS[b]};
                if (isAvailableOppHand(c1, c2, heroRiver)) {
                    oppHand = {c1, c2};
                    return true;
                }
            }
        }
    } 
    // Suited hands (i > j)
    else {
        // Try to find any available suited combination
        for (const auto &suit : SUITS) {
            Card c1{RANKS[i], suit};
            Card c2{RANKS[j], suit};
            if (isAvailableOppHand(c1, c2, heroRiver)) {
                oppHand = {c1, c2};
                return true;
            }
        }
        
        // If no suited hand was found, try offsuit combinations
        for (const auto &s1 : SUITS) {
            for (const auto &s2 : SUITS) {
                if (s1 == s2) continue;
                Card c1{RANKS[i], s1};
                Card c2{RANKS[j], s2};
                if (isAvailableOppHand(c1, c2, heroRiver)) {
                    oppHand = {c1, c2};
                    return true;
                }
            }
        }
    }
    return false;
}

vector<vector<double>> readEquityHeatmap(const string &filename) {
    ifstream file(filename);
    vector<vector<double>> heatmap(13, vector<double>(13, 0.0));
    string line;
    int row = 0;
    while (getline(file, line)) {
        if (row++ == 0) continue;
        stringstream ss(line);
        string value;
        int col = 0;
        while (getline(ss, value, ',')) {
            if (col++ == 0) continue;
            try {
                heatmap[row - 2][col - 2] = stod(value);
            } catch (...) {}
        }
    }
    return heatmap;
}

vector<int> computeOpponentHandClusters(const vector<vector<double>> &heatmap, int nb_clusters) {
    vector<vector<float>> data;
    for (const auto &row : heatmap)
        for (double val : row) data.push_back({static_cast<float>(val)});
    Kmeans kmeans;
    return kmeans.ClusterL2(data, nb_clusters, 1);
}

int evaluateRiverHand(const vector<Card> &heroHole, const vector<Card> &board, PokerEvaluator &evaluator) {
    return evaluator.evaluateHand(heroHole, board);
}

vector<double> computeRiverHistogram(const vector<Card> &river, const vector<int> &oppClusters,
                                    const vector<string> &RANKS, const vector<string> &SUITS,
                                    PokerEvaluator &evaluator) {
    vector<double> histogram(NUM_CLUSTERS_OPP, 0.0);
    vector<Card> heroHole(river.begin(), river.begin() + 2);
    vector<Card> board(river.begin() + 2, river.end());
    int heroValue = evaluateRiverHand(heroHole, board, evaluator);

    // Use a set to track which opponent hand types we've already processed
    unordered_set<int> processedHandTypes;
    
    // Iterate over opponent hand types (canonical representation with i >= j)
    for (int i = 0; i < 13; i++) {
        for (int j = 0; j <= i; j++) {  // Only need to consider canonical hand types (i >= j)
            // Get hand type index in a way that handles both orders (i,j) and (j,i)
            int handTypeIdx = (i * 13 + j);
            
            // Skip if we've already processed this hand type
            if (!processedHandTypes.insert(handTypeIdx).second) continue;
            
            vector<Card> oppHand;
            if (!assignRepresentativeOpponentHand(i, j, river, RANKS, SUITS, oppHand)) continue;
            int oppValue = evaluator.evaluateHand(oppHand, board);
            double outcome = (heroValue > oppValue) ? 1.0 : (heroValue == oppValue) ? 0.5 : 0.0;
            
            // Add outcome to the appropriate cluster
            histogram[oppClusters[handTypeIdx]] += outcome;
        }
    }
    
    // Normalize histogram to account for the reduced number of hand types we're evaluating
    double totalWeight = 0.0;
    for (double val : histogram) totalWeight += val;
    if (totalWeight > 0) {
        for (double &val : histogram) val /= totalWeight;
    }
    
    return histogram;
}

// Modified function to compute river equity directly without opponent clustering
vector<double> computeRiverEquity(const vector<Card> &river,
                                 const vector<string> &RANKS, const vector<string> &SUITS,
                                 PokerEvaluator &evaluator) {
    vector<Card> heroHole(river.begin(), river.begin() + 2);
    vector<Card> board(river.begin() + 2, river.end());
    int heroValue = evaluateRiverHand(heroHole, board, evaluator);
    
    // Precompute available cards for faster lookups
    unordered_set<string> usedCards;
    for (const auto& card : river) {
        usedCards.insert(card.rank + card.suit);
    }
    
    // Track wins, ties, losses against all possible opponent hands
    int wins = 0, ties = 0, total = 0;
    
    // Optimize opponent hand generation - use precomputed available cards
    vector<Card> oppHand(2);
    
    // Iterate over all possible opponent hand combinations
    for (int i = 0; i < 13; i++) {
        for (int j = 0; j <= i; j++) {  // Only need to consider canonical hand types (i >= j)
            // Fast-path for finding opponent hands using precomputed available cards
            bool handFound = false;
            
            // For pocket pairs
            if (i == j) {
                for (size_t a = 0; a < SUITS.size() && !handFound; a++) {
                    for (size_t b = a + 1; b < SUITS.size() && !handFound; b++) {
                        string c1 = RANKS[i] + SUITS[a];
                        string c2 = RANKS[i] + SUITS[b];
                        if (usedCards.find(c1) == usedCards.end() && 
                            usedCards.find(c2) == usedCards.end()) {
                            oppHand[0] = {RANKS[i], SUITS[a]};
                            oppHand[1] = {RANKS[i], SUITS[b]};
                            handFound = true;
                        }
                    }
                }
            } 
            // For suited and offsuit hands
            else {
                // Try suited first
                for (const auto &suit : SUITS) {
                    string c1 = RANKS[i] + suit;
                    string c2 = RANKS[j] + suit;
                    if (usedCards.find(c1) == usedCards.end() && 
                        usedCards.find(c2) == usedCards.end()) {
                        oppHand[0] = {RANKS[i], suit};
                        oppHand[1] = {RANKS[j], suit};
                        handFound = true;
                        break;
                    }
                }
                
                // Try offsuit if needed
                if (!handFound) {
                    for (const auto &s1 : SUITS) {
                        for (const auto &s2 : SUITS) {
                            if (s1 == s2) continue;
                            string c1 = RANKS[i] + s1;
                            string c2 = RANKS[j] + s2;
                            if (usedCards.find(c1) == usedCards.end() && 
                                usedCards.find(c2) == usedCards.end()) {
                                oppHand[0] = {RANKS[i], s1};
                                oppHand[1] = {RANKS[j], s2};
                                handFound = true;
                                break;
                            }
                        }
                        if (handFound) break;
                    }
                }
            }
            
            if (!handFound) continue;
            
            int oppValue = evaluator.evaluateHand(oppHand, board);
            if (heroValue > oppValue) wins++;
            else if (heroValue == oppValue) ties++;
            total++;
        }
    }
    
    // Calculate equity (win + 0.5*tie)
    double equity = total > 0 ? (wins + 0.5 * ties) / total : 0.0;
    
    // Return as a single value in a vector
    return {equity};
}

// Function to save river equities to CSV file
void saveRiverEquitiesToCSV(const vector<pair<string, double>> &riverEquities, 
                            const string &filename = "river_equities_processed.csv") {
    ofstream outFile(filename);
    outFile << "RiverID,Equity\n";
    for (const auto &p : riverEquities) {
        outFile << "\"" << p.first << "\"," << p.second << "\n";
    }
    outFile.close();
    cout << endl << "Saved " << riverEquities.size() << " river combinations to " << filename << endl;
}

// Function to load existing river equities from CSV file
bool loadRiverEquitiesFromCSV(vector<pair<string, double>> &riverEquities, 
                             unordered_set<string> &seenRiverCanon,
                             const string &filename = "river_equities_processed.csv") {
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        return false;
    }
    
    string line;
    getline(inFile, line); // Skip header
    
    while (getline(inFile, line)) {
        stringstream ss(line);
        string riverID;
        double equity;
        
        // Extract riverID (enclosed in quotes)
        getline(ss, riverID, ',');
        if (riverID.size() > 2) {
            riverID = riverID.substr(1, riverID.size() - 2); // Remove quotes
        }
        
        // Extract equity
        ss >> equity;
        
        // Add to data structures
        riverEquities.push_back({riverID, equity});
        seenRiverCanon.insert(riverID);
    }
    
    cout << "Loaded " << riverEquities.size() << " existing river combinations from " << filename << endl;
    return true;
}

// Process a specific range of river combinations and calculate their equities
void processRiverEquitiesRange(vector<pair<string, double>> &riverEquities,
                              int startLine, int endLine,
                              const vector<string> &RANKS, const vector<string> &SUITS) {
    
    // Create evaluator instance
    PokerEvaluator evaluator;
    
    // Variables for ETA calculation
    auto startTime = chrono::steady_clock::now();
    int totalLines = endLine - startLine;
    int processedLines = 0;
    
    // Process only the combinations within the specified range
    for (int i = startLine; i < endLine && i < riverEquities.size(); i++) {
        // Parse the canonical river representation back to card objects
        string riverID = riverEquities[i].first;
        vector<Card> river = parseCanonicalRiver(riverID, RANKS, SUITS);
        
        if (river.size() != 7) {
            cerr << "Error: Invalid river parsed from " << riverID << endl;
            continue;
        }
        
        // Calculate equity for this river combination
        vector<double> equityResult = computeRiverEquity(river, RANKS, SUITS, evaluator);
        
        // Update the equity value
        riverEquities[i].second = equityResult[0];
        
        // Update processed count
        processedLines++;
        
        // Print progress with ETA every 100 lines or so
        if (processedLines % 100 == 0) {
            auto currentTime = chrono::steady_clock::now();
            double elapsedSeconds = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();
            
            // Calculate average time per line
            double avgTimePerLine = elapsedSeconds / processedLines;
            
            // Calculate remaining lines and time
            int remainingLines = totalLines - processedLines;
            double remainingTime = avgTimePerLine * remainingLines;
            
            // Convert remaining time to hours, minutes, and seconds
            int remainingHours = static_cast<int>(remainingTime) / 3600;
            int remainingMinutes = (static_cast<int>(remainingTime) % 3600) / 60;
            int remainingSeconds = static_cast<int>(remainingTime) % 60;
            
            // Calculate percentage complete
            double percentComplete = (processedLines * 100.0) / totalLines;
            
            // Print progress and ETA
            cout << "Processed " << processedLines << " of " << totalLines 
                 << " combinations (" << fixed << setprecision(2) << percentComplete << "%). "
                 << "ETA: " << remainingHours << "h " << remainingMinutes << "m " 
                 << remainingSeconds << "s\r";
            cout.flush();
        }
    }
    
    // Final timing information
    auto endTime = chrono::steady_clock::now();
    double totalSeconds = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
    int hours = static_cast<int>(totalSeconds) / 3600;
    int minutes = (static_cast<int>(totalSeconds) % 3600) / 60;
    int seconds = static_cast<int>(totalSeconds) % 60;
    
    cout << endl << "Completed processing " << processedLines << " lines from " << startLine 
         << " to " << endLine << " in " << hours << "h " << minutes << "m " << seconds << "s" << endl;
    
    // Calculate average time per line
    double avgTimePerLine = totalSeconds / processedLines;
    cout << "Average processing time: " << fixed << setprecision(6) << avgTimePerLine 
         << " seconds per combination" << endl;
}

// Helper function to parse a canonical river string back to Card objects
vector<Card> parseCanonicalRiver(const string &riverID, 
                                const vector<string> &RANKS, 
                                const vector<string> &SUITS) {
    vector<Card> river;
    
    // Expected format: "H:XXs YYh |B:ZZs AAs BBh CCd DDd"
    size_t holeSeparator = riverID.find("|B:");
    if (holeSeparator == string::npos) return river;
    
    // Extract hole cards part
    string holePart = riverID.substr(2, holeSeparator - 2);
    // Extract board part
    string boardPart = riverID.substr(holeSeparator + 3);
    
    // Parse hole cards
    istringstream holeStream(holePart);
    string cardStr;
    while (holeStream >> cardStr && river.size() < 2) {
        if (cardStr.length() >= 2) {
            string rank = cardStr.substr(0, cardStr.length() - 1);
            string suit = cardStr.substr(cardStr.length() - 1, 1);
            river.push_back(Card{rank, suit});
        }
    }
    
    // Parse board cards
    istringstream boardStream(boardPart);
    while (boardStream >> cardStr && river.size() < 7) {
        if (cardStr.length() >= 2) {
            string rank = cardStr.substr(0, cardStr.length() - 1);
            string suit = cardStr.substr(cardStr.length() - 1, 1);
            river.push_back(Card{rank, suit});
        }
    }
    
    return river;
}

// Function to determine if two poker hands are isomorphic (identical under suit relabeling)
bool are_hands_isomorphic(const string &hand1, const string &hand2) {
    // Parse cards from both hands
    vector<string> ranks1, ranks2;
    vector<string> suits1, suits2;
    
    // Parse hand1
    size_t pos = hand1.find("H:") + 2;
    while (pos < hand1.length()) {
        size_t end = hand1.find_first_of(" |", pos);
        if (end == string::npos) end = hand1.length();
        string card = hand1.substr(pos, end - pos);
        
        if (card == "B:") {
            pos = end + 1;
            continue;
        }
        
        if (card.length() >= 2) {
            string rank = card.substr(0, card.length() - 1);
            string suit = card.substr(card.length() - 1);
            ranks1.push_back(rank);
            suits1.push_back(suit);
        }
        
        pos = end + 1;
    }
    
    // Parse hand2
    pos = hand2.find("H:") + 2;
    while (pos < hand2.length()) {
        size_t end = hand2.find_first_of(" |", pos);
        if (end == string::npos) end = hand2.length();
        string card = hand2.substr(pos, end - pos);
        
        if (card == "B:") {
            pos = end + 1;
            continue;
        }
        
        if (card.length() >= 2) {
            string rank = card.substr(0, card.length() - 1);
            string suit = card.substr(card.length() - 1);
            ranks2.push_back(rank);
            suits2.push_back(suit);
        }
        
        pos = end + 1;
    }
    
    // Check if ranks match
    if (ranks1.size() != ranks2.size()) return false;
    for (size_t i = 0; i < ranks1.size(); i++) {
        if (ranks1[i] != ranks2[i]) return false;
    }
    
    // Map suits from hand1 to hand2
    unordered_map<string, string> suitMap;
    unordered_set<string> usedSuits;
    
    for (size_t i = 0; i < suits1.size(); i++) {
        if (suitMap.find(suits1[i]) != suitMap.end()) {
            // If already mapped, must be consistent
            if (suitMap[suits1[i]] != suits2[i]) return false;
        } else {
            // If target suit already used by another source suit, not isomorphic
            if (usedSuits.find(suits2[i]) != usedSuits.end()) return false;
            
            // Create new mapping
            suitMap[suits1[i]] = suits2[i];
            usedSuits.insert(suits2[i]);
        }
    }
    
    return true;
}

// Function to process river equities CSV and remove isomorphic hands
void remove_isomorphic_hands(const string &inputFilename = "river_equities_processed.csv", 
                             const string &outputFilename = "river_equities_processed.csv") {
    // Read all records from the input file
    ifstream inFile(inputFilename);
    if (!inFile.is_open()) {
        cerr << "Error: Cannot open input file " << inputFilename << endl;
        return;
    }
    
    // Read header and all river entries
    string header;
    getline(inFile, header);
    
    // Create the output file with header
    ofstream outFile(outputFilename);
    if (!outFile.is_open()) {
        cerr << "Error: Cannot open output file " << outputFilename << endl;
        return;
    }
    outFile << header << endl;
    
    // Maps to quickly identify potential isomorphs based on rank patterns
    unordered_map<string, vector<pair<string, double>>> rankPatternToRivers;
    
    // Track progress
    int totalLines = 0;
    int uniqueCount = 0;
    int duplicateCount = 0;
    
    string line;
    while (getline(inFile, line)) {
        totalLines++;
        
        // Parse the river ID and equity
        stringstream ss(line);
        string riverID;
        double equity;
        
        // Extract riverID (enclosed in quotes)
        getline(ss, riverID, ',');
        if (riverID.size() > 2) {
            riverID = riverID.substr(1, riverID.size() - 2); // Remove quotes
        }
        
        // Extract equity
        ss >> equity;
        
        // Extract rank pattern (ignoring suits)
        string rankPattern = "";
        size_t pos = riverID.find("H:") + 2;
        while (pos < riverID.length()) {
            size_t end = riverID.find_first_of(" |", pos);
            if (end == string::npos) end = riverID.length();
            string card = riverID.substr(pos, end - pos);
            
            if (card == "B:") {
                rankPattern += "|";
                pos = end + 1;
                continue;
            }
            
            if (card.length() >= 2) {
                string rank = card.substr(0, card.length() - 1);
                rankPattern += rank + " ";
            }
            
            pos = end + 1;
        }
        
        // Check if this river is isomorphic to any with the same rank pattern
        bool isDuplicate = false;
        if (rankPatternToRivers.find(rankPattern) != rankPatternToRivers.end()) {
            for (const auto &existingRiver : rankPatternToRivers[rankPattern]) {
                if (are_hands_isomorphic(riverID, existingRiver.first)) {
                    isDuplicate = true;
                    duplicateCount++;
                    break;
                }
            }
        }
        
        // If not a duplicate, add to our unique set and write to output
        if (!isDuplicate) {
            rankPatternToRivers[rankPattern].push_back({riverID, equity});
            outFile << "\"" << riverID << "\"," << equity << endl;
            uniqueCount++;
        }
        
        // Print progress every 1000 lines
        if (totalLines % 1000 == 0) {
            cout << "Processed " << totalLines << " lines. Found " << uniqueCount 
                 << " unique rivers and " << duplicateCount << " duplicates.\r";
            cout.flush();
        }
    }
    
    cout << endl << "Finished processing " << totalLines << " lines." << endl;
    cout << "Found " << uniqueCount << " unique rivers and " << duplicateCount << " duplicates." << endl;
    cout << "Saved unique rivers to " << outputFilename << endl;
}


// Forward declaration of the helper function.
void generateCanonicalSuitAssignments(
    vector<pair<string, double>> &riverEquities,
    int &count,
    int maxSamples,
    const vector<string> &RANKS,
    const vector<string> &SUITS,
    int r1, int r2, int b1, int b2, int b3, int b4, int b5,
    const vector<int> &rankCounts,
    const chrono::steady_clock::time_point &startTime);

// Generate unique canonical rivers with proper ordering constraints
void generateUniqueCanonicalRivers(vector<pair<string, double>> &riverEquities,
                                    int maxSamples,
                                    const vector<string> &RANKS,
                                    const vector<string> &SUITS) {
    auto startTime = chrono::steady_clock::now();
    int count = riverEquities.size(); // Start counting from existing entries

    // Loop over all possible rank combinations for the 7 cards
    for (int r1 = 0; r1 < static_cast<int>(RANKS.size()); r1++) {
        for (int r2 = 0; r2 < static_cast<int>(RANKS.size()); r2++) {
            // Early pruning: Skip if we've reached our sample limit
            if (count >= maxSamples) {
                cout << "\nGenerated " << count << " unique canonical river combinations" << endl;
                return;
            }
            
            for (int b1 = 0; b1 < static_cast<int>(RANKS.size()); b1++) {
                for (int b2 = 0; b2 < static_cast<int>(RANKS.size()); b2++) {
                    for (int b3 = 0; b3 < static_cast<int>(RANKS.size()); b3++) {
                        for (int b4 = 0; b4 < static_cast<int>(RANKS.size()); b4++) {
                            for (int b5 = 0; b5 < static_cast<int>(RANKS.size()); b5++) {
                                // Count how many times each rank is used
                                vector<int> rankCounts(RANKS.size(), 0);
                                rankCounts[r1]++;
                                rankCounts[r2]++;
                                rankCounts[b1]++;
                                rankCounts[b2]++;
                                rankCounts[b3]++;
                                rankCounts[b4]++;
                                rankCounts[b5]++;
                                
                                // Skip illegal rank distributions (more than 4 cards per rank)
                                bool valid = true;
                                for (int cnt : rankCounts) {
                                    if (cnt > 4) { 
                                        valid = false; 
                                        break; 
                                    }
                                }
                                if (!valid)
                                    continue;
                                
                                // Call helper function to assign suits to the 7 cards
                                generateCanonicalSuitAssignments(riverEquities, count, maxSamples,
                                    RANKS, SUITS,
                                    r1, r2, b1, b2, b3, b4, b5,
                                    rankCounts, startTime);
                                
                                // Check if we've reached our sample limit
                                if (count >= maxSamples) {
                                    cout << "\nGenerated " << count << " unique canonical river combinations" << endl;
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    cout << "\nGenerated " << count << " unique canonical river combinations" << endl;
}

// HELPER FUNCTION: recursively assign suits with the rule that suits must follow the order:
// suit(r1) <= suit(r2) <= suit(b1) <= suit(b2) <= suit(b3) <= suit(b4) <= suit(b5)
// We also keep track to avoid picking the same physical card twice.
void generateCanonicalSuitAssignments(
    vector<pair<string, double>> &riverEquities,
    int &count,
    int maxSamples,
    const vector<string> &RANKS,
    const vector<string> &SUITS,
    int r1, int r2, int b1, int b2, int b3, int b4, int b5,
    const vector<int> &rankCounts,
    const chrono::steady_clock::time_point &startTime) {

    // Our fixed order for the 7 cards
    vector<int> rankIndices = { r1, r2, b1, b2, b3, b4, b5 };

    // Used cards in the deck
    vector<bool> usedCards(RANKS.size() * SUITS.size(), false);

    // partialHand will hold the 7 cards as they are assigned
    vector<Card> partialHand;
    partialHand.reserve(7);

    // Keep track of the current minimum suit index
    int currentMinSuitIndex = 0;

    // Use a hash set for O(1) lookups instead of linear search
    static unordered_set<string> seenCanonical;
    if (seenCanonical.empty()) {
        for (const auto &p : riverEquities)
            seenCanonical.insert(p.first);
    }

    // Recursive function to assign suits for card at index cardPos
    function<void (int)> assignSuits = [&](int cardPos) {
        // If we have assigned suits for all 7 cards, then create the canonical representation
        if (cardPos == 7) {
            string canStr = canonicalRiver(partialHand);
            
            // O(1) lookup instead of O(n) search
            if (seenCanonical.find(canStr) == seenCanonical.end()) {
                riverEquities.push_back({ canStr, 0.0 });
                seenCanonical.insert(canStr);
                count++;
                
                if (count % 1000 == 0) {
                    auto now = chrono::steady_clock::now();
                    auto elapsed = chrono::duration_cast<chrono::seconds>(now - startTime).count();
                    cout << "Generated " << count << " unique canonical rivers. Elapsed time: " 
                         << elapsed << " seconds\r" << flush;
                }
                
                if (count >= maxSamples)
                    return;
            }
            return;
        }
        
        // Determine the rank index for the current card
        int curRank = rankIndices[cardPos];
        
        // Loop over possible suits from currentMinSuitIndex to the maximum index
        for (int suitIndex = currentMinSuitIndex; suitIndex < static_cast<int>(SUITS.size()); suitIndex++) {
            int deckIndex = curRank * SUITS.size() + suitIndex;
            if (usedCards[deckIndex])
                continue;
            
            // Choose this card
            Card card { RANKS[curRank], SUITS[suitIndex] };
            partialHand.push_back(card);
            usedCards[deckIndex] = true;
            
            // Save the current minimum suit index
            int prevMinSuitIndex = currentMinSuitIndex;
            // Update the minimum suit index for the next card
            currentMinSuitIndex = suitIndex;
            
            assignSuits(cardPos + 1);
            
            // Backtrack: remove the card and restore previous state
            partialHand.pop_back();
            usedCards[deckIndex] = false;
            currentMinSuitIndex = prevMinSuitIndex;
            
            if (count >= maxSamples)
                return;
        }
    };
    
    assignSuits(0);
}

int main(int argc, char* argv[]) {
    vector<string> RANKS = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
    vector<string> SUITS = {"c", "d", "h", "s"}; // ♣ < ♦ < ♥ < ♠

    unordered_set<string> seenRiverCanon;
    vector<pair<string, double>> riverEquities;

    // Check for a specific command to remove isomorphic hands
    if (argc >= 2 && string(argv[1]) == "remove_isomorphic") {
        cout << "Removing isomorphic hands from CSV file..." << endl;
        remove_isomorphic_hands();
        return 0;
    }
    // Try to load existing data first
    cout << "Loading existing data..." << endl;
    bool dataLoaded = loadRiverEquitiesFromCSV(riverEquities, seenRiverCanon);
    
    // Replace the nested loops with direct canonical generation
    if (riverEquities.size() < N_RANDOM_RIVER_SAMPLES) {
        cout << "Generating canonical river configurations directly..." << endl;
        cout << "Starting with " << riverEquities.size() << " already processed combinations" << endl;
        
        auto startTime = chrono::steady_clock::now();
        
        // Generate unique canonical rivers directly
        generateUniqueCanonicalRivers(riverEquities, N_RANDOM_RIVER_SAMPLES, RANKS, SUITS);
        
        auto endTime = chrono::steady_clock::now();
        double elapsedSeconds = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
        cout << "Generation completed in " << elapsedSeconds << " seconds" << endl;
    }
    
    // Save results
    saveRiverEquitiesToCSV(riverEquities);
    
    return 0; // Indicate successful completion
}
