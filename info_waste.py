import pandas as pd
import numpy as np

# Load the two CSV files
csv1 = pd.read_csv("./utils/out.csv")  # First CSV
csv2 = pd.read_csv("./utils/out copy.csv")  # Second CSV


# Ensure numeric conversion, handling errors by coercing invalid values to NaN
for col in ["Equity2P", "Equity3P"]:
    csv1[col] = pd.to_numeric(csv1[col], errors="coerce")
    csv2[col] = pd.to_numeric(csv2[col], errors="coerce")

# Merge the two CSVs on the 'Hand' column
merged = csv1.merge(csv2, on="Hand", suffixes=('_old', '_new'))

# Ensure the new columns are numeric
for col in ["Equity2P_old", "Equity2P_new", "Equity3P_old", "Equity3P_new"]:
    merged[col] = pd.to_numeric(merged[col], errors="coerce")

# Compute the differences in Equity2P and Equity3P
merged["Equity2P_diff"] = merged["Equity2P_old"] - merged["Equity2P_new"]
merged["Equity3P_diff"] = merged["Equity3P_old"] - merged["Equity3P_new"]

# Compute Mean Squared Error (MSE) and Mean Absolute Error (MAE)
mse_2p = np.nanmean(merged["Equity2P_diff"] ** 2)  # Use nanmean to ignore NaN values
mae_2p = np.nanmean(abs(merged["Equity2P_diff"]))

mse_3p = np.nanmean(merged["Equity3P_diff"] ** 2)
mae_3p = np.nanmean(abs(merged["Equity3P_diff"]))

# Print the results
print(f"Equity2P Loss - MSE: {mse_2p:.6f}, MAE: {mae_2p:.6f}")
print(f"Equity3P Loss - MSE: {mse_3p:.6f}, MAE: {mae_3p:.6f}")