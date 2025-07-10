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
#include <cctype>
#include <cmath>
#include <unordered_set>
#include <chrono>
#include <set>
#include <iomanip>
#include <algorithm>

#include <simdjson.h>
// using namespace simdjson;

enum Error {
    LAYOUT_PARSE_ERROR_INVALID_FILE,
    MONOGRAM_PARSE_ERROR_MONOGRAM_TO_SHORT,
};

enum class Finger : std::uint8_t {
    LP = 0, LR = 1, LM = 2, LI = 3, LT = 4,
    RT = 5, RI = 6, RM = 7, RR = 8, RP = 9, 
    TB = 10,
};

std::string finger_name(Finger f) {
    switch(f) {
        case Finger::LP: return "LP";
        case Finger::LR: return "LR";
        case Finger::LM: return "LM";
        case Finger::LI: return "LI";
        case Finger::LT: return "LT";
        case Finger::RT: return "RT";
        case Finger::RI: return "RI";
        case Finger::RM: return "RM";
        case Finger::RR: return "RR";
        case Finger::RP: return "RP";
        case Finger::TB: return "TB";
        default: return "??";
    }
}

namespace std {
    template<>
    struct hash<std::tuple<Finger, Finger, Finger>> {
        size_t operator()(const std::tuple<Finger, Finger, Finger>& t) const noexcept {
            auto h1 = static_cast<std::uint8_t>(std::get<0>(t));
            auto h2 = static_cast<std::uint8_t>(std::get<1>(t));
            auto h3 = static_cast<std::uint8_t>(std::get<2>(t));
            return ((std::hash<uint8_t>{}(h1) << 16) ^ (std::hash<uint8_t>{}(h2) << 8) ^ (std::hash<uint8_t>{}(h3)));
        }
    };
}

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

    void print() {
        std::cout << name << "\n";
        for(int row = 0; row < 4; row++) {
            for(int col = 0; col < 5; col++) {
                if(matrix[row][col].value != '\0') {
                    if(matrix[row][col].value != ' ') [[likely]] std::cout << matrix[row][col].value;
                    else [[unlikely]] std::cout << "_";
                } else { std::cout << " "; }
                if(col < 4) std::cout << " ";
            }
            std::cout << "  |  ";
            for(int col = 5; col < 10; col++) {
                if(matrix[row][col].value != '\0') {
                    if(matrix[row][col].value != ' ') [[likely]] std::cout << matrix[row][col].value;
                    else [[unlikely]] std::cout << "_";
                } else { std::cout << " "; }
                if(col < 9) std::cout << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
};

struct CorpusData {
    std::string corpus_name;
    std::unordered_map<char, int> monogram_counts;
    std::unordered_map<std::string, int> bigram_counts;
    std::unordered_map<std::string, int> trigram_counts;
};

struct LayoutStats {
    std::string corpus_name;

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
    
    double trigram_repeat = 0.0; // "sfR"
    
    void print() {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "MT-QUOTES:\n";
        std::cout << "  Alt: " << alternate << "%\n";
        std::cout << "  Rol: " << roll_in + roll_out << "%   (In/Out: "
                  << roll_in << "% | " << roll_out << "%)\n";
        std::cout << "  One: " << oneh_in + oneh_out << "%   (In/Out: "
                  << oneh_in << "% | " << oneh_out << "%)\n";
        std::cout << "  Rtl: " << roll_in + roll_out + oneh_in + oneh_out
                  << "%   (In/Out: "
                  << roll_in + oneh_in << "% | "
                  << roll_out + oneh_out << "%)\n";
        std::cout << "  Red: " << redirect + bad_redirect << "%   (Bad: "
                  << bad_redirect << "%)\n";
        std::cout << "\n  SFB: " << (sfb / 2) << "%\n";
        std::cout << "  SFS: " << (dsfb_red + dsfb_alt) << "%   (Red/Alt: "
                  << dsfb_red << "% | " << dsfb_alt << "%)\n";
        std::cout << "\n  LH/RH: " << left_hand << "% | " << right_hand << "%\n";
    }
};

std::unordered_map<std::tuple<Finger, Finger, Finger>, std::string> combo_table;

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
    return Finger::LI;
}

std::expected<KeyboardLayout, Error> load_layout(const std::string& file) {
    KeyboardLayout layout;
    std::string layout_file = "../layouts/" + file;

    std::ifstream layout_file_stream(layout_file);
    if(!layout_file_stream) { return std::unexpected(LAYOUT_PARSE_ERROR_INVALID_FILE); }

    std::string line;
    std::getline(layout_file_stream, layout.name, ':');

    int row = 0;
    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 10; c++)
            layout.matrix[r][c] = { '\0', r, c, Finger::LI, Hand::LEFT };

    while(std::getline(layout_file_stream, line)) {
        if(line.empty()) continue;
        size_t pipe_pos = line.find('|');
        std::string left_side = line.substr(0, pipe_pos);
        std::string right_side = (pipe_pos != std::string::npos) ? line.substr(pipe_pos + 1) : "";

        int col = 0;
        for(char value : left_side) {
            if(value != ' ') {
                Key key = { value, row, col, get_finger(col, Hand::LEFT), Hand::LEFT };
                layout.char_to_key[value] = key;
                layout.matrix[row][col] = key;
                col++;
            }
        }
        col = 0;
        for(char value : right_side) {
            if(value != ' ') {
                Key key = { value, row, col, get_finger(col, Hand::RIGHT), Hand::RIGHT };
                layout.char_to_key[value] = key;
                layout.matrix[row][col + 5] = key;
                col++;
            }
        }
        row++;
    }

    Key left_space = Key { ' ', 3, 4, Finger::LT, Hand::LEFT };
    Key right_space = Key { ' ', 3, 5, Finger::RT, Hand::RIGHT };
    layout.char_to_key[' '] = left_space;
    layout.matrix[3][4] = left_space;
    layout.matrix[3][5] = right_space;

    std::unordered_set<char> valid_keys;
    for(const auto& [key, value] : layout.char_to_key) {
        valid_keys.insert(value.value);
    }
    layout.valid_keys = valid_keys;
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
    if (finger_str == "LT") return Finger::LT;
    if (finger_str == "RT") return Finger::RT;
    if (finger_str == "RI") return Finger::RI;
    if (finger_str == "RM") return Finger::RM;
    if (finger_str == "RR") return Finger::RR;
    if (finger_str == "RP") return Finger::RP;
    if (finger_str == "TB") return Finger::TB;
    return Finger::LI;
}

