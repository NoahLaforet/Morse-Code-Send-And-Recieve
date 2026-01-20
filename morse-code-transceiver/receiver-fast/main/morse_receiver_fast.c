/*
 * Author: Noah Laforet
 * Morse Code Receiver - Fast Mode (10ms dot duration, 10 chars/sec)
 *
 * ESP32-C3 Morse Code Receiver using ADC and Photodiode
 * Optimized for high-speed transmission with 1ms sampling rate
 * Features precise timing with FreeRTOS, ADC sampling, and state machine decoding
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"

const static char *TAG = "MORSE_RECEIVER";

/*---------------------------------------------------------------
        ADC Configuration for Photodiode
---------------------------------------------------------------*/
// ADC1 Channel 2 (GPIO2) - Photodiode input
#if CONFIG_IDF_TARGET_ESP32
#define PHOTODIODE_ADC_CHAN         ADC_CHANNEL_4
#else
#define PHOTODIODE_ADC_CHAN         ADC_CHANNEL_2  // ESP32C3: GPIO2
#endif

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12  // 0-3.3V range

// Single variables for photodiode reading
static int adc_raw_value;
static int voltage_mv;

/*---------------------------------------------------------------
        Morse Code Configuration - Fast Mode (10x faster)
---------------------------------------------------------------*/
#define SAMPLE_RATE_MS      1       // Sample ADC every 1ms (fast for 10ms dot duration)
#define LIGHT_THRESHOLD     80      // ADC raw value threshold (adjusted for weak photodiode signal)

// Morse timing (in milliseconds) - 10x faster for 10 chars/sec
#define DOT_DURATION_MS     10      // Dot duration (10ms)
#define DASH_MIN_MS         20      // Minimum duration for dash (2x dot)
#define SYMBOL_GAP_MS       10      // Gap between dots/dashes
#define LETTER_GAP_MS       30      // Gap between letters (3*DOT)
#define WORD_GAP_MS         70      // Gap between words (7*DOT)

// Morse code lookup table
typedef struct {
    char letter;
    const char* code;
} morse_t;

