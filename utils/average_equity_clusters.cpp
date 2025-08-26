#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>

struct HandData {
    std::string hand;
    int cluster;
    double equity2P;
    double equity3P;
};

void calculateAverageEquityAndUpdateCSV(const std::string& inputFile, const std::string& outputFile) {
    // Read the input CSV file
    std::ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        std::cerr << "Error opening input file: " << inputFile << std::endl;
        return;
    }

    // Parse the CSV and collect data
    std::vector<HandData> hands;
    std::string line;
    std::getline(inFile, line); // Read header line
    std::string header = line;

    while (std::getline(inFile, line)) {
        std::stringstream ss(line);
        std::string token;
        HandData data;
        
        // Parse CSV line (actual format: hand,equity2P,equity3P,cluster,...)
        std::getline(ss, data.hand, ',');
        
        // Parse equity2P (second column)
        if (!std::getline(ss, token, ',') || token.empty()) {
            std::cerr << "Error parsing equity2P for line: " << line << std::endl;
            continue;
        }
        try {
            data.equity2P = std::stod(token);
        } catch (const std::exception& e) {
            std::cerr << "Error converting equity2P to double: " << token << " in line: " << line << std::endl;
            continue;
        }
        
        // Parse equity3P (third column)
        if (!std::getline(ss, token, ',') || token.empty()) {
            std::cerr << "Error parsing equity3P for line: " << line << std::endl;
            continue;
        }
        try {
            data.equity3P = std::stod(token);
        } catch (const std::exception& e) {
            std::cerr << "Error converting equity3P to double: " << token << " in line: " << line << std::endl;
            continue;
        }
        
        // Parse cluster (fourth column)
        if (!std::getline(ss, token, ',') || token.empty()) {
            std::cerr << "Error parsing cluster for line: " << line << std::endl;
            continue;
        }
        try {
            data.cluster = std::stoi(token);
        } catch (const std::exception& e) {
            std::cerr << "Error converting cluster to integer: " << token << " in line: " << line << std::endl;
            continue;
        }
        
        hands.push_back(data);
    }
    inFile.close();

    // Calculate average equity for each cluster
    std::unordered_map<int, std::pair<double, double>> clusterAverages; // cluster -> (avg2P, avg3P)
    std::unordered_map<int, int> clusterCounts;

    for (const auto& hand : hands) {
        clusterAverages[hand.cluster].first += hand.equity2P;
        clusterAverages[hand.cluster].second += hand.equity3P;
        clusterCounts[hand.cluster]++;
    }

    for (auto& [cluster, equities] : clusterAverages) {
        equities.first /= clusterCounts[cluster];
        equities.second /= clusterCounts[cluster];
    }

    // Write to output file with added average equity columns
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        std::cerr << "Error opening output file: " << outputFile << std::endl;
        return;
    }

    // Write header with new columns
    outFile << header << ",avg_equity2P,avg_equity3P" << std::endl;

    // Reopen input file to process line by line
    inFile.open(inputFile);
    std::getline(inFile, line); // Skip header

    while (std::getline(inFile, line)) {
        std::stringstream ss(line);
        std::string token;
        HandData data;
        
        // Parse CSV line (actual format: hand,equity2P,equity3P,cluster,...)
        std::getline(ss, data.hand, ',');
        std::getline(ss, token, ','); // Parse equity2P
        std::getline(ss, token, ','); // Parse equity3P
        std::getline(ss, token, ','); // Parse cluster
        data.cluster = std::stoi(token);
        
        // Write original line plus average equities
        outFile << line << ","
                << clusterAverages[data.cluster].first << ","
                << clusterAverages[data.cluster].second << std::endl;
    }
    
    inFile.close();
    outFile.close();
    
    std::cout << "Processed " << inputFile << " and saved to " << outputFile << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <l2_flop_clustered_hands.csv> <emd_flop_clustered_hands.csv>" << std::endl;
        return 1;
    }

    std::string l2File = argv[1];
    std::string emdFile = argv[2];
    
    // Process L2 clustered hands
    std::string l2OutputFile = l2File.substr(0, l2File.find_last_of('.')) + "_with_avg_equity.csv";
    calculateAverageEquityAndUpdateCSV(l2File, l2OutputFile);
    
    // Process EMD clustered hands
    std::string emdOutputFile = emdFile.substr(0, emdFile.find_last_of('.')) + "_with_avg_equity.csv";
    calculateAverageEquityAndUpdateCSV(emdFile, emdOutputFile);
    
    return 0;
}
