import pandas as pd
import matplotlib.pyplot as plt

average_win_rate = 0
for strategic_player_index in [0, 1, 2, 3]:
    df = pd.read_csv(f'game_results_{strategic_player_index}.csv')

    # Print the DataFrame to check values
    print(f"Data for Strategic Player {strategic_player_index}:")

    average_win_rates = df[['win_rate 2', 'win_rate 3', 'win_rate 0', 'win_rate 1']].sum()

    # Calculate the cumulative sum for the strategic player's win rate
    strategic_win_rate_column = f'win_rate {strategic_player_index}'
    df['cumulative_win_rate'] = df[strategic_win_rate_column].cumsum()


    # Plotting the cumulative sum of the strategic player's win rate
    plt.figure(figsize=(10, 6))
    plt.plot(df['cumulative_win_rate'], label=f'Cumulative Win Rate of Strategic Player {strategic_player_index}', color='blue')
    plt.title(f'Cumulative Sum of Win Rate for Strategic Player {strategic_player_index}')
    plt.xlabel('Game Number')
    plt.ylabel('Cumulative Win Rate')
    plt.axhline(0, color='red', linewidth=1)
    plt.legend()
    plt.grid()
    plt.savefig(f'cumulative_win_rate_strategic_player_{strategic_player_index}.png')  # Save the plot as a PNG file
    plt.close()  # Close the plot to free up memory

    # Print the final cumulative value
    final_cumulative_value = df['cumulative_win_rate'].iloc[-1]
    print(f"Final Cumulative Win Rate for Strategic Player {strategic_player_index}: {final_cumulative_value}")
    average_win_rate += final_cumulative_value

print(f"Final averaged Cumulative Win Rate for Strategic Gameplay /100 hands: {average_win_rate/(4*1000000)*100}/100 BB")