const morse_t morse_table[] = {
    {'A', ".-"},    {'B', "-..."},  {'C', "-.-."},  {'D', "-.."},
    {'E', "."},     {'F', "..-."},  {'G', "--."},   {'H', "...."},
    {'I', ".."},    {'J', ".---"},  {'K', "-.-"},   {'L', ".-.."},
    {'M', "--"},    {'N', "-."},    {'O', "---"},   {'P', ".--."},
    {'Q', "--.-"},  {'R', ".-."},   {'S', "..."},   {'T', "-"},
    {'U', "..-"},   {'V', "...-"},  {'W', ".--"},   {'X', "-..-"},
    {'Y', "-.--"},  {'Z', "--.."},
    {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
    {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."},
    {'8', "---.."}, {'9', "----."},
    {'\0', ""}  // End marker
};

// State machine variables
static bool current_light_state = false;
static bool previous_light_state = false;
static int64_t pulse_start_time = 0;
static int64_t gap_start_time = 0;
static int64_t last_activity_time = 0;  // Track time of last signal
static char morse_buffer[10] = {0};  // Store dots/dashes for current letter
static int buffer_index = 0;

// Output collection
static char output_message[256] = {0};  // Store decoded message
static int output_index = 0;
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

/*---------------------------------------------------------------
        Morse Code Decoding Function
---------------------------------------------------------------*/
char decode_morse(const char* pattern) {
    if (pattern == NULL || pattern[0] == '\0') {
        return '\0';  // Empty pattern
    }

    // Search through morse table
    for (int i = 0; morse_table[i].letter != '\0'; i++) {
        if (strcmp(pattern, morse_table[i].code) == 0) {
            return morse_table[i].letter;
        }
    }

    return '?';  // Unknown pattern
}

void process_morse_buffer(void) {
    if (buffer_index > 0) {
        morse_buffer[buffer_index] = '\0';  // Null terminate

        char decoded = decode_morse(morse_buffer);

        if (decoded != '\0') {
            ESP_LOGI(TAG, "  → Decoded: '%s' = '%c'", morse_buffer, decoded);
            // Add to output message
            if (output_index < sizeof(output_message) - 1) {
                output_message[output_index++] = decoded;
            }
        } else {
            ESP_LOGI(TAG, "  → Unknown pattern: '%s'", morse_buffer);
        }

        // Clear buffer
        buffer_index = 0;
        memset(morse_buffer, 0, sizeof(morse_buffer));
    }
}

void app_main(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, PHOTODIODE_ADC_CHAN, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_handle = NULL;
    bool do_calibration = example_adc_calibration_init(ADC_UNIT_1, PHOTODIODE_ADC_CHAN, EXAMPLE_ADC_ATTEN, &adc1_cali_handle);

    ESP_LOGI(TAG, "Morse Code Receiver Ready - 10x faster for 10 chars/sec");
    ESP_LOGI(TAG, "Waiting for signal on GPIO2...");
    ESP_LOGI(TAG, "Light threshold: %d (raw ADC value)", LIGHT_THRESHOLD);

    // Initialize timing
    gap_start_time = esp_timer_get_time() / 1000;  // Convert to ms
    last_activity_time = gap_start_time;

    ESP_LOGI(TAG, "Starting Morse code detection...");
    ESP_LOGI(TAG, "Send Morse code from Pi now!");

    // Main loop - Morse code detection state machine
    while (1) {
        // Read ADC from photodiode
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, PHOTODIODE_ADC_CHAN, &adc_raw_value));

        int64_t current_time = esp_timer_get_time() / 1000;  // Convert to ms

        // Determine current light state (ON or OFF)
        current_light_state = (adc_raw_value > LIGHT_THRESHOLD);

        // Detect rising edge (light turns ON)
        if (current_light_state && !previous_light_state) {
            int64_t gap_duration = current_time - gap_start_time;

            // Check if gap indicates end of letter or word
            if (gap_duration >= WORD_GAP_MS) {
                ESP_LOGI(TAG, "Word gap detected (%lld ms)", gap_duration);
                process_morse_buffer();
                // Add space to output
                if (output_index < sizeof(output_message) - 1) {
                    output_message[output_index++] = ' ';
                }
            } else if (gap_duration >= LETTER_GAP_MS) {
                ESP_LOGI(TAG, "Letter gap detected (%lld ms)", gap_duration);
                process_morse_buffer();
            }

            pulse_start_time = current_time;
            last_activity_time = current_time;
        }
        // Detect falling edge (light turns OFF)
        else if (!current_light_state && previous_light_state) {
            int64_t pulse_duration = current_time - pulse_start_time;

            // Classify pulse as dot or dash
            if (pulse_duration >= DASH_MIN_MS) {
                // Dash detected
                if (buffer_index < sizeof(morse_buffer) - 1) {
                    morse_buffer[buffer_index++] = '-';
                    ESP_LOGI(TAG, "Dash detected (%lld ms)", pulse_duration);
                }
            } else if (pulse_duration >= (DOT_DURATION_MS / 2)) {
                // Dot detected (with some tolerance)
                if (buffer_index < sizeof(morse_buffer) - 1) {
                    morse_buffer[buffer_index++] = '.';
                    ESP_LOGI(TAG, "Dot detected (%lld ms)", pulse_duration);
                }
            }

            gap_start_time = current_time;
            last_activity_time = current_time;
        }

        // Timeout: if no activity for LETTER_GAP_MS and buffer has data, decode it
        if (!current_light_state && buffer_index > 0) {
            int64_t idle_time = current_time - last_activity_time;
            if (idle_time > LETTER_GAP_MS) {
                process_morse_buffer();
                last_activity_time = current_time;  // Reset after processing
            }
        }

        // Print complete output if idle for very long (end of message)
        static int64_t last_print_time = 0;
        if (!current_light_state && (current_time - last_activity_time) > WORD_GAP_MS * 2) {
            if (current_time - last_print_time > WORD_GAP_MS * 2 && output_index > 0) {
                // Null terminate and print final output
                output_message[output_index] = '\0';

                ESP_LOGI(TAG, "");
                ESP_LOGI(TAG, "================================");
                ESP_LOGI(TAG, "   Transmission Complete!");
                ESP_LOGI(TAG, "================================");
                ESP_LOGI(TAG, "Output: %s", output_message);
                ESP_LOGI(TAG, "================================");
                ESP_LOGI(TAG, "");

                // Reset output buffer for next message
                memset(output_message, 0, sizeof(output_message));
                output_index = 0;

                last_print_time = current_time;
            }
        }

        previous_light_state = current_light_state;

        // Ensure we delay at least 1 tick to feed the watchdog
        int delay_ticks = pdMS_TO_TICKS(SAMPLE_RATE_MS);
        if (delay_ticks == 0) {
            delay_ticks = 1;
        }
        vTaskDelay(delay_ticks);
    }

    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration) {
        example_adc_calibration_deinit(adc1_cali_handle);
    }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
