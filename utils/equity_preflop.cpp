#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "poker_evaluator.cpp"
#include "Kmeans.cpp" // Include the Kmeans class
#include <filesystem> // C++17 or later
#include <utility> // For std::pair
#include <unordered_map> // Include for unordered_map
#include <sstream> // Include for stringstream

using namespace std;


// Change NUM_PLAYERS as desired.
const int NUM_PLAYERS = 3; // e.g. 3 players total (1 hero + 2 opponents)

// Define a mapping for poker ranks
unordered_map<string, int> rankMapping = {
    {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6},
    {"7", 7}, {"8", 8}, {"9", 9}, {"10", 10}, {"J", 11},
    {"Q", 12}, {"K", 13}, {"A", 14}
};

// Return a string representation of a card (for debugging if needed).
string cardToStr(const Card& c) {
    return c.rank + c.suit;
}

// Generate a standard 52-card deck.
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

// A mapping function: Ace highest.
int rankValue(const string &r) {
    if (r == "A") return 14;
    if (r == "K") return 13;
    if (r == "Q") return 12;
    if (r == "J") return 11;
    return stoi(r);
}

// Update the evaluateHand function to accept both hole cards and community cards
int evaluate_hand(const vector<Card> &holeCards, const vector<Card> &communityCards, PokerEvaluator &evaluator) {
    return evaluator.evaluateHand(holeCards, communityCards);
}

double simulateEquity(const vector<Card> &heroHand, int nSims) {
    double winFraction = 0.0;
    random_device rd;
    mt19937 rng(rd());
    PokerEvaluator evaluator; // Create an instance of PokerEvaluator

    for (int sim = 0; sim < nSims; sim++) {
        vector<Card> deck = generateDeck();
        for (const Card &card : heroHand) {
            auto iter = find_if(deck.begin(), deck.end(),
                [&card](const Card &d) { return d.rank == card.rank && d.suit == card.suit; });
            if (iter != deck.end())
                deck.erase(iter);
        }

        shuffle(deck.begin(), deck.end(), rng);
        vector<vector<Card>> opponents;
        int cardsDealt = 0;
        for (int p = 1; p < NUM_PLAYERS; p++) {
            vector<Card> oppHand;
            oppHand.push_back(deck[cardsDealt]); 
            oppHand.push_back(deck[cardsDealt + 1]);
            cardsDealt += 2;
            opponents.push_back(oppHand);
        }

        vector<Card> board;
        for (int i = 0; i < 5; i++) {
            board.push_back(deck[cardsDealt + i]);
        }

        // Evaluate the hero's hand with both hole cards and community cards
        int heroScore = evaluate_hand(heroHand, board, evaluator); // Updated call

        vector<int> oppScores;
        for (const vector<Card> &oppHand : opponents) {
            // Evaluate each opponent's hand with both hole cards and community cards
            int score = evaluate_hand(oppHand, board, evaluator); // Updated call
            oppScores.push_back(score);
        }

        int bestScore = heroScore;
        for (int score : oppScores) {
            if (score > bestScore)
                bestScore = score;
        }

        int winners = 0;
        if (heroScore == bestScore)
            winners++;
        for (int score : oppScores) {
            if (score == bestScore)
                winners++;
        }

        if (heroScore == bestScore)
            winFraction += (1.0 / winners);
    }

    double equity = (winFraction / nSims) * 100.0;
    return equity;
}

// New function to cluster equities
vector<int> clusterEquities(const vector<vector<double>>& heatmap, int k) {
    vector<vector<float>> data;
    for (const auto& row : heatmap) {
        for (const auto& value : row) {
            data.push_back(vector<float>{static_cast<float>(value)}); // Explicitly cast to float
        }
    }

    Kmeans kmeans;
    return kmeans.ClusterEMD(data, k, 3); // Using L2 distance and 10 runs
}

// Function to calculate the sum of squared distances (SSD)
double calculateSSD(const vector<vector<double>>& data, const vector<int>& clusterAssignments, int k) {
    double ssd = 0.0;
    vector<vector<double>> centroids(k, vector<double>(data[0].size(), 0.0));
    vector<int> counts(k, 0);

    // Calculate centroids
    for (size_t i = 0; i < data.size(); i++) {
        int cluster = clusterAssignments[i];
        for (size_t j = 0; j < data[i].size(); j++) {
            centroids[cluster][j] += data[i][j];
        }
        counts[cluster]++;
    }

    // Average to get the centroid
    for (int i = 0; i < k; i++) {
        if (counts[i] > 0) {
            for (size_t j = 0; j < centroids[i].size(); j++) {
                centroids[i][j] /= counts[i];
            }
        }
    }

    // Calculate SSD
    for (size_t i = 0; i < data.size(); i++) {
        int cluster = clusterAssignments[i];
        double distance = 0.0;
        for (size_t j = 0; j < data[i].size(); j++) {
            distance += pow(data[i][j] - centroids[cluster][j], 2);
        }
        ssd += distance;
    }

    return ssd;
}

