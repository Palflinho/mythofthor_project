import json

with open('Config/packet_lengths_20211103.json') as f:
    packets = json.load(f)

# Override / ensure known client-server packets
packets['0x0ac4'] = -1  # AC_ACCEPT_LOGIN
packets['0x083e'] = 26  # AC_REFUSE_LOGIN
packets['0x0081'] = 3   # SC_NOTIFY_BAN
packets['0x006b'] = -1  # HC_ACCEPT_ENTER
packets['0x006c'] = 3   # HC_REFUSE_ENTER
packets['0x0ac5'] = 156 # HC_NOTIFY_ZONESVR
packets['0x082d'] = -1  # HC_CHAR_SLOT_INFO
packets['0x02eb'] = 13  # ZC_ACCEPT_ENTER
packets['0x0074'] = 3   # ZC_REFUSE_ENTER
packets['0x0080'] = 7   # ZC_NOTIFY_VANISH
packets['0x0087'] = 12  # ZC_NOTIFY_PLAYERMOVE
packets['0x08c8'] = 34  # ZC_NOTIFY_ACT
packets['0x00b0'] = 8   # ZC_PAR_CHANGE
packets['0x00b1'] = 8   # ZC_LONGPAR_CHANGE
packets['0x01d7'] = 15  # ZC_SPRITE_CHANGE
packets['0x09ff'] = 108 # idle_unitType
packets['0x09fe'] = 107 # spawn_unitType
packets['0x09fd'] = 114 # unit_walkingType

sorted_items = sorted([(int(k, 16), v) for k, v in packets.items()])

max_id = max(k for k, v in sorted_items)
print(f"Max Packet ID: 0x{max_id:04x} ({max_id})")

header_content = """#pragma once

#include "CoreMinimal.h"

/**
 * Tabela de tamanhos de pacotes para o protocolo rAthena (PACKETVER 20211103).
 * Gerado automaticamente diretamente dos fontes e binários do servidor rAthena.
 * 
 * Regra de framing:
 *  - Retorno > 0 : Pacote de tamanho fixo em bytes.
 *  - Retorno == -1: Pacote de tamanho dinâmico (comprimento total informado em uint16 nos bytes 2 e 3).
 *  - Retorno == 0 : Pacote desconhecido ou não registrado.
 */
namespace RagnarokPackets
{
    // Constantes de Cabeçalho dos Pacotes Canônicos
    static constexpr uint16 HEADER_CA_LOGIN             = 0x0064; // 55 bytes
    static constexpr uint16 HEADER_AC_ACCEPT_LOGIN      = 0x0ac4; // Dinâmico (-1)
    static constexpr uint16 HEADER_AC_REFUSE_LOGIN      = 0x083e; // 26 bytes
    static constexpr uint16 HEADER_SC_NOTIFY_BAN        = 0x0081; // 3 bytes

    static constexpr uint16 HEADER_CH_ENTER             = 0x0065; // 17 bytes
    static constexpr uint16 HEADER_HC_ACCEPT_ENTER      = 0x006b; // Dinâmico (-1)
    static constexpr uint16 HEADER_HC_REFUSE_ENTER      = 0x006c; // 3 bytes
    static constexpr uint16 HEADER_CH_SELECT_CHAR       = 0x0066; // 3 bytes
    static constexpr uint16 HEADER_HC_NOTIFY_ZONESVR    = 0x0ac5; // 156 bytes
    static constexpr uint16 HEADER_HC_CHAR_SLOT_INFO    = 0x082d; // Dinâmico (-1)

    static constexpr uint16 HEADER_CZ_ENTER             = 0x0888; // 19 bytes
    static constexpr uint16 HEADER_ZC_ACCEPT_ENTER      = 0x02eb; // 13 bytes
    static constexpr uint16 HEADER_ZC_REFUSE_ENTER      = 0x0074; // 3 bytes
    static constexpr uint16 HEADER_CZ_NOTIFY_ACTORINIT  = 0x007d; // 2 bytes
    static constexpr uint16 HEADER_CZ_REQUEST_MOVE      = 0x035f; // 5 bytes
    static constexpr uint16 HEADER_ZC_NOTIFY_PLAYERMOVE = 0x0087; // 12 bytes
    static constexpr uint16 HEADER_CZ_REQUEST_ACT       = 0x088e; // 7 bytes (ou 0x0360)
    static constexpr uint16 HEADER_ZC_NOTIFY_ACT        = 0x08c8; // 34 bytes
    static constexpr uint16 HEADER_ZC_NOTIFY_VANISH     = 0x0080; // 7 bytes
    static constexpr uint16 HEADER_ZC_PAR_CHANGE        = 0x00b0; // 8 bytes
    static constexpr uint16 HEADER_ZC_LONGPAR_CHANGE    = 0x00b1; // 8 bytes
    static constexpr uint16 HEADER_ZC_SPRITE_CHANGE     = 0x01d7; // 15 bytes
    static constexpr uint16 HEADER_ZC_IDLE_UNIT         = 0x09ff; // 108 bytes
    static constexpr uint16 HEADER_ZC_SPAWN_UNIT        = 0x09fe; // 107 bytes
    static constexpr uint16 HEADER_ZC_UNIT_WALKING      = 0x09fd; // 114 bytes

    /**
     * Retorna o comprimento do pacote de acordo com a tabela de packetdb do rAthena.
     */
    inline int16 GetPacketLength(uint16 PacketID)
    {
        switch (PacketID)
        {
"""

for pid, length in sorted_items:
    header_content += f"            case 0x{pid:04x}: return {length};\n"

header_content += """            default: return 0;
        }
    }

    /**
     * Desempacota coordenadas de célula única (RBUFPOS do rAthena: 3 bytes -> X: 10 bits, Y: 10 bits, Dir: 4 bits).
     */
    inline void UnpackCellCoord(const uint8* Data, int32& OutX, int32& OutY, uint8& OutDir)
    {
        if (!Data) return;
        OutX = ((static_cast<int32>(Data[0]) & 0xFF) << 2) | (Data[1] >> 6);
        OutY = ((static_cast<int32>(Data[1]) & 0x3F) << 4) | (Data[2] >> 4);
        OutDir = Data[2] & 0x0F;
    }

    /**
     * Desempacota caminho de movimento (RBUFPOS2 do rAthena: 6 bytes -> Origem X/Y, Destino X/Y).
     */
    inline void UnpackMovePath(const uint8* Data, int32& OutFromX, int32& OutFromY, int32& OutToX, int32& OutToY)
    {
        if (!Data) return;
        OutFromX = ((static_cast<int32>(Data[0]) & 0xFF) << 2) | (Data[1] >> 6);
        OutFromY = ((static_cast<int32>(Data[1]) & 0x3F) << 4) | (Data[2] >> 4);
        OutToX   = ((static_cast<int32>(Data[2]) & 0x0F) << 6) | (Data[3] >> 2);
        OutToY   = ((static_cast<int32>(Data[3]) & 0x03) << 8) | (Data[4]);
    }
}
"""

with open('Source/MythOfThor/Network/RagnarokPacketDatabase.h', 'w', encoding='utf-8') as f:
    f.write(header_content)

print(f"Generated Source/MythOfThor/Network/RagnarokPacketDatabase.h successfully with {len(sorted_items)} packets!")
