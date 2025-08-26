/*
To compile with OpenMP (for example using g++):
g++ -fopenmp -O2 -o kmeans_rounds Kmeans_rounds.cpp

Usage:
./kmeans_rounds <round>
  round: flop, turn, or river

Example:
./kmeans_rounds flop    # Processes flop_equities.csv
./kmeans_rounds turn    # Processes turn_equities.csv  
./kmeans_rounds river   # Processes river_equities.csv
*/
#include <vector>
#include <iostream>
#include <chrono>
#include <random>
#include <numeric>
#include <cmath>
#include <omp.h>
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <sstream>

using namespace std;
using Clock = chrono::high_resolution_clock;

int sampleDistribution(const vector<double>& distribution, mt19937 &rng) {
    vector<double> cumulative(distribution.size());
    partial_sum(distribution.begin(), distribution.end(), cumulative.begin());
    uniform_real_distribution<double> uni(0.0, 1.0);
    double rnd = uni(rng);
    for (size_t i = 0; i < cumulative.size(); i++) {
        if (rnd <= cumulative[i]) return i;
    }
    return distribution.size()-1;
}

class Kmeans {
public:
    vector<int> ClusterEMD(const vector<vector<float>>& data, int k, int nofRuns, const vector<int>* _bestCenters = nullptr) {
        int nElements = data.size();
        int dim = data[0].size();
        cout << "K-means++ (EMD) clustering " << nElements << " elements into " << k
             << " clusters with " << nofRuns << " runs..." << endl;
        random_device rd;
        mt19937 rng(rd());
        int filenameId = uniform_int_distribution<int>(0, 9999999)(rng);

        auto startTime = Clock::now();
        vector<int> bestCentersAssignment(nElements, 0);
        vector<int> recordCenters(nElements, 0);
        bool skipInit = false;
        if (_bestCenters != nullptr) {
            skipInit = true;
            bestCentersAssignment = *_bestCenters;
            recordCenters = *_bestCenters;
        }
        double recordDistance = numeric_limits<double>::max();

        for (int run = 0; run < nofRuns; ++run) {
            cout << "K-means++ starting clustering run " << run+1 << "..." << endl;
            double lastDistance = numeric_limits<double>::max();
            bool distanceChanged = true;
            vector<vector<float>> centers(k, vector<float>(dim, 0.0f));

            if (!skipInit) {
                bestCentersAssignment.assign(nElements, 0);
                centers = FindStartingCentersEMD(data, k, rng);
            }
            else {
                centers = CalculateNewCenters(data, bestCentersAssignment, k);
                skipInit = false;
            }

            vector<vector<float>> centerCenterDistances(k, vector<float>(k, 0.0f));
            while (distanceChanged) {
                CalculateClusterDistancesEMD(centerCenterDistances, centers);
                double totalDistance = 0.0;

                #pragma omp parallel for reduction(+:totalDistance)
                for (int j = 0; j < nElements; j++) {
                    double distance = GetEarthMoverDistance(data, centers, j, bestCentersAssignment[j]);
                    int bestIndex = bestCentersAssignment[j];
                    for (int m = 0; m < k; m++) {
                        if (m != bestIndex && centerCenterDistances[bestIndex][m] < 2 * distance) {
                            double tempDistance = GetEarthMoverDistance(data, centers, j, m);
                            if (tempDistance < distance) {
                                distance = tempDistance;
                                bestIndex = m;
                            }
                        }
                    }
                    bestCentersAssignment[j] = bestIndex;
                    totalDistance += distance;
                }
                centers = CalculateNewCenters(data, bestCentersAssignment, k);
                totalDistance /= nElements;
                distanceChanged = !(fabs(totalDistance - lastDistance) < 1e-8);
                double diff = lastDistance - totalDistance;
                cout << "Current average distance: " << totalDistance << " Improvement: " << diff
                     << " (" << 100.0 * (1.0 - totalDistance / lastDistance) << "%)" << endl;
                if (totalDistance < recordDistance) {
                    recordDistance = totalDistance;
                    recordCenters = bestCentersAssignment;
                }
                lastDistance = totalDistance;
            }
        }
        cout << "Best distance found: " << recordDistance << endl;
        auto elapsed = chrono::duration_cast<chrono::seconds>(Clock::now() - startTime);
        cout << "K-means++ clustering (EMD) completed in " << elapsed.count() << " seconds." << endl;
        return recordCenters;
    }

