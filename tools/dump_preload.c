#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct s_packet_db {
    int16_t len;
    void (*func)(int32_t, void *);
    int16_t pos[20];
};

extern struct s_packet_db packet_db[0xCFF + 1];

__attribute__((destructor)) void dump() {
    FILE *f = fopen("/tmp/packet_lengths.json", "w");
    if (!f) {
        perror("fopen");
        return;
    }
    fprintf(f, "{\n");
    int count = 0;
    for (int i = 0; i <= 0xCFF; i++) {
        if (packet_db[i].len != 0) {
            if (count > 0) fprintf(f, ",\n");
            fprintf(f, "  \"0x%04x\": %d", i, (int)packet_db[i].len);
            count++;
        }
    }
    fprintf(f, "\n}\n");
    fclose(f);
    printf("Successfully dumped %d packets to /tmp/packet_lengths.json\n", count);
}
