# Author: Noah Laforet
# Component CMakeLists.txt for Morse Code Receiver - Fast Mode

idf_component_register(SRCS "morse_receiver_fast.c"
                    PRIV_REQUIRES esp_adc esp_timer
                    INCLUDE_DIRS ".")