    vector<int> ClusterL2(const vector<vector<float>>& data, int k, int nofRuns, const vector<int>* _bestCenters = nullptr) {
        int nElements = data.size();
        int dim = data[0].size();
        cout << "K-means++ clustering (L2) " << nElements << " elements into " << k 
             << " clusters with " << nofRuns << " runs..." << endl;
        random_device rd;
        mt19937 rng(rd());
        auto startTime = Clock::now();

        vector<int> bestCentersAssignment(nElements, 0);
        vector<int> recordCenters(nElements, 0);
        bool skipInit = false;
        if (_bestCenters != nullptr) {
            skipInit = true;
            bestCentersAssignment = *_bestCenters;
            recordCenters = *_bestCenters;
        }
        double recordDistance = numeric_limits<double>::max();

        for (int run = 0; run < nofRuns; ++run) {
            cout << "K-means++ starting clustering run " << run + 1 << "..." << endl;
            double lastDistance = numeric_limits<double>::max();
            bool distanceChanged = true;
            vector<vector<float>> centers(k, vector<float>(dim, 0.0f));

            if (!skipInit) {
                bestCentersAssignment.assign(nElements, 0);
                centers = FindStartingCentersL2(data, k, rng);
            } else {
                centers = CalculateNewCenters(data, bestCentersAssignment, k);
                skipInit = false;
            }

            vector<vector<float>> centerCenterDistances(k, vector<float>(k, 0.0f));
            auto runStartTime = Clock::now(); // Start time for the current run
            int iterationCount = 0; // Count iterations for ETA calculation

            while (distanceChanged) {
                CalculateClusterDistancesL2(centerCenterDistances, centers);
                double totalDistance = 0.0;

                #pragma omp parallel for reduction(+:totalDistance)
                for (int j = 0; j < nElements; j++) {
                    double distance = GetL2DistanceSquared(data, centers, j, bestCentersAssignment[j]);
                    int bestIndex = bestCentersAssignment[j];
                    for (int m = 0; m < k; m++) {
                        if (m != bestIndex && centerCenterDistances[bestIndex][m] < 2 * sqrt(distance)) {
                            double tempDistance = GetL2DistanceSquared(data, centers, j, m);
                            if (tempDistance < distance) {
                                distance = tempDistance;
                                bestIndex = m;
                            }
                        }
                    }
                    bestCentersAssignment[j] = bestIndex;
                    totalDistance += sqrt(distance);
                }
                centers = CalculateNewCenters(data, bestCentersAssignment, k);
                totalDistance /= nElements;
                distanceChanged = !(fabs(totalDistance - lastDistance) < 1e-8);
                double diff = lastDistance - totalDistance;

                // ETA Calculation
                iterationCount++;
                auto currentTime = Clock::now();
                double elapsedTime = chrono::duration_cast<chrono::seconds>(currentTime - runStartTime).count();
                double averageTimePerIteration = elapsedTime / iterationCount;
                double remainingIterations = (nofRuns - run - 1) * (distanceChanged ? 1 : 0); // Adjust based on distanceChanged
                double eta = averageTimePerIteration * remainingIterations;

                cout << "Current average distance: " << totalDistance << " Improvement: " << diff
                     << " (" << 100.0 * (1.0 - totalDistance / lastDistance) << "%)"
                     << " | ETA: " << eta << " seconds" << endl;

                if (totalDistance < recordDistance) {
                    recordDistance = totalDistance;
                    recordCenters = bestCentersAssignment;
                }
                lastDistance = totalDistance;
            }
        }
        cout << "Best distance found: " << recordDistance << endl;
        auto elapsed = chrono::duration_cast<chrono::seconds>(Clock::now() - startTime);
        cout << "K-means++ clustering (L2) completed in " << elapsed.count() << " seconds." << endl;
        return recordCenters;
    }

private:
    vector<vector<float>> CalculateNewCenters(const vector<vector<float>>& data, const vector<int>& bestCenters, int k) {
        int nElements = data.size();
        int dim = data[0].size();
        vector<vector<float>> centers(k, vector<float>(dim, 0.0f));
        vector<int> occurrences(k, 0);
        for (int j = 0; j < nElements; ++j) {
            int cluster = bestCenters[j];
            for (int m = 0; m < dim; ++m) {
                centers[cluster][m] += data[j][m];
            }
            occurrences[cluster]++;
        }
        for (int n = 0; n < k; ++n) {
            if (occurrences[n] != 0) {
                for (int m = 0; m < dim; ++m) {
                    centers[n][m] /= occurrences[n];
                }
            }
        }
        return centers;
    }

