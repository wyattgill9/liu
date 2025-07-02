#include <stdio.h>
#include <stdint.h>
#include <string.h>

uint8_t Layout[32];

void setup_qwerty() {
    const char *keys = 
        "qwertyuiop"
        "asdfghjkl;"
        "zxcvbnm,./"
        " 0";
    memcpy(Layout, keys, 32);
}

void eval_layout(uint8_t* layout) {
    printf("%s", layout);
}

int main() {
    setup_qwerty();
    eval_layout(Layout);
    return 0;
}
