/*
KickOff Poker in C++
This implementation follows the logic of the Python demo:
– Three players with an initial stack.
– A deck of cards is built from ranks and suits.
– Game rounds (“preflop”, “flop”, “turn”, “river”, “showdown”).
– Legal actions include folding, posting blinds, calling, betting, raising,
  checking, all‐in and dealing (a chance node).
– A simple PokerEvaluator that scores a hand by taking the best
  (maximum) sum of “card values” from any combination of 5 cards out of 7.

Compile with, e.g.:
g++ -std=c++17 -O2 kickoff_poker.cpp -o kickoff_poker
*/

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
#include <functional>
#include <iomanip>  // for setprecision
#include <unordered_set>

using namespace std;

// Constants definitions
constexpr int NUM_PLAYERS = 3;
constexpr float INITIAL_STACK = 15.0;
static const float RAKE_PERCENTAGE = 0.07;  // 7% rake

// ----------------------------------------------------------------------------
// Card, Deck, and Helper Functions
// ----------------------------------------------------------------------------

struct Card {
    std::string rank;
    std::string suit;

    std::string toString() const {
        return rank + suit;
    }
};

const vector<string> RANKS = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
const vector<char> SUITS = {'h', 'd', 'c', 's'};
const unordered_set<string> FOLD_BTN = {
"3Ao", "2Ao",
"8Ko", "7Ko", "6Ko", "5Ko", "4Ko", "3Ko", "2Ko",
"8Qo", "7Qo", "6Qo", "5Qo", "4Qo", "3Qo", "2Qo",
"8Jo", "7Jo", "6Jo", "5Jo", "4Jo", "3Jo", "2Jo",
"810o", "710o", "710o", "610o", "510o", "410o", "310o", "210o",
"89o", "79o", "69o", "59o", "49o", "39o", "29o",
"78o", "68o", "58o", "48o", "38o", "28o",
"67o", "57o", "47o", "37o", "27o",
"56o", "46o", "36o", "26o",
"45o", "35o", "25o",
"34o", "24o",
"23o",
"3Ks", "2Ks",
"5Qs", "4Qs", "3Qs", "2Qs",
"6Js", "5Js", "4Js", "3Js", "2Js",
"610s", "510s", "410s", "310s", "210s",
"69s", "59s", "49s", "39s", "29s",
"58s", "48s", "38s", "28s",
"57s", "47s", "37s", "27s",
"56s", "46s", "36s", "26s",
"45s", "35s", "25s",
"34s", "24s",
"23s"
};

