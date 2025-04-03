#include <iostream> // Include the iostream header for cout
#include "poker_evaluator.cpp"

PokerEvaluator evaluator;

void initializeEvaluator() {
    evaluator = PokerEvaluator();
}

int main() {
    initializeEvaluator();
    std::vector<Card> holeCards = {{"A", "h"}, {"A", "d"}};
    std::vector<Card> communityCards = {{"A", "c"}, {"A", "s"},{"3","c"},{"3","h"},{"2","d"}};
    vector<Card> allCards = holeCards;
                allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
        
    vector<vector<Card>> combinations;
    vector<Card> currentCombo;
    evaluator.generateCombinations(allCards, 5, 0, currentCombo, combinations);
    
    // Print all combinations
    for (const auto& combo : combinations) {
        for (const auto& card : combo) {
            std::cout << card.toString() << " ";
        }
        std::cout << evaluator.evaluateFiveCardHand(combo) << " ";
        std::cout << std::endl; // New line for each combination
    }
    
    int score = evaluator.evaluateHand(holeCards, communityCards);
    
    // Print the score
    std::cout << "Score: " << score << std::endl; // Use std::cout to print the score
    return score;
}