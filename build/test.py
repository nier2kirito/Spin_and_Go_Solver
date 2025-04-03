import time
import sys
from collections import defaultdict
from parser import aggregate_hand_data_from_files
import random
import csv
from hand_evaluator import monte_carlo_simulation
from treys import Evaluator, Card, Deck

TOP_40_PERCENT_HANDS = [
    "A Ao", "K Ko", "Q Qo", "J Jo", "10 10o", "9 9o", "8 8o", "7 7o", "6 6o", "5 5o", 
    "4 4o", "3 3o", "2 2o", "A As", "K As", "Q As", "J As", "10 As", "9 As", "8 As", 
    "7 As", "6 As", "5 As", "4 As", "3 As", "2 As", "A Ks", "K Ks", "Q Ks", "J Ks", "10 Ks", 
    "9 Ks", "8 Ks", "7 Ks", "6 Ks", "5 Ks", "4 Ks", "3 Ks", 
    "J Qs", "10 Qs", "9 Qs", "8 Qs", "7 Qs", "6 Qs", "5 Qs",
    "K Js", "Q Js", "J Js", "10 Js", "9 Js", "8 Js", 
    "2 Js", "A Ko", "K Qo", "Q Jo", "J 10o", "10 9o", "9 8o"
]

opponent_hand_ranges = {
        'CO': [
            "2 2o", "3 3o", "4 4o", "5 5o", "6 6o", "7 7o", "8 8o", "9 9o",
            "2 3s", "2 4s", "2 5s", "2 6s", "2 7s", "2 8s", "2 9s", "2 10s", "2 Js", "2 Qs", "2 Ks", "2 As",
            "3 4s", "3 5s", "3 6s", "3 7s", "3 8s", "3 9s", "3 10s", "3 Js", "3 Qs", "3 Ks", "3 As",
            "4 5s", "4 6s", "4 7s", "4 8s", "4 9s", "4 10s", "4 Js", "4 Qs", "4 Ks", "4 As",
            "5 6s", "5 7s", "5 8s", "5 9s", "5 10s", "5 Js", "5 Qs", "5 Ks", "5 As",
            "6 7s", "6 8s", "6 9s", "6 10s", "6 Js", "6 Qs", "6 Ks", "6 As",
            "7 8s", "7 9s", "7 10s", "7 Js", "7 Qs", "7 Ks", "7 As",
            "8 9s", "8 10s", "8 Js", "8 Qs", "8 Ks", "8 As",
            "9 10s", "9 Js", "9 Qs", "9 Ks", "9 As",
            "10 Js", "10 Qs", "10 Ks", "10 As",
            "J Qs", "J Ks", "J As",
            "Q Ks", "Q As",
            "K As",
            "10 10o", "J Jo", "Q Qo", "K Ko", "A Ao"
        ],
        'Button': [
            "2 2o", "3 3o", "4 4o", "5 5o", "6 6o", "7 7o", "8 8o", "9 9o",
            "2 3s", "2 4s", "2 5s", "2 6s", "2 7s", "2 8s", "2 9s", "2 10s", "2 Js", "2 Qs", "2 Ks", "2 As",
            "3 4s", "3 5s", "3 6s", "3 7s", "3 8s", "3 9s", "3 10s", "3 Js", "3 Qs", "3 Ks", "3 As",
            "4 5s", "4 6s", "4 7s", "4 8s", "4 9s", "4 10s", "4 Js", "4 Qs", "4 Ks", "4 As",
            "5 6s", "5 7s", "5 8s", "5 9s", "5 10s", "5 Js", "5 Qs", "5 Ks", "5 As",
            "6 7s", "6 8s", "6 9s", "6 10s", "6 Js", "6 Qs", "6 Ks", "6 As",
            "7 8s", "7 9s", "7 10s", "7 Js", "7 Qs", "7 Ks", "7 As",
            "8 9s", "8 10s", "8 Js", "8 Qs", "8 Ks", "8 As",
            "9 10s", "9 Js", "9 Qs", "9 Ks", "9 As",
            "10 Js", "10 Qs", "10 Ks", "10 As",
            "J Qs", "J Ks", "J As",
            "Q Ks", "Q As",
            "K As",
            "2 3o", "2 4o", "2 5o", "2 6o", "2 7o", "2 8o", "2 9o", "2 10o", "2 Jo", "2 Qo", "2 Ko", "2 Ao",
            "3 4o", "3 5o", "3 6o", "3 7o", "3 8o", "3 9o", "3 10o", "3 Jo", "3 Qo", "3 Ko", "3 Ao",
            "4 5o", "4 6o", "4 7o", "4 8o", "4 9o", "4 10o", "4 Jo", "4 Qo", "4 Ko", "4 Ao",
            "5 6o", "5 7o", "5 8o", "5 9o", "5 10o", "5 Jo", "5 Qo", "5 Ko", "5 Ao",
            "6 7o", "6 8o", "6 9o", "6 10o", "6 Jo", "6 Qo", "6 Ko", "6 Ao",
            "7 8o", "7 9o", "7 10o", "7 Jo", "7 Qo", "7 Ko", "7 Ao",
            "8 9o", "8 10o", "8 Jo", "8 Qo", "8 Ko", "8 Ao",
            "9 10o", "9 Jo", "9 Qo", "9 Ko", "9 Ao",
            "10 Jo", "10 Qo", "10 Ko", "10 Ao",
            "J Qo", "J Ko", "J Ao",
            "Q Ko", "Q Ao",
            "K Ao"
        ],
        'SB': [
            "2 2o", "3 3o", "4 4o", "5 5o", "6 6o", "7 7o", "8 8o", "9 9o",
            "2 3s", "2 4s", "2 5s", "2 6s", "2 7s", "2 8s", "2 9s", "2 10s", "2 Js", "2 Qs", "2 Ks", "2 As",
            "3 4s", "3 5s", "3 6s", "3 7s", "3 8s", "3 9s", "3 10s", "3 Js", "3 Qs", "3 Ks", "3 As",
            "4 5s", "4 6s", "4 7s", "4 8s", "4 9s", "4 10s", "4 Js", "4 Qs", "4 Ks", "4 As",
            "5 6s", "5 7s", "5 8s", "5 9s", "5 10s", "5 Js", "5 Qs", "5 Ks", "5 As",
            "6 7s", "6 8s", "6 9s", "6 10s", "6 Js", "6 Qs", "6 Ks", "6 As",
            "7 8s", "7 9s", "7 10s", "7 Js", "7 Qs", "7 Ks", "7 As",
            "8 9s", "8 10s", "8 Js", "8 Qs", "8 Ks", "8 As",
            "9 10s", "9 Js", "9 Qs", "9 Ks", "9 As",
            "10 Js", "10 Qs", "10 Ks", "10 As",
            "J Qs", "J Ks", "J As",
            "Q Ks", "Q As",
            "K As",
            "2 3o", "2 4o", "2 5o", "2 6o", "2 7o", "2 8o", "2 9o", "2 10o", "2 Jo", "2 Qo", "2 Ko", "2 Ao",
            "3 4o", "3 5o", "3 6o", "3 7o", "3 8o", "3 9o", "3 10o", "3 Jo", "3 Qo", "3 Ko", "3 Ao",
            "4 5o", "4 6o", "4 7o", "4 8o", "4 9o", "4 10o", "4 Jo", "4 Qo", "4 Ko", "4 Ao",
            "5 6o", "5 7o", "5 8o", "5 9o", "5 10o", "5 Jo", "5 Qo", "5 Ko", "5 Ao",
            "6 7o", "6 8o", "6 9o", "6 10o", "6 Jo", "6 Qo", "6 Ko", "6 Ao",
            "7 8o", "7 9o", "7 10o", "7 Jo", "7 Qo", "7 Ko", "7 Ao",
            "8 9o", "8 10o", "8 Jo", "8 Qo", "8 Ko", "8 Ao",
            "9 10o", "9 Jo", "9 Qo", "9 Ko", "9 Ao",
            "10 Jo", "10 Qo", "10 Ko", "10 Ao",
            "J Qo", "J Ko", "J Ao",
            "Q Ko", "Q Ao",
            "K Ao"
        ]
    }


