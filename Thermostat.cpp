#include "mbed.h"
#include "board_freedom.h"
#include "adc.h"
#include "oled_ssd1322.h"
#include <cstdint>
#include <cstdio>

#define MESSAGE_MAX_SIZE 50

#define ADC_MAX_VALUE 65535.0
#define ADC_VOLTAGE_REF 3.0
#define TEMP_COEFFICIENT 19.5
#define TEMP_OFFSET 400

#define SEC_THRESHOLD_LOW 0
#define SEC_THRESHOLD_HIGH 33000

bool SEC(uint16_t analog_in_value) {
    if (analog_in_value < SEC_THRESHOLD_LOW || analog_in_value > SEC_THRESHOLD_HIGH) {
        return true;
    } else {
        return false; 
    }
}

void handle_temperature(float temperature, PwmOut& heater_power, DigitalOut& green_led, DigitalOut& red_led) {
    static bool heater_status = false;  // Static variable to remember heater status

    if (temperature > 35) {
        heater_power = 0;   // Heater off
        heater_status = false;  // Update heater status
        green_led = 0;      // Green LED off
        red_led = 0;        // Red LED off
    } else if (temperature < 30) {
        heater_power = 1;   // Heater on
        heater_status = true;  // Update heater status
        red_led = 1;        // Red LED on when heater is on
        green_led = 0;      // Green LED off
    } else {
        // Maintain the previous state of the heater
        heater_power = heater_status; // Heater stays as it was
        green_led = 1; 
        red_led = heater_status ? 1 : 0;  // Red LED on only if heater is on
    }
}

int main() {
    board_init();
    u8g2_ClearBuffer(&oled);
    u8g2_SetFont(&oled, u8g2_font_6x12_mr);
    u8g2_SendBuffer(&oled);
    PwmOut heater_power(PTC2);
    DigitalOut green_led(PTB3, 0);
    DigitalOut red_led(PTB2, 0);

    heater_power = 1;

    char message[MESSAGE_MAX_SIZE + 1];
    message[MESSAGE_MAX_SIZE] = '\0';

    while (true) {
        // Read analog and convert
        uint16_t analog_in_value = adc_read(0);
        
        // Updated conversion formula for more accurate temperature calculation
        float voltage = (analog_in_value / ADC_MAX_VALUE) * ADC_VOLTAGE_REF;
        
        // Safety control
        bool safetyControl = SEC(analog_in_value);
        
        // Only calculate temperature if the sensor is present
        float temperature;
        if (!safetyControl) {
            temperature = (voltage * 1000 - TEMP_OFFSET) / TEMP_COEFFICIENT;
        } else {
            temperature = 0; /
        }

        handle_temperature(temperature, heater_power, green_led, red_led);

        // Construct message
        if (!safetyControl) {
            snprintf(message, MESSAGE_MAX_SIZE, "Value is %-5d, temperature is %5.02f", analog_in_value, temperature);
        } else {
            snprintf(message, MESSAGE_MAX_SIZE, "Sensor absence detected. Temperature reading not available.");
        }

        // Clear screen and write a message.
        u8g2_ClearBuffer(&oled);
        u8g2_DrawUTF8(&oled, 10, 10, message);
        u8g2_SendBuffer(&oled);
        
        // Also write the message over USB to the serial monitor
        printf("%s\n", message);

        // Wait a bit
        ThisThread::sleep_for(100ms);
    }
}
