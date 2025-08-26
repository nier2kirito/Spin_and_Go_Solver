# Spin and Go Poker Solver with Rake Abstraction

This is a comprehensive poker solver specifically designed for 3-player Spin and Go tournaments with rake consideration. The codebase implements Monte Carlo Counterfactual Regret Minimization (MCCFR) for finding optimal strategies in poker games with complex abstractions.

## Overview

The project solves 3-player Spin and Go poker games where each player starts with 15 big blinds. It uses advanced abstractions to handle the enormous state space of poker, including:
- **Hand clustering** using K-means clustering with both L2 and Earth Mover's Distance (EMD) metrics
- **Equity calculations** for all betting rounds (flop, turn, river)
- **Canonical representations** to reduce isomorphic hand combinations
- **Strategy aggregation** from multiple training runs

## Project Structure

### Core Game Engine (`spingo/`)

#### `spingo.h` & `spingo.cpp`
- **Purpose**: Core poker game engine implementing the Spin and Go game rules
- **Key Features**:
  - 3-player game with 15 BB starting stacks
  - Complete betting round implementation (preflop, flop, turn, river, showdown)
  - Action definitions (fold, check, call, bet sizing from 1x to 8x, all-in)
  - Game state management and transitions
  - Rake and jackpot fee calculations

#### `mccfr.h` & `mccfr.cpp`
- **Purpose**: Monte Carlo Counterfactual Regret Minimization implementation
- **Key Features**:
  - MCCFR training algorithm for finding optimal strategies
  - Information set management
  - Regret accumulation and strategy updates
  - Node-based game tree representation

### Training Executables

#### `main.cpp`
- **Purpose**: Basic training entry point
- **Features**: Simple MCCFR training with 20 million iterations
- **Output**: Saves information sets to timestamped files

#### `train.cpp`
- **Purpose**: Standard training implementation
- **Features**: Basic MCCFR training with configurable parameters

#### `train_enhanced.cpp`
- **Purpose**: Enhanced training with additional features
- **Features**: Improved MCCFR implementation with better performance

#### `train_optimized.cpp`
- **Purpose**: Optimized training for better performance
- **Features**: Performance optimizations and parallel processing

#### `train_optimized_google_drive.cpp`
- **Purpose**: Google Drive-integrated training (largest file)
- **Features**: Advanced training with external storage integration
- **Size**: 62KB - most comprehensive training implementation

### Strategy Management (`strategies/`)

#### `aggregate.cpp`
- **Purpose**: Aggregates strategies from multiple training runs
- **Key Features**:
  - Combines strategies from different training sessions
  - Weighted averaging based on strategy update counts
  - Handles different betting rounds and player positions
  - Outputs consolidated strategy files
- **Input**: Multiple strategy CSV files
- **Output**: Single aggregated strategy file

### Utility Functions (`utils/`)

#### Equity Calculation Files

##### `equity_flop.cpp`
- **Purpose**: Calculates equity for flop betting rounds
- **Key Features**:
  - Generates canonical flop representations
  - Calculates equity against random opponents
  - Handles 2-player and 3-player scenarios
  - Uses 1 billion random flop samples
  - Outputs equity data for clustering

##### `equity_turn.cpp`
- **Purpose**: Calculates equity for turn betting rounds
- **Key Features**:
  - Turn-specific equity calculations
  - Canonical turn representations
  - Large-scale equity computation

##### `equity_river.cpp`
- **Purpose**: Calculates equity for river betting rounds
- **Key Features**:
  - River-specific equity calculations
  - Final hand evaluation
  - Largest equity calculation file (95KB)

##### `equity_flop_calculation.cpp`
- **Purpose**: Core flop equity calculation logic
- **Features**: Modular equity computation functions

##### `equity_turn_calculation.cpp`
- **Purpose**: Core turn equity calculation logic
- **Features**: Modular turn equity computation functions

##### `equity_river_calculation.cpp`
- **Purpose**: Core river equity calculation logic
- **Features**: Modular river equity computation functions

#### Clustering and Abstraction

##### `Kmeans_rounds.cpp`
- **Purpose**: K-means clustering for hand abstraction
- **Key Features**:
  - Clusters hands based on equity similarity
  - Supports both L2 distance and Earth Mover's Distance (EMD)
  - OpenMP parallelization for performance
  - Handles flop, turn, and river rounds
  - Outputs cluster assignments and centroids

##### `average_equity_clusters.cpp`
- **Purpose**: Calculates average equity for each cluster
- **Features**: Aggregates equity data within clusters

#### Poker Evaluation

##### `poker_evaluator.cpp`
- **Purpose**: Core poker hand evaluation
- **Features**:
  - 5-card hand ranking (high card to straight flush)
  - Hand comparison and scoring
  - Combination generation for evaluation

#### Python Utilities

##### `extract_clusters_mapping.py`
- **Purpose**: Extracts and processes cluster mappings
- **Features**: CSV processing for cluster data

##### `enhance_hands_repr.py`
- **Purpose**: Enhances hand representations
- **Features**: Hand data preprocessing and enhancement

##### `canonical_representations.py`
- **Purpose**: Generates canonical hand representations
- **Key Features**:
  - Eliminates isomorphic hand combinations
  - Handles suit and rank patterns
  - Reduces state space for abstraction
  - Supports 7-card river hands

### Testing and Validation

