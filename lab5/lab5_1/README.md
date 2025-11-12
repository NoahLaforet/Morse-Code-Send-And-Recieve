# Lab 5.1: Morse LED Transmitter for Raspberry Pi 4

This program transmits messages in Morse code using an LED connected to a Raspberry Pi 4.

## Hardware Setup

### Components Needed
- Raspberry Pi 4
- LED
- 330Ω resistor (or similar value between 220Ω - 1kΩ)
- Breadboard and jumper wires

### Wiring Instructions
1. Connect the LED anode (long leg, positive) to GPIO 17 (Physical Pin 11)
2. Connect a 330Ω resistor to the LED cathode (short leg, negative)
3. Connect the other end of the resistor to GND (e.g., Physical Pin 6, 9, 14, 20, 25, 30, 34, or 39)

```
GPIO 17 (Pin 11) ----> LED Anode (long leg)
                       LED Cathode (short leg) ----> 330Ω Resistor ----> GND
```

**WARNING:** Always use a current-limiting resistor! Without it, the LED may burn out.

## Software Setup

### Compilation
```bash
make
```

This will create an executable called `send`.

### Usage
The program must be run with sudo to access GPIO:

```bash
sudo ./send <repeat_count> <message>
```

#### Example
```bash
sudo ./send 4 "hello ESP32"
```

This will transmit "hello ESP32" in Morse code 4 times:
```
.... . .-.. .-.. --- / . ... .--. ...-- ..---
.... . .-.. .-.. --- / . ... .--. ...-- ..---
.... . .-.. .-.. --- / . ... .--. ...-- ..---
.... . .-.. .-.. --- / . ... .--. ...-- ..---
```

### Supported Characters
- Letters: A-Z (case insensitive)
- Numbers: 0-9
- Space: Creates a longer gap between words

## Morse Code Timing
- Dot: 200ms
- Dash: 600ms (3x dot)
- Gap between symbols: 200ms
- Gap between letters: 600ms (3x dot)
- Gap between words: 1400ms (7x dot)

## GPIO Configuration
The program uses GPIO 17 (BCM numbering) by default, which corresponds to Physical Pin 11 on the 40-pin header.

If you need to use a different GPIO pin, modify the `GPIO_PIN` constant in [morse_send.c](morse_send.c#L9).

## Troubleshooting

### Permission Denied Errors
Make sure you run the program with `sudo`:
```bash
sudo ./send 4 "hello ESP32"
```

### LED Not Lighting Up
1. Check your wiring connections
2. Verify the LED polarity (long leg = anode/positive)
3. Confirm the resistor is in series with the LED
4. Try a different GPIO pin if GPIO 17 is not working

### GPIO Already Exported Error
If you see an error about GPIO already being exported, the program will attempt to continue. If it fails, you can manually unexport:
```bash
echo 17 | sudo tee /sys/class/gpio/unexport
```

## Clean Up
```bash
make clean
```
