#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <set>
#include <stdexcept>
#include <numeric>
#include <iterator>
#include <unordered_map>
#include <map>
#include <cassert>
#include <chrono>
#include <utility>
#include <tuple>
#include <vector>
#include <fstream>

using namespace std;
// ----------------------------------------------------------------------------
// PokerEvaluator
// ----------------------------------------------------------------------------
struct Card {
    std::string rank;
    std::string suit;

    // Overload the equality operator
    bool operator==(const Card& other) const {
        return rank == other.rank && suit == other.suit;
    }

    std::string toString() const {
        return rank + suit;
    }
};

const vector<string> RANKS = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
const vector<char> SUITS = {'h', 'd', 'c', 's'};

class PokerEvaluator {
    public:
        // Rank-to-value mapping
        static const unordered_map<string, int> RANK_VALUES;
    
        // Standard poker hand rankings (higher is better)
        enum HandRank {
            HIGH_CARD = 0,
            PAIR = 1,
            TWO_PAIR = 2,
            THREE_KIND = 3,
            STRAIGHT = 4,
            FLUSH = 5,
            FULL_HOUSE = 6,
            FOUR_KIND = 7,
            STRAIGHT_FLUSH = 8
        };
    
        // Evaluate the best hand given hole cards and community cards.
        int evaluateHand(const vector<Card>& holeCards, const vector<Card>& communityCards) {
            vector<Card> allCards = holeCards;
            allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
            
            vector<vector<Card>> combinations;
            vector<Card> currentCombo;
            generateCombinations(allCards, 5, 0, currentCombo, combinations);
            
            int bestScore = 0;
            for (const auto &combo : combinations) {
                int score = evaluateFiveCardHand(combo);
                if (score > bestScore) {
                    bestScore = score;
                }
            }
            return bestScore;
        }
    
        int evaluateFiveCardHand(const vector<Card>& hand) {
            vector<int> cardValues;
            for (const auto &c : hand)
                cardValues.push_back(RANK_VALUES.at(c.rank));
            sort(cardValues.rbegin(), cardValues.rend());
    
            map<int, int, greater<int>> freq;
            for (int v : cardValues)
                freq[v]++;
    
            bool isFlush = checkFlush(hand);
            int straightHigh = 0;
            bool isStraight = checkStraight(hand, straightHigh);
    
            if (isFlush && isStraight) return makeScore(HandRank::STRAIGHT_FLUSH, {straightHigh});
            
            for (const auto &entry : freq) {
                if (entry.second == 4) {
                    // Find the kicker as the highest card not part of the quads
                    vector<int> kickers;
                    for (int v : cardValues) {
                        if (v != entry.first) {
                            kickers.push_back(v);
                        }
                    }
                    sort(kickers.rbegin(), kickers.rend()); // Sort kickers to get the strongest ones
                    return makeScore(HandRank::FOUR_KIND, {entry.first, kickers[0]}); // Use the strongest kicker
                }
            }
            
            int tripleRank = 0, pairRank = 0;
            for (const auto &entry : freq) {
                if (entry.second >= 3 && tripleRank == 0)
                    tripleRank = entry.first;
            }
            if (tripleRank) {
                for (const auto &entry : freq) {
                    if (entry.first != tripleRank && entry.second >= 2) {
                        pairRank = entry.first;
                        break;
                    }
                }
                if (pairRank)
                    return makeScore(HandRank::FULL_HOUSE, {tripleRank, pairRank});
            }
            
            if (isFlush) return makeScore(HandRank::FLUSH, cardValues);
            if (isStraight) return makeScore(HandRank::STRAIGHT, {straightHigh});
            
            for (const auto &entry : freq) {
                if (entry.second == 3) {
                    vector<int> kickers;
                    for (int v : cardValues) {
                        if (v != entry.first) kickers.push_back(v);
                    }
                    sort(kickers.rbegin(), kickers.rend()); // Sort kickers to get the strongest ones
                    return makeScore(HandRank::THREE_KIND, {entry.first, kickers[0], kickers[1]}); // Use the strongest kickers
                }
            }
            
            vector<int> pairs;
            int kicker = 0;
            for (const auto &entry : freq) {
                if (entry.second >= 2)
                    pairs.push_back(entry.first);
            }
            if (pairs.size() >= 2) {
                sort(pairs.begin(), pairs.end(), greater<int>());
                for (int v : cardValues) {
                    if (v != pairs[0] && v != pairs[1]) { kicker = v; break; }
                }
                return makeScore(HandRank::TWO_PAIR, {pairs[0], pairs[1], kicker});
            }
            
            for (const auto &entry : freq) {
                if (entry.second == 2) {
                    vector<int> kickers;
                    for (int v : cardValues) {
                        if (v != entry.first) kickers.push_back(v);
                    }
                    sort(kickers.rbegin(), kickers.rend()); // Sort kickers to get the strongest ones
                    return makeScore(HandRank::PAIR, {entry.first, kickers[0], kickers[1], kickers[2]}); // Use the strongest kickers
                }
            }
            
            return makeScore(HandRank::HIGH_CARD, cardValues);
        }
    
