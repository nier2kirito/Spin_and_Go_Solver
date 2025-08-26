import itertools

#River
suits = ['c', 'd', 'h', 's']
ranks = ['2', '3', '4', '5', '6', '7', '8', '9', 't', 'j', 'q', 'k', 'a']

partitions = [
    (4, 3),
    (4, 2, 1),
    (4, 1, 1, 1),
    (3, 3, 1),
    (3, 2, 2),
    (3, 2, 1, 1),
    (3, 1, 1, 1, 1),
    (2, 2, 2, 1),
    (2, 2, 1, 1, 1),
    (2, 1, 1, 1, 1, 1),
    (1, 1, 1, 1, 1, 1, 1)
]
dictionary = {}
for partition in partitions:
    dictionary[partition] = 0

rankcounts = [0] * len(ranks)
for i in range(len(ranks)):
    for j in range(i,len(ranks)):
        for k in range(j,len(ranks)):
            for l in range(k,len(ranks)):
                for m in range(l,len(ranks)):
                    for n in range(m,len(ranks)):
                            for o in range(n,len(ranks)):
                                rankcounts[i] += 1
                                rankcounts[j] += 1
                                rankcounts[k] += 1
                                rankcounts[l] += 1
                                rankcounts[m] += 1
                                rankcounts[n] += 1
                                rankcounts[o] += 1
                                if max(rankcounts) <= 4:
                                    partition = tuple(sorted([rankcount for rankcount in rankcounts if rankcount > 0], reverse=True))
                                    # print(partition)
                                    dictionary[partition] += 1
                                rankcounts = [0] * len(ranks)

def is_isomorphic(suit_arrangement1, suit_arrangement2, rank_pattern):
    # If lengths don't match, they can't be isomorphic
    if len(suit_arrangement1) != len(suit_arrangement2):
        return False
    
    # Create a "signature" for each arrangement based on where each suit appears
    signature1 = {}
    signature2 = {}
    
    pos = 0
    for group_idx, group_size in enumerate(rank_pattern):
        for i in range(group_size):
            suit1 = suit_arrangement1[pos + i]
            suit2 = suit_arrangement2[pos + i]
            
            if suit1 not in signature1:
                signature1[suit1] = []
            if suit2 not in signature2:
                signature2[suit2] = []
            
            signature1[suit1].append(group_idx)
            signature2[suit2].append(group_idx)
        
        pos += group_size
    
    # Sort the signatures by their patterns
    pattern1 = sorted(signature1.values())
    pattern2 = sorted(signature2.values())
    # If the patterns match, the arrangements are isomorphic
    return pattern1 == pattern2

def suit_arrangement(partition):
    suits_combinations = []
    for index in range(len(partition)):
        count = partition[index]
        if index == 0:
            for combination in itertools.permutations(suits, count):
                suits_combinations.append(tuple(sorted(combination)))
        else:
            new_combinations = []
            for suit_combination in suits_combinations:
                for combination in itertools.permutations(suits, count):
                    new_suit_combination = suit_combination + tuple(sorted(combination))
                    new_combinations.append(new_suit_combination)
            suits_combinations = new_combinations
    results = sorted(list(set(suits_combinations)))
    
    # Filter out isomorphic arrangements
    unique_arrangements = []
    for result in results:
        is_unique = True
        for unique in unique_arrangements:
            if is_isomorphic(result, unique, partition):
                is_unique = False
                break
        if is_unique:
            unique_arrangements.append(result)
    
    return unique_arrangements
def format_suit_arrangements(suit_arrangements):
    """Format suit arrangements in the desired output format."""
    formatted_arrangements = []
    for arrangement in suit_arrangements:
        # Convert tuple of suits to comma-separated string with quotes
        suits_str = ", ".join([f'"{suit}"' for suit in arrangement])
        formatted_arrangements.append(f"{{{suits_str}}}")
    
    # Join all arrangements with commas and newlines
    return ",\n            ".join(formatted_arrangements)
