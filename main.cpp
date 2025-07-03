#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <cstring>

typedef uint8_t u8;
typedef double f64;

enum Finger : u8 {
    None = 0,
    L_Pinky = 1,
    L_Ring = 2,
    L_Middle = 3,
    L_Index = 4,
    R_Index = 5,
    R_Middle = 6,
    R_Ring = 7,
    R_Pinky = 8 
};

// SoA
struct Layout { u8 values[30]; };

Layout qwerty_layout;

const Finger finger_map[30] = {
    L_Pinky, L_Ring, L_Middle, L_Index, L_Index, R_Index, R_Index, R_Middle, R_Ring, R_Pinky,
    L_Pinky, L_Ring, L_Middle, L_Index, L_Index, R_Index, R_Index, R_Middle, R_Ring, R_Pinky,
    L_Pinky, L_Ring, L_Middle, L_Index, L_Index, R_Index, R_Index, R_Middle, R_Ring, R_Pinky
};

constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
char file_buffer[MAX_FILE_SIZE];

void read_file_to_static_buffer(const char* filename, char* buffer, size_t buffer_size) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) { throw std::runtime_error(std::string("Failed to open file: ") + filename); }

    file.read(buffer, buffer_size);
}

f64 evaluate_layout(Layout layout, const char* text) {
    f64 score = 0.0;
    return score;
}

void verify_layout(const Layout& layout); 

int main() {
    try {
        u8 temp[30] = {
            'q','w','e','r','t','y','u','i','o','p',
            'a','s','d','f','g','h','j','k','l',';',
            'z','x','c','v','b','n','m',',','.','/',
        };

        std::memcpy(qwerty_layout.values, temp, sizeof(temp));

        verify_layout(qwerty_layout);

        static char file_buffer[MAX_FILE_SIZE];
        read_file_to_static_buffer("./corpus/test.txt", file_buffer, MAX_FILE_SIZE);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}

void verify_layout(const Layout& layout) {
    const char* finger_names[] = {
        "", "L.Pinky", "L.Ring", "L.Middle", "L.Index",
        "R.Index", "R.Middle", "R.Ring", "R.Pinky"
    };
    
    std::cout << "Layout:\n";
    std::cout << "Pos | Key | Finger\n";
    std::cout << "----|-----|--------\n";
    
    for (int i = 0; i < 30; i++) {
        std::cout << i << "  | '" << (char)layout.values[i] << "' | " 
                  << finger_names[finger_map[i]] << '\n';
    }
    
    std::cout << "\nVisual layout:\n";
    std::cout << "Top:    ";
    for (int i = 0; i < 10; i++) std::cout << (char)layout.values[i] << ' ';
    std::cout << "\nMiddle: ";
    for (int i = 10; i < 20; i++) std::cout << (char)layout.values[i] << ' ';
    std::cout << "\nBottom: ";
    for (int i = 20; i < 30; i++) std::cout << (char)layout.values[i] << ' ';
    std::cout << "\n";
}
