#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <execution>
#include <expected>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mt_quotes.h"

enum Error {
    LAYOUT_PARSE_ERROR_INVALID_FILE,
    CORPUS_ERROR_INVALID_FILE,
};

enum class Finger : std::uint8_t {
    LP = 0,
    LR = 1,
    LM = 2,
    LI = 3,
    LT = 4,
    RT = 5,
    RI = 6,
    RM = 7,
    RR = 8,
    RP = 9,
    TB = 10,
};

enum class Hand : std::uint8_t {
    LEFT = 0,
    RIGHT = 1,
};

struct Key {
    char value = '\0';
    int row;
    int column;
    Finger finger;
    Hand hand;
};

struct KeyboardLayout {
    std::string name;
    std::unordered_map<char, Key> char_to_key;
    std::array<std::array<Key, 10>, 4> matrix;
    std::unordered_set<char> valid_keys;

    double score = 0;

    void print() {
        std::cout << name << ": \n\n"; 

        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 10; col++) {
                std::cout << matrix[row][col].value << " ";
            }
            std::cout << "\n";
        }
        std::cout << "Score:" << score << "\n";
    }
};

Finger get_finger(int col, Hand hand) {
    if (hand == Hand::LEFT) {
        switch (col) {
        case 0: return Finger::LP; // q, a, z
        case 1: return Finger::LR; // w, s, x
        case 2: return Finger::LM; // e, d, c
        case 3: return Finger::LI; // r, f, v
        case 4: return Finger::LI; // t, g, b (index finger handles two columns)
        default: return Finger::LI;
        }
    } else {
        switch (col) {
        case 0: return Finger::RI; // y, h, n (right index handles two columns)
        case 1: return Finger::RI; // u, j, m
        case 2: return Finger::RM; // i, k, comma
        case 3: return Finger::RR; // o, l, period
        case 4: return Finger::RP; // p, semicolon, slash
        default: return Finger::RI;
        }
    }
}

std::expected<KeyboardLayout, Error> load_layout(const std::string &file) {
    KeyboardLayout layout;
    std::string layout_file = "../layouts/" + file;

    std::ifstream layout_file_stream(layout_file);
    if (!layout_file_stream) {
        return std::unexpected(LAYOUT_PARSE_ERROR_INVALID_FILE);
    }

    std::getline(layout_file_stream, layout.name, ':');

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 10; c++) {
            layout.matrix[r][c] = {'\0', r, c, Finger::LI, Hand::LEFT};
        }
    }

    std::string line;
    int row = 0;

    while (std::getline(layout_file_stream, line) && row < 4) {
        if (line.empty())
            continue;

        size_t pipe_pos = line.find('|');
        std::string left_side = line.substr(0, pipe_pos);
        std::string right_side =
            (pipe_pos != std::string::npos) ? line.substr(pipe_pos + 1) : "";

        int left_col = 0;
        for (char ch : left_side) {
            if (ch != ' ' && left_col < 5) {
                Key key = {ch, row, left_col, get_finger(left_col, Hand::LEFT),
                           Hand::LEFT};
                layout.char_to_key[ch] = key;
                layout.matrix[row][left_col] = key;
                left_col++;
            }
        }

        int right_col = 0;
        for (char ch : right_side) {
            if (ch != ' ' && right_col < 5) {
                Key key = {ch, row, right_col + 5,
                           get_finger(right_col, Hand::RIGHT), Hand::RIGHT};
                layout.char_to_key[ch] = key;
                layout.matrix[row][right_col + 5] = key;
                right_col++;
            }
        }

        row++;
    }

    Key left_space = {' ', 3, 4, Finger::LT, Hand::LEFT};
    Key right_space = {' ', 3, 5, Finger::RT, Hand::RIGHT};
    layout.char_to_key[' '] = left_space;
    layout.matrix[3][4] = left_space;
    layout.matrix[3][5] = right_space;

    layout.valid_keys.clear();
    for (const auto &[ch, key] : layout.char_to_key) {
        layout.valid_keys.insert(ch);
    }

    return layout;
}

