#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <cctype>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

template <typename T>
T abs(T x) { return x < 0 ? -x : x; }

enum Finger : u8 {
    LP=0, LR=1, LM=2, LI=3, RI=4, RM=5, RR=6, RP=7
};

struct Layout {
    u8 char_to_pos[30];
};

struct KeyInfo {
    u8 finger;
    u8 hand;
    u8 row;
    u8 column;
};

struct Metrics {
    u32 SFB;
    u32 SFS;
    u32 Redirects;
    u32 LSB;
    u32 Alts;
    u32 OutRoll;
    u32 InRoll;
    u32 Redirect;
    u32 OneHand;
    
    u32 total_bigrams;
    u32 total_trigrams;
};

struct CorpusData {
    std::vector<std::pair<u8, u8>> bigrams;
    std::vector<std::tuple<u8, u8, u8>> trigrams;
};

constexpr u8 LSB_THRESHOLD = 2;

constexpr KeyInfo KEY_INFO[30] = {
    {LP, 0, 0, 0}, // Q
    {LR, 0, 0, 1}, // W
    {LM, 0, 0, 2}, // E
    {LI, 0, 0, 3}, // R
    {LI, 0, 0, 4}, // T
    {RI, 1, 0, 5}, // Y
    {RI, 1, 0, 6}, // U
    {RM, 1, 0, 7}, // I
    {RR, 1, 0, 8}, // O
    {RP, 1, 0, 9}, // P
    {LP, 0, 1, 0}, // A
    {LR, 0, 1, 1}, // S
    {LM, 0, 1, 2}, // D
    {LI, 0, 1, 3}, // F
    {LI, 0, 1, 4}, // G
    {RI, 1, 1, 5}, // H
    {RI, 1, 1, 6}, // J
    {RM, 1, 1, 7}, // K
    {RR, 1, 1, 8}, // L
    {RP, 1, 1, 9}, // ;
    {LP, 0, 2, 0}, // Z
    {LR, 0, 2, 1}, // X
    {LM, 0, 2, 2}, // C
    {LI, 0, 2, 3}, // V
    {LI, 0, 2, 4}, // B
    {RI, 1, 2, 5}, // N
    {RI, 1, 2, 6}, // M
    {RM, 1, 2, 7}, // ,
    {RR, 1, 2, 8}, // .
    {RP, 1, 2, 9}, // /
};

Metrics evaluate_layout(const Layout& layout, const CorpusData& corpus) {
    Metrics metrics{};
    
    for (auto [c1, c2] : corpus.bigrams) {
        metrics.total_bigrams++; 
        
        uint8_t k1 = layout.char_to_pos[c1];
        uint8_t k2 = layout.char_to_pos[c2];
        
        const KeyInfo& ki1 = KEY_INFO[k1];
        const KeyInfo& ki2 = KEY_INFO[k2];
        
        if (ki1.finger == ki2.finger) metrics.SFB++;
        if (ki1.hand != ki2.hand) metrics.Alts++;
        
        if (ki1.hand == ki2.hand) {
            int8_t col_diff = int8_t(ki2.column) - int8_t(ki1.column);
            
            if (col_diff == 1) metrics.OutRoll++;
            else if (col_diff == -1) metrics.InRoll++;
        }
        
        if (ki1.hand == ki2.hand && abs(int(ki1.column) - int(ki2.column)) > LSB_THRESHOLD)
            metrics.LSB++;
    }
    
    for (auto [c1, c2, c3] : corpus.trigrams) {
        metrics.total_trigrams++; 

        uint8_t k1 = layout.char_to_pos[c1];
        uint8_t k2 = layout.char_to_pos[c2];
        uint8_t k3 = layout.char_to_pos[c3];
        
        const KeyInfo& ki1 = KEY_INFO[k1];
        const KeyInfo& ki2 = KEY_INFO[k2];
        const KeyInfo& ki3 = KEY_INFO[k3];
        
        if (ki1.finger == ki3.finger) metrics.SFS++;
        
        if (ki1.hand == ki2.hand && ki2.hand == ki3.hand) {
            int dir1 = int(ki2.column) - int(ki1.column);
            int dir2 = int(ki3.column) - int(ki2.column);
            
            if (dir1 * dir2 < 0) metrics.Redirect++;
            else metrics.OneHand++;
        }
    }
    
    return metrics;
}

CorpusData load_corpus(const std::string& filename) {
    CorpusData corpus;
    std::ifstream file(filename);
    std::string text, line;
    
    while (std::getline(file, line)) {
        text += line + " ";
    }
    
    std::string filtered;
    for (char c : text) {
        if (std::isalpha(c)) {
            filtered += std::tolower(c);
        }
    }
    
    for (size_t i = 0; i < filtered.size() - 1; i++) {
        u8 c1 = filtered[i] - 'a';
        u8 c2 = filtered[i + 1] - 'a';
        corpus.bigrams.push_back({c1, c2});
    }
    
    for (size_t i = 0; i < filtered.size() - 2; i++) {
        u8 c1 = filtered[i] - 'a';
        u8 c2 = filtered[i + 1] - 'a';
        u8 c3 = filtered[i + 2] - 'a';
        corpus.trigrams.push_back({c1, c2, c3});
    }
    
    return corpus;
}

Layout create_qwerty_layout() {
    Layout layout;
    std::string keys = "qwertyuiopasdfghjkl;zxcvbnm,./";
    for (int i = 0; i < 26; i++) {
        char c = keys[i];
        if (c >= 'a' && c <= 'z') {
            layout.char_to_pos[c - 'a'] = i;
        }
    }
    return layout;
}

void print_metrics(const Metrics& m) {
    std::cout << "Bigrams processed: " << m.total_bigrams << std::endl;
    std::cout << "Trigrams processed: " << m.total_trigrams << std::endl;

    auto pct = [](u32 count, u32 total) -> double {
        if (total == 0) return 0.0;
        return 100.0 * double(count) / double(total);
    };

    std::cout << "SFB: " << m.SFB << " (" << pct(m.SFB, m.total_bigrams) << "%)" << std::endl;
    std::cout << "LSB: " << m.LSB << " (" << pct(m.LSB, m.total_bigrams) << "%)" << std::endl;
    std::cout << "Alts: " << m.Alts << " (" << pct(m.Alts, m.total_bigrams) << "%)" << std::endl;
    std::cout << "OutRoll: " << m.OutRoll << " (" << pct(m.OutRoll, m.total_bigrams) << "%)" << std::endl;
    std::cout << "InRoll: " << m.InRoll << " (" << pct(m.InRoll, m.total_bigrams) << "%)" << std::endl;

    std::cout << "SFS: " << m.SFS << " (" << pct(m.SFS, m.total_trigrams) << "%)" << std::endl;
    std::cout << "Redirect: " << m.Redirect << " (" << pct(m.Redirect, m.total_trigrams) << "%)" << std::endl;
    std::cout << "OneHand: " << m.OneHand << " (" << pct(m.OneHand, m.total_trigrams) << "%)" << std::endl;
}

int main() {
    std::string filename;
    std::cout << "Enter corpus filename: ";
    std::getline(std::cin, filename);
    
    CorpusData corpus = load_corpus(filename);
    Layout qwerty = create_qwerty_layout();
    Metrics metrics = evaluate_layout(qwerty, corpus);
    
    print_metrics(metrics);
    
    return 0;
}
