#include "clif.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

int main() {
    packetdb_readdb();

    std::ofstream out("/mnt/e/Documentos/Repositórios/mythofthor_project/Config/packet_lengths_20211103.json");
    out << "{\n";
    bool first = true;
    int count = 0;
    for (int i = 0; i <= MAX_PACKET_DB; ++i) {
        if (packet_db[i].len != 0) {
            if (!first) out << ",\n";
            first = false;
            char hex_buf[16];
            snprintf(hex_buf, sizeof(hex_buf), "0x%04x", i);
            out << "  \"" << hex_buf << "\": " << packet_db[i].len;
            count++;
        }
    }
    out << "\n}\n";
    std::cout << "Successfully extracted " << count << " packet lengths!" << std::endl;
    return 0;
}
