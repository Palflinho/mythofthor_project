#include <iostream>
#include <map>
#include <fstream>
#include <cstdio>

std::map<int, int> packet_lengths;

void packetdb_addpacket(int cmd, int length, ...) {
    packet_lengths[cmd] = length;
}

#define packet(cmd, length) packetdb_addpacket(cmd, length)
#define parseable_packet(cmd, length, ...) packetdb_addpacket(cmd, length)

#define PACKETVER 20211103
#define PACKETVER_RE_NUM 20211103
#define PACKETVER_MAIN_NUM 20211103
#define PACKETVER_ZERO_NUM 0

// Dummy definitions for handler functions referenced in clif_packetdb.hpp
#define clif_parse_WantToConnection nullptr
#define clif_parse_LoadEndAck nullptr
#define clif_parse_TickSend nullptr
#define clif_parse_WalkToXY nullptr
#define clif_parse_ActionRequest nullptr
#define clif_parse_GlobalMessage nullptr
#define clif_parse_dull nullptr
#define clif_parse_Mail_Receiver_Check nullptr
#define clif_parse_Mail_return nullptr
#define clif_parse_unequipall nullptr
#define clif_parse_item_reform_close nullptr
#define clif_parse_item_reform_start nullptr
#define clif_parse_enchantgrade_start nullptr
#define clif_parse_enchantgrade_close nullptr
#define clif_parse_enchantwindow_general nullptr
#define clif_parse_enchantwindow_perfect nullptr
#define clif_parse_enchantwindow_upgrade nullptr
#define clif_parse_enchantwindow_reset nullptr
#define clif_parse_enchantwindow_close nullptr

// Dummy packet structs if needed
#define DEFINE_DUMMY_STRUCT(name, sz) struct name { char dummy[sz]; };
struct PACKET_CZ_GRADE_ENCHANT_REQUEST { char d[10]; };
struct PACKET_CZ_GRADE_ENCHANT_CLOSE_UI { char d[2]; };
struct PACKET_CZ_REQUEST_RANDOM_ENCHANT { char d[10]; };
struct PACKET_CZ_REQUEST_PERFECT_ENCHANT { char d[10]; };
struct PACKET_CZ_REQUEST_UPGRADE_ENCHANT { char d[10]; };
struct PACKET_CZ_REQUEST_RESET_ENCHANT { char d[10]; };
struct PACKET_CZ_CLOSE_UI_ENCHANT { char d[2]; };
struct PACKET_CZ_CHECKNAME2 { char d[28]; };
struct PACKET_CZ_RODEX_RETURN { char d[14]; };
struct PACKET_CZ_REQ_TAKEOFF_EQUIP_ALL { char d[2]; };
struct PACKET_CZ_CLOSE_REFORM_UI { char d[2]; };
struct PACKET_CZ_ITEM_REFORM { char d[10]; };

#define HEADER_CZ_GRADE_ENCHANT_REQUEST 0x0b8a
#define HEADER_CZ_GRADE_ENCHANT_CLOSE_UI 0x0b8b
#define HEADER_CZ_REQUEST_RANDOM_ENCHANT 0x0b8e
#define HEADER_CZ_REQUEST_PERFECT_ENCHANT 0x0b8f
#define HEADER_CZ_REQUEST_UPGRADE_ENCHANT 0x0b90
#define HEADER_CZ_REQUEST_RESET_ENCHANT 0x0b91
#define HEADER_CZ_CLOSE_UI_ENCHANT 0x0b92
#define HEADER_CZ_CHECKNAME2 0x0a10
#define HEADER_CZ_RODEX_RETURN 0x0a11
#define HEADER_CZ_REQ_TAKEOFF_EQUIP_ALL 0x0b7e
#define HEADER_CZ_CLOSE_REFORM_UI 0x0b80
#define HEADER_CZ_ITEM_REFORM 0x0b81

void init_packets() {
#include "/home/palf/mythofthor-server/rathena/src/map/clif_packetdb.hpp"
}

int main() {
    init_packets();
    std::ofstream out("Config/packet_lengths_20211103.json");
    out << "{\n";
    bool first = true;
    for (auto const& pair : packet_lengths) {
        if (!first) out << ",\n";
        first = false;
        char hex_buf[16];
        snprintf(hex_buf, sizeof(hex_buf), "0x%04x", pair.first);
        out << "  \"" << hex_buf << "\": " << pair.second;
    }
    out << "\n}\n";
    std::cout << "Successfully dumped " << packet_lengths.size() << " packet lengths to Config/packet_lengths_20211103.json!" << std::endl;
    return 0;
}
