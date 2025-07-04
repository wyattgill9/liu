#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cctype>
#include <array>
#include <unordered_map>
#include <optional>
#include <expected>
#include <fstream>

#include <simdjson.h>
// using namespace simdjson;

enum Error {
    LAYOUT_PARSE_ERROR_INVALID_FILE,
};

enum class Finger : std::uint8_t {
    LP = 0,
    LR = 1,
    LM = 2,
    LI = 3,
    // LT = 4,
    
    // RT = 5
    RI = 4,
    RM = 5,
    RR = 6,
    RP = 7,

    // TB = 10;
};

namespace std {
    template<>
    struct hash<std::tuple<Finger, Finger, Finger>> {
        size_t operator()(const std::tuple<Finger, Finger, Finger>& t) const noexcept {
            auto h1 = static_cast<std::uint8_t>(std::get<0>(t));
            auto h2 = static_cast<std::uint8_t>(std::get<1>(t));
            auto h3 = static_cast<std::uint8_t>(std::get<2>(t));

            return std::hash<uint8_t>{}(h1) ^ (std::hash<uint8_t>{}(h2) << 1) ^ (std::hash<uint8_t>{}(h3) << 2);
        }
    };
}

enum class Hand : std::uint8_t {
    LEFT = 0,
    RIGHT = 1,
};

struct Key {
    char value = '\0';  // value field for matrix
    int row;
    int column; 
    Finger finger;
    Hand hand;
};

struct KeyboardLayout {
    std::string name;
    std::unordered_map<char, Key> char_to_key;
    std::array<std::array<Key, 10>, 3> matrix; 
 
    // better printing with matrix instead of reverse hashmap lookup
    void print() {
        std::cout << name << ":\n";
        for(int row = 0; row < 3; row++) {
            // left 
            for(int col = 0; col < 5; col++) {
                if(matrix[row][col].value != '\0') {
                    std::cout << matrix[row][col].value;
                } else {
                    std::cout << " ";
                }
                if(col < 4) std::cout << " ";
            }
            
            std::cout << "  |  ";
            
            // right
            for(int col = 5; col < 10; col++) {
                if(matrix[row][col].value != '\0') {
                    std::cout << matrix[row][col].value;
                } else {
                    std::cout << " ";
                }
                if(col < 9) std::cout << " ";
            }
            
            std::cout << "\n";
        }
    }
};

struct CorpusData {
    std::unordered_map<std::string, int> monogram_counts;
    std::unordered_map<std::string, int> bigram_counts;
    std::unordered_map<std::string, int> trigram_counts;
};

struct LayoutStats {
    double alternate = 0.0;

    double roll_in = 0.0;
    double roll_out = 0.0;

    double oneh_in = 0.0;
    double oneh_out = 0.0;

    double redirect = 0.0;
    double bad_redirect = 0.0;

    double sfb = 0.0;

    double dsfb_red = 0.0;
    double dsfb_alt = 0.0;

    double left_hand = 0.0;
    double right_hand = 0.0;
};

std::unordered_map<std::tuple<Finger, Finger, Finger>, std::string> trigram_table;

Finger get_finger(int col, Hand hand) {
    if(hand == Hand::LEFT) {
        switch(col) {
            case 0: return Finger::LP;
            case 1: return Finger::LR;
            case 2: return Finger::LM;
            case 3: case 4: return Finger::LI;
        } 
    } else {
        switch(col) {
            case 0: return Finger::RP;
            case 1: return Finger::RR;
            case 2: return Finger::RM;
            case 3: case 4: return Finger::RI;
        }
    }
    return Finger::LI; // fallback
}

std::expected<KeyboardLayout, Error> load_layout(const std::string& file) {
    KeyboardLayout layout;
    
    std::string layout_file = "../layouts/" + file;

    std::ifstream layout_file_stream(layout_file);
    if(!layout_file_stream) { return std::unexpected(LAYOUT_PARSE_ERROR_INVALID_FILE); }
    
    std::string line; 
    std::getline(layout_file_stream, layout.name, ':'); 
     
    int row = 0;
    
    for(int r = 0; r < 3; r++) {
        for(int c = 0; c < 10; c++) {
            layout.matrix[r][c] = { '\0', r, c, Finger::LI, Hand::LEFT};
        }
    }
    
    while(std::getline(layout_file_stream, line)) {
        if(line.empty()) continue;
        
        size_t pipe_pos = line.find('|');
        std::string left_side = line.substr(0, pipe_pos);
        std::string right_side = (pipe_pos != std::string::npos) ? line.substr(pipe_pos + 1) : "";
        
        int col = 0;
        
        for(char value : left_side) {
            if(value != ' ') {
                Key key = {
                    value, 
                    row, 
                    col, 
                    get_finger(col, Hand::LEFT), 
                    Hand::LEFT,
                };
                layout.char_to_key[value] = key;
                layout.matrix[row][col] = key;
                col++;
            }
        }
        
        col = 0;
        for(char value : right_side) {
            if(value != ' ') {
                Key key = { 
                    value, 
                    row, 
                    col, 
                    get_finger(col, Hand::RIGHT), 
                    Hand::RIGHT,
                };
                layout.char_to_key[value] = key;
                layout.matrix[row][col + 5] = key;  // 5 offset for right side
                col++;
            }
        }
        
        row++;
    }
    
    return layout;
}

