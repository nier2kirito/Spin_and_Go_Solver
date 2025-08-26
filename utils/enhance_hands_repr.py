import csv

def parse_hand_ranks(hand):
    # Split the hand into individual cards
    cards = hand.split(' ')
    
    # Extract ranks from the cards
    ranks = [card[:-1] for card in cards]  # Assuming last character is the suit
    
    # Count occurrences of each rank
    rank_counts = {}
    for rank in ranks:
        if rank in rank_counts:
            rank_counts[rank] += 1
        else:
            rank_counts[rank] = 1
    
    # Sort the rank counts and return as a tuple
    sorted_rank_counts = sorted(tuple(rank_counts.values()),reverse=True)
    return sorted_rank_counts

def parse_hand(hand):
    # Split the hand into individual cards
    cards = hand.split(' ')
    
    # Extract ranks from the cards
    ranks = [card[:-1] for card in cards]  # Assuming last character is the suit
    
    # Count occurrences of each rank
    rank_counts = {}
    for rank in ranks:
        if rank in rank_counts:
            rank_counts[rank] += 1
        else:
            rank_counts[rank] = 1

    return rank_counts


def suit_pattern(hand):
    rank_pattern = parse_hand_ranks(hand)
    suit_arrangement = [card[-1] for card in hand.split(' ')]
    # Create a "signature" for each arrangement based on where each suit appears
    signature = {}
    
    pos = 0
    for group_idx, group_size in enumerate(rank_pattern):
        for i in range(group_size):
            suit = suit_arrangement[pos + i]
            
            if suit not in signature:
                signature[suit] = []
            
            signature[suit].append(group_idx)
        
        pos += group_size
    
    # Sort the signatures by their patterns
    pattern = sorted(signature.values())
    return pattern

# Example usage
if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        csv_file = sys.argv[1]
        output_file = 'repr_'+csv_file  # Define the output file name
        with open(csv_file, mode='r') as file:
            reader = csv.DictReader(file)
            fieldnames = reader.fieldnames + ['rank_pattern', 'suit_pattern']  # Add new columns
            rows = []
            for row in reader:
                hand = row['Hand']
                rank_pattern = parse_hand(hand)  # Get rank pattern
                suit_pattern_value = suit_pattern(hand)  # Get suit pattern
                row['rank_pattern'] = rank_pattern  # Add rank pattern to row
                row['suit_pattern'] = suit_pattern_value  # Add suit pattern to row
                rows.append(row)
        
        # Write the updated rows to a new CSV file
        with open(output_file, mode='w', newline='') as file:
            writer = csv.DictWriter(file, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)
