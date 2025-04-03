#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>

struct Card {
    std::string rank; // e.g., "A", "2", ..., "K"
    std::string suit; // e.g., "h", "d", "c", "s"
};

// Function to encode cards into feature vectors
std::vector<int> encodeCards(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) {
    // Combine hole and community cards
    std::vector<Card> allCards = holeCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());

    // Rank and suit counts
    std::unordered_map<std::string, int> rankCount;
    std::unordered_map<std::string, int> suitCount;
    std::array<int, 13> rankVector = {0}; // 2 to Ace
    std::array<int, 4> suitVector = {0}; // h, d, c, s

    // Rank mapping
    std::unordered_map<std::string, int> rankMapping = {
        {"2", 0}, {"3", 1}, {"4", 2}, {"5", 3}, {"6", 4},
        {"7", 5}, {"8", 6}, {"9", 7}, {"T", 8}, {"J", 9},
        {"Q", 10}, {"K", 11}, {"A", 12}
    };

    // Suit mapping
    std::unordered_map<std::string, int> suitMapping = {
        {"h", 0}, {"d", 1}, {"c", 2}, {"s", 3}
    };

    // Count ranks and suits
    for (const auto& card : allCards) {
        rankCount[card.rank]++;
        suitCount[card.suit]++;
    }

    // Fill rank and suit vectors
    for (const auto& [rank, count] : rankCount) {
        if (rankMapping.find(rank) != rankMapping.end()) {
            rankVector[rankMapping[rank]] = count;
        }
    }
    for (const auto& [suit, count] : suitCount) {
        if (suitMapping.find(suit) != suitMapping.end()) {
            suitVector[suitMapping[suit]] = count;
        }
    }

    // Create the feature vector
    std::vector<int> featureVector;

    // Add rank counts
    featureVector.insert(featureVector.end(), rankVector.begin(), rankVector.end());

    // Add suit counts
    featureVector.insert(featureVector.end(), suitVector.begin(), suitVector.end());

    // Add hand type indicators (example: has pair, three-of-a-kind, etc.)
    bool hasPair = false, hasThreeOfAKind = false, hasFourOfAKind = false;
    for (const auto& count : rankVector) {
        if (count == 2) hasPair = true;
        if (count == 3) hasThreeOfAKind = true;
        if (count == 4) hasFourOfAKind = true;
    }
    featureVector.push_back(hasPair ? 1 : 0);
    featureVector.push_back(hasThreeOfAKind ? 1 : 0);
    featureVector.push_back(hasFourOfAKind ? 1 : 0);

    // Add Ace count
    featureVector.push_back(rankVector[rankMapping["A"]]);

    // Add additional features as needed...

    return featureVector;
}

int main() {
    std::vector<Card> holeCards = {{"A", "h"}, {"A", "d"}};
    std::vector<Card> communityCards = {{"A", "c"}, {"A", "s"}, {"3", "c"}, {"3", "h"}, {"2", "d"}};

    std::vector<int> featureVector = encodeCards(holeCards, communityCards);

    // Output the feature vector
    std::cout << "Feature Vector: ";
    for (int feature : featureVector) {
        std::cout << feature << " ";
    }
    std::cout << std::endl;
    holeCards = {{"A", "h"}, {"A", "d"}};
    communityCards = {{"A", "c"}, {"A", "s"}, {"3", "c"}, {"3", "h"}};

    featureVector = encodeCards(holeCards, communityCards);

    // Output the feature vector
    std::cout << "Feature Vector: ";
    for (int feature : featureVector) {
        std::cout << feature << " ";
    }
    std::cout << std::endl;
    return 0;
}
