#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

float globalEnergySpent = 0.0f;

float evCalculateWh(float current, float voltage, uint32_t timeMs)
{
    // Переводим время из миллисекунд в часы
    float time_hours = timeMs / (1000.0f * 3600.0f);

    // Вычисляем затраченную энергию в ватт-часах
    float energy = current * voltage * time_hours;

    return energy;
}

void CanDecode1DBMessage(uint8_t data[8])
{
    int16_t current;
    uint16_t voltage;

    // 0.5A/bit
    current = (data[0] << 3) | (data[1] & 0xe0) >> 5;
    if (current & 0x0400)
    {
        current |= 0xf800;
    }

    // 0.5V/bit
    voltage = (data[2] << 2) | ((data[3] & 0xc0) >> 6);

    // 3FF is unavailable value. Can happen directly on reboot.
    if (voltage == 0x3ff)
    {
        return;
    }

    float voltage1 = voltage / 2.0f;
    float current1 = current / 2.0f;

    float energySpent = evCalculateWh(current1, voltage1, 10); // 10ms

    fprintf(stdout, "Voltage: %.2fV, Current: %.2fA, Energy Spent: %.10fWh\n", voltage1, current1, energySpent);

    globalEnergySpent += energySpent;
}

int main() {
    FILE *file = fopen("ev_1db.txt", "r"); // Updated file name
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    double start_time = -1.0;

    while (fgets(line, sizeof(line), file)) {
        double timestamp;
        char can_id[16];
        char can_data[64];

        // Corrected sscanf format string to match the input line structure
        if (sscanf(line, " (%lf) %*s %15s %*c%*d%*c %63[^\n]", &timestamp, can_id, can_data) == 3) {
            if (start_time < 0) {
                start_time = timestamp;
            }

            double relative_time = timestamp - start_time;
            int seconds = (int)relative_time;
            int milliseconds = (int)((relative_time - seconds) * 1000);

            uint8_t data[8];
            char *token = strtok(can_data, " ");
            for (int i = 0; i < 8 && token != NULL; i++) {
                sscanf(token, "%2hhx", &data[i]);
                token = strtok(NULL, " ");
            }

            printf("%d.%03d - ", seconds, milliseconds);
            for (int i = 0; i < 8; i++) {
                printf("%02X ", data[i]);
            }
            printf(" - ");

            CanDecode1DBMessage(data);
        } else {
            fprintf(stderr, "Failed to parse line: %s", line); // Changed to stderr for better debugging
        }
    }

    fprintf(stdout, "Total Energy Spent: %.10fWh\n", globalEnergySpent);

    fclose(file);
    return 0;
}