#### `test_uniqueness_infosets.cpp`
- **Purpose**: Tests uniqueness of information sets
- **Features**: Validation of information set generation

#### `infosets_mpi.cpp`
- **Purpose**: MPI-based information set processing
- **Features**: Parallel information set generation

### Build System

#### `CMakeLists.txt`
- **Purpose**: CMake build configuration
- **Features**:
  - C++17 standard requirement
  - OpenMP and pthread support
  - Static library creation for spingo
  - Optimized compilation flags

## Download CSV Files

You can download the necessary CSV files from the following link and place them in the `utils/` folder:

[https://www.dropbox.com/scl/fo/ln3e95uryi8skr46b1v7y/ABgAZgyl1Pu0VJpHhVOqSr8?rlkey=7ajiuhns3vxxomliyhq03pqh7&st=e1bb272d&dl=0](https://www.dropbox.com/scl/fo/ln3e95uryi8skr46b1v7y/ABgAZgyl1Pu0VJpHhVOqSr8?rlkey=7ajiuhns3vxxomliyhq03pqh7&st=e1bb272d&dl=0)
The system requires several CSV files for proper operation. These files contain pre-computed equity data and cluster mappings:

### Cluster Mapping Files
These files map hands to clusters and contain average equity values:

- **`emd_flop_clusters_mapping.csv`** (21KB)
  - Maps flop hands to EMD-based clusters
  - Contains average equity for 2-player and 3-player scenarios
  - Used for flop betting round abstractions

- **`emd_turn_clusters_mapping.csv`** (21KB)
  - Maps turn hands to EMD-based clusters
  - Used for turn betting round abstractions

- **`emd_river_clusters_mapping.csv`** (21KB)
  - Maps river hands to EMD-based clusters
  - Used for river betting round abstractions

- **`l2_flop_clusters_mapping.csv`** (21KB)
  - Maps flop hands to L2-distance-based clusters
  - Alternative clustering method for flop rounds

- **`l2_turn_clusters_mapping.csv`** (21KB)
  - Maps turn hands to L2-distance-based clusters
  - Alternative clustering method for turn rounds

- **`l2_river_clusters_mapping.csv`** (21KB)
  - Maps river hands to L2-distance-based clusters
  - Alternative clustering method for river rounds

### Equity Representation Files
These large files contain detailed equity data for clustered hands:

- **`repr_emd_flop_equities_clustered_hands_with_avg_equity.csv`** (15MB)
  - Contains flop hand representations with EMD clustering
  - Includes equity values, cluster assignments, and canonical representations

- **`repr_emd_turn_equities_clustered_hands_with_avg_equity.csv`** (117MB)
  - Contains turn hand representations with EMD clustering
  - Larger file due to increased hand combinations

- **`repr_emd_river_equities_clustered_hands_with_avg_equity.csv`** (812MB)
  - Contains river hand representations with EMD clustering
  - Largest file due to 7-card hand combinations

- **`repr_l2_flop_equities_clustered_hands_with_avg_equity.csv`** (15MB)
  - Contains flop hand representations with L2 clustering

- **`repr_l2_turn_equities_clustered_hands_with_avg_equity.csv`** (117MB)
  - Contains turn hand representations with L2 clustering

- **`repr_l2_river_equities_clustered_hands_with_avg_equity.csv`** (813MB)
  - Contains river hand representations with L2 clustering

## How to Use

### Prerequisites
- C++17 compatible compiler
- CMake 3.10 or higher
- OpenMP support
- Python 3.x (for utility scripts)

### Building
```bash
mkdir build
cd build
cmake ..
make
```

### Training
```bash
# Basic training
./train

# Enhanced training
./train_enhanced

# Optimized training
./train_optimized

# Google Drive integrated training
./train_optimized_google_drive
```

### Strategy Aggregation
```bash
# Aggregate strategies from multiple files
./aggregate input1.csv input2.csv output.csv
```

### Equity Calculation
```bash
# Calculate flop equities
./equity_flop

# Calculate turn equities  
./equity_turn

# Calculate river equities
./equity_river
```

### Clustering
```bash
# Cluster hands for specific rounds
./kmeans_rounds flop
./kmeans_rounds turn
./kmeans_rounds river
```

## Key Concepts

### Hand Abstraction
The system uses K-means clustering to group similar hands based on equity, reducing the state space from trillions of possible hands to manageable clusters.

### Canonical Representations
Hands are converted to canonical forms to eliminate isomorphic combinations (e.g., AhKh and AsKs are treated as equivalent).

### Equity Calculation
Pre-computed equity values for all possible hand combinations against random opponents, enabling fast strategy evaluation.

### MCCFR Training
Monte Carlo Counterfactual Regret Minimization finds optimal strategies by minimizing regret over many training iterations.

## Performance Considerations

- **Parallel Processing**: OpenMP is used for equity calculations and clustering
- **Memory Management**: Large CSV files are processed efficiently
- **Optimization**: Multiple training implementations with varying performance characteristics
- **Storage**: Total CSV data size is approximately 2.5GB

## File Size Summary

- **Small files** (< 50KB): Core game logic, basic utilities
- **Medium files** (50KB - 1MB): Training implementations, equity calculations
- **Large files** (1MB - 100MB): Turn and river equity data
- **Very large files** (100MB+): River equity data with full 7-card combinations

This codebase represents a sophisticated approach to solving complex poker games through advanced abstractions, efficient clustering, and state-of-the-art game theory algorithms.