std::string get_path(const std::string_view& corpus, const std::string_view& extension) {
   return "../corpus/" + std::string(corpus) + std::string(extension) + ".json"; 
}

bool load_json_file(const std::string& filename, simdjson::padded_string& json) {
    try {
        json = simdjson::padded_string::load(filename);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading file " << filename << ": " << e.what() << std::endl; 
        return false;
    }
}

Finger string_to_finger(const std::string& finger_str) {
    if (finger_str == "LP") return Finger::LP;
    if (finger_str == "LR") return Finger::LR;
    if (finger_str == "LM") return Finger::LM;
    if (finger_str == "LI") return Finger::LI;
    // if (finger_str == "LT") return Finger::LT; // not adding thumbs yet
    if (finger_str == "RI") return Finger::RI;
    // if (finger_str == "RT") return Finger::RT; 
    if (finger_str == "RM") return Finger::RM;
    if (finger_str == "RR") return Finger::RR;
    if (finger_str == "RP") return Finger::RP;
    // if (finger_str == "TB") return Finger::TB; 
    
    return Finger::LI; // fallback
}

void parse_trigram_table(const simdjson::padded_string& json_data) {
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(json_data);

    for (auto field : doc.get_object()) {
        std::string_view key = field.unescaped_key();
        std::string_view value = field.value().get_string();
        
        std::string key_str(key);
        std::string value_str(value);

        std::array<std::string, 3> finger_codes = {key_str.substr(0, 2), key_str.substr(2, 2), key_str.substr(4, 2)};
        
        Finger finger1 = string_to_finger(finger_codes[0]);
        Finger finger2 = string_to_finger(finger_codes[1]);
        Finger finger3 = string_to_finger(finger_codes[2]);
        
        trigram_table[std::make_tuple(finger1, finger2, finger3)] = value_str;
    }
}

void load_trigram_table(const std::string_view& filename) {
    simdjson::padded_string json;
    if(load_json_file(get_path("", "trigram_table"), json)) {
        parse_trigram_table(json);
    }
}

void parse_ngram_counts(const simdjson::padded_string& json_data, std::unordered_map<std::string, int>& map, int ngram_length) {
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(json_data);
    
    for (auto field : doc.get_object()) {
        std::string_view key = field.unescaped_key();
        int value = field.value().get_int64();
        
        // convert string_view to string to compare
        std::string key_str(key);
        if(key_str.length() == ngram_length) {
            map[key_str] = value;
        } 
    }
}

void load_corpus(CorpusData& data, const std::string_view& corpus) {
    simdjson::padded_string monogram_json, bigram_json, trigram_json; 
     
    if(load_json_file(get_path(corpus, "/monograms"), monogram_json)) {
        parse_ngram_counts(monogram_json, data.monogram_counts, 1);
    }

    if (load_json_file(get_path(corpus, "/bigrams"), bigram_json)) {
        parse_ngram_counts(bigram_json, data.bigram_counts, 2);
    }

    if (load_json_file(get_path(corpus, "/trigrams"), trigram_json)) {
        parse_ngram_counts(trigram_json, data.trigram_counts, 3);
    }
}

void print_map(const std::unordered_map<std::string, int>& map, const std::string& name) {
    std::cout << name << " (" << map.size() << " entries):\n";
    for(const auto& [key, value] : map) {
        std::cout << "'" << key << "': " << value << "\n";
    }
    std::cout << "\n";
}

void print_trigram_table() {
    std::cout << "Trigram table (" << trigram_table.size() << " entries):\n";
    for(const auto& [key, value] : trigram_table) {
        std::cout << "(" << static_cast<int>(std::get<0>(key)) << "," 
                  << static_cast<int>(std::get<1>(key)) << "," 
                  << static_cast<int>(std::get<2>(key)) << "): " << value << "\n";
    }
    std::cout << "\n";
}

int main() {
    CorpusData data;
    load_corpus(data, "mt-quotes");
    load_trigram_table("mt-quotes");

    print_map(data.monogram_counts, "Monograms");
    print_map(data.bigram_counts, "Bigrams");
    print_map(data.trigram_counts, "Trigrams");
    print_trigram_table();

    auto layout = load_layout("semimak");    
    if(layout.has_value()) {
        layout->print(); 
    }
    return 0;
}
