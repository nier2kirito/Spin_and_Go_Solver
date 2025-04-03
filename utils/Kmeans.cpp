/*
To compile with OpenMP (for example using g++):
g++ -fopenmp -O2 -o kmeans kmeans.cpp
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

// int main() {
//     int nPoints = 100000;
//     int dim = 30;
//     int k = 5;
//     int nofRuns = 3;
//     vector<vector<float>> data(nPoints, vector<float>(dim, 0.0f));

//     random_device rd;
//     mt19937 rng(rd());
//     uniform_real_distribution<float> uni(0.0f, 1.0f);
//     for (int i = 0; i < nPoints; i++) {
//         for (int j = 0; j < dim; j++) {
//             data[i][j] = uni(rng);
//         }
//     }

//     Kmeans kmeans;
//     vector<int> clusterAssignments = kmeans.ClusterEMD(data, k, nofRuns);

//     cout << "Cluster assignments for first 10 points:" << endl;
//     for (int i = 0; i < 10; i++) {
//         cout << clusterAssignments[i] << " ";
//     }
//     cout << endl;
//     return 0;
// }
