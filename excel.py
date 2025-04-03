import pandas as pd
import glob

# # Define the order of the files
# file_order = [
#     "malleole", "albatros", "autruche", "bengali", "coucou", "dindon",
#     "epervier", "faisan", "sternum", "phalange", "sacrum",
#     "bentley", "mazda", "nissan", "porsche"
# ]

# # Create a list to hold the DataFrames
# dataframes = []

# # Loop through the ordered file names and read the CSVs
# for name in file_order:
#     file_path = f"output_{name}.csv"
#     try:
#         df = pd.read_csv(file_path)
#         dataframes.append(df)
#     except FileNotFoundError:
#         print(f"File {file_path} not found.")

# # Concatenate all DataFrames
# final_df = pd.concat(dataframes, ignore_index=True)

# # Save the final output to a CSV file
# final_df.to_csv("final_output.csv", index=False)
# Load the CSV files
remaining_data = pd.read_csv('output_remaining.csv')
final_data = pd.read_csv('final_output.csv')

# Append the remaining data to the final data
final_data = pd.concat([final_data, remaining_data], ignore_index=True)

# Save the updated final data back to the CSV
final_data.to_csv('final_output.csv', index=False)