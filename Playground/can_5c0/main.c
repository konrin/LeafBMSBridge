#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 256

uint32_t cellVoltMin = 0;

uint32_t unpack_value_from_byte(const uint8_t *data, int start_bit, int bit_length) {
    uint32_t value = 0;
    int bits_remaining = bit_length;
    int bit_pos = start_bit;  // позиция старшего бита в первом переданном байте (0..7, где 7 — самый старший)

    // Если поле полностью умещается в одном байте, то цикл выполнится один раз.
    // Если же поле занимает несколько байт, то data будет указывать на следующий байт при переходе.
    while (bits_remaining > 0) {
        // Сколько бит можно извлечь из текущего байта, начиная с bit_pos
        int bits_from_here = bit_pos + 1;
        if (bits_from_here > bits_remaining)
            bits_from_here = bits_remaining;

        // Сдвигаем текущий байт вправо так, чтобы нужные биты оказались в младших разрядах,
        // затем замаскируем их.
        int shift = bit_pos - bits_from_here + 1;
        uint8_t part = (data[0] >> shift) & ((1 << bits_from_here) - 1);

        // Добавляем извлечённые биты к накопленному значению
        value = (value << bits_from_here) | part;

        bits_remaining -= bits_from_here;
        data++;         // переходим к следующему байту, если поле продолжается
        bit_pos = 7;    // в следующих байтах начинаем с самого старшего бита
    }
    return value;
}

void CanDecode5C0Message(uint8_t data[8])
{
    u_int8_t mx = data[0] >> 6;

    float tempMax = .0;
    float tempMin = 0;
    float tempAvg = 0;

    uint32_t cellVoltMax = 0;

    uint32_t cellVoltAvg = 0;

    bool battery_HeatExist = (data[4] & 0x01);
    bool battery_Heating_Stop = ((data[0] & 0x10) >> 4);
    bool battery_Heating_Start = ((data[0] & 0x20) >> 5);
    bool battery_Batt_Heater_Mail_Send_Request = (data[1] & 0x01);

    uint32_t rawVoltage = data[5] >> 2;//(data[5] >> 2) & 0x3F;

    uint32_t decoded = unpack_value_from_byte(&data[5], 7, 6);

    // Вычисляем физическое значение напряжения (в mV)
    uint32_t voltage = rawVoltage * 40 + 1900;

    switch (mx)
    {
    case 1:
        printf("MX = 1 (max)");
        tempMax = ((data[2] / 2) - 40);
        printf(" Temp: %.2f", tempMax);

        printf(" Cell: %d", (rawVoltage * 5) + cellVoltMin);

        break;
    case 2:
        printf("MX = 2 (avg)");
        tempAvg = ((data[2] / 2) - 40);
        printf(" Temp: %.2f", tempAvg);

        printf(" Cell: %d", voltage);
        break;
    case 3:
        printf("MX = 3 (min)");
        tempMin = ((data[2] / 2) - 40);
        printf(" Temp: %.2f", tempMin);

        cellVoltMin = voltage;

        printf(" Cell: %d", voltage);

        break;
    default:
        printf("Unknown MX value");
        break;
    }

    // printf(" Heat Exist: %s", battery_HeatExist ? "true" : "false");
    // printf(" Heating Stop: %s", battery_Heating_Stop ? "true" : "false");
    // printf(" Heating Start: %s", battery_Heating_Start ? "true" : "false");
    // printf(" Heater Mail Send Request: %s", battery_Batt_Heater_Mail_Send_Request ? "true" : "false");

    printf("\n");
}

int main()
{
    FILE *file = fopen("5c0_03.31_3.64_3.62_restart.txt", "r"); // Updated file name
    if (!file)
    {
        perror("Failed to open file");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    double start_time = -1.0;

    while (fgets(line, sizeof(line), file))
    {
        double timestamp;
        char can_id[16];
        char can_data[64];

        // Corrected sscanf format string to match the input line structure
        if (sscanf(line, " (%lf) %*s %15s %*c%*d%*c %63[^\n]", &timestamp, can_id, can_data) == 3)
        {
            if (start_time < 0)
            {
                start_time = timestamp;
            }

            double relative_time = timestamp - start_time;
            int seconds = (int)relative_time;
            int milliseconds = (int)((relative_time - seconds) * 1000);

            uint8_t data[8];
            char *token = strtok(can_data, " ");
            for (int i = 0; i < 8 && token != NULL; i++)
            {
                sscanf(token, "%2hhx", &data[i]);
                token = strtok(NULL, " ");
            }

            printf("%d.%03d - ", seconds, milliseconds);
            for (int i = 0; i < 8; i++)
            {
                printf("%02X ", data[i]);
            }
            printf(" - ");

            CanDecode5C0Message(data);
        }
        else
        {
            fprintf(stderr, "Failed to parse line: %s", line); // Changed to stderr for better debugging
        }
    }

    fclose(file);
    return 0;
}
