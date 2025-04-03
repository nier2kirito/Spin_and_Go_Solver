import matplotlib.pyplot as plt
import numpy as np
import re
from collections import defaultdict

# mapping = { 'P0:[P1:P][P2:A][P3:A]' : 0,
#             'P0:[P1:P][P2:A][P3:F]' : 1,
#             'P0:[P1:P][P2:F][P3:F]' : 2,
#             'P0:[P1:P][P2:F][P3:A]' : 3,
#             'P1:[P0:A][P2:F][P3:F]' : 4,
#             'P1:[P0:A][P2:F][P3:A]' : 5,        
#             'P1:[P0:A][P2:A][P3:F]' : 6,
#             'P1:[P0:A][P2:A][P3:A]' : 7,
#             'P1:[P0:F][P2:A][P3:F]' : 8,
#             'P1:[P0:F][P2:F][P3:A]' : 9,
#             'P1:[P0:F][P2:A][P3:A]' : 10,
#             'P2:[P0:P][P1:P]' : 11,
#             'P3:[P0:P][P1:P][P2:F]': 12,
#             'P3:[P0:P][P1:P][P2:A]': 13
# }
# Function to parse the infosets.txt file
def parse_infosets(file_path):
    pattern = r"(.*\])[^]]*$"  

    hand_data = {}
    with open(file_path, 'r') as file:
        lines = file.readlines()
        for line in lines:

            if "InfoSet" in line:
                # Split the line to extract the relevant parts
                decisions = re.sub(pattern, r"\1", line.strip().split(' ')[1])
                # Find the index of the last ']'
                last_bracket_index = line.rfind(']')

                # If "Pot:" is always present, you can locate its index; otherwise, extract to the end.
                pot_index = line.find("Pot:")
                if pot_index == -1:
                    card_section = line[last_bracket_index + 1:].strip()
                else:
                    card_section = line[last_bracket_index + 1:pot_index].strip()


            # Skip empty lines and lines without "Strategy"
            if "Strategy" not in line:
                continue
            
            # Split the line into parts
            parts = line.strip().split(' ')
            
            # Extract fold and all-in probabilities
            strategy = parts[-2:]  # Last two elements are the strategy probabilities
            fold_prob = float(strategy[0])  # Fold probability
            all_in_prob = float(strategy[1])  # All-in probability
            
            hand_data[(decisions,card_section)] = (fold_prob, all_in_prob)
    
    return hand_data

# Function to map hands to matrix position
def hand_to_position(hand):
    ranks = '23456789TJQKA'
    
    try:
        rank1, rank2 = hand.split(' ')
        rank2 = rank2[:-1]
        
        if rank1 == '10':
            rank1 = 'T'
        if rank2 == '10':
            rank2 = 'T'
        # Check if the hand is suited or offsuit
        is_suited = (hand[-1] == 's')
        
        # Get rank indices
        rank1_value = ranks.index(rank1[0])
        rank2_value = ranks.index(rank2[0])
        
        # Return position for suited (above diagonal) or offsuit (below diagonal)
        if is_suited:
            return (rank1_value, rank2_value) if rank1_value < rank2_value else (rank2_value, rank1_value)
        else:
            return (rank1_value, rank2_value) if rank1_value > rank2_value else (rank2_value, rank1_value)
    
    except ValueError:
        # In case of invalid hands (e.g., malformed input), return invalid position
        return None
    

def aggregate_hand_data(hand_datas):
    if not hand_datas:
        raise ValueError("hand_datas is empty. Cannot aggregate hand data.")

    hand_data_final = {}
    
    # Initialize hand_data_final with the first hand_data's items
    for (decisions, hand), (fold_prob, all_in_prob) in hand_datas[0].items():
        hand_data_final[(decisions, hand)] = (0, 0)

    # Aggregate data from all hand_data entries
    for hand_data in hand_datas:
        for (decisions, hand), (fold_prob, all_in_prob) in hand_data.items():
            if (decisions, hand) in hand_data_final:
                # Update the cumulative values
                current_fold_prob, current_all_in_prob = hand_data_final[(decisions, hand)]
                hand_data_final[(decisions, hand)] = (
                    current_fold_prob + fold_prob,
                    current_all_in_prob + all_in_prob
                )
            else:
                # If the key doesn't exist, initialize it
                hand_data_final[(decisions, hand)] = (fold_prob, all_in_prob)

    # Average the results
    total_hand_datas = len(hand_datas)
    for key in hand_data_final:
        fold_prob_sum, all_in_prob_sum = hand_data_final[key]
        hand_data_final[key] = (
            fold_prob_sum / total_hand_datas,
            all_in_prob_sum / total_hand_datas
        )

    return hand_data_final

# Main function
import glob

def aggregate_hand_data_from_files():
    file_paths = glob.glob('infosets_*.txt')
    hand_data_all = []
    for file_path in file_paths:
        hand_data_all.append(parse_infosets(file_path))
    hand_data_final = aggregate_hand_data(hand_data_all)
    return hand_data_final

# def main():
#     hand_data_final = aggregate_hand_data_from_files()
#     print(hand_data_final[('P0:[P1:P][P2:F][P3:F]','10 As')])

# main()