Cards = ['A', 'K', 'Q', 'J', 'T', '9', '8', '7', '6', '5', '4', '3', '2']


hands_combinations = [
    [Card.new(f"{rank1}{suit1}"), Card.new(f"{rank2}{suit2}")]
    for rank1 in Cards for rank2 in Cards 
    for suit1 in ['c', 'd', 'h', 's'] for suit2 in ['c', 'd', 'h', 's'] 
    if rank1 != rank2 or suit1 != suit2
]

def parse_hand(hand):
    ranking ='23456789TJQKA'
    card1, card2 = Card.int_to_str(hand[0]), Card.int_to_str(hand[1])
    card1_rank, card2_rank = card1[0], card2[0]
    card1_suit, card2_suit = card1[1], card2[1]
    card1_rank_index, card2_rank_index = ranking.index(card1_rank), ranking.index(card2_rank)

    if card1_rank == 'T':
        card1_rank = '10'
    if card2_rank == 'T':
        card2_rank = '10'
    if card1_suit == card2_suit:
        if card1_rank_index > card2_rank_index:
            return f"{card2_rank} {card1_rank}s"
        else:
            return f"{card1_rank} {card2_rank}s"
    else:
        if card1_rank_index > card2_rank_index:
            return f"{card2_rank} {card1_rank}o"
        else:
            return f"{card1_rank} {card2_rank}o"
                
