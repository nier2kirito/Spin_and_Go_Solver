#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

int main(int argc, char* argv[]) {
    // Check if a filename was provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <csv_filename>" << std::endl;
        return 1;
    }

    // Open the CSV file
    std::string filename = argv[1];
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    // Map to store line counts
    std::unordered_map<std::string, int> line_counts;
    std::string line;
    int line_number = 0;
    bool duplicates_found = false;

    // Read the file line by line
    while (std::getline(file, line)) {
        line_number++;
        line_counts[line]++;
        
        // If this is the second occurrence, report it
        if (line_counts[line] == 2) {
            std::cout << "Duplicate found: \"" << line << "\"" << std::endl;
            duplicates_found = true;
        }
    }

    // Report the result
    if (!duplicates_found) {
        std::cout << "No duplicates found in " << filename << std::endl;
        return 0;
    } else {
        std::cout << "Duplicates found in " << filename << std::endl;
        return 1;  // Return non-zero to indicate duplicates were found
    }
}
