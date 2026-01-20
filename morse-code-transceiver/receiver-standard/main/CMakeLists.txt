# Author: Noah Laforet
# Component CMakeLists.txt for Morse Code Receiver - Standard Mode

idf_component_register(SRCS "morse_receiver_standard.c"
                    PRIV_REQUIRES esp_adc esp_timer
                    INCLUDE_DIRS ".")