hand_data = aggregate_hand_data_from_files()

# Function to simulate naive player decisions
def naive_player_decisions(hand):
    naive_decisions = defaultdict(lambda: (0, 0))  # (fold_prob, all_in_prob)
    if hand in  TOP_40_PERCENT_HANDS:
        return 'ALL_IN'
    return 'FOLD'

def hard_player_decisions(hand, nb_previous_decisions):
    if nb_previous_decisions == 0: #CO
        if hand in opponent_hand_ranges['CO']:
            return 'ALL_IN'
        else:
            return 'FOLD'
    elif nb_previous_decisions == 1: #Button
        if hand in opponent_hand_ranges['Button']:
            return 'ALL_IN'
        else:
            return 'FOLD'
    elif nb_previous_decisions in {2,3}: #SB, BB
        if hand in opponent_hand_ranges['SB']:
            return 'ALL_IN'
        else:
            return 'FOLD'

def decision_to_infoset(decisions):
    infoset = ""
    if len(decisions) == 3:
        infoset = f"P{1}:"
        for i in [0, 2, 3]:  # Order: P2, P0, P1
            decision = decisions[i] if i==0 else decisions[i-1]
            if decision == 'ALL_IN':
                infoset += f"[P{i}:A]"  # Player i goes all-in
            else:
                infoset += f"[P{i}:F]"  # Player i folds
    elif len(decisions) == 2:
        infoset = f"P{0}:[P1:P]"
        for i, decision in enumerate(decisions):
            if decision == 'ALL_IN':
                infoset += f"[P{(i+2)%4}:A]"  # Player i goes all-in
            else:
                infoset += f"[P{(i+2)%4}:F]"  # Player i folds
    elif len(decisions) == 1:
        infoset = f"P{3}:[P0:P][P1:P]"
        for i, decision in enumerate(decisions):
            if decision == 'ALL_IN':
                infoset += f"[P{(i+2)%4}:A]"  # Player i goes all-in
            else:
                infoset += f"[P{(i+2)%4}:F]"  # Player i folds
    else:
        infoset = f"P{2}:[P0:P][P1:P]"
    return infoset

mapping ={2 : 0, 3 : 1, 0 : 2, 1 : 3}
# Function to simulate the game
def simulate_game(num_games=1, strategic_player_index=2, player= naive_player_decisions,runs =1, rake = 0.03, jackpot_fee = 0.035, jackpot_payout = 1.5, threshold_value = 9, starting_stacks = [8,8,7.5,7], evaluator=None):
    results = []
    
    for _ in range(num_games):
        #Here manby cards we should decide if they can be dealt twice or not 
        hands = random.sample(hands_combinations, 4)
        decisions = []
        
        # Naive players
        for player_index in [2,3,0,1]:
            if player_index == strategic_player_index:
                if decisions == ['FOLD', 'FOLD', 'FOLD']:
                    decisions.append('ALL_IN')
                else:
                    infoset = decision_to_infoset(decisions)
                    fold_prob, all_in_prob = hand_data[(infoset, parse_hand(hands[mapping[player_index]]))]
                    #print(infoset, parse_hand(hands[mapping[player_index]]), all_in_prob, fold_prob)
                    if random.random() < all_in_prob:
                        decisions.append('ALL_IN')
                    else:
                        decisions.append('FOLD')
            else:
                if decisions == ['FOLD', 'FOLD', 'FOLD']:
                    decisions.append('ALL_IN')
                else:
                    if player == naive_player_decisions:
                        decision = naive_player_decisions(parse_hand(hands[mapping[player_index]]))
                    else:
                        decision = player(parse_hand(hands[mapping[player_index]]), len(decisions))
                    decisions.append(decision)

        alive_hands = [(i, hands[i]) for i in range(4) if decisions[i] != 'FOLD']
    
        wins = [0,0,-0.5,-1]
        
        for _ in range(runs):
            deck = Deck()
            # Remove the cards in the hands from the deck
            for hand in hands:
                for card in hand:
                    if card in deck.cards:
                        deck.cards.remove(card)
            
            board = deck.draw(5)
            best_values = []
            if (len(alive_hands) == 1):
                wins[alive_hands[0][0]] += 1.5
            else:
                for i,(player_index,hand) in enumerate(alive_hands):
                    best_values.append((player_index,evaluator.evaluate(board, hand)))
                
                min_val = min([best_values[i][1] for i in range(len(best_values))])
                jackpot_eligible = False
                winners = [i for (i, val) in best_values if val == min_val]
                for (i,card) in alive_hands:
                    if i in winners:
                        # Check for jackpot eligibility
                        if min_val <= threshold_value:
                            jackpot_eligible = True
                            wins[i] += (sum([starting_stacks[j] for (j,hand) in alive_hands]) + 1.5 - rake + jackpot_payout - jackpot_fee) / len(winners) - starting_stacks[i]
                        else:
                            wins[i] += (sum([starting_stacks[j] for (j,hand) in alive_hands]) + 1.5 - rake) / len(winners) - starting_stacks[i]
                    else:
                        wins[i] -= starting_stacks[i]   # Assign negative values for losers
        
        win_rates = [wins[i] / runs for i in range(4)]    
        try:
            assert (sum(win_rates) == 0 ) or (sum(win_rates) + 0.03) < 1e-5 or (jackpot_eligible and(sum(win_rates) +0.03 - jackpot_payout + jackpot_fee) < 1e-5 )#whether a pot was constructed or not
        except AssertionError:
            print('Alive hands:', alive_hands)
            print("Decisions:", decisions)
            print("Wins:", wins)
        results.append(([parse_hand(hand) for hand in hands], decisions, win_rates))
    return results