const unordered_set<string> FOLD_SB_BTN_FOLD = {"25o", "29o", "57o", "45o", "39o", "46o", "35o", "410o", "6Jo", "3Qo", "2Jo", "23o", "59o", "610o", "58o", "26s", "47o", "510o", "49o", "28s", "27s", "69o", "37o", "38s", "36o", "210o", "34o", "24o", "5Jo", "48o", "27o", "28o", "29s", "38o", "310o", "2Qo", "26o", "24s", "37s", "4Jo", "3Jo", "23s", "4Qo"};
const unordered_set<string> FOLD_SB_BTN_BET_2 = {"59o", "3Ks", "2Js", "35s", "68s", "69o", "3Js", "5Qs", "45o", "68o", "25s", "6Ks", "7Ks", "29o", "5Js", "58s", "2Qo", "37s", "3Jo", "210s", "57o", "5Qo", "8Ko", "8Js", "34s", "67o", "37o", "59s", "5Ko", "49o", "36o", "3Ao", "610s", "67s", "8Jo", "10Qo", "8Qo", "4Ko", "78o", "9Ko", "2Qs", "7Ko", "48s", "910o", "4Ks", "38s", "26s", "57s", "28s", "6Jo", "23s", "24o", "510o", "28o", "58o", "2Ks", "410s", "4Ao", "6Qo", "410o", "39s", "4Jo", "23o", "39o", "34o", "79s", "25o", "7Qo", "310o", "9Jo", "5Jo", "7Qs", "27o", "49s", "35o", "9Qo", "8Qs", "210o", "2Ao", "2Jo", "710o", "69s", "4Qo", "45s", "810o", "46o", "7Js", "610o", "6Qs", "27s", "79o", "36s", "6Ko", "38o", "710s", "48o", "47s", "3Ko", "7Jo", "3Qs", "24s", "510s", "2Ko", "56o", "3Qo", "6Js", "29s", "47o", "4Js", "310s", "46s", "26o", "56s", "4Qs", "5Ks", "89o"};
const unordered_set<string> FOLD_SB_BTN_ALL_IN = {"10Qs", "810s", "8Ko", "5Ao", "6Qs", "23s", "6Qo", "210o", "810o", "37o", "47o", "4As", "9Qo", "9Qs", "10Ko", "4Ks", "8Ks", "37s", "58s", "2Jo", "27o", "78o", "5Ko", "610o", "78s", "47s", "26o", "5As", "38o", "2Qs", "3Qs", "39s", "610s", "3Ks", "79o", "410o", "29s", "8Qs", "5Qo", "25o", "3Js", "10Ks", "JQo", "22s", "4Ko", "35s", "25s", "510o", "5Qs", "89o", "3As", "8Js", "2Ao", "6Ks", "3Ko", "59o", "59s", "27s", "2Ko", "6As", "36o", "2As", "9Jo", "6Jo", "JKo", "26s", "56s", "2Ks", "5Ks", "5Jo", "10Js", "4Qs", "4Js", "910o", "7Ko", "68s", "710s", "68o", "39o", "7Ao", "35o", "46s", "9Ko", "29o", "23o", "57o", "58o", "9Ks", "57s", "6Ao", "10Jo", "49s", "410s", "69s", "45o", "45s", "5Js", "4Ao", "56o", "510s", "89s", "7Ks", "710o", "69o", "4Jo", "48s", "9Js", "10Qo", "28o", "3Ao", "48o", "46o", "34s", "28s", "67s", "4Qo", "2Qo", "79s", "6Js", "7Qs", "24o", "3Jo", "7Js", "67o", "38s", "24s", "36s", "7Jo", "8Jo", "2Js", "310o", "8Qo", "3Qo", "49o", "7Qo", "310s", "910s", "34o", "210s", "6Ko"};
// const unordered_set<string> FOLD_BB_BTN_FOLD_SB_CALL = ; {no fold range}
const unordered_set<string> FOLD_BB_BTN_FOLD_SB_BET_3 = {"29o", "36o", "49o", "26o", "38s", "310o", "38o", "5Qo", "25o", "510o", "3Qo", "610o", "5Jo", "28o", "210o", "3Ko", "58o", "23o", "28s", "410o", "39o", "4Jo", "59o", "27s", "2Ko", "6Jo", "4Qo", "2Jo", "2Qo", "48o", "35o", "24o", "57o", "3Jo", "4Ko", "37o", "34o", "46o", "710o", "68o", "47o", "69o", "27o"};
const unordered_set<string> FOLD_BB_BTN_FOLD_SB_ALL_IN = {"58o", "810s", "27o", "5Ks", "4Ks", "48o", "4Qs", "4Js", "9Jo", "69s", "6Qo", "6Jo", "48s", "510s", "78o", "9Qo", "29s", "210o", "8Qs", "38s", "23o", "4Qo", "45o", "210s", "26o", "79s", "46o", "29o", "36o", "89s", "49o", "6Ko", "37s", "49s", "69o", "68s", "4Jo", "910o", "7Qs", "9Ko", "710o", "3Qs", "310o", "510o", "78s", "28s", "34o", "24s", "610s", "56o", "8Qo", "5Jo", "3Qo", "57o", "8Ko", "7Qo", "67o", "5Ko", "59o", "2Qo", "410o", "28o", "35s", "25s", "10Qo", "35o", "36s", "45s", "810o", "24o", "47o", "610o", "5Qo", "26s", "6Js", "310s", "2Ao", "2Qs", "2Ko", "8Jo", "27s", "7Ko", "3Jo", "5Qs", "38o", "3Ko", "2Jo", "37o", "710s", "39s", "58s", "47s", "79o", "34s", "7Jo", "5Js", "6Ks", "7Js", "410s", "59s", "46s", "8Js", "7Ks", "2Ks", "3Js", "25o", "39o", "67s", "3Ao", "57s", "3Ks", "23s", "89o", "4Ko", "2Js", "56s", "68o", "6Qs"};
const unordered_set<string> FOLD_BB_BTN_BET_2_SB_FOLD = {"5Jo", "25o", "4Jo", "28o", "48o", "47o", "26o", "59o", "2Qo", "35o", "39o", "57o", "37o", "23o", "6Jo", "410o", "38o", "36o", "310o", "58o", "27o", "510o", "2Jo", "24o", "3Jo", "610o", "49o", "210o", "29o"};
const unordered_set<string> FOLD_BB_BTN_BET_2_SB_CALL = {"47o", "6Qo", "4Jo", "5Jo", "23o", "68o", "510o", "37o", "8Jo", "24o", "2Ko", "34o", "39o", "310o", "8Ko", "36o", "29o", "79o", "410o", "710o", "3Jo", "49o", "26o", "25o", "78o", "7Ko", "7Qo", "6Jo", "38o", "8Qo", "5Ko", "59o", "3Qo", "2Qo", "610o", "58o", "2Jo", "4Qo", "5Qo", "7Jo", "3Ko", "69o", "28o", "48o", "4Ko", "6Ko", "210o", "27o"};
const unordered_set<string> FOLD_BB_BTN_BET_2_SB_BET_4 = {"4Js", "2Qo", "6Qs", "29s", "28s", "10Ko", "9Ks", "69o", "69s", "4Ks", "44o", "49o", "3Js", "5Ks", "2Ko", "29o", "8Js", "49s", "5Qo", "27o", "7Js", "45s", "410s", "5Ao", "6Ko", "810o", "7Qs", "710s", "8Qo", "36o", "8Qs", "59o", "25o", "JKo", "4Ao", "47s", "4Ko", "5Ko", "8Jo", "10Qo", "56s", "7Qo", "9Qo", "5Jo", "910s", "78o", "48s", "37s", "57s", "7Ko", "2Qs", "34o", "7Ks", "48o", "39o", "35o", "34s", "3Ko", "22o", "8Ao", "68o", "2Js", "37o", "8Ks", "6Ks", "89s", "2Jo", "89o", "67s", "8Ko", "7Jo", "2Ao", "9Ao", "47o", "23o", "26s", "7Ao", "510s", "10Qs", "3Qs", "610s", "55o", "39s", "4Jo", "57o", "78s", "210s", "56o", "46s", "410o", "38o", "310s", "3Jo", "25s", "6Ao", "4Qs", "46o", "9Js", "35s", "36s", "6Qo", "10Jo", "24o", "79o", "59s", "6Jo", "45o", "2As", "3Ks", "5Js", "JQo", "610o", "2Ks", "810s", "24s", "58s", "3Ao", "510o", "9Jo", "6Js", "4Qo", "68s", "710o", "27s", "3Qo", "67o", "910o", "33o", "5Qs", "58o", "9Qs", "9Ko", "28o", "210o", "79s", "26o", "38s", "310o", "23s"};
const unordered_set<string> FOLD_BB_BTN_BET_2_SB_ALL_IN = {"4As", "9Jo", "210o", "47o", "QKo", "610o", "7As", "35s", "36o", "JKo", "57s", "510o", "26o", "27o", "45o", "6Qo", "8Ks", "8Js", "JQo", "410o", "45s", "37o", "10Jo", "6Ao", "6Jo", "89o", "710o", "7Js", "6Ko", "6Ks", "5Qs", "2Qs", "5As", "7Ks", "78s", "48s", "39o", "710s", "5Ko", "29o", "49s", "8Qo", "4Qs", "3As", "33o", "2Qo", "810s", "46s", "24o", "7Qs", "36s", "310s", "2Js", "5Ao", "5Qo", "89s", "25s", "79s", "38o", "410s", "68o", "27s", "59o", "59s", "57o", "9Qs", "2Ao", "69s", "6Js", "9Qo", "68s", "9Ao", "4Jo", "8Ko", "23o", "10Qo", "310o", "510s", "9Js", "37s", "910o", "10Ko", "79o", "58o", "210s", "9Ks", "58s", "67o", "3Ao", "2Ko", "4Ao", "6As", "78o", "4Js", "5Js", "56o", "3Qo", "8Qs", "910s", "25o", "28o", "39s", "46o", "3Qs", "610s", "26s", "10Qs", "8Jo", "2Jo", "7Qo", "4Ks", "6Qs", "8Ao", "4Ko", "22o", "24s", "7Ko", "2Ks", "3Js", "4Qo", "8As", "49o", "28s", "7Ao", "3Ks", "9Ko", "47s", "55o", "35o", "69o", "3Ko", "29s", "2As", "23s", "5Jo", "67s", "48o", "10Ks", "34o", "38s", "34s", "56s", "3Jo", "7Jo", "5Ks", "810o"};
const unordered_set<string> FOLD_BB_BTN_ALL_IN_SB_FOLD = {"49s", "29o", "7Js", "8Qo", "10Qo", "3Ao", "2Ks", "2Jo", "35o", "910o", "7Qo", "7Qs", "10Ko", "6Jo", "27o", "8Qs", "610s", "6Ks", "56s", "3Js", "67s", "8Ks", "2Qs", "9Ko", "210s", "710s", "9Qs", "6Ao", "34o", "3Qo", "4Jo", "6Qo", "48s", "3Ko", "89o", "23o", "38s", "4Ao", "39o", "22o", "9Jo", "56o", "25o", "4Js", "78s", "610o", "89s", "58o", "2Js", "2As", "310o", "79s", "37s", "7Ko", "5Ks", "3Qs", "510s", "210o", "4Ko", "23s", "710o", "510o", "69o", "2Qo", "8Jo", "48o", "10Jo", "45s", "67o", "9Js", "49o", "36o", "79o", "45o", "57o", "24s", "46o", "5Qs", "2Ao", "9Qo", "36s", "2Ko", "5Jo", "38o", "8Js", "69s", "3Ks", "5Qo", "6Js", "7Ks", "810o", "29s", "6Ko", "410s", "5Ko", "8Ko", "59o", "27s", "78o", "47o", "4Ks", "68o", "58s", "25s", "3Jo", "7Jo", "47s", "68s", "35s", "55o", "26o", "810s", "34s", "410o", "9Ks", "24o", "26s", "310s", "37o", "57s", "39s", "4Qs", "59s", "6Qs", "5Ao", "28s", "5Js", "3As", "46s", "4Qo", "910s", "28o"};
const unordered_set<string> FOLD_BB_BTN_ALL_IN_SB_ALL_IN = {"310o", "3Ko", "5Jo", "8Ko", "410s", "68o", "9Qo", "510s", "610o", "5As", "9As", "69s", "6Qo", "69o", "5Js", "310s", "810s", "7Ao", "2Qo", "4Qo", "5Qs", "5Ko", "6Ao", "4Jo", "28o", "5Qo", "6Js", "58o", "46s", "36o", "9Ks", "39s", "10Ao", "8Qo", "2Ko", "24s", "9Ko", "810o", "6As", "JQo", "5Ao", "4Ko", "10Jo", "410o", "56s", "8Ks", "610s", "59s", "6Ko", "56o", "26s", "45o", "9Js", "2Js", "JKo", "25o", "8As", "3Js", "78s", "37s", "24o", "89o", "29s", "47o", "6Jo", "710o", "67o", "23o", "4As", "89s", "22o", "3As", "510o", "7Qs", "2As", "8Js", "7Ks", "10Qo", "3Jo", "46o", "33o", "49s", "2Ao", "47s", "10Ko", "39o", "36s", "38o", "58s", "6Ks", "2Ks", "68s", "8Jo", "5Ks", "9Jo", "7Qo", "38s", "48s", "3Ao", "4Qs", "37o", "7Ko", "910o", "210o", "9Ao", "2Jo", "4Js", "23s", "910s", "28s", "29o", "78o", "3Qs", "710s", "210s", "6Qs", "67s", "79s", "8Qs", "35s", "48o", "3Ks", "45s", "8Ao", "55o", "9Qs", "34s", "57s", "7As", "3Qo", "7Jo", "7Js", "49o", "27o", "59o", "34o", "4Ao", "27s", "26o", "57o", "35o", "25s", "2Qs", "4Ks", "79o"};