template <std::size_t T>
void get_stats(KeyboardLayout &layout, const std::array<char, T> &corpus) {
    constexpr double SFB_WEIGHT = 1;
    constexpr double SFS_WEIGHT = 1;

    double sfb = 0;
    double total_bigram_count = 1;

    int sfs = 0;
    int total_sfs_count = 1;

    for (std::size_t i = 0; i + 1 < T; ++i) {
        char first = corpus[i];
        char second = corpus[i + 1];

        if (layout.valid_keys.find(first) == layout.valid_keys.end() ||
            layout.valid_keys.find(second) == layout.valid_keys.end() ||
            first == ' ' || second == ' ')
            continue;

        total_bigram_count++;

        if (first == second) continue;
        
        if (layout.char_to_key.at(first).finger == layout.char_to_key.at(second).finger)
            sfb++;
    }

    double sfb_score = (sfb * 100) / total_bigram_count;

    layout.score = sfb_score * SFB_WEIGHT;
}

void swap_keys(KeyboardLayout &layout, char char1, char char2) {
    // both characters are in the layout
    if (layout.char_to_key.find(char1) == layout.char_to_key.end() ||
        layout.char_to_key.find(char2) == layout.char_to_key.end()) {
        return; 
    }
    
    Key key1 = layout.char_to_key.at(char1);
    Key key2 = layout.char_to_key.at(char2);
    
    // swap char in matrix
    layout.matrix[key1.row][key1.column].value = char2;
    layout.matrix[key2.row][key2.column].value = char1;
    
    layout.char_to_key[char1] = Key { char1, key2.row, key2.column, 
                                    get_finger(key2.column, key2.hand), key2.hand};
    layout.char_to_key[char2] = Key{ char2, key1.row, key1.column, 
                                    get_finger(key1.column, key1.hand), key1.hand};
    
    layout.matrix[key1.row][key1.column].finger = get_finger(key1.column, key1.hand);
    layout.matrix[key1.row][key1.column].hand = key1.hand;
    
    layout.matrix[key2.row][key2.column].finger = get_finger(key2.column, key2.hand);
    layout.matrix[key2.row][key2.column].hand = key2.hand;
}

template <std::size_t T>
KeyboardLayout gen_layout(KeyboardLayout layout, std::array<char, T> corpus) {
    KeyboardLayout new_layout = layout;
    std::string characters = "qwertyuiopasdfghjkl;zxcvbnm,./";
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 10; col++) {
            char current_char = new_layout.matrix[row][col].value;
            
            if (current_char == '\0' || current_char == ' ') {
                continue;
            }
            
            KeyboardLayout best_for_position = new_layout;
            double best_score = std::numeric_limits<double>::max();
            char best_swap_char = current_char;
            
            // try swap every character
            for (char test_char : characters) {
                if (test_char == current_char) continue;
                
                if (new_layout.char_to_key.find(test_char) == new_layout.char_to_key.end()) {
                    continue;
                }
                
                KeyboardLayout temp_layout = new_layout;
                swap_keys(temp_layout, current_char, test_char);
                
                get_stats(temp_layout, corpus);
                
                if (temp_layout.score < best_score) {
                    best_score = temp_layout.score;
                    best_for_position = temp_layout;
                    best_swap_char = test_char;
                }
            }
            
            new_layout = best_for_position;
        }
    }
    
    return new_layout;
}

int main() {
    std::array<char, mt_quotes.size()> corpus;
    std::transform(
        mt_quotes.begin(), mt_quotes.end(), corpus.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    auto base_layout = load_layout("semimak");
    get_stats(*base_layout, corpus);
    base_layout->print();
    
    KeyboardLayout optimized_layout = static_cast<KeyboardLayout>(*base_layout); 
    
    auto start = std::chrono::high_resolution_clock::now();
    optimized_layout = gen_layout(optimized_layout, corpus); 
    get_stats(optimized_layout, corpus);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    optimized_layout.print(); 


    std::cout << duration.count() << " ns\n"; 
    
    return 0;
}


// Explore the following later:
// 1. Simulated annealing: Allows occasional "bad" moves to escape local optima
// 2. Genetic algorithms: Evolves multiple candidate solutions simultaneously
// 3. Hill climbing with restarts: Tries multiple random starting points
// 4. Branch and bound: Systematically explores the solution space

void debug_finger_assignments(const KeyboardLayout &layout) {
    std::cout << "Finger assignments:\n";
    for (const auto &[ch, key] : layout.char_to_key) {
        if (ch != ' ') {
            std::cout << "'" << ch << "' -> finger "
                      << static_cast<int>(key.finger) << " (row=" << key.row
                      << ", col=" << key.column << ")\n";
        }
    }
    std::cout << "\n";
}
