#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <expected>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <execution>

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
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 10; col++) {
                std::cout << matrix[row][col].value << " ";
            }
            std::cout << "\n";
        }
        // std::cout << "Score:" << score;
    }
};

Finger get_finger(int col, Hand hand) {
    if (hand == Hand::LEFT) {
        switch (col) {
            case 0: return Finger::LP;  // q, a, z
            case 1: return Finger::LR;  // w, s, x
            case 2: return Finger::LM;  // e, d, c
            case 3: return Finger::LI;  // r, f, v
            case 4: return Finger::LI;  // t, g, b (index finger handles two columns)
            default: return Finger::LI;
        }
    } else {
        switch (col) {
            case 0: return Finger::RI;  // y, h, n (right index handles two columns)
            case 1: return Finger::RI;  // u, j, m
            case 2: return Finger::RM;  // i, k, comma
            case 3: return Finger::RR;  // o, l, period
            case 4: return Finger::RP;  // p, semicolon, slash
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

void get_stats(KeyboardLayout &layout) {
    constexpr double SFB_WEIGHT = 1;
    constexpr double SFS_WEIGHT = 1;
    
    int sfb = 0;
    int total_bigram_count = 0; 
    
    int sfs = 0;
    int total_sfs_count = 0; 

    for (std::size_t i = 0; i < mt_quotes.size() - 1; i++) { 

        char first = std::tolower(static_cast<char>(mt_quotes[i]));
        char second = std::tolower(static_cast<char>(mt_quotes[i + 1]));

        if (layout.valid_keys.find(first) == layout.valid_keys.end() ||
            layout.valid_keys.find(second) == layout.valid_keys.end())
            continue;

        if(first == ' ' || second == ' ') continue;
        if(first == second) continue;
        
        total_bigram_count++;

        if(layout.char_to_key.find(first)->second.finger == layout.char_to_key.find(second)->second.finger) { 
            sfb++;
        } 
    }

    double sfb_score = ((sfb * 100) / total_bigram_count); 
    std::cout << "SFB: " << sfb_score << "%\n"; 
    double sfs_score = ((sfs * 100) / total_sfs_count);
    
    double score = sfb_score + sfs_score; 
    layout.score = score;
}

void debug_finger_assignments(const KeyboardLayout &layout) {
    std::cout << "Finger assignments:\n";
    for (const auto &[ch, key] : layout.char_to_key) {
        if (ch != ' ') {
            std::cout << "'" << ch << "' -> finger " << static_cast<int>(key.finger) 
                      << " (row=" << key.row << ", col=" << key.column << ")\n";
        }
    }
    std::cout << "\n";
}

int main() {
    auto layout = load_layout("qwerty");
    
    // debug_finger_assignments(*layout);

    auto now = std::chrono::high_resolution_clock::now(); 
    get_stats(*layout);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - now);

    layout->print();

    std::cout << "\n" << duration.count() << " ns\n";
    return 0;
}