vector<Card> make_deck() {
    std::vector<Card> deck;
    for (const auto& rank : RANKS) {
        for (const auto& suit : SUITS) {
            Card card;
            card.rank = rank;
            card.suit = suit;
            deck.push_back(card);
        }
    }
    return deck;
}

// Add betting round enum
enum class BettingRound {
    PREFLOP = 0,
    FLOP = 1,
    TURN = 2,
    RIVER = 3,
    SHOWDOWN = 4
};

// ----------------------------------------------------------------------------
// PokerEvaluator
// ----------------------------------------------------------------------------

class PokerEvaluator {
    public:
        // Rank-to-value mapping
        const unordered_map<string, int> RANK_VALUES = {
            {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5},
            {"6", 6}, {"7", 7}, {"8", 8}, {"9", 9},
            {"10", 10}, {"J", 11}, {"Q", 12}, {"K", 13}, {"A", 14}
        };
    
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
                    int kicker = 0;
                    for (int v : cardValues) {
                        if (v != entry.first) { kicker = v; break; }
                    }
                    return makeScore(HandRank::FOUR_KIND, {entry.first, kicker});
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
                    return makeScore(HandRank::THREE_KIND, {entry.first, kickers[0], kickers[1]});
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
                    return makeScore(HandRank::PAIR, {entry.first, kickers[0], kickers[1], kickers[2]});
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
        void generateCombinations(const vector<Card>& cards,
                                  size_t combinationSize,
                                  size_t start,
                                  vector<Card>& currentCombo,
                                  vector<vector<Card>>& allCombinations) {
            // If we've picked enough cards, save this combo.
            if (currentCombo.size() == combinationSize) {
                allCombinations.push_back(currentCombo);
                return;
            }
            // How many more cards we still need
            size_t needed = combinationSize - currentCombo.size();
            // Loop while there's still room for `needed` cards in the tail of `cards`
            for (size_t i = start; i + needed <= cards.size(); ++i) {
                currentCombo.push_back(cards[i]);
                generateCombinations(cards, combinationSize, i + 1, currentCombo, allCombinations);
                currentCombo.pop_back();
            }
        }
    