void parse_combo_table(const simdjson::padded_string& json_data) {
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(json_data);

    for (auto field : doc.get_object()) {
        std::string_view key = field.unescaped_key();
        std::string_view value = field.value().get_string();
        std::string key_str(key);
        std::string value_str(value);

        std::array<std::string, 3> finger_codes = {
            key_str.substr(0, 2),
            key_str.substr(3, 2),
            key_str.substr(6, 2)
        };

        Finger finger1 = string_to_finger(finger_codes[0]);
        Finger finger2 = string_to_finger(finger_codes[1]);
        Finger finger3 = string_to_finger(finger_codes[2]);

        combo_table[std::make_tuple(finger1, finger2, finger3)] = value_str;
    }
}

void load_combo_table() {
    simdjson::padded_string json;
    if(load_json_file(get_path("", "combo_table"), json)) {
        parse_combo_table(json);
    }
}

void parse_ngram_counts(const simdjson::padded_string& json_data, std::unordered_map<std::string, int>& map, int ngram_length) {
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(json_data);
    
    for (auto field : doc.get_object()) {
        std::string_view key = field.unescaped_key();
        int value = field.value().get_int64();
        
        std::string key_str(key);
        if(key_str.length() == ngram_length) {
            map[key_str] = value;
        }
    }
}

void parse_monogram_counts(const simdjson::padded_string& json_data, std::unordered_map<char, int>& map) {
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(json_data);
    
    for (auto field : doc.get_object()) {
        std::string_view key = field.unescaped_key();
        
        int value = field.value().get_int64();
        char key_char = key[0];
        map[key_char] = value;
    }
}

void load_corpus(CorpusData& data, const std::string_view& corpus) {
    simdjson::padded_string monogram_json, bigram_json, trigram_json;
    data.corpus_name = std::string(corpus);
    std::transform(data.corpus_name.begin(), data.corpus_name.end(), data.corpus_name.begin(), ::toupper);

    if(load_json_file(get_path(corpus, "/monograms"), monogram_json)) {
        parse_monogram_counts(monogram_json, data.monogram_counts);
    }
    if (load_json_file(get_path(corpus, "/bigrams"), bigram_json)) {
        parse_ngram_counts(bigram_json, data.bigram_counts, 2);
    }
    if (load_json_file(get_path(corpus, "/trigrams"), trigram_json)) {
        parse_ngram_counts(trigram_json, data.trigram_counts, 3);
    }
}

std::pair<std::unordered_map<Finger, double>, double> get_usage(const KeyboardLayout& layout, const CorpusData& data) {
    std::unordered_map<Finger, double> fingers;
    for(const auto& [gram, count] : data.monogram_counts) {
        char gram_char = std::tolower(gram);
        if(!layout.char_to_key.contains(gram_char)) {
            continue;
        }
        Finger finger = layout.char_to_key.find(gram_char)->second.finger;
        fingers[finger] += count;
    }
    
    double total = 0;
    for(auto& [finger, count] : fingers) total += count;

    double right_hand = 0;
    
    for(auto& [finger, usage] : fingers) {
        usage /= total;
        switch(finger) {
            case Finger::RT:
            case Finger::RI:
            case Finger::RM:
            case Finger::RR:
            case Finger::RP:
            case Finger::TB:    
                right_hand += usage;
                break;
            default:
                break; 
        }
    }
    return std::pair(fingers, right_hand * 100);
}

