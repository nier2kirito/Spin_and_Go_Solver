#include "poker_evaluator.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>
#include <thread>
#include <atomic>
#include <mutex>

using namespace std;

// Parse a card string (e.g., "As" for Ace of spades or "10h" for Ten of hearts)
Card parseCard(const string& cardStr) {
    string cleanCardStr = cardStr;
    
    // Remove any quotes from the card string
    cleanCardStr.erase(remove(cleanCardStr.begin(), cleanCardStr.end(), '"'), cleanCardStr.end());
    
    if (cleanCardStr.length() < 2) {
        throw runtime_error("Invalid card format: " + cardStr);
    }
    
    string rank;
    string suit;
    
    // Handle 10 as a special case (two characters for rank)
    if (cleanCardStr.length() >= 3 && cleanCardStr.substr(0, 2) == "10") {
        rank = "10";
        suit = cleanCardStr.substr(2, 1);
    } else {
        rank = cleanCardStr.substr(0, cleanCardStr.length() - 1);
        suit = cleanCardStr.substr(cleanCardStr.length() - 1, 1);
    }
    
    // Validate the rank and suit
    if (find(RANKS.begin(), RANKS.end(), rank) == RANKS.end()) {
        throw runtime_error("Invalid card rank: " + rank);
    }
    
    if (find(SUITS.begin(), SUITS.end(), suit[0]) == SUITS.end()) {
        throw runtime_error("Invalid card suit: " + suit);
    }
    
    return {rank, suit};
}

// Parse a hand string (e.g., "2c 2d 2h 2s 4s 10s As")
vector<Card> parseHand(const string& handStr) {
    vector<Card> hand;
    hand.reserve(7); // Reserve space for 7 cards
    stringstream ss(handStr);
    string cardStr;
    
    while (ss >> cardStr) {
        hand.push_back(parseCard(cardStr));
    }
    
    return hand;
}

// Calculate equity for 2 players using full enumeration or Monte Carlo
double calculateEquity2Players(const vector<Card>& sevenCards, PokerEvaluator &evaluator, int trials = 0) {
    // Generate the remaining deck
    vector<Card> deck;
    deck.reserve(52 - sevenCards.size()); // Pre-allocate memory
    
    // Use a lookup table to track which cards are already used
    bool usedCards[4][13] = {false}; // [suit][rank]
    
    // Mark the cards that are already in the hand
    for (const auto& card : sevenCards) {
        int suitIndex = distance(SUITS.begin(), find(SUITS.begin(), SUITS.end(), card.suit[0]));
        int rankIndex = distance(RANKS.begin(), find(RANKS.begin(), RANKS.end(), card.rank));
        usedCards[suitIndex][rankIndex] = true;
    }
    
    // Build the deck using the lookup table
    for (int s = 0; s < 4; s++) {
        for (int r = 0; r < 13; r++) {
            if (!usedCards[s][r]) {
                deck.push_back({RANKS[r], string(1, SUITS[s])});
            }
        }
    }
    
    int wins = 0, ties = 0, total = 0;
    
    // Use Monte Carlo if trials > 0, otherwise full enumeration
    if (trials > 0) {
        // Monte Carlo simulation
        random_device rd;
        mt19937 g(rd());
        
        for (int i = 0; i < trials; i++) {
            // Randomly select 5 cards from the 7 cards for the board
            vector<int> indices(7);
            iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., 6
            shuffle(indices.begin(), indices.end(), g);
            
            vector<Card> communityCards;
            communityCards.reserve(5);
            for (int j = 0; j < 5; j++) {
                communityCards.push_back(sevenCards[indices[j]]);
            }
            
            // Calculate main player's score with the remaining 2 cards as hole cards
            vector<Card> mainHole;
            mainHole.reserve(2);
            for (int j = 5; j < 7; j++) {
                mainHole.push_back(sevenCards[indices[j]]);
            }
            
            int mainScore = evaluator.evaluateHand(mainHole, communityCards);
            
            // Instead of shuffling the entire deck, just pick 2 random cards
            int idx1 = uniform_int_distribution<>(0, deck.size() - 1)(g);
            int idx2;
            do {
                idx2 = uniform_int_distribution<>(0, deck.size() - 1)(g);
            } while (idx2 == idx1);
            
            vector<Card> opponentHole = {deck[idx1], deck[idx2]};
            int opponentScore = evaluator.evaluateHand(opponentHole, communityCards);
            
            if (mainScore > opponentScore) wins++;
            else if (mainScore == opponentScore) ties++;
            
            total++;
        }
    } else {
        // Full enumeration
        // For full enumeration, we need to consider all possible 5-card combinations from the 7 cards
        vector<int> indices(7);
        iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., 6
        
        // Generate all combinations of 5 indices from 7
        vector<bool> v(7);
        fill(v.begin(), v.begin() + 5, true);
        
        do {
            vector<Card> communityCards;
            vector<Card> mainHole;
            
            for (int i = 0; i < 7; i++) {
                if (v[i]) {
                    communityCards.push_back(sevenCards[indices[i]]);
                } else {
                    mainHole.push_back(sevenCards[indices[i]]);
                }
            }
            
            int mainScore = evaluator.evaluateHand(mainHole, communityCards);
            
            for (size_t i = 0; i < deck.size(); i++) {
                for (size_t j = i + 1; j < deck.size(); j++) {
                    vector<Card> opponentHole = {deck[i], deck[j]};
                    
                    int opponentScore = evaluator.evaluateHand(opponentHole, communityCards);
                    
                    if (mainScore > opponentScore) wins++;
                    else if (mainScore == opponentScore) ties++;
                    
                    total++;
                }
            }
        } while (prev_permutation(v.begin(), v.end()));
    }
    
    double equity = (wins + ties / 2.0) / total;
    return equity;
}