    void CalculateClusterDistancesL2(vector<vector<float>>& distances, const vector<vector<float>>& clusterCenters) {
        int k = clusterCenters.size();
        #pragma omp parallel for
        for (int j = 0; j < k; j++) {
            for (int m = 0; m < j; m++) {
                double d2 = GetL2DistanceSquared(clusterCenters, clusterCenters, j, m);
                distances[j][m] = (float) sqrt(d2);
            }
        }
        for (int j = 0; j < k; j++) {
            for (int m = 0; m < j; m++) {
                distances[m][j] = distances[j][m];
            }
        }
    }

    void CalculateClusterDistancesEMD(vector<vector<float>>& distances, const vector<vector<float>>& clusterCenters) {
        int k = clusterCenters.size();
        #pragma omp parallel for
        for (int j = 0; j < k; j++) {
            for (int m = 0; m < j; m++) {
                distances[j][m] = GetEarthMoverDistance(clusterCenters, clusterCenters, j, m);
            }
        }
        for (int j = 0; j < k; j++) {
            for (int m = 0; m < j; m++) {
                distances[m][j] = distances[j][m];
            }
        }
    }

    vector<vector<float>> GetUniqueRandomNumbers(const vector<vector<float>>& data, int nofSamples, mt19937 &rng) {
        int nElements = data.size();
        int dim = data[0].size();
        vector<vector<float>> tempData;
        tempData.reserve(nofSamples);
        unordered_set<int> chosen;
        uniform_int_distribution<int> dist(0, nElements - 1);
        while ((int)chosen.size() < nofSamples) {
            int index = dist(rng);
            if (chosen.find(index) == chosen.end()) {
                chosen.insert(index);
                tempData.push_back(data[index]);
            }
        }
        return tempData;
    }

    vector<vector<float>> FindStartingCentersL2(const vector<vector<float>>& data, int k, mt19937 &rng) {
        cout << "K-means++ finding good starting centers (L2)..." << endl;
        int nElements = data.size();
        int dim = data[0].size();
        int maxSamples = min(k * 100, nElements);
        vector<vector<float>> dataTemp = GetUniqueRandomNumbers(data, maxSamples, rng);
        int nTemp = dataTemp.size();
        vector<vector<float>> centers(k, vector<float>(dim, 0.0f));
        vector<int> centerIndices;

        for (int c = 0; c < k; c++) {
            vector<double> distancesToBest(nTemp, numeric_limits<double>::max());
            if (c == 0) {
                int index = uniform_int_distribution<int>(0, nTemp - 1)(rng);
                centerIndices.push_back(index);
                centers[c] = dataTemp[index];
            } else {
                #pragma omp parallel for
                for (int j = 0; j < nTemp; j++) {
                    for (int m = 0; m < c; m++) {
                        double d2 = GetL2DistanceSquared(dataTemp, centers, j, m);
                        if (d2 < distancesToBest[j]) distancesToBest[j] = d2;
                    }
                }
                double sum = accumulate(distancesToBest.begin(), distancesToBest.end(), 0.0);
                for (auto &val : distancesToBest) val /= sum;
                int centerIndexSample = sampleDistribution(distancesToBest, rng);
                while (find(centerIndices.begin(), centerIndices.end(), centerIndexSample) != centerIndices.end()) {
                    centerIndexSample = sampleDistribution(distancesToBest, rng);
                }
                centers[c] = dataTemp[centerIndexSample];
                centerIndices.push_back(centerIndexSample);
            }
        }
        return centers;
    }

