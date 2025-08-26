#include <filesystem>
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
#include <map>
#include <vector>

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

void aggregateFilesInFolder(const std::string& folderPath, const std::string& outputFile) {
    std::vector<std::string> inputFiles;
    
    // Iterate through all files in the directory
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".csv") {
            inputFiles.push_back(entry.path().string());
        }
    }
    
    if (inputFiles.empty()) {
        std::cerr << "No CSV files found in directory: " << folderPath << std::endl;
        return;
    }
    
    // Use the existing aggregateStrategies function
    aggregateStrategies(inputFiles, outputFile);
}

int main() {
    try {
        aggregateFilesInFolder("../data", "aggregated_output.csv");  // Look for data folder one level up
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