// New function to save clusters to a binary file
void saveClustersToBin(const vector<vector<string>>& clusterHands, const vector<vector<double>>& heatmap, const string& filename) {
    ofstream outFile(filename, ios::binary);
    if (!outFile) {
        cerr << "Error opening output file!" << endl;
        return;
    }

    // Ensure the number of clusters matches the number of equities
    if (clusterHands.empty()) {
        cerr << "Error: No clusters to save!" << endl;
        return;
    }

    for (const auto& cluster : clusterHands) {
        // Calculate average equity for the current cluster
        double totalEquity = 0.0;
        int handCount = cluster.size();
        
        for (const auto& hand : cluster) {
            // Split the hand into ranks and types
            stringstream ss(hand);
            string firstCard, secondCard;
            ss >> firstCard >> secondCard; // Read the two cards

            // Determine the hand type based on the last character of each card
            string handType = (firstCard.back() == 's' || secondCard.back() == 's') ? "s" : "o"; // Determine type for the hand

            // Remove the suit character from the card strings
            secondCard = secondCard.substr(0, secondCard.size() - 1); // Remove the last character (suit)
            
            // Determine indices for the heatmap
            int rankIndex1 = 14 - rankMapping[firstCard]; // Get index for first card
            int rankIndex2 = 14 - rankMapping[secondCard]; // Get index for second card

            // Get the equity from the heatmap
            double equity = heatmap[rankIndex1][rankIndex2];
            totalEquity += equity;
        }
        
        double averageEquity = (handCount > 0) ? (totalEquity / handCount) : 0.0;

        // Write the average equity for the current cluster
        outFile.write(reinterpret_cast<const char*>(&averageEquity), sizeof(averageEquity)); // Write average equity

        // Write each hand in the cluster
        for (const auto& hand : cluster) {
            // Split the hand into ranks and types
            stringstream ss(hand);
            string firstCard, secondCard;
            ss >> firstCard >> secondCard; // Read the two cards

            // Determine the hand type based on the last character of each card
            string handType = (firstCard.back() == 's' || secondCard.back() == 's') ? "s" : "o"; // Determine type for the hand

            // Remove the suit character from the card strings
            secondCard = secondCard.substr(0, secondCard.size() - 1); // Remove the last character (suit)

            // Determine which is lower and which is higher using the rank mapping
            int lowerValue = rankMapping[firstCard];
            int higherValue = rankMapping[secondCard];

            // Construct the new hand representation
            string newHand;
            if (lowerValue < higherValue) {
                newHand = firstCard + " " + secondCard + (handType == "s" ? "s" : "o");
            } else {
                newHand = secondCard + " " + firstCard + (handType == "s" ? "s" : "o");
            }

            // Write each hand as a string
            size_t length = newHand.size();
            outFile.write(reinterpret_cast<const char*>(&length), sizeof(length)); // Write length of the string
            outFile.write(newHand.c_str(), length); // Write the string itself
        }
    }

    outFile.close();
    cout << "Clusters and average equities saved to " << filename << endl;
}

// Function to read clusters and their average equity from a binary file
void readClustersFromBin(const string& filename) {
    ifstream inFile(filename, ios::binary);
    if (!inFile) {
        cerr << "Error opening input file!" << endl;
        return;
    }

    while (true) {
        double averageEquity;
        // Read average equity for the current cluster
        if (!inFile.read(reinterpret_cast<char*>(&averageEquity), sizeof(averageEquity))) {
            break; // Break if reading fails (EOF or error)
        }

        cout << "Average Equity: " << averageEquity << "%, Hands: ";

        while (true) {
            size_t length;
            // Read length of the hand string
            if (!inFile.read(reinterpret_cast<char*>(&length), sizeof(length))) {
                break; // Break if reading fails (EOF or error)
            }

            // Read the hand string
            string hand(length, '\0'); // Create a string of the appropriate length
            if (!inFile.read(&hand[0], length)) {
                cerr << "Error reading hand string." << endl;
                break; // Break if reading fails
            }

            cout << hand << ", "; // Print each hand directly
        }
        cout << endl; // New line after each cluster
    }

    inFile.close();
}

// Comment out the duplicate main function