        int makeScore(int handRank, const vector<int>& vals) {
            int score = handRank * 1000000; // Base score for hand rank, scaled up
            for (int v : vals) {
                score += v; // Add the values of the cards
            }
            return score;
        }
    
        void generateCombinations(const vector<Card>& cards, int combinationSize, int start,
                                  vector<Card>& currentCombo, vector<vector<Card>>& allCombinations) {
            if (currentCombo.size() == combinationSize) {
                allCombinations.push_back(currentCombo);
                return;
            }
            for (int i = start; i <= (int)cards.size() - (combinationSize - currentCombo.size()); ++i) {
                currentCombo.push_back(cards[i]);
                generateCombinations(cards, combinationSize, i + 1, currentCombo, allCombinations);
                currentCombo.pop_back();
            }
        }
    
        bool checkFlush(const vector<Card>& hand) {
            string suit = hand.front().suit;
            return std::all_of(hand.begin(), hand.end(), [&suit](const Card &c) { return c.suit == suit; });
        }
    
        bool checkStraight(const vector<Card>& hand, int &highStraightValue) {
            std::vector<int> values;
            for (const auto &card : hand)
                values.push_back(RANK_VALUES.at(card.rank));
            std::sort(values.begin(), values.end());
            
            bool normalStraight = (values[4] - values[0] == 4);
            if (normalStraight) {
                for (int i = 0; i < 4; i++) {
                    if (values[i+1] - values[i] != 1) return false;
                }
                highStraightValue = values[4];
                return true;
            }
            
            if (values == vector<int>{2, 3, 4, 5, 14}) {
                highStraightValue = 5;
                return true;
            }
            return false;
        }
    };

// Define the static member outside the class
const unordered_map<string, int> PokerEvaluator::RANK_VALUES = {
    {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5},
    {"6", 6}, {"7", 7}, {"8", 8}, {"9", 9},
    {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
};


class PokerEquityCalculator {
public:
    PokerEquityCalculator() : rng(random_device{}()) {}
    
    double calculateEquity(vector<Card> holeCards, vector<Card> opponentHoleCards, vector<Card> communityCards, int trials = 100000) {
        int wins = 0, ties = 0;
        vector<Card> deck = generateDeck(holeCards, opponentHoleCards, communityCards);
        
        for (int i = 0; i < trials; ++i) {
            vector<Card> fullBoard = communityCards;
            shuffleDeck(deck);
            while (fullBoard.size() < 5) fullBoard.push_back(deck[fullBoard.size()]);
            
            int myScore = evaluator.evaluateHand(holeCards, fullBoard);
            int opponentScore = evaluator.evaluateHand(opponentHoleCards, fullBoard);
            
            if (myScore > opponentScore) ++wins;
            else if (myScore == opponentScore) ++ties;
        }
        return (wins + ties / 2.0) / trials;
    }

private:
    PokerEvaluator evaluator;
    mt19937 rng;
    
    vector<Card> generateDeck(const vector<Card>& hole, const vector<Card>& opponent, const vector<Card>& community) {
        vector<Card> deck;
        for (const auto& rank : RANKS) {
            for (char suit : SUITS) {
                Card card = {rank, string(1, suit)};
                if (find(hole.begin(), hole.end(), card) == hole.end() &&
                    find(opponent.begin(), opponent.end(), card) == opponent.end() &&
                    find(community.begin(), community.end(), card) == community.end()) {
                    deck.push_back(card);
                }
            }
        }
        return deck;
    }

    void shuffleDeck(vector<Card>& deck) {
        shuffle(deck.begin(), deck.end(), rng);
    }
};