import pandas as pd
import random
import matplotlib.pyplot as plt
# strategic_player_index = 1
# df = pd.read_csv(f'game_results_{strategic_player_index}.csv')

# average_win_rates = df[['win_rate 2', 'win_rate 3', 'win_rate 0', 'win_rate 1']].sum()
# print("Average Win Rates:")
# for player_index in [2, 3, 0, 1]:
#     if player_index == strategic_player_index:
#         print(f"Strategic Player {player_index}: {average_win_rates.iloc[player_index]}")
#     else:
#      print(f"Player {player_index}: {average_win_rates.iloc[player_index]}")

df = pd.read_csv(f'game_results.csv')

average_win_rate = df['win_rate'].sum()
print("Average Win Rate:", average_win_rate)


# Read the CSV file
df = pd.read_csv('game_results.csv')

# Calculate the cumulative sum of the win rates
# Assuming 'win_rate' is the column name for win rates
df['cumulative_win_rate'] = df['win_rate'].cumsum()

# Plotting the cumulative sum of average win rate
plt.figure(figsize=(10, 6))
plt.plot(df['cumulative_win_rate'], label='Cumulative Win Rate', color='blue')
plt.title('Cumulative Sum of Average Win Rate')
plt.xlabel('Game Number')
plt.ylabel('Cumulative Win Rate')
plt.legend()
plt.axhline(0, color='red', linewidth=1)
plt.grid()
plt.savefig('cumulative_win_rate.png')  # Save the plot as a PNG file

winners = [2]
starting_stacks = [8,8,7.5,7]
rake = 0.03
print(len(winners))
print((sum([starting_stacks[j] for j in range(4) if j in winners]) + 1.5 - rake) / 1)
