from treys import Evaluator, Card, Deck
import random
import sys  # Make sure to import sys at the top of your file

# evaluator = Evaluator()
# # Example Board (5 community cards)
# board = [Card.new('2s'), Card.new('3s'), Card.new('4s'), Card.new('5s'), Card.new('Jd')]
# # Example Player Hand
# # Define the weakest possible Straight Flush: A♠ 2♠ 3♠ 4♠ 5♠
# reference_hand = [Card.new('As'), Card.new('7d')]
# threshold_value = evaluator.evaluate(board, reference_hand)
#threshold_value = 9  finally

def monte_carlo_simulation(hands, runs=10000, starting_stacks = [8,8,7.5,7], rake = 0.03, jackpot_fee = 0.035, jackpot_payout = 1.5, threshold_value = 9,evaluator=None):
    
    n_hands = len(hands)
    
    wins = [0] * n_hands
    
    for _ in range(runs):
        deck = Deck()
        # Remove the cards in the hands from the deck
        for hand in hands:
            for card in hand:
                if card in deck.cards:
                    deck.cards.remove(card)
        
        board = deck.draw(5)
        best_values = []
        for h in hands:
            best_values.append(evaluator.evaluate(board, h))
        
        min_val = min(best_values)
        winners = [i for i, val in enumerate(best_values) if val == min_val]
        
        for i in range(n_hands):
            if i in winners:
                # Check for jackpot eligibility
                if min_val <= threshold_value:
                    wins[i] += (sum([starting_stacks[j] for j in range(4) if j in winners]) + 1.5 - rake + jackpot_payout - jackpot_fee) / len(winners) - starting_stacks[i]
                else:
                    wins[i] += (sum([starting_stacks[j] for j in range(4) if j in winners]) + 1.5 - rake) / len(winners) - starting_stacks[i]
            else:
                wins[i] -= starting_stacks[i]   # Assign negative values for losers
    
    win_rates = [wins[i] / runs for i in range(n_hands)]
    return win_rates