counter = 0
for partition in partitions:
    frequency = dictionary[partition]
    suit_arrangements = suit_arrangement(partition)
    
    # Format the rank pattern as a comma-separated string
    rank_pattern_str = ",".join(str(x) for x in partition)
    
    # Open a file to write the output
    with open('output.txt', 'a') as f:
        f.write('******************************\n')
        f.write(f'// {partition}\n')
        f.write(f'rankPatternToSuitPatterns["{rank_pattern_str}"] = {{\n')
        f.write(f'    {format_suit_arrangements(suit_arrangements)}\n')
        f.write('};\n')
    
    counter += frequency * len(suit_arrangements)

# Write the final counter to the file
with open('output.txt', 'a') as f:
    f.write(f'{counter}\n')

print(f"Output saved to output.txt with total count: {counter}")

#Turn 

partitions = [
    (4, 2),
    (4, 1, 1),
    (3, 3),
    (3, 2, 1),
    (3, 1, 1, 1),
    (2, 2, 2),
    (2, 2, 1, 1),
    (2, 1, 1, 1, 1),
    (1, 1, 1, 1, 1, 1)
]
dictionary = {}
for partition in partitions:
    dictionary[partition] = 0

rankcounts = [0] * len(ranks)
for i in range(len(ranks)):
    for j in range(i,len(ranks)):
        for k in range(j,len(ranks)):
            for l in range(k,len(ranks)):
                for m in range(l,len(ranks)):
                    for n in range(m,len(ranks)):
                                rankcounts[i] += 1
                                rankcounts[j] += 1
                                rankcounts[k] += 1
                                rankcounts[l] += 1
                                rankcounts[m] += 1
                                rankcounts[n] += 1
                                if max(rankcounts) <= 4:
                                    partition = tuple(sorted([rankcount for rankcount in rankcounts if rankcount > 0], reverse=True))
                                    # print(partition)
                                    dictionary[partition] += 1
                                rankcounts = [0] * len(ranks)

def is_isomorphic(suit_arrangement1, suit_arrangement2, rank_pattern):
    # If lengths don't match, they can't be isomorphic
    if len(suit_arrangement1) != len(suit_arrangement2):
        return False
    
    # Create a "signature" for each arrangement based on where each suit appears
    signature1 = {}
    signature2 = {}
    
    pos = 0
    for group_idx, group_size in enumerate(rank_pattern):
        for i in range(group_size):
            suit1 = suit_arrangement1[pos + i]
            suit2 = suit_arrangement2[pos + i]
            
            if suit1 not in signature1:
                signature1[suit1] = []
            if suit2 not in signature2:
                signature2[suit2] = []
            
            signature1[suit1].append(group_idx)
            signature2[suit2].append(group_idx)
        
        pos += group_size
    
    # Sort the signatures by their patterns
    pattern1 = sorted(signature1.values())
    pattern2 = sorted(signature2.values())
    # If the patterns match, the arrangements are isomorphic
    return pattern1 == pattern2

def suit_arrangement(partition):
    suits_combinations = []
    for index in range(len(partition)):
        count = partition[index]
        if index == 0:
            for combination in itertools.permutations(suits, count):
                suits_combinations.append(tuple(sorted(combination)))
        else:
            new_combinations = []
            for suit_combination in suits_combinations:
                for combination in itertools.permutations(suits, count):
                    new_suit_combination = suit_combination + tuple(sorted(combination))
                    new_combinations.append(new_suit_combination)
            suits_combinations = new_combinations
    results = sorted(list(set(suits_combinations)))
    
    # Filter out isomorphic arrangements
    unique_arrangements = []
    for result in results:
        is_unique = True
        for unique in unique_arrangements:
            if is_isomorphic(result, unique, partition):
                is_unique = False
                break
        if is_unique:
            unique_arrangements.append(result)
    
    return unique_arrangements

def format_suit_arrangements(suit_arrangements):
    """Format suit arrangements in the desired output format."""
    formatted_arrangements = []
    for arrangement in suit_arrangements:
        # Convert tuple of suits to comma-separated string with quotes
        suits_str = ", ".join([f'"{suit}"' for suit in arrangement])
        formatted_arrangements.append(f"{{{suits_str}}}")
    
    # Join all arrangements with commas and newlines
    return ",\n            ".join(formatted_arrangements)

