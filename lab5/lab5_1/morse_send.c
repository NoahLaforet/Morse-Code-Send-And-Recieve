#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

// GPIO pin configuration - using GPIO 17 (Physical pin 11)
#define GPIO_PIN 17
#define GPIO_PATH "/sys/class/gpio"

// Morse code timing (in milliseconds)
#define DOT_DURATION 200      // Duration of a dot
#define DASH_DURATION 600     // Duration of a dash (3x dot)
#define SYMBOL_GAP 200        // Gap between dots and dashes within a letter
#define LETTER_GAP 600        // Gap between letters (3x dot)
#define WORD_GAP 1400         // Gap between words (7x dot)

// Morse code lookup table
const char* morse_code[] = {
    ".-",    // A
    "-...",  // B
    "-.-.",  // C
    "-..",   // D
    ".",     // E
    "..-.",  // F
    "--.",   // G
    "....",  // H
    "..",    // I
    ".---",  // J
    "-.-",   // K
    ".-..",  // L
    "--",    // M
    "-.",    // N
    "---",   // O
    ".--.",  // P
    "--.-",  // Q
    ".-.",   // R
    "...",   // S
    "-",     // T
    "..-",   // U
    "...-",  // V
    ".--",   // W
    "-..-",  // X
    "-.--",  // Y
    "--..",  // Z
    "-----", // 0
    ".----", // 1
    "..---", // 2
    "...--", // 3
    "....-", // 4
    ".....", // 5
    "-....", // 6
    "--...", // 7
    "---..", // 8
    "----."  // 9
};

// GPIO control functions
int gpio_export(int pin) {
    int fd = open(GPIO_PATH "/export", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open GPIO export");
        return -1;
    }

    char buf[3];
    snprintf(buf, sizeof(buf), "%d", pin);
    write(fd, buf, strlen(buf));
    close(fd);

    // Give the system time to create the GPIO files
    usleep(100000);
    return 0;
}

int gpio_unexport(int pin) {
    int fd = open(GPIO_PATH "/unexport", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open GPIO unexport");
        return -1;
    }

    char buf[3];
    snprintf(buf, sizeof(buf), "%d", pin);
    write(fd, buf, strlen(buf));
    close(fd);
    return 0;
}

int gpio_set_direction(int pin, const char* direction) {
    char path[64];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", pin);

    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to set GPIO direction");
        return -1;
    }

    write(fd, direction, strlen(direction));
    close(fd);
    return 0;
}

int gpio_write(int pin, int value) {
    char path[64];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);

    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to write GPIO value");
        return -1;
    }

    char buf[2];
    snprintf(buf, sizeof(buf), "%d", value);
    write(fd, buf, 1);
    close(fd);
    return 0;
}

// LED control functions
void led_on() {
    gpio_write(GPIO_PIN, 1);
}

void led_off() {
    gpio_write(GPIO_PIN, 0);
}

// Morse code transmission functions
void send_dot() {
    led_on();
    usleep(DOT_DURATION * 1000);
    led_off();
}

void send_dash() {
    led_on();
    usleep(DASH_DURATION * 1000);
    led_off();
}

void send_morse_char(char c) {
    c = toupper(c);
    const char* code = NULL;

    // Get morse code for the character
    if (c >= 'A' && c <= 'Z') {
        code = morse_code[c - 'A'];
    } else if (c >= '0' && c <= '9') {
        code = morse_code[26 + (c - '0')];
    } else if (c == ' ') {
        // Word gap (already have letter gap from previous char)
        usleep((WORD_GAP - LETTER_GAP) * 1000);
        return;
    } else {
        // Unknown character, skip
        return;
    }

    // Send the morse code
    for (int i = 0; code[i] != '\0'; i++) {
        if (code[i] == '.') {
            send_dot();
        } else if (code[i] == '-') {
            send_dash();
        }

        // Gap between symbols within a letter
        if (code[i + 1] != '\0') {
            usleep(SYMBOL_GAP * 1000);
        }
    }

    // Gap between letters
    usleep(LETTER_GAP * 1000);
}

void send_morse_message(const char* message) {
    for (int i = 0; message[i] != '\0'; i++) {
        send_morse_char(message[i]);
    }
}

void print_morse_message(const char* message) {
    for (int i = 0; message[i] != '\0'; i++) {
        char c = toupper(message[i]);
        const char* code = NULL;

        if (c >= 'A' && c <= 'Z') {
            code = morse_code[c - 'A'];
        } else if (c >= '0' && c <= '9') {
            code = morse_code[26 + (c - '0')];
        } else if (c == ' ') {
            printf("/ ");
            continue;
        } else {
            continue;
        }

        printf("%s ", code);
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <repeat_count> <message>\n", argv[0]);
        printf("Example: %s 4 \"hello ESP32\"\n", argv[0]);
        return 1;
    }

    int repeat_count = atoi(argv[1]);
    const char* message = argv[2];

    if (repeat_count <= 0) {
        printf("Error: Repeat count must be greater than 0\n");
        return 1;
    }

    // Initialize GPIO
    printf("Initializing GPIO %d...\n", GPIO_PIN);
    if (gpio_export(GPIO_PIN) == -1) {
        // GPIO might already be exported, try to continue
    }

    if (gpio_set_direction(GPIO_PIN, "out") == -1) {
        printf("Error: Failed to set GPIO direction. Are you running with sudo?\n");
        gpio_unexport(GPIO_PIN);
        return 1;
    }

    printf("Sending message '%s' %d time(s)...\n\n", message, repeat_count);

    // Send the message the specified number of times
    for (int i = 0; i < repeat_count; i++) {
        print_morse_message(message);
        send_morse_message(message);

        // Gap between repetitions (if not the last one)
        if (i < repeat_count - 1) {
            usleep(WORD_GAP * 1000);
        }
    }

    printf("\nTransmission complete!\n");

    // Cleanup GPIO
    led_off();
    gpio_unexport(GPIO_PIN);

    return 0;
}
