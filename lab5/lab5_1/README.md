# Morse Code Sender

A Python script that sends Morse code messages using an LED on a Raspberry Pi.

## Usage

```bash
python3 send.py <repetitions> <message>
```

### Arguments
- `<repetitions>`: Number of times to repeat the message (must be at least 1)
- `<message>`: The text message to send in Morse code (use quotes for multi-word messages)

### Examples

Send "hello" 4 times:
```bash
python3 send.py 4 "hello"
```

Send "SOS" 3 times:
```bash
python3 send.py 3 "SOS"
```

Send "hello ESP32" once:
```bash
python3 send.py 1 "hello ESP32"
```

## Hardware Setup
- LED connected to GPIO pin 18
- Runs on Raspberry Pi with RPi.GPIO library

## Notes
- Press Ctrl+C to stop transmission early
- The script prints the Morse code pattern to the console as it sends