counter = 0
for partition in partitions:
    frequency = dictionary[partition]
    suit_arrangements = suit_arrangement(partition)
    print('******************************')
    
    # Format the rank pattern as a comma-separated string
    rank_pattern_str = ",".join(str(x) for x in partition)
    
    # Format and print in the desired style
    print(f'// {partition}')
    print(f'rankPatternToSuitPatterns["{rank_pattern_str}"] = {{')
    print(f'    {format_suit_arrangements(suit_arrangements)}')
    print('};')
    
    counter += frequency * len(suit_arrangements)
print(counter)

#Flop
partitions = [
    (4, 1),
    (3, 2),
    (3, 1, 1),
    (2, 2, 1),
    (2, 1, 1, 1),
    (1, 1, 1, 1, 1)
]
dictionary = {}
for partition in partitions:
    dictionary[partition] = 0

rankcounts = [0] * len(ranks)
for i in range(len(ranks)):
    for j in range(i,len(ranks)):
        for k in range(j,len(ranks)):
            for l in range(k,len(ranks)):
                for m in range(l,len(ranks)):
                            rankcounts[i] += 1
                            rankcounts[j] += 1
                            rankcounts[k] += 1
                            rankcounts[l] += 1
                            rankcounts[m] += 1
                            if max(rankcounts) <= 4:
                                partition = tuple(sorted([rankcount for rankcount in rankcounts if rankcount > 0], reverse=True))
                                # print(partition)
                                dictionary[partition] += 1
                            rankcounts = [0] * len(ranks)

def is_isomorphic(suit_arrangement1, suit_arrangement2, rank_pattern):
    # If lengths don't match, they can't be isomorphic
    if len(suit_arrangement1) != len(suit_arrangement2):
        return False
    
    # Create a "signature" for each arrangement based on where each suit appears
    signature1 = {}
    signature2 = {}
    
    pos = 0
    for group_idx, group_size in enumerate(rank_pattern):
        for i in range(group_size):
            suit1 = suit_arrangement1[pos + i]
            suit2 = suit_arrangement2[pos + i]
            
            if suit1 not in signature1:
                signature1[suit1] = []
            if suit2 not in signature2:
                signature2[suit2] = []
            
            signature1[suit1].append(group_idx)
            signature2[suit2].append(group_idx)
        
        pos += group_size
    
    # Sort the signatures by their patterns
    pattern1 = sorted(signature1.values())
    pattern2 = sorted(signature2.values())
    # If the patterns match, the arrangements are isomorphic
    return pattern1 == pattern2

def suit_arrangement(partition):
    suits_combinations = []
    for index in range(len(partition)):
        count = partition[index]
        if index == 0:
            for combination in itertools.permutations(suits, count):
                suits_combinations.append(tuple(sorted(combination)))
        else:
            new_combinations = []
            for suit_combination in suits_combinations:
                for combination in itertools.permutations(suits, count):
                    new_suit_combination = suit_combination + tuple(sorted(combination))
                    new_combinations.append(new_suit_combination)
            suits_combinations = new_combinations
    results = sorted(list(set(suits_combinations)))
    
    # Filter out isomorphic arrangements
    unique_arrangements = []
    for result in results:
        is_unique = True
        for unique in unique_arrangements:
            if is_isomorphic(result, unique, partition):
                is_unique = False
                break
        if is_unique:
            unique_arrangements.append(result)
    
    return unique_arrangements
def format_suit_arrangements(suit_arrangements):
    """Format suit arrangements in the desired output format."""
    formatted_arrangements = []
    for arrangement in suit_arrangements:
        # Convert tuple of suits to comma-separated string with quotes
        suits_str = ", ".join([f'"{suit}"' for suit in arrangement])
        formatted_arrangements.append(f"{{{suits_str}}}")
    
    # Join all arrangements with commas and newlines
    return ",\n            ".join(formatted_arrangements)
counter = 0
for partition in partitions:
    frequency = dictionary[partition]
    suit_arrangements = suit_arrangement(partition)
    print('******************************')
    
    # Format the rank pattern as a comma-separated string
    rank_pattern_str = ",".join(str(x) for x in partition)
    
    # Format and print in the desired style
    print(f'// {partition}')
    print(f'rankPatternToSuitPatterns["{rank_pattern_str}"] = {{')
    print(f'    {format_suit_arrangements(suit_arrangements)}')
    print('};')
    
    counter += frequency * len(suit_arrangements)
print(counter)