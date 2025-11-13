#!/usr/bin/env python3

import RPi.GPIO as GPIO
import time
import sys

# LED Configuration
LED_PIN = 17  # GPIO pin number 

# Morse code timing (in seconds) - 2x faster than lab5_2
DOT = 0.1              # Duration of a dot (100ms - 2x faster than lab5_2's 200ms)
DASH = DOT * 3         # Duration of a dash (3 dots)
SYMBOL_SPACE = DOT     # Space between dots and dashes within a character
LETTER_SPACE = DOT * 3 # Space between letters
WORD_SPACE = DOT * 7   # Space between words

# Morse code dictionary
MORSE_CODE = {
    'A': '.-',    'B': '-...',  'C': '-.-.',  'D': '-..',   'E': '.',
    'F': '..-.',  'G': '--.',   'H': '....',  'I': '..',    'J': '.---',
    'K': '-.-',   'L': '.-..',  'M': '--',    'N': '-.',    'O': '---',
    'P': '.--.',  'Q': '--.-',  'R': '.-.',   'S': '...',   'T': '-',
    'U': '..-',   'V': '...-',  'W': '.--',   'X': '-..-',  'Y': '-.--',
    'Z': '--..',
    '0': '-----', '1': '.----', '2': '..---', '3': '...--', '4': '....-',
    '5': '.....', '6': '-....', '7': '--...', '8': '---..', '9': '----.',
    ' ': '/'  # Space between words
}

def setup_gpio():
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    GPIO.setup(LED_PIN, GPIO.OUT)
    GPIO.output(LED_PIN, GPIO.LOW)

def cleanup_gpio():
    GPIO.output(LED_PIN, GPIO.LOW)
    GPIO.cleanup()

def send_dot():
    GPIO.output(LED_PIN, GPIO.HIGH)
    time.sleep(DOT)
    GPIO.output(LED_PIN, GPIO.LOW)
    time.sleep(SYMBOL_SPACE)

def send_dash():
    GPIO.output(LED_PIN, GPIO.HIGH)
    time.sleep(DASH)
    GPIO.output(LED_PIN, GPIO.LOW)
    time.sleep(SYMBOL_SPACE)

def send_character(char):
    char = char.upper()

    if char == ' ':
        # For space, wait word space duration
        time.sleep(WORD_SPACE - LETTER_SPACE)  # Subtract letter space since it's added after each char
        return

    if char not in MORSE_CODE:
        # Skip characters not in Morse code dictionary
        return

    morse = MORSE_CODE[char]
    print(morse, end=' ', flush=True)

    for symbol in morse:
        if symbol == '.':
            send_dot()
        elif symbol == '-':
            send_dash()

    # Space between letters (subtract symbol space since it was already added)
    time.sleep(LETTER_SPACE - SYMBOL_SPACE)

def send_message(message):
    for char in message:
        send_character(char)
    print()  # New line after message

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 send.py <repetitions> <message>")
        print('Example: python3 send.py 4 "hello ESP32"')
        sys.exit(1)

    try:
        repetitions = int(sys.argv[1])
        message = sys.argv[2]
    except ValueError:
        print("Error: First argument must be a number")
        sys.exit(1)

    if repetitions < 1:
        print("Error: Number of repetitions must be at least 1")
        sys.exit(1)

    print(f"Sending message '{message}' {repetitions} time(s)")
    print("Morse code pattern:")

    try:
        setup_gpio()

        for i in range(repetitions):
            send_message(message)

        print("Transmission complete!")

    except KeyboardInterrupt:
        print("\nTransmission interrupted by user")
    finally:
        cleanup_gpio()

if __name__ == "__main__":
    main()