        bool checkFlush(const vector<Card>& hand) {
            string suit = hand.front().suit;
            return all_of(hand.begin(), hand.end(), [&suit](const Card &c) { return c.suit == suit; });
        }
    
        bool checkStraight(const vector<Card>& hand, int &highStraightValue) {
            vector<int> values;
            for (const auto &card : hand)
                values.push_back(RANK_VALUES.at(card.rank));
            sort(values.begin(), values.end());
            
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

// ----------------------------------------------------------------------------
// Action enumeration and action amounts mapping
// ----------------------------------------------------------------------------

// Add betting actions
enum class Action {
     FOLD = 0,
     CHECK = 1,
     CALL = 2,
     BET_1 = 3,
     BET_1_5 = 4,
     BET_2 = 5,
     BET_3 = 6,
     BET_4 = 7,
     BET_5 = 8,
     BET_6 = 9,
     BET_7 = 10, 
     ALL_IN = 11,
     DEAL = 12,
     POST_SB = 13,
     POST_BB = 14,
     UNKNOWN = 15
 };


unordered_map<Action, double> ACTION_AMOUNTS = {
    {Action::BET_1, 1.0},
    {Action::BET_1_5, 1.5},
    {Action::BET_2, 2.0}, 
    {Action::BET_3, 3.0},
    {Action::BET_4, 4.0},
    {Action::BET_5, 5.0},
    {Action::BET_6, 6.0},
    {Action::BET_7, 7.0},
    {Action::POST_SB, 0.5},
    {Action::POST_BB, 1.0}
};

double amount(Action a) {
    if(ACTION_AMOUNTS.find(a) != ACTION_AMOUNTS.end())
        return ACTION_AMOUNTS[a];
    return 0.0;
}

// Function prototype
// Convert Action enum to string
string action_to_string(Action action) {
    switch (action) {
        case Action::FOLD: return "FOLD";
        case Action::CHECK: return "CHECK";
        case Action::CALL: return "CALL";
        case Action::BET_1: return "BET_1";
        case Action::BET_1_5: return "BET_1_5";
        case Action::BET_2: return "BET_2";
        case Action::BET_3: return "BET_3";
        case Action::BET_4: return "BET_4";
        case Action::BET_5: return "BET_5";
        case Action::BET_6: return "BET_6";
        case Action::BET_7: return "BET_7";
        case Action::ALL_IN: return "ALL_IN";
        case Action::DEAL: return "DEAL";
        case Action::POST_SB: return "POST_SB";
        case Action::POST_BB: return "POST_BB";
        case Action::UNKNOWN: return "UNKNOWN";
        default: return "INVALID_ACTION";
    }
}
// ----------------------------------------------------------------------------
// SpinGoState class
// ----------------------------------------------------------------------------

class SpinGoState {
public:
    // Game state variables:
    vector<Card> cards; // all hole cards (2 per player)
    vector<Action> bets; // last action for each player
    vector<double> pot; // current round contributions per player
    vector<double> players_stack; // stacks remaining per player
    bool game_over;
    int next_player;
    string round; // "preflop", "flop", "turn", "river", "showdown"
    set<int> active_players; // players not folded
    vector<Card> community_cards; // community cards
    vector<double> cumulative_pot; // total chips contributed per player over rounds
    double current_bet = 1.0;
    vector<Card> deck;             // the deck (shuffled)
    map<string, vector<pair<int, Action>>> round_action_history; // Stores action history per round

    // Random engine for shuffling.
    mt19937 rng;

    SpinGoState() {
        bets = vector<Action>(NUM_PLAYERS, Action::UNKNOWN);
        bets[0] = Action::POST_SB;
        bets[1] = Action::POST_BB;
        pot = vector<double>(NUM_PLAYERS, 0.0);
        pot[0] = 0.5;  // Initialize first player with 0.5
        pot[1] = 1.0;  // Initialize second player with 1.0
        players_stack = vector<double>(NUM_PLAYERS, INITIAL_STACK);
        players_stack[0] = INITIAL_STACK - 0.5;
        players_stack[1] = INITIAL_STACK - 1.0;
        cumulative_pot = vector<double>(NUM_PLAYERS, 0.0);
        game_over = false;
        next_player = 2;
        round = "preflop";
        for (int p = 0; p < NUM_PLAYERS; p++)
            active_players.insert(p);
        deck = make_deck();
        // initialize random engine
        rng.seed(random_device{}());
        shuffle(deck.begin(), deck.end(), rng);
    }

    // Returns the index of the current player (or -1 if game is over)
    int current_player() const {
        if (game_over)
            return -1;
        return next_player;
    }

    // Is this a chance node? (cards still to be dealt)
    bool is_chance_node() const {
        if (round == "preflop" && cards.size() < NUM_PLAYERS * 2)
            return true;
        else if (round == "flop" && community_cards.size() < 3)
            return true;
        else if (round == "turn" && community_cards.size() < 4)
            return true;
        else if (round == "river" && community_cards.size() < 5)
            return true;
        return false;
    }

    // Deal cards according to the current round.
    void deal_cards() {
        if (round == "preflop" && cards.size() < NUM_PLAYERS * 2) {
            // Deal two hole cards per player.
            for (int i = 0; i < NUM_PLAYERS * 2; i++) {
                cards.push_back(deck.back());
                deck.pop_back();
            }
        } else if (round == "flop" && community_cards.size() < 3) {
            for (int i = 0; i < 3; i++) {
                community_cards.push_back(deck.back());
                deck.pop_back();
            }
            // Commenting out the print statement for dealt flop cards
            // cout << "Dealt Flop: ";
            // for (const auto& card : community_cards) {
            //     cout << card.toString() << " ";
            // }
            // cout << endl; // Print the dealt flop cards
        } else if (round == "turn" && community_cards.size() == 3) {
            community_cards.push_back(deck.back());
            deck.pop_back();
        } else if (round == "river" && community_cards.size() == 4) {
            community_cards.push_back(deck.back());
            deck.pop_back();
        }
    }

    // Return vector of legal actions for the current player.
    vector<Action> legal_actions() const {
        vector<Action> actions;
        if (is_chance_node()) {
            actions.push_back(Action::DEAL);
            return actions;
        }
        if (game_over)
            return actions;
        int player = current_player();

        // Calculate total pot (current round + cumulative)
        // Calculate commitment ratio
        double player_total_contribution = pot[player] + cumulative_pot[player];
        double total_pot = 0.0;
        for (int p = 0; p < NUM_PLAYERS; p++) {
            total_pot += pot[p] + cumulative_pot[p];
        }
        
        // Calculate commitment ratio (player's contribution divided by their total stack)
        double commitment_ratio = player_total_contribution / (player_total_contribution + players_stack[player]);
        
        // If commitment ratio is above threshold, player can't fold
        const double COMMITMENT_THRESHOLD = 0.7; // 70% commitment threshold (seems logical)
        bool too_committed_to_fold = commitment_ratio >= COMMITMENT_THRESHOLD;
        
        // Declare hand_strs vector outside the if statement
        std::vector<std::string> hand_strs(NUM_PLAYERS);
        
        if (round == "preflop" && cards.size() >= (NUM_PLAYERS * 2)) {
            // Fill hand_strs for each player
            for (int p = 0; p < NUM_PLAYERS; ++p) {
                Card card1 = cards[p * 2];
                Card card2 = cards[p * 2 + 1];
                bool is_suited = (card1.suit == card2.suit);
                
                int rank1 = -1, rank2 = -1;
                for (size_t i = 0; i < RANKS.size(); i++) {
                    if (card1.rank == RANKS[i]) rank1 = i;
                    if (card2.rank == RANKS[i]) rank2 = i;
                }
                
                if (rank1 > rank2) std::swap(rank1, rank2);
                hand_strs[p] = RANKS[rank1] + RANKS[rank2] + (is_suited ? "s" : "o");
            }
        }
        bool allowed_to_play_preflop = true;
        if (round == "preflop") {
            if(player == 2){
                if(FOLD_BTN.find(hand_strs[player]) != FOLD_BTN.end()){
                    allowed_to_play_preflop = false;
                } 
            }
            else if(player == 0){
                if(bets[2] == Action::FOLD && FOLD_SB_BTN_FOLD.find(hand_strs[player]) != FOLD_SB_BTN_FOLD.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::BET_2 && FOLD_SB_BTN_BET_2.find(hand_strs[player]) != FOLD_SB_BTN_BET_2.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::ALL_IN && FOLD_SB_BTN_ALL_IN.find(hand_strs[player]) != FOLD_SB_BTN_ALL_IN.end()){
                    allowed_to_play_preflop = false;
                }
            }
            else if(player == 1){
                if (bets[2] == Action::FOLD && bets[0] == Action::BET_3 && FOLD_BB_BTN_FOLD_SB_BET_3.find(hand_strs[player]) != FOLD_BB_BTN_FOLD_SB_BET_3.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::FOLD && bets[0] == Action::ALL_IN && FOLD_BB_BTN_FOLD_SB_ALL_IN.find(hand_strs[player]) != FOLD_BB_BTN_FOLD_SB_ALL_IN.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::BET_2 && bets[0] == Action::FOLD && FOLD_BB_BTN_BET_2_SB_FOLD.find(hand_strs[player]) != FOLD_BB_BTN_BET_2_SB_FOLD.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::BET_2 && bets[0] == Action::CALL && FOLD_BB_BTN_BET_2_SB_CALL.find(hand_strs[player]) != FOLD_BB_BTN_BET_2_SB_CALL.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::BET_2 && bets[0] == Action::BET_4 && FOLD_BB_BTN_BET_2_SB_BET_4.find(hand_strs[player]) != FOLD_BB_BTN_BET_2_SB_BET_4.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::BET_2 && bets[0] == Action::ALL_IN && FOLD_BB_BTN_BET_2_SB_ALL_IN.find(hand_strs[player]) != FOLD_BB_BTN_BET_2_SB_ALL_IN.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::ALL_IN && bets[0] == Action::FOLD && FOLD_BB_BTN_ALL_IN_SB_FOLD.find(hand_strs[player]) != FOLD_BB_BTN_ALL_IN_SB_FOLD.end()){
                    allowed_to_play_preflop = false;
                }
                else if (bets[2] == Action::ALL_IN && bets[0] == Action::ALL_IN && FOLD_BB_BTN_ALL_IN_SB_ALL_IN.find(hand_strs[player]) != FOLD_BB_BTN_ALL_IN_SB_ALL_IN.end()){
                    allowed_to_play_preflop = false;
                }
                
            }
        if(!allowed_to_play_preflop){
            actions.push_back(Action::FOLD);
            return actions;
        }
        }
        // FOLD is always allowed unless check is free or player is too committed
        if (current_bet > pot[player] && !too_committed_to_fold) {
            actions.push_back(Action::FOLD);
        }
        
        // CHECK is only legal if no bet has been made or player has already matched current bet
        if (current_bet == 0 || pot[player] == current_bet) {
            actions.push_back(Action::CHECK);
        }
        
        // CALL is legal if there's a bet to call
        if (!(round == "preflop" && current_bet == 1 && player == 2) && (current_bet > pot[player] && players_stack[player] >= 3 && abs(players_stack[player] - current_bet) >= 4 && players_stack[player] > current_bet)) {
            actions.push_back(Action::CALL);
        }
        
        // In preflop, allow BET_2 as a BET option
        if (round == "preflop") {
            if (current_bet ==1) {
            if (player == 2 && players_stack[player] >= 4 && current_bet < amount(Action::BET_2)) {
                actions.push_back(Action::BET_2);  // This effectively works as a raise in preflop
            }
            else if (player !=2  && players_stack[player] >= 6 && current_bet < amount(Action::BET_3)) {
                actions.push_back(Action::BET_3);  // This effectively works as a raise in preflop
            }
        }
           if (current_bet == 2 && player != 2) {
            actions.push_back(Action::BET_4);
        }
        }
        if (round != "preflop") {
            if (current_bet == 0) {
            if (players_stack[player] >= 4 && total_pot <= 6) {
                actions.push_back(Action::BET_1_5);
            }
            if (players_stack[player] >= 6 && total_pot >= 4 && total_pot <= 11) {
                actions.push_back(Action::BET_3);
            }
            }
        }
        if (current_bet > 0) {
            if (current_bet > 1 && current_bet <= 2 && total_pot >= 6 && total_pot <= 15) {
            if (players_stack[player] >= 9) {
                actions.push_back(Action::BET_4);
            }
            }
            else if (current_bet > 2 && current_bet <= 3 && total_pot >= 10 && total_pot <= 20) {
                if (players_stack[player] >= 12) {
                    actions.push_back(Action::BET_6);
                }
            }
            else if (current_bet > 3 && current_bet <= 3.5 && total_pot >= 12 && total_pot <= 23) {
                if (players_stack[player] >= 14) {
                    actions.push_back(Action::BET_7);
                }
            }
        }
        
        // All-In is allowed if the player has any chips.
        // If player is too committed, force all-in as the only option when facing a bet
        if (too_committed_to_fold && current_bet > pot[player]) {
            // Clear other actions and only allow all-in
            actions.clear();
            if (players_stack[player] > 0) {
                actions.push_back(Action::ALL_IN);
            }
        } else if (players_stack[player] > 0) {
            // Normal case - all-in is an option
            actions.push_back(Action::ALL_IN);
        }
        
        return actions;
    }

    // Check whether the betting round is complete.
    bool betting_round_complete() {
        // If all active players have 0 chips, then round is complete.
        bool allZero = true;
        for (int p : active_players) {
            if (players_stack[p] != 0) {
                allZero = false;
                break;
            }
        }
        if (allZero)
            return true;

        // If there's only one active player left, the round is complete
        if (active_players.size() <= 1) {
            return true;
        }

        // Check if all active players have matched the current bet
        for (int p : active_players) {
            if (pot[p] < current_bet) {
                // This player hasn't matched the current bet yet
                return false;
            }
        }

        // Check if all active players have acted at least once
        for (int p : active_players) {
            if (bets[p] == Action::UNKNOWN) {
                return false;
            }
        }

        // In preflop, we need to ensure the correct betting sequence
        if (round == "preflop") {
            // In preflop, we need to make sure all three players have acted
            // Check if we have the correct number of actions for preflop
            int action_count = 0;
            for (int p = 0; p < NUM_PLAYERS; p++) {
                if (bets[p] != Action::UNKNOWN) {
                    action_count++;
                }
            }
            
            // In preflop, we need at least 3 actions (one from each player)
            // unless players have folded
            if (action_count < min(3, static_cast<int>(active_players.size()))) {
                return false;
            }
        }

        // Find the last player who raised
        int last_raiser = -1;
        double highest_bet = 0;
        
        for (int p = 0; p < NUM_PLAYERS; p++) {
            if (active_players.find(p) != active_players.end()) {
                if ((bets[p] == Action::BET_1 || bets[p] == Action::BET_1_5 || 
                    bets[p] == Action::BET_2 || bets[p] == Action::BET_3 || 
                    bets[p] == Action::BET_4 || bets[p] == Action::BET_5 || 
                    bets[p] == Action::BET_6 || bets[p] == Action::BET_7) && 
                    pot[p] > highest_bet) {
                    highest_bet = pot[p];
                    last_raiser = p;
                }
            }
        }
        
        // If there was a raise, check if everyone has acted after the last raise
        if (last_raiser != -1) {
            // Start checking from the player after the last raiser
            int p = (last_raiser + 1) % NUM_PLAYERS;
            while (p != last_raiser) {
                if (active_players.find(p) != active_players.end()) {
                    // If this active player hasn't acted after the last raise
                    if (bets[p] == Action::UNKNOWN) {
                        return false;
                    }
                }
                p = (p + 1) % NUM_PLAYERS;
            }
        }

        return true; // If all conditions are met, the round is complete
    }

    // Advance _next_player to the next active player.
    void advance_to_next_player() {
        if (active_players.empty()) {
            game_over = true;
            return;
        }
        
        // Try to find the next active player in circular order
        int original_next = next_player;
        do {
            next_player = (next_player + 1) % NUM_PLAYERS;
            // If we've checked all players and come back to the original, take the first active player
            if (next_player == original_next) {
                vector<int> act(active_players.begin(), active_players.end());
                sort(act.begin(), act.end());
                next_player = act.front();
                break;
            }
        } while (active_players.find(next_player) == active_players.end());
    }

    // Advance the round and move this round's contributions to cumulative.
    void advance_round() {
        for (int p = 0; p < NUM_PLAYERS; p++) {
            cumulative_pot[p] += pot[p];
            pot[p] = 0.0;
            bets[p] = Action::UNKNOWN;
        }
        current_bet = 0.0;
        if (round == "preflop")
            round = "flop";
        else if (round == "flop")
            round = "turn";
        else if (round == "turn")
            round = "river";
        else if (round == "river")
            round = "showdown";
    }

    // Check whether the game should end.
    bool should_end_game() {
        if (active_players.size() == 1 || round == "showdown")
            return true;
        // If all active players are all in.
        bool allIn = true;
        for (int p : active_players) {
            if (players_stack[p] > 0)
                allIn = false;
        }
        if (allIn)
            return true;
        return false;
    }

    // Apply the given action.
    void apply_action(Action action) {
        // If chance node then deal cards.
        if (is_chance_node()) {
            deal_cards();
            return;
        }
        int player = current_player();
        vector<Action> legal = legal_actions();
        if (find(legal.begin(), legal.end(), action) == legal.end()) {
            throw runtime_error("Illegal action chosen for player " + std::to_string(player));
        }
        bets[player] = action;

        // Record action in round history, unless it's a setup or deal action
        if (action != Action::DEAL && action != Action::POST_SB && action != Action::POST_BB) {
            round_action_history[round].push_back({player, action});
        }
        
        // Handle actions
        if (action == Action::FOLD) {
            active_players.erase(player);
            if (active_players.size() <= 1) {
                // Make sure to update cumulative_pot before ending the game
                for (int p = 0; p < NUM_PLAYERS; p++) {
                    cumulative_pot[p] += pot[p];
                    pot[p] = 0.0;
                }
                game_over = true;
                return;  // Exit early to prevent further processing
            }
        } else if (action == Action::POST_SB) {
            current_bet = amount(Action::POST_SB);
            pot[player] = current_bet;
            players_stack[player] -= current_bet;
            next_player = 1;  // Set next player to BB
            return;  // Return early to prevent advance_to_next_player
        } else if (action == Action::POST_BB) {
            current_bet = amount(Action::POST_BB);
            pot[player] = current_bet;
            players_stack[player] -= current_bet;
            next_player = (player + 1) % NUM_PLAYERS;  // Set next player to the first player who will act
            return;  // Return early to prevent advance_to_next_player
        } else if (action == Action::CALL) {
            double call_amt = current_bet - pot[player];
            pot[player] = current_bet;
            players_stack[player] -= call_amt;
        } else if (action_to_string(action).substr(0, 4) == "BET_") {
            double bet_amt = amount(action);
            current_bet = bet_amt;
            double call_amt = current_bet - pot[player];
            pot[player] = current_bet;
            players_stack[player] -= call_amt;
        } else if (action == Action::ALL_IN) {
            double all_in_amt = pot[player] + players_stack[player];
            if (all_in_amt > current_bet) {
                current_bet = all_in_amt;
            }
            pot[player] = all_in_amt;
            players_stack[player] = 0;
        } else if (action == Action::CHECK) {
            // Do nothing.
        } else if (action == Action::DEAL) {
            deal_cards();
        }

        // After the action, check if betting round is complete.
        if (betting_round_complete()) {
            advance_round();
            if (should_end_game()) {
                // if game should end but some cards missing, complete the deal.
                while (round != "showdown") {
                    deal_cards();
                    advance_round();
                }
                game_over = true;
            } else {
                // Set next player based on the round
                if (round == "flop" && active_players.size() > 1) {
                    // After preflop, the first player to act should be the first active player
                    vector<int> act(active_players.begin(), active_players.end());
                    sort(act.begin(), act.end());
                    next_player = act.front();
                } else if (round == "preflop") {
                    // In preflop, after blinds are posted, player 2 should act first if active
                    if (active_players.find(2) != active_players.end()) {
                        next_player = 2;
                    } else {
                        // If player 2 is not active, find the next active player
                        vector<int> act(active_players.begin(), active_players.end());
                        sort(act.begin(), act.end());
                        next_player = act.front();
                    }
                } else {
                    // In other rounds, start with the first active player
                    vector<int> act(active_players.begin(), active_players.end());
                    sort(act.begin(), act.end());
                    next_player = act.front();
                }
            }
        } else {
            advance_to_next_player();
        }
    }

    // Compute and return the rewards (for each player) at game end.
    vector<double> returns() {
        vector<double> rewards(NUM_PLAYERS, 0.0);
        if (!game_over)
            return rewards;
        double total_pot = 0.0;
        for (double x : cumulative_pot)
            total_pot += x;
        if (active_players.size() == 1) {  // one remaining winner.
            int winner = *active_players.begin();
            for (int p = 0; p < NUM_PLAYERS; p++)
                rewards[p] = -cumulative_pot[p];
            rewards[winner] = total_pot - cumulative_pot[winner];
        } else {
            PokerEvaluator evaluator;
            unordered_map<int,int> best_hands;
            for (int p : active_players) {
                vector<Card> hole_cards;
                // compute as size_t so we can compare safely
                size_t i1 = static_cast<size_t>(p*2),
                       i2 = static_cast<size_t>(p*2 + 1);
                // only read if both indices exist
                if (cards.size() > i2) {
                    hole_cards.push_back(cards[i1]);
                    hole_cards.push_back(cards[i2]);
                }
                int hand_value = evaluator.evaluateHand(hole_cards, community_cards);
                best_hands[p] = hand_value;
            }
            int max_value = 0;
            for (auto &entry : best_hands) {
                if (entry.second > max_value)
                    max_value = entry.second;
            }
            vector<int> winners;
            for (auto &entry : best_hands) {
                if (entry.second == max_value)
                    winners.push_back(entry.first);
            }
            double reward_share = winners.empty() ? 0.0 : total_pot / winners.size();
            for (int p = 0; p < NUM_PLAYERS; p++) {
                if (find(winners.begin(), winners.end(), p) != winners.end())
                    rewards[p] = reward_share - cumulative_pot[p];
                else
                    rewards[p] = -cumulative_pot[p];
            }
        }
        return rewards;
    }

    // A string representation of the state.
    string to_string() {
        ostringstream oss;
        oss << "Cards: ";
        for (const Card &c : cards)
            oss << c.toString() << " ";
        oss << "\nCommunity: ";
        for (const Card &c : community_cards)
            oss << c.toString() << " ";
        oss << "\nBets: ";
        for (int p = 0; p < NUM_PLAYERS; p++)
            oss << "P" << p << ": " << static_cast<float>(players_stack[p]) << "  ";
        oss << "\nPot (this round): ";
        for (int p = 0; p < NUM_PLAYERS; p++)
            oss << "P" << p << ": " << pot[p] << "  ";
        oss << "\nCurrent Bet: " << current_bet;
        oss << "\nNext Player: " << next_player;
        oss << "\nRound: " << round;
        oss << "\nStatus: " << (game_over ? "Game Over" : "In Progress");
        return oss.str();
    }

    // Returns the current player's stack
    double get_current_player_stack() {
        if (game_over) {
            throw runtime_error("Game is over, no current player.");
        }
        return players_stack[next_player];
    }

    // Modify the write_infosets method to append to the file
    void write_infosets(const string& filename) {
        // Check if the current state is a chance node
        if (is_chance_node()) {
            return; // Do not write infosets if it's a chance node
        }

        ofstream file(filename, ofstream::app); // Open file in append mode
        if (!file.is_open()) {
            throw runtime_error("Could not open file for writing: " + filename);
        }

        // Log the current round and current player
        file << "Round: " << round << "\n";
        file << "Current Player: Player " << next_player << "\n";

        // Log action history for the current round
        file << "Action History for Round " << round << ": ";
        if (round_action_history.count(round)) {
            for (const auto& p_action : round_action_history.at(round)) {
                file << "(Player " << p_action.first << ", " << action_to_string(p_action.second) << ") ";
            }
        } else {
            file << "No actions recorded for this round yet.";
        }
        file << "\n";

        // Log hole cards for all players with round information
        for (int p = 0; p < NUM_PLAYERS; ++p) {
            file << "Player " << p << " Hole Cards (Round: " << round << "): " 
                 << cards[p * 2].toString() << " " << cards[p * 2 + 1].toString() << "\n";
        }

        // Log the ranks of the community cards
        file << "Community Cards (Round: " << round << "): ";
        for (const auto& card : community_cards) {
            file << card.toString() << " "; // Log each community card
        }
        file << "\n\n";

        // Log the overall pot
        double total_pot = accumulate(pot.begin(), pot.end(), 0.0);
        file << "Overall Pot: " << total_pot << "\n\n"; // Add space after each infoset

        file.close();
    }

};

// ----------------------------------------------------------------------------
// KickOffGame class
// ----------------------------------------------------------------------------

class SpinGoGame {
public:
    SpinGoGame() {}
    
    // This method creates and returns a new initial state for the game
    SpinGoState new_initial_state() {
        SpinGoState state;
        // Deal initial cards
        state.apply_action(Action::DEAL);
        return state;
    }
};

// ----------------------------------------------------------------------------
// Example Usage (main function)
// ----------------------------------------------------------------------------

// Add this before main()
ostream& operator<<(ostream& os, const vector<Action>& actions) {
    os << "[";
    for (size_t i = 0; i < actions.size(); ++i) {
        os << action_to_string(actions[i]);
        if (i < actions.size() - 1) os << ", ";
    }
    os << "]";
    return os;
}

// int main() {
//     SpinGoGame game;
//     mt19937 rng(random_device{}());
//     const int num_samples = 1e3;
//     SpinGoState state = game.new_initial_state(); 
//     // state.apply_action(Action::DEAL);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::POST_SB);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::POST_BB);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::BET_2);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::CALL);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::CALL);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::DEAL);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::BET_2);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::CALL);
//     // cout << state.legal_actions() << "\n";
//     // state.apply_action(Action::CALL);
//     // cout << state.legal_actions() << "\n";

//     // Open file for writing and check if it is open
//     ofstream file("infosets.txt");
//     if (!file) {
//         cerr << "Could not open file for writing: infosets.txt" << endl;
//         return 1;
//     }

//     set<string> logged_sequences; // Set to track logged action sequences
//     auto start_time = chrono::high_resolution_clock::now();

//     for (int sample = 0; sample < num_samples; ++sample) {
//         SpinGoState state = game.new_initial_state(); // Reinitialize the game
//         string action_sequence;
//         string previous_round = state.round; // Initialize previous_round

//         // Calculate and print ETA
//         if (sample % 100 == 0 && sample > 0) {
//             auto current_time = chrono::high_resolution_clock::now();
//             chrono::duration<double> elapsed = current_time - start_time;
//             double eta = (elapsed.count() / sample) * (num_samples - sample);
//             cout << "\rETA: " << fixed << setprecision(2) << eta << " seconds remaining" << flush;
//         }

//         while (!state.game_over) {
//             vector<Action> actions = state.legal_actions();
//             if (actions.empty()) break; // No legal actions available

//             // Randomly select an action
//             uniform_int_distribution<int> dist(0, actions.size() - 1);
//             Action random_action = actions[dist(rng)];

//             // Apply the selected action
//             state.apply_action(random_action);

//             // Append the action to the sequence if it's not DEAL
//             if (random_action != Action::DEAL) {
//                 action_sequence += action_to_string(random_action) + " ";
//             }

//             // Add a separator when the round changes
//             if (state.round != previous_round) { // Check if the round has changed
//                 action_sequence += "| "; // Add separator
//                 previous_round = state.round; // Update previous_round to current round
//             }
//         }
//         action_sequence += "Pot: " + to_string(state.cumulative_pot[0] + state.cumulative_pot[1] + state.cumulative_pot[2]);
//         if (!action_sequence.empty()) {
//             string log_entry = action_sequence;
//             if (logged_sequences.find(log_entry) == logged_sequences.end()) {
//                 file << log_entry << "\n";
//                 logged_sequences.insert(log_entry); // Add to the set of logged sequences
//             }
//         }
//     }

//     file.close(); // Close the file
//     return 0;
// }

// int main() {
//     SpinGoGame game;
//     SpinGoState state = game.new_initial_state();
//     state.apply_action(Action::ALL_IN);
//     state.apply_action(Action::FOLD);
//     state.apply_action(Action::FOLD);

//     cout << "state:\n" << state.to_string() << "\n\n";
//     cout << "Legal actions: " << state.legal_actions() << "\n";
//     cout << "Returns: " << state.returns() << "\n";
//     return 0;
// }
