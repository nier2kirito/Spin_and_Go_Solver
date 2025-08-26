import pandas as pd
import sys

def extract_cluster_info(input_file, output_file):
    """
    Extract unique clusters with their average equity values from a CSV file.
    
    Args:
        input_file (str): Path to the input CSV file
        output_file (str): Path to the output CSV file
    """
    try:
        # Read the input CSV file
        df = pd.read_csv(input_file)
        
        # Check if required columns exist
        required_columns = ['Cluster', 'avg_equity2P', 'avg_equity3P']
        missing_columns = [col for col in required_columns if col not in df.columns]
        
        if missing_columns:
            print(f"Error: Missing required columns: {', '.join(missing_columns)}")
            return
        
        # Group by Cluster and get the first occurrence of avg_equity values
        # (since they should be the same for each cluster)
        cluster_info = df.groupby('Cluster').first().reset_index()
        
        # Select only the columns we need
        cluster_info = cluster_info[['Cluster', 'avg_equity2P', 'avg_equity3P']]
        
        # Save to output CSV file
        cluster_info.to_csv(output_file, index=False)
        
        print(f"Successfully extracted {len(cluster_info)} unique clusters to {output_file}")
        
    except Exception as e:
        print(f"Error processing file: {e}")

if __name__ == "__main__":
    # Check if correct number of arguments is provided
    if len(sys.argv) != 3:
        print("Usage: python extract_clusters_mapping.py <input_csv_file> <output_csv_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    extract_cluster_info(input_file, output_file)
