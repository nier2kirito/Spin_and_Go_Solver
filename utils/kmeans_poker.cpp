/*
kmeans_poker.cpp

This program:
1. Uses the PokerEvaluator (and Card) from "poker_evaluator.cpp"
2. Uses fixed hole cards and community cards.
3. Generates all possible subsets (combinations) of community cards of size 3, 4, and 5.
4. For each subset, uses evaluateHand() (with the fixed hole cards) to compute a hand “strength”.
5. Clusters the resulting scores using a k–means++ algorithm.
*/

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <numeric>
#include <limits> // for std::numeric_limits

#include "poker_evaluator.cpp" // Must define: Card, PokerEvaluator

using std::vector;
using std::string;
using std::cout;
using std::endl;

// Global evaluator – note: in this example we re‐initialize it.
PokerEvaluator evaluator;

void initializeEvaluator() {
    evaluator = PokerEvaluator();
}

// Helper function: Generate combinations (of generic type) recursively.
template <typename T>
void generateCombinations(const vector<T>& arr, int r, int index, vector<T>& current, vector<vector<T>>& result) {
    if (current.size() == r) {
        result.push_back(current);
        return;
    }
    for (size_t i = index; i < arr.size(); i++) {
        current.push_back(arr[i]);
        generateCombinations(arr, r, i + 1, current, result);
        current.pop_back();
    }
}

// A DataPoint is one hand configuration (hole cards fixed and a chosen community subset) and the computed score.
struct DataPoint {
    double score;
    vector<Card> communityCombo;
};

// k-means++ algorithm for one-dimensional data
//
// Parameters:
// data: vector of scores (feature dimension = 1)
// k: number of clusters
//
// Returns a vector "assignments" where assignments[i] is the cluster index assigned to data[i]
// and the vector "centroids" is updated to the final centroid values.
void kmeansPlusPlus1D(const vector<double>& data, int k, vector<int>& assignments, vector<double>& centroids) {
    int n = data.size();
    centroids.clear();
    assignments.assign(n, -1);
    // Seed the random number generator
    std::srand(std::time(0));

    // Choose the first centroid uniformly at random from the data
    int idx = std::rand() % n;
    centroids.push_back(data[idx]);

    // Choose remaining centroids with probability proportional to D(x)^2.
    while (centroids.size() < (size_t)k) {
        vector<double> distances(n, 0.0);
        double total = 0.0;
        for (int i = 0; i < n; i++) {
            // Compute distance from data[i] to its nearest already chosen centroid.
            double bestDist = std::numeric_limits<double>::max();
            for (double c : centroids) {
                double d = std::fabs(data[i] - c);
                if (d < bestDist)
                    bestDist = d;
            }
            distances[i] = bestDist * bestDist; // use squared distance
            total += distances[i];
        }
        // Choose a random value in [0, total)
        double r = ((double) std::rand() / RAND_MAX) * total;
        double cumulative = 0.0;
        int nextIndex = 0;
        for (int i = 0; i < n; i++) {
            cumulative += distances[i];
            if (r <= cumulative) {
                nextIndex = i;
                break;
            }
        }
        centroids.push_back(data[nextIndex]);
    }

    bool changed = true;
    int iterations = 0;
    while (changed && iterations < 1000) { // limit iterations to avoid infinite loops
        changed = false;
        iterations++;

        // Assignment step: assign each point to the closest centroid
        for (int i = 0; i < n; i++) {
            double bestDist = std::numeric_limits<double>::max();
            int bestCluster = -1;
            for (int c = 0; c < k; c++) {
                double d = std::fabs(data[i] - centroids[c]);
                if (d < bestDist) {
                    bestDist = d;
                    bestCluster = c;
                }
            }
            if (assignments[i] != bestCluster) {
                assignments[i] = bestCluster;
                changed = true;
            }
        }

        // Update step: recalc centroids as mean of points in each cluster
        vector<double> newCentroids(k, 0.0);
        vector<int> counts(k, 0);
        for (int i = 0; i < n; i++) {
            int cluster = assignments[i];
            newCentroids[cluster] += data[i];
            counts[cluster]++;
        }
        // If a cluster ended up empty, reinitialize it randomly.
        for (int c = 0; c < k; c++) {
            if (counts[c] == 0) {
                newCentroids[c] = data[std::rand() % n];
            }
            else {
                newCentroids[c] /= counts[c];
            }
        }
        centroids = newCentroids;
    }
}

int main() {
    initializeEvaluator();

    // Define fixed hole cards.
    vector<Card> holeCards = { {"A", "h"}, {"A", "d"} };

    // Define fixed community cards.
    vector<Card> communityCards = { {"A", "c"}, {"A", "s"}, {"3", "c"}, {"3", "h"}, {"2", "d"} };

    // Generate all possible community card subsets of sizes 3, 4, and 5.
    vector<DataPoint> dataPoints;
    for (int r : {3, 4, 5}) {
        vector<vector<Card>> combos;
        vector<Card> currentCombo;
        generateCombinations(communityCards, r, 0, currentCombo, combos);

        // For each combination, evaluate the hand (using the fixed holeCards) and store the score.
        for (const auto& combo : combos) {
            int score = evaluator.evaluateHand(holeCards, combo);
            DataPoint dp;
            dp.score = static_cast<double>(score);
            dp.communityCombo = combo;
            dataPoints.push_back(dp);
        }
    }

    // Print out the number of data points generated.
    cout << "Generated " << dataPoints.size() << " data points." << endl;

    // Prepare 1D feature vector: here the strength score.
    vector<double> features;
    for (const auto& dp : dataPoints) {
        features.push_back(dp.score);
    }

    // Set desired number of clusters, e.g., k = 3.
    int k = 3;
    vector<int> assignments;
    vector<double> centroids;
    kmeansPlusPlus1D(features, k, assignments, centroids);

    // Report the clustering results.
    cout << "\nClustering results: " << endl;
    // For each cluster print the centroid and list the corresponding data points.
    for (int cluster = 0; cluster < k; cluster++) {
        cout << "\nCluster " << cluster << " (centroid = " << centroids[cluster] << "):\n";
        for (size_t i = 0; i < dataPoints.size(); i++) {
            if (assignments[i] == cluster) {
                cout << "  Score: " << dataPoints[i].score << " — Community Cards: ";
                for (const auto& card : dataPoints[i].communityCombo) {
                    cout << card.toString() << " ";
                }
                cout << endl;
            }
        }
    }

    return 0;
}
