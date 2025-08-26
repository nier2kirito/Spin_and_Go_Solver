#include "spingo.h"
#include <iostream>
#include <sstream>
#include <random>
#include <ctime>
#include <numeric>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <algorithm>

// Standalone Node class (not inside AoFState)
class Node {
public:
    Node();
    Node(int numActions);
    std::vector<double> getStrategy(double realizationWeight);
    std::vector<double> getAverageStrategy() const;
    std::vector<double> regretSum;
    std::vector<double> strategy;
    std::vector<double> strategySum;
};


// Add this function declaration at the top with other function declarations
void saveInfoSetsToFile(const std::string& filename);

// Add this near the top of the file with other function declarations
void trainMCCFR(SpinGoGame& game, int iterations);