    vector<vector<float>> FindStartingCentersEMD(const vector<vector<float>>& data, int k, mt19937 &rng) {
        cout << "K-means++ finding good starting centers (EMD)..." << endl;
        int nElements = data.size();
        int dim = data[0].size();
        int maxSamples = min(k * 20, nElements);
        vector<vector<float>> dataTemp = GetUniqueRandomNumbers(data, maxSamples, rng);
        int nTemp = dataTemp.size();
        vector<vector<float>> centers(k, vector<float>(dim, 0.0f));
        vector<int> centerIndices;

        for (int c = 0; c < k; c++) {
            vector<float> distancesToBest(nTemp, numeric_limits<float>::max());
            if (c == 0) {
                int index = uniform_int_distribution<int>(0, nTemp - 1)(rng);
                centerIndices.push_back(index);
                centers[c] = dataTemp[index];
            } else {
                #pragma omp parallel for
                for (int j = 0; j < nTemp; j++) {
                    for (int m = 0; m < c; m++) {
                        float d = GetEarthMoverDistance(dataTemp, centers, j, m);
                        if (d < distancesToBest[j]) distancesToBest[j] = d;
                    }
                }
                for (auto &val : distancesToBest) val = val * val;
                float sum = accumulate(distancesToBest.begin(), distancesToBest.end(), 0.0f);
                vector<double> probabilities(nTemp);
                for (int p = 0; p < nTemp; ++p) probabilities[p] = distancesToBest[p] / sum;
                int centerIndexSample = sampleDistribution(probabilities, rng);
                while (find(centerIndices.begin(), centerIndices.end(), centerIndexSample) != centerIndices.end()) {
                    centerIndexSample = sampleDistribution(probabilities, rng);
                }
                centers[c] = dataTemp[centerIndexSample];
                centerIndices.push_back(centerIndexSample);
            }
        }
        return centers;
    }

    float GetEarthMoverDistance(const vector<vector<float>>& data, const vector<vector<float>>& centers, int index1, int index2) {
        float emd = 0;
        float totalDistance = 0;
        int dim = data[0].size();
        for (int i = 0; i < dim; i++) {
            emd = (data[index1][i] + emd) - centers[index2][i];
            totalDistance += fabs(emd);
        }
        return totalDistance;
    }

    double GetL2DistanceSquared(const vector<vector<float>>& data, const vector<vector<float>>& centers, int index1, int index2) {
        double totalDistance = 0.0;
        int dim = data[0].size();
        int i = 0;
        for (; i <= dim - 4; i += 4) {
            double diff0 = data[index1][i] - centers[index2][i];
            double diff1 = data[index1][i+1] - centers[index2][i+1];
            double diff2 = data[index1][i+2] - centers[index2][i+2];
            double diff3 = data[index1][i+3] - centers[index2][i+3];
            totalDistance += diff0 * diff0 + diff1 * diff1 + diff2 * diff2 + diff3 * diff3;
        }
        for (; i < dim; i++) {
            double diff = data[index1][i] - centers[index2][i];
            totalDistance += diff * diff;
        }
        return totalDistance;
    }
};