// Calculate equity for 3 players using full enumeration or Monte Carlo
double calculateEquity3Players(const vector<Card>& sevenCards, PokerEvaluator &evaluator, int trials = 0) {
    // Generate the remaining deck with the same optimization as in calculateEquity2Players
    vector<Card> deck;
    deck.reserve(52 - sevenCards.size()); // Pre-allocate memory
    
    // Use a lookup table to track which cards are already used
    bool usedCards[4][13] = {false}; // [suit][rank]
    
    // Mark the cards that are already in the hand
    for (const auto& card : sevenCards) {
        int suitIndex = distance(SUITS.begin(), find(SUITS.begin(), SUITS.end(), card.suit[0]));
        int rankIndex = distance(RANKS.begin(), find(RANKS.begin(), RANKS.end(), card.rank));
        usedCards[suitIndex][rankIndex] = true;
    }
    
    // Build the deck using the lookup table
    for (int s = 0; s < 4; s++) {
        for (int r = 0; r < 13; r++) {
            if (!usedCards[s][r]) {
                deck.push_back({RANKS[r], string(1, SUITS[s])});
            }
        }
    }
    
    int wins = 0, ties = 0, total = 0;
    
    // Use Monte Carlo if trials > 0, otherwise full enumeration
    if (trials > 0) {
        // Monte Carlo simulation
        random_device rd;
        mt19937 g(rd());
        
        for (int i = 0; i < trials; i++) {
            // Randomly select 5 cards from the 7 cards for the board
            vector<int> indices(7);
            iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., 6
            shuffle(indices.begin(), indices.end(), g);
            
            vector<Card> communityCards;
            communityCards.reserve(5);
            for (int j = 0; j < 5; j++) {
                communityCards.push_back(sevenCards[indices[j]]);
            }
            
            // Calculate main player's score with the remaining 2 cards as hole cards
            vector<Card> mainHole;
            mainHole.reserve(2);
            for (int j = 5; j < 7; j++) {
                mainHole.push_back(sevenCards[indices[j]]);
            }
            
            int mainScore = evaluator.evaluateHand(mainHole, communityCards);
            
            // Instead of shuffling the entire deck, just pick 4 random cards
            vector<int> deckIndices;
            for (int j = 0; j < 4; j++) {
                int idx;
                do {
                    idx = uniform_int_distribution<>(0, deck.size() - 1)(g);
                } while (find(deckIndices.begin(), deckIndices.end(), idx) != deckIndices.end());
                deckIndices.push_back(idx);
            }
            
            vector<Card> opponent1Hole = {deck[deckIndices[0]], deck[deckIndices[1]]};
            vector<Card> opponent2Hole = {deck[deckIndices[2]], deck[deckIndices[3]]};
            
            int opponent1Score = evaluator.evaluateHand(opponent1Hole, communityCards);
            int opponent2Score = evaluator.evaluateHand(opponent2Hole, communityCards);
            
            if (mainScore > opponent1Score && mainScore > opponent2Score) wins++;
            else if (mainScore == opponent1Score && mainScore == opponent2Score) ties += 2; // 3-way tie
            else if (mainScore == opponent1Score || mainScore == opponent2Score) ties++; // 2-way tie
            
            total++;
        }
    } else {
        // Full enumeration - this can be very computationally intensive
        // For full enumeration, we need to consider all possible 5-card combinations from the 7 cards
        vector<int> indices(7);
        iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., 6
        
        // Generate all combinations of 5 indices from 7
        vector<bool> v(7);
        fill(v.begin(), v.begin() + 5, true);
        
        do {
            vector<Card> communityCards;
            vector<Card> mainHole;
            
            for (int i = 0; i < 7; i++) {
                if (v[i]) {
                    communityCards.push_back(sevenCards[indices[i]]);
                } else {
                    mainHole.push_back(sevenCards[indices[i]]);
                }
            }
            
            int mainScore = evaluator.evaluateHand(mainHole, communityCards);
            
            for (size_t i = 0; i < deck.size(); i++) {
                for (size_t j = i + 1; j < deck.size(); j++) {
                    vector<Card> opponent1Hole = {deck[i], deck[j]};
                    int opponent1Score = evaluator.evaluateHand(opponent1Hole, communityCards);
                    
                    for (size_t k = j + 1; k < deck.size(); k++) {
                        for (size_t l = k + 1; l < deck.size(); l++) {
                            vector<Card> opponent2Hole = {deck[k], deck[l]};
                            int opponent2Score = evaluator.evaluateHand(opponent2Hole, communityCards);
                            
                            if (mainScore > opponent1Score && mainScore > opponent2Score) wins++;
                            else if (mainScore == opponent1Score && mainScore == opponent2Score) ties += 2; // 3-way tie
                            else if (mainScore == opponent1Score || mainScore == opponent2Score) ties++; // 2-way tie
                            
                            total++;
                        }
                    }
                }
            }
        } while (prev_permutation(v.begin(), v.end()));
    }
    
    double equity = (wins + ties / 3.0) / total;
    return equity;
}

