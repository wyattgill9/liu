#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>

typedef uint8_t u8;
typedef double f64;

// SoA
struct Layout {
    u8 fingers[30];
    u8 values[30];
    
    u8 getFinger(u8 index) { return fingers[index]; }
    u8 getValue(u8 index) { return values[index]; }
};

Layout qwerty_layout;

void setup_qwerty() {
    const char keys[30] = {
        'q','w','e','r','t','y','u','i','o','p',
        'a','s','d','f','g','h','j','k','l',';',
        'z','x','c','v','b','n','m',',','.','/',
    };
    
    const u8 finger_map[30] = {
        1, 2, 3, 4, 4, 5, 5, 6, 7, 8,
        1, 2, 3, 4, 4, 5, 5, 6, 7, 8,
        1, 2, 3, 4, 4, 5, 5, 6, 7, 8
    };
    
    for (int i = 0; i < 30; i++) {
        qwerty_layout.fingers[i] = finger_map[i];
        qwerty_layout.values[i] = keys[i];
    }
}

void verify_layout(const Layout& layout) {
    const char* finger_names[] = {
        "", "L.Pinky", "L.Ring", "L.Middle", "L.Index",
        "R.Index", "R.Middle", "R.Ring", "R.Pinky"
    };
    
    std::cout << "Layout (Structure of Arrays):\n";
    std::cout << "Pos | Key | Finger\n";
    std::cout << "----|-----|--------\n";
    
    for (int i = 0; i < 30; i++) {
        std::cout << i << "  | '" << (char)layout.values[i] << "' | " 
                  << finger_names[layout.fingers[i]] << '\n';
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

constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
char file_buffer[MAX_FILE_SIZE];

void read_file_to_static_buffer(const char* filename, char* buffer, size_t buffer_size) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) { throw std::runtime_error(std::string("Failed to open file: ") + filename); }

    file.read(buffer, buffer_size);
}

f64 evaluate_layout(Layout layout, const char* text) {
    f64 score = 0.0;
    int i = 0;
    while(i < 1000000) {
        layout.getFinger(text[i]);
        i++; 
    } 
    return score;
}

int main() {
    try {
        setup_qwerty();
        verify_layout(qwerty_layout);

        static char file_buffer[MAX_FILE_SIZE];
        read_file_to_static_buffer("./corpus/test.txt", file_buffer, MAX_FILE_SIZE);

        std::cout.write(file_buffer, 512);

        f64 score = evaluate_layout(qwerty_layout, file_buffer);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}