// Add this function declaration before main()
double calculateSilhouetteScore(const vector<vector<float>>& data, const vector<int>& clusters, int k);

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <round>" << endl;
        cerr << "  round: flop, turn, or river" << endl;
        return 1;
    }
    
    string round = argv[1];
    if (round != "flop" && round != "turn" && round != "river") {
        cerr << "Error: round must be 'flop', 'turn', or 'river'" << endl;
        return 1;
    }
    
    // Construct CSV filename based on round
    string csvFilename = round + "_equities.csv";
    
    // Read CSV file
    ifstream file(csvFilename);
    if (!file.is_open()) {
        cerr << "Error: Could not open " << csvFilename << endl;
        return 1;
    }

    // Parse header
    string header;
    getline(file, header);

    // Read data
    vector<string> hands;
    vector<vector<float>> equityData;
    string line;
    
    while (getline(file, line)) {
        stringstream ss(line);
        string hand, equity2p, equity3p;
        
        // Parse CSV line (Hand,Equity2P,Equity3P)
        getline(ss, hand, ',');
        getline(ss, equity2p, ',');
        getline(ss, equity3p, ',');
        
        try {
            // Convert equity values to float
            float eq2p = stof(equity2p);
            float eq3p = stof(equity3p);
            
            hands.push_back(hand);
            equityData.push_back({eq2p, eq3p});
        } catch (const std::invalid_argument& e) {
            cerr << "Error converting to float: " << line << endl;
            continue;
        } catch (const std::out_of_range& e) {
            cerr << "Out of range error: " << line << endl;
            continue;
        }
    }
    
    if (equityData.empty()) {
        cerr << "No valid data was loaded from the CSV file." << endl;
        return 1;
    }
    
    int nPoints = equityData.size();
    cout << "Loaded " << nPoints << " hands from " << csvFilename << endl;
    
    // Parameters for clustering
    int k = 1000;  // Number of clusters
    int nofRuns = 3;  // Number of runs
    
    Kmeans kmeans;
    
    // Run both L2 and EMD clustering
    cout << "Running L2 clustering..." << endl;
    vector<int> l2Assignments = kmeans.ClusterL2(equityData, k, nofRuns);
    
    cout << "Running EMD clustering..." << endl;
    vector<int> emdAssignments = kmeans.ClusterEMD(equityData, k, nofRuns);
    
    // Save results to CSV for L2 clustering
    string l2OutFilename = "l2_" + round + "_clustered_hands.csv";
    ofstream l2OutFile(l2OutFilename);
    l2OutFile << "Hand,Equity2P,Equity3P,Cluster" << endl;
    for (int i = 0; i < nPoints; i++) {
        l2OutFile << hands[i] << "," << equityData[i][0] << "," 
                  << equityData[i][1] << "," << l2Assignments[i] << endl;
    }
    l2OutFile.close();
    
    cout << "L2 results saved to " << l2OutFilename << endl;

    // Save results to CSV for EMD clustering
    string emdOutFilename = "emd_" + round + "_clustered_hands.csv";
    ofstream emdOutFile(emdOutFilename);
    emdOutFile << "Hand,Equity2P,Equity3P,Cluster" << endl;
    for (int i = 0; i < nPoints; i++) {
        emdOutFile << hands[i] << "," << equityData[i][0] << "," 
                   << equityData[i][1] << "," << emdAssignments[i] << endl;
    }
    emdOutFile.close();
    
    cout << "EMD results saved to " << emdOutFilename << endl;
    
    return 0;
}

// Helper function to calculate silhouette score
double calculateSilhouetteScore(const vector<vector<float>>& data, const vector<int>& clusters, int k) {
    int n = data.size();
    double totalScore = 0.0;
    
    for (int i = 0; i < n; i++) {
        int clusterI = clusters[i];
        
        // Calculate average distance to points in same cluster (a)
        double a = 0.0;
        int sameClusterCount = 0;
        for (int j = 0; j < n; j++) {
            if (i != j && clusters[j] == clusterI) {
                double dist = 0.0;
                for (size_t d = 0; d < data[i].size(); d++) {
                    double diff = data[i][d] - data[j][d];
                    dist += diff * diff;
                }
                a += sqrt(dist);
                sameClusterCount++;
            }
        }
        if (sameClusterCount > 0) {
            a /= sameClusterCount;
        }
        
        // Calculate minimum average distance to points in other clusters (b)
        double b = numeric_limits<double>::max();
        for (int c = 0; c < k; c++) {
            if (c != clusterI) {
                double avgDist = 0.0;
                int otherClusterCount = 0;
                for (int j = 0; j < n; j++) {
                    if (clusters[j] == c) {
                        double dist = 0.0;
                        for (size_t d = 0; d < data[i].size(); d++) {
                            double diff = data[i][d] - data[j][d];
                            dist += diff * diff;
                        }
                        avgDist += sqrt(dist);
                        otherClusterCount++;
                    }
                }
                if (otherClusterCount > 0) {
                    avgDist /= otherClusterCount;
                    b = min(b, avgDist);
                }
            }
        }
        
        // Calculate silhouette score for this point
        double s;
        if (sameClusterCount <= 1) {
            s = 0; // Single element cluster
        } else if (a < b) {
            s = 1 - a / b;
        } else if (a > b) {
            s = b / a - 1;
        } else {
            s = 0;
        }
        
        totalScore += s;
    }
    
    return totalScore / n;
}