int main() {
    // The ranks in order.

    // uncomment to simulate equities again

    vector<string> RANKS = {"A", "K", "Q", "J", "10", "9", "8", "7", "6", "5", "4", "3", "2"};
    // const int nSims = 10000; // simulations per hand. Increase for more accuracy.

    // // Create a 13x13 equity matrix.
    // vector< vector<double> > heatmap(13, vector<double>(13, 0.0));

    // cout << "Simulating equities for NUM_PLAYERS = " << NUM_PLAYERS << endl;

    // // Loop over the 13x13 grid.
    // for (int i = 0; i < 13; i++) {
    //     for (int j = 0; j < 13; j++) {
    //         vector<Card> heroHand;
    //         if (i == j) {
    //             // Pocket pair. Use two different suits.
    //             heroHand.push_back(Card{RANKS[i], "h"});
    //             heroHand.push_back(Card{RANKS[i], "d"});
    //         }
    //         else if (i < j) {
    //             // Suited hand. Use hearts.
    //             heroHand.push_back(Card{RANKS[i], "h"});
    //             heroHand.push_back(Card{RANKS[j], "h"});
    //         }
    //         else { // i > j
    //             // Offsuit hand. Use different suits.
    //             heroHand.push_back(Card{RANKS[i], "h"});
    //             heroHand.push_back(Card{RANKS[j], "d"});
    //         }

    //         double eq = simulateEquity(heroHand, nSims);
    //         heatmap[i][j] = eq;

    //         cout << "Hand " << cardToStr(heroHand[0]) << " " << cardToStr(heroHand[1])
    //              << " -> " << eq << "%" << endl;
    //     }
    // }

    // After calculating the heatmap
    // Read the equity heatmap from the CSV file
    ifstream file("equity_heatmap.csv");
    if (!file.is_open()) {
        cerr << "Error: Could not open the file 'utils/equity_heatmap.csv'" << endl;
        return 1; // Exit or handle the error as needed
    }
    string line;
    vector<vector<double>> heatmap(13, vector<double>(13, 0.0));
    int row = 0;

    while (getline(file, line)) {
        if (row == 0) {
            // Skip the header line
            row++;
            continue;
        }
        stringstream ss(line);
        string value;
        int col = 0;

        while (getline(ss, value, ',')) {
            if (col > 0) { // Skip the first column which is the hand
                cout << stod(value) << " ";
                try {
                    heatmap[row - 1][col - 1] = stod(value);
                } catch (const invalid_argument& e) {
                    cerr << "Invalid value at row " << row << ", column " << col << ": " << value << endl;
                }
            }
            col++;
        }
        row++;
    }

    // Uncomment to run the method of Elbow to determien the optimal empirical number of clusters
    // Range of clusters to test
    // int maxClusters = 160; // Set the maximum number of clusters
    // vector<double> ssdValues; // To store SSD values

    // // Loop through different numbers of clusters
    // for (int nb_clusters = 1; nb_clusters <= maxClusters; nb_clusters++) {
    //     vector<int> clusterAssignments = clusterEquities(heatmap, nb_clusters); // Cluster into specified number of clusters

    //     // Calculate and print SSD for the current clustering
    //     double ssd = calculateSSD(heatmap, clusterAssignments, nb_clusters);
    //     ssdValues.push_back(ssd); // Store the SSD value
    // }

    // // Print the list of SSD values in one line with commas
    // cout << "List of SSD values for different numbers of clusters: ";
    // for (size_t i = 0; i < ssdValues.size(); i++) {
    //     cout << ssdValues[i];
    //     if (i < ssdValues.size() - 1) {
    //         cout << ", "; // Print a comma after each value except the last one
    //     }
    // }
    // cout << endl; // New line after the list


    int nb_clusters = 20;
    vector<int> clusterAssignments = clusterEquities(heatmap, nb_clusters); // Cluster into specified number of clusters

    // Calculate and print SSD for the current clustering
    double ssd = calculateSSD(heatmap, clusterAssignments, nb_clusters);
    cout << "Sum of Squared Distances (SSD) for " << nb_clusters << " clusters: " << ssd << endl;

    // Create a mapping of clusters to hands
    vector<vector<string>> clusterHands(nb_clusters); 

    // Populate the clusterHands with card representations
    for (int i = 0; i < 13; i++) {
        for (int j = 0; j < 13; j++) {
            // Create the hero hand
            string hand;
            string handType; // Variable to hold the type of hand (suited or offsuit)
            if (i == j) {
                // Pocket pair (e.g., A A)
                hand = RANKS[i] + " " + RANKS[i]; // Same rank, suited
                handType = "o"; // Offsuit
            } else if (i < j) {
                // Suited hand (e.g., K A suited)
                hand = RANKS[i] + " " + RANKS[j]; // Different ranks, suited
                handType = "s"; // Suited
            } else {
                // Offsuit hand (e.g., K A offsuit)
                hand = RANKS[i] + " " + RANKS[j]; // Different ranks, offsuit
                handType = "o"; // Offsuit
            }

            // Get the cluster index for the current hand
            int clusterIndex = clusterAssignments[i * 13 + j];
            clusterHands[clusterIndex].push_back(hand + handType); // Add the hand to the corresponding cluster
        }
    }
    

    // Output clusters and their corresponding hands with average equity from heatmap
    cout << "Clusters and their corresponding hands with average equity from heatmap:" << endl;
    for (int cluster = 0; cluster < clusterHands.size(); ++cluster) {
        cout << "Cluster " << cluster << ": ";
        
        // Calculate average equity for the current cluster
        double totalEquity = 0.0;
        int handCount = clusterHands[cluster].size();
        
        for (const auto& hand : clusterHands[cluster]) {
            // Split the hand into ranks and types
            stringstream ss(hand);
            string firstCard, secondCard;
            ss >> firstCard >> secondCard; // Read the two cards

            // Determine the hand type based on the last character of each card
            string handType = (firstCard.back() == 's' || secondCard.back() == 's') ? "s" : "o"; // Determine type for the hand

            // Remove the suit character from the card strings
            secondCard = secondCard.substr(0, secondCard.size() - 1); // Remove the last character (suit)
            
            // Determine indices for the heatmap
            int rankIndex1 = 14 -  rankMapping[firstCard]; // Get index for first card
            int rankIndex2 = 14 - rankMapping[secondCard]; // Get index for second card

            // Get the equity from the heatmap
            double equity = heatmap[rankIndex1][rankIndex2];
            totalEquity += equity;
        }
        
        double averageEquity = (handCount > 0) ? (totalEquity / handCount) : 0.0;
        cout << "Average Equity: " << averageEquity << "%, Hands: ";
        
        for (const auto& hand : clusterHands[cluster]) {
            cout << hand << ", "; // Print each hand in the cluster
        }
        cout << endl; // New line after each cluster
    }

    // // Output ranks, hand type (suited or offsuit), and cluster assignments
    // cout << "Ranks, hand types, and cluster assignments:" << endl;
    // for (int i = 0; i < 13; i++) {
    //     for (int j = 0; j < 13; j++) {
    //         // Determine hand type based on indices
    //         string handType = (i < j) ? "s" : "o"; // Suited if i < j, offsuit otherwise

    //         // Print the ranks, hand type, and cluster assignment
    //         cout << "Hand: " << RANKS[i] << " " << RANKS[j] << "" << handType << " is in cluster " 
    //              << clusterAssignments[i * 13 + j] << endl;
    //     }
    // }
        // Output clusters in a grid format
    cout << "Calculated starting hand clusters:" << endl;

    const string RESET = "\033[0m";
    const string COLORS[] = {
        "\033[31m", // Red
        "\033[32m", // Green
        "\033[33m", // Yellow
        "\033[34m", // Blue
        "\033[35m", // Magenta
        "\033[36m", // Cyan
        "\033[37m", // White
        "\033[90m", // Bright Black (Gray)
        "\033[91m", // Bright Red
        "\033[92m", // Bright Green
        "\033[93m", // Bright Yellow
        "\033[94m", // Bright Blue
        "\033[95m", // Bright Magenta
        "\033[96m", // Bright Cyan
        "\033[97m"  // Bright White
    };
    for (int i = 0; i < 13; i++) {
        for (int j = 0; j < 13; j++) {
            // Get the cluster assignment
            int clusterIndex = clusterAssignments[i * 13 + j];

            // Use a color based on the cluster index (modulo to stay within bounds)
            string color = COLORS[clusterIndex % (sizeof(COLORS) / sizeof(COLORS[0]))];

            // Print the cluster assignment using a character in the corresponding color
            cout << color << 'X' << RESET << " "; // 'X' represents the cluster
        }
        cout << endl; // New line after each row
    }
    // Write the heatmap to a CSV file.
    ofstream outFile("equity_heatmap.csv");
    if (!outFile) {
        cerr << "Error opening output file!" << endl;
        return 1;
    }

    // Write a header row: columns = RANKS.
    outFile << "Hand,";
    for (int j = 0; j < 13; j++) {
        outFile << RANKS[j];
        if (j < 12)
            outFile << ",";
    }
    outFile << "\n";

    // For each row, output the label and equity values.
    for (int i = 0; i < 13; i++) {
        outFile << RANKS[i] << ",";
        for (int j = 0; j < 13; j++) {
            outFile << heatmap[i][j];
            if (j < 12)
                outFile << ",";
        }
        outFile << "\n";
    }
    outFile.close();

    cout << "Equity heatmap saved to equity_heatmap.csv" << endl;

    // Call the new function to save clusters
    saveClustersToBin(clusterHands, heatmap, "preflop_clusters.bin");
    
    readClustersFromBin("preflop_clusters.bin"); // Call the function to read and print clusters
    
    return 0;
}