// Calculate equity for 2 players using full enumeration or Monte Carlo with multithreading
double calculateEquity2PlayersParallel(const vector<Card>& sevenCards, PokerEvaluator &evaluator, 
                                      mt19937 &randomEngine, int trials, int numThreads) {
    // Generate the remaining deck
    vector<Card> deck;
    deck.reserve(52 - sevenCards.size()); // Pre-allocate memory
    
    // Use a lookup table to track which cards are already used
    bool usedCards[4][13] = {false}; // [suit][rank]
    
    // Mark the cards that are already in the hand
    for (const auto& card : sevenCards) {
        int suitIndex = distance(SUITS.begin(), find(SUITS.begin(), SUITS.end(), card.suit[0]));
        int rankIndex = distance(RANKS.begin(), find(RANKS.begin(), RANKS.end(), card.rank));
        usedCards[suitIndex][rankIndex] = true;
    }
    
    // Build the deck using the lookup table
    for (int s = 0; s < 4; s++) {
        for (int r = 0; r < 13; r++) {
            if (!usedCards[s][r]) {
                deck.push_back({RANKS[r], string(1, SUITS[s])});
            }
        }
    }
    
    // Prepare accumulation variables
    atomic<int> wins(0);
    atomic<int> ties(0);
    
    // Determine trials per thread
    int trialsPerThread = trials / numThreads;
    int extraTrials = trials % numThreads;
    
    // Create a mutex to protect access to the shared random engine
    mutex randomMutex;
    
    // Lambda for each thread's simulation
    auto worker = [&](int threadTrials) {
        // Create a thread-local random engine seeded from the shared one
        mt19937 threadRandom;
        {
            lock_guard<mutex> lock(randomMutex);
            threadRandom.seed(randomEngine());
        }
        
        for (int i = 0; i < threadTrials; i++) {
            // Randomly select 5 cards from the 7 cards for the board
            vector<int> indices(7);
            iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., 6
            
            // Use thread-local random engine for shuffling
            shuffle(indices.begin(), indices.end(), threadRandom);
            
            vector<Card> communityCards;
            communityCards.reserve(5);
            for (int j = 0; j < 5; j++) {
                communityCards.push_back(sevenCards[indices[j]]);
            }
            
            // Calculate main player's score with the remaining 2 cards as hole cards
            vector<Card> mainHole;
            mainHole.reserve(2);
            for (int j = 5; j < 7; j++) {
                mainHole.push_back(sevenCards[indices[j]]);
            }
            
            int mainScore = evaluator.evaluateHand(mainHole, communityCards);
            
            // For each trial, randomly pick two different deck indices for the opponent's hole cards.
            int idx1 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            int idx2 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            while (idx2 == idx1)
                idx2 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            
            vector<Card> opponentHole = { deck[idx1], deck[idx2] };
            int oppScore = evaluator.evaluateHand(opponentHole, communityCards);
            
            if (mainScore > oppScore)
                wins++;
            else if (mainScore == oppScore)
                ties++;
        }
    };
    
    // Create and launch threads
    vector<thread> threads;
    for (int t = 0; t < numThreads; t++) {
        int thisTrialCount = trialsPerThread + (t < extraTrials ? 1 : 0);
        threads.push_back(thread(worker, thisTrialCount));
    }
    
    // Join threads
    for (auto &th : threads)
        th.join();
    
    int total = trials;  // total trials
    double equity = (wins + ties/2.0) / total;
    return equity;
}

