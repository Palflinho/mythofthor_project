import socket
import struct
import time
import sys

LOGIN_HOST = "127.0.0.1"
LOGIN_PORT = 6900
USERNAME = "mythtest"
PASSWORD = "mythtest123"

def run_test():
    print("=== [1] Conectando ao Login Server (6900) ===")
    s_login = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_login.settimeout(5.0)
    s_login.connect((LOGIN_HOST, LOGIN_PORT))

    # CA_LOGIN (0x0064) - 55 bytes
    ca_login = struct.pack("<HI24s24sB", 0x0064, 20211103, USERNAME.encode('ascii').ljust(24, b'\x00'), PASSWORD.encode('ascii').ljust(24, b'\x00'), 0)
    s_login.sendall(ca_login)

    resp = s_login.recv(1024)
    print(f"Recebido do Login Server: {len(resp)} bytes")
    cmd, = struct.unpack_from("<H", resp, 0)
    print(f"Packet recebido: 0x{cmd:04x}")
    if cmd == 0x083e:
        err, = struct.unpack_from("<I", resp, 2)
        print(f"Login falhou com erro: {err}")
        return False
    elif cmd != 0x0ac4:
        print(f"Packet inesperado: 0x{cmd:04x}")
        return False

    packet_type, packet_len, login_id1, account_id, login_id2, last_ip = struct.unpack_from("<HHIIII", resp, 0)
    sex = resp[46]
    token = resp[47:64]
    print(f"Login OK! AccountID: {account_id}, LoginID1: {login_id1}, LoginID2: {login_id2}, Sex: {sex}")

    char_ip_raw, char_port, char_name = struct.unpack_from("<IH20s", resp, 64)
    char_ip = socket.inet_ntoa(struct.pack("<I", char_ip_raw))
    print(f"Char Server: {char_name.decode('ascii', errors='ignore').strip(chr(0))} em {char_ip}:{char_port}")
    s_login.close()

    print("\n=== [2] Conectando ao Char Server (6121) ===")
    s_char = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_char.settimeout(5.0)
    s_char.connect((LOGIN_HOST, char_port if char_port != 0 else 6121))

    # CH_ENTER (0x0065) - 17 bytes
    ch_enter = struct.pack("<HIIIHB", 0x0065, account_id, login_id1, login_id2, 0, sex)
    s_char.sendall(ch_enter)

    # First receive raw account_id (4 bytes)
    ack_aid = s_char.recv(4)
    print(f"Char Server ACK AID: {len(ack_aid)} bytes -> {struct.unpack('<I', ack_aid)[0]}")

    # Next receive packets (e.g. 0x082d and 0x006b)
    char_data = s_char.recv(4096)
    offset = 0
    char_gid = None
    char_name = ""
    while offset < len(char_data):
        p_cmd, = struct.unpack_from("<H", char_data, offset)
        if p_cmd == 0x006b:
            p_len, = struct.unpack_from("<H", char_data, offset + 2)
            # Character info at offset + 27
            char_info_raw = char_data[offset + 27 : offset + 27 + 175]
            char_gid, = struct.unpack_from("<I", char_info_raw, 0)
            char_name = char_info_raw[108:132].decode('utf-8', errors='ignore').strip(chr(0))
            print(f"0x006b recebido! Personagem: GID={char_gid}, Nome='{char_name}'")
            offset += p_len
        elif p_cmd == 0x082d:
            p_len, = struct.unpack_from("<H", char_data, offset + 2)
            print(f"0x082d (CharSlotInfo) recebido! Len={p_len}")
            offset += p_len
        else:
            print(f"Outro pacote do Char Server: 0x{p_cmd:04x}")
            break

    # Select character (0x0066) - 3 bytes (cmd: 0x0066, slot: 0)
    ch_select = struct.pack("<HB", 0x0066, 0)
    s_char.sendall(ch_select)

    # Receive HC_NOTIFY_ZONESVR (0x0ac5) - 156 bytes
    zone_data = s_char.recv(1024)
    z_cmd, z_cid = struct.unpack_from("<HI", zone_data, 0)
    z_map = zone_data[6:22].decode('ascii', errors='ignore').strip(chr(0))
    z_ip_raw, z_port = struct.unpack_from("<IH", zone_data, 22)
    z_ip = socket.inet_ntoa(struct.pack("<I", z_ip_raw))
    print(f"Zone Server Notify: 0x{z_cmd:04x}, CID={z_cid}, Map='{z_map}', Zone={z_ip}:{z_port}")
    s_char.close()

    print("\n=== [3] Conectando ao Map Server (5121) ===")
    s_map = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_map.settimeout(5.0)
    s_map.connect((LOGIN_HOST, z_port if z_port != 0 else 5121))

    # CZ_ENTER (0x0888) - 19 bytes (for PACKETVER 20211103)
    # cmd (2B), account_id (4B), char_id (4B), login_id1 (4B), client_tick (4B), sex (1B)
    client_tick = int(time.time() * 1000) & 0xFFFFFFFF
    cz_enter = struct.pack("<HIIIIB", 0x0888, account_id, z_cid, login_id1, client_tick, sex)
    s_map.sendall(cz_enter)

    # Helper function for Ragnarok cell coordinate packing (WBUFPOS)
    def pack_pos(x, y, dir_val=0):
        b0 = (x >> 2) & 0xFF
        b1 = ((x << 6) & 0xC0) | ((y >> 4) & 0x3F)
        b2 = ((y << 4) & 0xF0) | (dir_val & 0x0F)
        return bytes([b0, b1, b2])

    def unpack_pos(p):
        x = (p[0] << 2) | (p[1] >> 6)
        y = ((p[1] & 0x3F) << 4) | (p[2] >> 4)
        dir_val = p[2] & 0x0F
        return x, y, dir_val

    # Wait for ZC_ACCEPT_ENTER (0x02eb)
    got_accept_enter = False
    deadline = time.time() + 5.0
    initial_buffer = b""

    while time.time() < deadline and not got_accept_enter:
        chunk = s_map.recv(4096)
        if not chunk:
            break
        print(f"Map Server raw chunk received ({len(chunk)} bytes): {chunk.hex()}")
        initial_buffer += chunk
        if b'\xeb\x02' in initial_buffer:
            idx = initial_buffer.find(b'\xeb\x02')
            if idx + 13 <= len(initial_buffer):
                got_accept_enter = True
                start_tick, = struct.unpack_from("<I", initial_buffer, idx + 2)
                cur_x, cur_y, cur_dir = unpack_pos(initial_buffer[idx + 6 : idx + 9])
                print(f">>> ZC_ACCEPT_ENTER (0x02eb) recebido! Pos inicial: ({cur_x}, {cur_y}), Dir: {cur_dir}, Tick: {start_tick}")
                break
        elif b'\x74\x00' in initial_buffer:
            idx = initial_buffer.find(b'\x74\x00')
            err = initial_buffer[idx + 2]
            print(f"ZC_REFUSE_ENTER: erro {err}")
            return False

    if not got_accept_enter:
        print("Falha ao receber ZC_ACCEPT_ENTER")
        return False

    # Send CZ_NOTIFY_ACTORINIT (0x007d) - 2 bytes
    cz_load = struct.pack("<H", 0x007d)
    s_map.sendall(cz_load)
    print("Enviado CZ_NOTIFY_ACTORINIT (0x007d)")

    # Receive initial world packets (idle units, player info, etc.)
    time.sleep(0.3)
    s_map.settimeout(1.0)
    world_packets = b""
    try:
        while True:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            world_packets += chunk
            if len(chunk) < 4096:
                break
    except (socket.timeout, BlockingIOError):
        pass
    print(f"Recebidos {len(world_packets)} bytes de pacotes do mundo no Map Server!")

    # Test move request (CZ_REQUEST_MOVE: 0x035F) - 5 bytes
    # Move to (158, 182)
    target_x, target_y = 158, 182
    cz_move = struct.pack("<H", 0x035F) + pack_pos(target_x, target_y)
    s_map.settimeout(5.0)
    s_map.sendall(cz_move)
    print(f"Enviado CZ_REQUEST_MOVE (0x035F) para ({target_x}, {target_y}) - Hex: {cz_move.hex()}")

    # Listen for ZC_NOTIFY_PLAYERMOVE (0x0087)
    move_resp = b""
    got_move = False
    start_move_wait = time.time()
    while time.time() - start_move_wait < 5.0 and not got_move:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            move_resp += chunk
            if b'\x87\x00' in move_resp:
                # Find 0x0087 offset
                offset_87 = move_resp.find(b'\x87\x00')
                if offset_87 + 12 <= len(move_resp):
                    m_tick, = struct.unpack_from("<I", move_resp, offset_87 + 2)
                    # moveData: x0, y0, x1, y1
                    md = move_resp[offset_87 + 6 : offset_87 + 12]
                    # unpack WBUFPOS2:
                    x0 = (md[0] << 2) | (md[1] >> 6)
                    y0 = ((md[1] & 0x3F) << 4) | (md[2] >> 4)
                    x1 = ((md[2] & 0x0F) << 6) | (md[3] >> 2)
                    y1 = ((md[3] & 0x03) << 8) | md[4]
                    print(f">>> SUCESSO: ZC_NOTIFY_PLAYERMOVE (0x0087) recebido! Rota: ({x0}, {y0}) -> ({x1}, {y1}), Tick: {m_tick}")
                    got_move = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    if not got_move:
        print(f"Bytes recebidos após movimento ({len(move_resp)} bytes): {move_resp[:64].hex()}")
        return False

    s_map.close()
    print("\n>>> FLUXO COMPLETO LOGIN -> CHAR -> MAP VALIDADO COM SUCESSO! <<<")
    return True

if __name__ == "__main__":
    ok = run_test()
    sys.exit(0 if ok else 1)