double get_sfb(const KeyboardLayout& layout, const CorpusData& data) {
    double counts = 0;
    double total_bigrams = 0;
    
    for (const auto& [_, count] : data.bigram_counts) {
        total_bigrams += count;
    }
    
    for(const auto& [gram, count] : data.bigram_counts) {
        std::string gram_str(gram);
        std::transform(gram_str.begin(), gram_str.end(), gram_str.begin(), ::tolower);
        
        if(std::any_of(gram_str.begin(), gram_str.end(),
            [&](char c) { return layout.valid_keys.find(c) == layout.valid_keys.end(); })) continue;
        
        if(gram_str.find(' ') != std::string::npos || gram_str[0] == gram_str[1]) continue;
        
        if(layout.char_to_key.find(gram_str[0])->second.finger == layout.char_to_key.find(gram_str[1])->second.finger) {
            counts += count;
        }
    }
    
    return (counts / total_bigrams) * 100;
}

LayoutStats get_stats(const KeyboardLayout& layout, const CorpusData& data) {
    LayoutStats stats;
    stats.corpus_name = data.corpus_name;

    auto [finger_usage, right_hand_usage] = get_usage(layout, data);
    stats.right_hand = right_hand_usage;
    stats.left_hand = 100 - stats.right_hand;
    stats.sfb = get_sfb(layout, data);

    double total_trigrams = 0;
    double alternate_count = 0, roll_in_count = 0, roll_out_count = 0, oneh_in_count = 0,
           oneh_out_count = 0, redirect_count = 0, bad_redirect_count = 0, dsfb_red_count = 0, dsfb_alt_count = 0;

    for(const auto& [gram, count] : data.trigram_counts) {
        total_trigrams += count;
        std::string gram_str(gram);
        std::transform(gram_str.begin(), gram_str.end(), gram_str.begin(), ::tolower);

        if(gram_str.find(' ') != std::string::npos) continue;
        if(std::any_of(gram_str.begin(), gram_str.end(), [&](char c) { return layout.valid_keys.find(c) == layout.valid_keys.end(); })) continue;

        Finger finger1 = layout.char_to_key.find(gram_str[0])->second.finger;
        Finger finger2 = layout.char_to_key.find(gram_str[1])->second.finger;
        Finger finger3 = layout.char_to_key.find(gram_str[2])->second.finger;

        if(finger1 == Finger::TB) finger1 = Finger::LT;
        if(finger2 == Finger::TB) finger2 = Finger::LT;
        if(finger3 == Finger::TB) finger3 = Finger::LT;

        auto combo_key = std::make_tuple(finger1, finger2, finger3);
        std::string combo_type = combo_table.contains(combo_key) ? combo_table[combo_key] : "unknown";

        if(combo_type == "alternate") alternate_count += count;
        else if(combo_type == "roll-in") roll_in_count += count;
        else if(combo_type == "roll-out") roll_out_count += count;
        else if(combo_type == "oneh-in") oneh_in_count += count;
        else if(combo_type == "oneh-out") oneh_out_count += count;
        else if(combo_type == "redirect") redirect_count += count;
        else if(combo_type == "bad-redirect") bad_redirect_count += count;
        else if(combo_type == "dsfb-red") dsfb_red_count += count;
        else if(combo_type == "dsfb-alt") dsfb_alt_count += count;
    }

    if(total_trigrams > 0) {
        stats.alternate = (alternate_count / total_trigrams) * 100;
        stats.roll_in = (roll_in_count / total_trigrams) * 100;
        stats.roll_out = (roll_out_count / total_trigrams) * 100;
        stats.oneh_in = (oneh_in_count / total_trigrams) * 100;
        stats.oneh_out = (oneh_out_count / total_trigrams) * 100;
        stats.redirect = (redirect_count / total_trigrams) * 100;
        stats.bad_redirect = (bad_redirect_count / total_trigrams) * 100;
        stats.dsfb_red = (dsfb_red_count / total_trigrams) * 100;
        stats.dsfb_alt = (dsfb_alt_count / total_trigrams) * 100;
    }

    return stats;
}

int main() {
    CorpusData data;
    load_corpus(data, "mt-quotes");
    load_combo_table();

    auto now = std::chrono::high_resolution_clock::now();

    auto layout = load_layout("semimak");

    LayoutStats stats = get_stats(*layout, data);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - now);

    layout->print();
    stats.print();
    std::cout << "\n" << duration.count() << " ns\n";
    return 0;
}