// Calculate equity for 3 players using Monte Carlo with multithreading
double calculateEquity3PlayersParallel(const vector<Card>& sevenCards, PokerEvaluator &evaluator, 
                                      mt19937 &randomEngine, int trials, int numThreads) {
    // Generate the remaining deck
    vector<Card> deck;
    deck.reserve(52 - sevenCards.size()); // Pre-allocate memory
    
    // Use a lookup table to track which cards are already used
    bool usedCards[4][13] = {false}; // [suit][rank]
    
    // Mark the cards that are already in the hand
    for (const auto& card : sevenCards) {
        int suitIndex = distance(SUITS.begin(), find(SUITS.begin(), SUITS.end(), card.suit[0]));
        int rankIndex = distance(RANKS.begin(), find(RANKS.begin(), RANKS.end(), card.rank));
        usedCards[suitIndex][rankIndex] = true;
    }
    
    // Build the deck using the lookup table
    for (int s = 0; s < 4; s++) {
        for (int r = 0; r < 13; r++) {
            if (!usedCards[s][r]) {
                deck.push_back({RANKS[r], string(1, SUITS[s])});
            }
        }
    }
    
    atomic<int> wins(0);
    atomic<int> ties(0);
    
    int trialsPerThread = trials / numThreads;
    int extraTrials = trials % numThreads;
    
    // Create a mutex to protect access to the shared random engine
    mutex randomMutex;
    
    auto worker = [&](int threadTrials) {
        // Create a thread-local random engine seeded from the shared one
        mt19937 threadRandom;
        {
            lock_guard<mutex> lock(randomMutex);
            threadRandom.seed(randomEngine());
        }
        
        for (int i = 0; i < threadTrials; i++) {
            // Randomly select 5 cards from the 7 cards for the board
            vector<int> indices(7);
            iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., 6
            
            // Use thread-local random engine for shuffling
            shuffle(indices.begin(), indices.end(), threadRandom);
            
            vector<Card> communityCards;
            communityCards.reserve(5);
            for (int j = 0; j < 5; j++) {
                communityCards.push_back(sevenCards[indices[j]]);
            }
            
            // Calculate main player's score with the remaining 2 cards as hole cards
            vector<Card> mainHole;
            mainHole.reserve(2);
            for (int j = 5; j < 7; j++) {
                mainHole.push_back(sevenCards[indices[j]]);
            }
            
            int mainScore = evaluator.evaluateHand(mainHole, communityCards);
            
            // Pick four distinct random indices for the two opponents
            int idx1 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            int idx2 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            while (idx2 == idx1)
                idx2 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            int idx3 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            while (idx3 == idx1 || idx3 == idx2)
                idx3 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            int idx4 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            while (idx4 == idx1 || idx4 == idx2 || idx4 == idx3)
                idx4 = uniform_int_distribution<>(0, deck.size() - 1)(threadRandom);
            
            vector<Card> opp1Hole = { deck[idx1], deck[idx2] };
            vector<Card> opp2Hole = { deck[idx3], deck[idx4] };

            int opp1Score = evaluator.evaluateHand(opp1Hole, communityCards);
            int opp2Score = evaluator.evaluateHand(opp2Hole, communityCards);
            
            if (mainScore > opp1Score && mainScore > opp2Score) {
                wins++;
            } else if (mainScore == opp1Score && mainScore == opp2Score) {
                // For a full tie, we count as two "tie-credits" to be divided by 3 later
                ties += 2;
            } else if (mainScore == opp1Score || mainScore == opp2Score) {
                ties++;
            }
        }
    };
    
    vector<thread> threads;
    for (int t = 0; t < numThreads; t++) {
        int thisTrialCount = trialsPerThread + (t < extraTrials ? 1 : 0);
        threads.push_back(thread(worker, thisTrialCount));
    }
    for (auto &th : threads)
        th.join();
    
    int total = trials;
    double equity = (wins + ties / 3.0) / total;
    return equity;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_csv_file> [monte_carlo_trials] [num_threads] [start_line] [end_line] [output_file]" << endl;
        return 1;
    }
    
    string inputFile = argv[1];
    int monteCarloTrials = 100000; // default trial count
    if (argc > 2)
        monteCarloTrials = stoi(argv[2]);
    
    // default number of threads: use hardware concurrency if available, else 4
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 4;
    if (argc > 3)
        numThreads = stoi(argv[3]);
    
    // Default line range: process all lines
    int startLine = 1;
    int endLine = numeric_limits<int>::max();
    
    // Parse start and end line arguments if provided
    if (argc > 4)
        startLine = max(1, stoi(argv[4])); // Ensure startLine is at least 1
    
    if (argc > 5)
        endLine = stoi(argv[5]);
    
    // Default output file name
    string outputFile = "out.csv";
    if (argc > 6)
        outputFile = argv[6];
    
    cout << "Input file: " << inputFile << "\nTrials: " << monteCarloTrials 
         << "\nThreads: " << numThreads 
         << "\nProcessing lines: " << startLine << " to " << (endLine == numeric_limits<int>::max() ? "end" : to_string(endLine))
         << "\nOutput file: " << outputFile
         << endl;
    
    ifstream inFile(inputFile);
    if (!inFile) {
        cerr << "Error: cannot open file " << inputFile << endl;
        return 1;
    }
    
    // Count total lines in the file for ETA calculation
    int totalLines = 0;
    string countLine;
    {
        ifstream countFile(inputFile);
        while (getline(countFile, countLine)) {
            if (!countLine.empty()) {
                totalLines++;
            }
        }
    }
    
    ofstream outFile(outputFile);
    if (!outFile) {
        cerr << "Error: cannot create output file." << endl;
        return 1;
    }
    
    // Write CSV header
    outFile << "Hand,Equity2P,Equity3P" << endl;
    
    string line;
    int lineCount = 0;
    auto startTime = chrono::steady_clock::now();
    
    // Create a single PokerEvaluator instance to be used throughout the program
    PokerEvaluator evaluator;
    
    // Create a single random engine with a good seed
    random_device rd;
    mt19937 randomEngine(rd());
    
    // Skip lines before startLine
    while (lineCount < startLine - 1 && getline(inFile, line)) {
        lineCount++;
    }
    
    // Process input file for the specified range
    while(getline(inFile, line) && lineCount < endLine) {
        lineCount++;
        
        // Calculate and display ETA
        if (lineCount > 1) {
            auto currentTime = chrono::steady_clock::now();
            auto elapsedSeconds = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();
            
            if (elapsedSeconds > 0) {
                double linesPerSecond = static_cast<double>(lineCount - 1) / elapsedSeconds;
                int remainingLines = totalLines - lineCount;
                int estimatedSecondsRemaining = static_cast<int>(remainingLines / linesPerSecond);
                
                int hours = estimatedSecondsRemaining / 3600;
                int minutes = (estimatedSecondsRemaining % 3600) / 60;
                int seconds = estimatedSecondsRemaining % 60;
                
                cout << "\rProcessing line " << lineCount << "/" << totalLines 
                     << " (" << fixed << setprecision(1) << (lineCount * 100.0 / totalLines) << "%) - "
                     << "ETA: " << hours << "h " << minutes << "m " << seconds << "s"
                     << " - " << fixed << setprecision(2) << linesPerSecond << " lines/sec    " << flush;
            }
        } else {
            cout << "\rProcessing line " << lineCount << "/" << totalLines << flush;
        }
        
        if (line.empty())
            continue;
            
        stringstream ss(line);
        string handStr, riverId;
        getline(ss, handStr, ',');
        getline(ss, riverId, ',');  // not used
        
        // Remove quotes and trim
        handStr.erase(remove(handStr.begin(), handStr.end(), '"'), handStr.end());
        handStr = handStr.substr(handStr.find_first_not_of(" "), 
                                handStr.find_last_not_of(" ") - handStr.find_first_not_of(" ") + 1);
        
        try {
            vector<Card> sevenCards = parseHand(handStr);
            if (sevenCards.size() != 7) {
                cerr << "\nWarning: Expected 7 cards, got " << sevenCards.size() << " in line: " << line << endl;
                outFile << handStr << ",ERROR,ERROR" << endl;
                continue;
            }
            
            // Pass the shared evaluator and random engine to all functions:
            double equity2p = calculateEquity2PlayersParallel(sevenCards, evaluator, randomEngine, monteCarloTrials, numThreads);
            double equity3p = calculateEquity3PlayersParallel(sevenCards, evaluator, randomEngine, monteCarloTrials, numThreads);
            
            outFile << handStr << "," << fixed << setprecision(6) << equity2p << "," << equity3p << endl;
        } catch (const exception &e) {
            cerr << "\nError processing line: " << line << " - " << e.what() << endl;
            outFile << handStr << ",ERROR,ERROR" << endl;
        }
    }
    
    // Display final statistics
    auto endTime = chrono::steady_clock::now();
    auto totalSeconds = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    cout << "\n\nProcessing complete. Results written to " << outputFile << endl;
    cout << "Processed " << lineCount << " lines in " 
         << hours << "h " << minutes << "m " << seconds << "s" << endl;
    if (totalSeconds > 0) {
        cout << "Average processing speed: " << fixed << setprecision(2) 
             << (lineCount / static_cast<double>(totalSeconds)) << " lines/sec" << endl;
    }
    // Delete the second row of the CSV file
    ifstream tempFile(outputFile);
    ofstream outFileTemp(outputFile + ".temp");
    string anotherLine;

    int linesCount = 0;

    while (getline(tempFile, anotherLine)) {
        if (linesCount != 1) { // Skip the second row (index 1)
            outFileTemp << anotherLine << endl;
        }
        linesCount++;
    }

    tempFile.close();
    outFileTemp.close();

    remove(outputFile.c_str());
    rename((outputFile + ".temp").c_str(), outputFile.c_str());

    inFile.close();
    outFile.close();
    return 0;
}