#Harder version
def harder_player_decisions(hand, nb_decisions):
    if nb_decisions == 0: #CO
        if hand in opponent_hand_ranges['CO']:
            return 'ALL_IN'
        else:
            return 'FOLD'
    elif nb_decisions == 1: #Button
        if hand in opponent_hand_ranges['Button']:
            return 'ALL_IN'
        else:
            return 'FOLD'
    elif nb_decisions == 2:  # SB
        if hand in opponent_hand_ranges['SB']:
            return 'ALL_IN'
        else:
            return 'FOLD'

# Function to save results to CSV
def save_to_csv(strategic_player_index=2, num_games = 1, player= naive_player_decisions):
    evaluator = Evaluator()
    results = simulate_game(num_games, strategic_player_index, player = naive_player_decisions, evaluator=evaluator)
    filename = f"game_results_{strategic_player_index}.csv"
    
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['Hand 2', 'Hand 3', 'Hand 0', 'Hand 1', 'Player 2 Decision', 
                         'Player 3 Decision', 
                         'Player 0 Decision', 
                         'Player 1  Decision', 'win_rate 2', 'win_rate 3', 'win_rate 0', 'win_rate 1'])
        
        for i, (hands, decisions, win_rates) in enumerate(results):
            writer.writerow(hands + decisions + win_rates)
#More general version 
def save_to_csv_general(num_games = 1, player= naive_player_decisions):

    evaluator = Evaluator()
    results = []
    for index in range(4):
        results+=simulate_game(num_games, index, player = naive_player_decisions,evaluator=evaluator)
    filename = f"game_results.csv"
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        columns = ['Hand 2', 'Hand 3', 'Hand 0', 'Hand 1']
        for i in [2,3,0,1]:
            if i == index:
                columns.append('Strategic Player')
            else:
                columns.append(f'Player {i} Decision')
        for i in [2,3,0,1]:
            if i == index:
                columns.append('win_rate')
            else:
                columns.append(f'win_rate {i}')
        writer.writerow(columns)
        for i, (hands, decisions, win_rates) in enumerate(results):
            writer.writerow(hands + decisions + win_rates)
# Function to calculate exploitability

# Call the baseline test function
import time
if __name__ == "__main__":
    aggregated_data = aggregate_hand_data_from_files()
    start_time = time.time()
    player = hard_player_decisions
    # save_to_csv(1, 10000,player= player)
    # save_to_csv(2, 10000,player= player)
    # save_to_csv(3, 10000,player= player)
    # save_to_csv(0, 10000,player= player)
    #save_to_csv_general(100000)
    exploitability = calculate_exploitability(100000, 2, naive_player_decisions)
    print(f"Exploitability of the strategy: {exploitability}")
    end_time = time.time()
    print(f"Time taken: {(end_time - start_time)} seconds")
