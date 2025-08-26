#include <iostream>
#include "spingo/spingo.h"
#include "spingo/mccfr.h"
#include <random>
#include <chrono>
#include <ctime>
#include <fstream>

// Add this external declaration to access nodeMap from mccfr.cpp
extern std::unordered_map<std::string, Node> nodeMap;

// Implementation of getGameParameters
GameParameters getGameParameters(const std::pair<float, float>& stakes) {
    GameParameters params;
    params.smallBlind = stakes.first;
    params.bigBlind = stakes.second;
    // Set other parameters as needed
    return params;
}

void saveInfoSetsToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    
    // Iterate through all infosets and write them to the file
    for (const auto& infoset_pair : nodeMap) {
        const std::string& infoset_key = infoset_pair.first;
        const Node& node = infoset_pair.second;
        
        outFile << "InfoSet: " << infoset_key << std::endl;
        outFile << "Strategy: ";
        for (size_t i = 0; i < node.strategy.size(); ++i) {
            outFile << node.strategy[i];
            if (i < node.strategy.size() - 1) {
                outFile << ", ";
            }
        }
        outFile << std::endl;
        outFile << "Regrets: ";
        for (size_t i = 0; i < node.regretSum.size(); ++i) {
            outFile << node.regretSum[i];
            if (i < node.regretSum.size() - 1) {
                outFile << ", ";
            }
        }
        outFile << std::endl;
        outFile << "-------------------" << std::endl;
    }
    
    outFile.close();
    std::cout << "InfoSets saved to " << filename << std::endl;
}

int main() {
    std::cout << "Program started\n";
    
    try {
        std::cout << "Creating random device...\n";
        std::random_device rd;
        std::cout << "Creating generator...\n";
        std::mt19937 gen(rd());
        
        std::cout << "Creating game...\n";

        std::pair<float, float> stakes = {0.10f, 0.20f};
        GameParameters params = getGameParameters(stakes);
        
        // Create game with parameters
        SpinGoGame game;
        
        std::cout << "Creating initial state...\n";
        SpinGoState state = game.new_initial_state();
        
        std::cout << "Starting MCCFR training...\n";
        int iterations = 2e7; 
        trainMCCFR(game, iterations);
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time);
        
        std::string filename = "infosets_" + 
                               std::to_string(now_tm->tm_year + 1900) + "_" +
                               std::to_string(now_tm->tm_mon + 1) + "_" +
                               std::to_string(now_tm->tm_mday) + "_" +
                               std::to_string(now_tm->tm_hour) + "_" +
                               std::to_string(now_tm->tm_min) + "_" +
                               std::to_string(now_tm->tm_sec) + ".txt";
        saveInfoSetsToFile(filename);
        
    } catch (const std::exception& e) {
        std::cerr << "Error caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error caught" << std::endl;
        return 1;
    }
    
    std::cout << "Program completed\n";
    return 0;
}