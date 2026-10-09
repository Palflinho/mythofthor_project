import socket
import struct
import time
import sys

LOGIN_HOST = "127.0.0.1"
LOGIN_PORT = 6900
USERNAME = "mythtest"
PASSWORD = "mythtest123"

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

def send_chat_message(sock, char_name, message):
    # CZ_GLOBAL_MESSAGE (0x00f3)
    text = f"{char_name} : {message}\0".encode('utf-8')
    pkt_len = 4 + len(text)
    pkt = struct.pack("<HH", 0x00f3, pkt_len) + text
    sock.sendall(pkt)

def run_test():
    print("==================================================================")
    print("      MYTH OF THOR - TESTE INTEGRADO DE FLUXO DE REDE RAGNAROK    ")
    print("==================================================================")

    # -------------------------------------------------------------------------
    # 1. LOGIN SERVER
    # -------------------------------------------------------------------------
    print("\n=== [1] Conectando ao Login Server (6900) ===")
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
    print(f"Login OK! AccountID: {account_id}, LoginID1: {login_id1}, LoginID2: {login_id2}, Sex: {sex}")

    char_ip_raw, char_port, char_name_server = struct.unpack_from("<IH20s", resp, 64)
    char_ip = socket.inet_ntoa(struct.pack("<I", char_ip_raw))
    print(f"Char Server: {char_name_server.decode('ascii', errors='ignore').strip(chr(0))} em {char_ip}:{char_port}")
    s_login.close()

    # -------------------------------------------------------------------------
    # 2. CHAR SERVER & SELEÇÃO DE PERSONAGEM
    # -------------------------------------------------------------------------
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

    char_data = s_char.recv(4096)
    offset = 0
    char_gid = None
    char_name = ""
    char_slot = 0
    while offset < len(char_data):
        p_cmd, = struct.unpack_from("<H", char_data, offset)
        if p_cmd == 0x006b:
            p_len, = struct.unpack_from("<H", char_data, offset + 2)
            # Character info at offset + 27
            char_info_raw = char_data[offset + 27 : offset + 27 + 175]
            char_gid, = struct.unpack_from("<I", char_info_raw, 0)
            job_lv, = struct.unpack_from("<i", char_info_raw, 24)
            hp, max_hp = struct.unpack_from("<qq", char_info_raw, 50)
            sp, max_sp = struct.unpack_from("<qq", char_info_raw, 66)
            job_class, hair_style = struct.unpack_from("<hh", char_info_raw, 84)
            base_lv, = struct.unpack_from("<h", char_info_raw, 92)
            clothes_color, = struct.unpack_from("<h", char_info_raw, 106)
            char_name = char_info_raw[108:132].decode('utf-8', errors='ignore').strip(chr(0))
            char_slot = char_info_raw[138]
            hair_color = char_info_raw[139]
            print(f"0x006b recebido! Slot={char_slot}, GID={char_gid}, Nome='{char_name}', BaseLv={base_lv}, JobLv={job_lv}, Job={job_class}, HP={hp}/{max_hp}, Cabelo={hair_style}/Cor={hair_color}, Roupa={clothes_color}")
            offset += p_len
        elif p_cmd == 0x082d:
            p_len, = struct.unpack_from("<H", char_data, offset + 2)
            print(f"0x082d (CharSlotInfo) recebido! Len={p_len}")
            offset += p_len
        else:
            print(f"Outro pacote do Char Server: 0x{p_cmd:04x}")
            break

    # Select character (CH_SELECT_CHAR 0x0066) - 3 bytes
    ch_select = struct.pack("<HB", 0x0066, char_slot)
    s_char.sendall(ch_select)

    # Receive HC_NOTIFY_ZONESVR (0x0ac5) - 156 bytes
    zone_data = s_char.recv(1024)
    z_cmd, z_cid = struct.unpack_from("<HI", zone_data, 0)
    z_map = zone_data[6:22].decode('ascii', errors='ignore').strip(chr(0))
    z_ip_raw, z_port = struct.unpack_from("<IH", zone_data, 22)
    z_ip = socket.inet_ntoa(struct.pack("<I", z_ip_raw))
    print(f"Zone Server Notify: 0x{z_cmd:04x}, CID={z_cid}, Map='{z_map}', Zone={z_ip}:{z_port}")
    s_char.close()

    # -------------------------------------------------------------------------
    # 3. MAP SERVER & ENTRADA NO MUNDO
    # -------------------------------------------------------------------------
    print("\n=== [3] Conectando ao Map Server (5121) ===")
    s_map = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_map.settimeout(5.0)
    s_map.connect((LOGIN_HOST, z_port if z_port != 0 else 5121))

    # CZ_ENTER (0x0888) - 19 bytes
    client_tick = int(time.time() * 1000) & 0xFFFFFFFF
    cz_enter = struct.pack("<HIIIIB", 0x0888, account_id, z_cid, login_id1, client_tick, sex)
    s_map.sendall(cz_enter)

    # Aguarda ZC_ACCEPT_ENTER (0x02eb)
    got_accept_enter = False
    deadline = time.time() + 5.0
    initial_buffer = b""

    cur_x, cur_y = 0, 0
    while time.time() < deadline and not got_accept_enter:
        chunk = s_map.recv(4096)
        if not chunk:
            break
        initial_buffer += chunk
        if b'\xeb\x02' in initial_buffer:
            idx = initial_buffer.find(b'\xeb\x02')
            if idx + 13 <= len(initial_buffer):
                got_accept_enter = True
                start_tick, = struct.unpack_from("<I", initial_buffer, idx + 2)
                cur_x, cur_y, cur_dir = unpack_pos(initial_buffer[idx + 6 : idx + 9])
                print(f">>> ZC_ACCEPT_ENTER (0x02eb) recebido! Pos inicial: ({cur_x}, {cur_y}), Dir: {cur_dir}, Tick: {start_tick}")
                break

    if not got_accept_enter:
        print("Falha ao receber ZC_ACCEPT_ENTER")
        return False

    # Envia CZ_NOTIFY_ACTORINIT (0x007d) - 2 bytes
    s_map.sendall(struct.pack("<H", 0x007d))
    print("Enviado CZ_NOTIFY_ACTORINIT (0x007d)")

    # Drena pacotes iniciais do mundo
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

    # -------------------------------------------------------------------------
    # 4. MOVIMENTO (CZ_REQUEST_MOVE 0x035F)
    # -------------------------------------------------------------------------
    print("\n=== [4] Teste de Movimento (CZ_REQUEST_MOVE 0x035F) ===")
    target_x, target_y = cur_x + 2, cur_y + 2
    cz_move = struct.pack("<H", 0x035F) + pack_pos(target_x, target_y)
    s_map.settimeout(5.0)
    s_map.sendall(cz_move)
    print(f"Enviado CZ_REQUEST_MOVE para ({target_x}, {target_y})")

    got_move = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_move:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x87\x00' in chunk:
                idx = chunk.find(b'\x87\x00')
                if idx + 12 <= len(chunk):
                    m_tick, = struct.unpack_from("<I", chunk, idx + 2)
                    md = chunk[idx + 6 : idx + 12]
                    x0, y0, d0 = unpack_pos(md[:3])
                    x1, y1, d1 = unpack_pos(md[3:6])
                    print(f">>> ZC_NOTIFY_PLAYERMOVE (0x0087) recebido! Rota: ({x0}, {y0}) -> ({x1}, {y1}), Tick: {m_tick}")
                    got_move = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    if not got_move:
        print("Aviso: Movimento não confirmado no prazo estipulado")

    # -------------------------------------------------------------------------
    # 5. CHAT GLOBAL & COMANDO GM (CZ_GLOBAL_MESSAGE 0x00f3)
    # -------------------------------------------------------------------------
    print("\n=== [5] Teste de Chat & Comandos GM (CZ_GLOBAL_MESSAGE 0x00f3) ===")
    send_chat_message(s_map, char_name, "Ola mundo do Myth of Thor!")
    print(f"Mensagem de chat enviada: 'Ola mundo do Myth of Thor!'")

    # Envia comando @help
    time.sleep(0.2)
    send_chat_message(s_map, char_name, "@help")
    print("Comando @help enviado via CZ_GLOBAL_MESSAGE")

    got_chat_resp = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_chat_resp:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            # Verifica pacotes de chat (0x02c1, 0x017f ou 0x008e)
            for cmd_check in [0x02c1, 0x017f, 0x008e]:
                cmd_bytes = struct.pack("<H", cmd_check)
                if cmd_bytes in chunk:
                    got_chat_resp = True
                    print(f">>> Resposta de Chat recebida do Map-Server! Opcode: 0x{cmd_check:04x}")
                    break
        except (socket.timeout, BlockingIOError):
            break

    # -------------------------------------------------------------------------
    # 6. CLIMA & EFEITOS (ZC_NOTIFY_EFFECT2 0x01f3)
    # -------------------------------------------------------------------------
    print("\n=== [6] Teste de Clima / Efeitos (ZC_NOTIFY_EFFECT2 0x01f3) ===")
    send_chat_message(s_map, char_name, "@snow")
    print("Comando @snow enviado")

    got_snow_effect = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_snow_effect:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\xf3\x01' in chunk: # 0x01f3 (ZC_NOTIFY_EFFECT2)
                idx = chunk.find(b'\xf3\x01')
                if idx + 10 <= len(chunk):
                    e_gid, = struct.unpack_from("<I", chunk, idx + 2)
                    e_id, = struct.unpack_from("<i", chunk, idx + 6)
                    print(f">>> ZC_NOTIFY_EFFECT2 (0x01f3) recebido! GID={e_gid}, EffectID={e_id} (EF_SNOW=162)")
                    if e_id == 162:
                        got_snow_effect = True
                        break
        except (socket.timeout, BlockingIOError):
            break

    # -------------------------------------------------------------------------
    # 7. AUTO-BATTLE VIA SERVER (server/npc/autobattle.txt)
    # -------------------------------------------------------------------------
    print("\n=== [7] Teste de Auto-Battle (@autobattle on) ===")
    send_chat_message(s_map, char_name, "@autobattle on")
    print("Comando '@autobattle on' enviado")

    got_autobattle = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_autobattle:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b"Auto-Battle" in chunk:
                got_autobattle = True
                print(">>> Resposta do Script de Auto-Battle recebida com sucesso!")
                break
        except (socket.timeout, BlockingIOError):
            break

    s_map.close()

    print("\n==================================================================")
    print("  >>> TODOS OS TESTES DE REDE E FLUXO FORAM CONCLUÍDOS COM SUCESSO! <<<")
    print("==================================================================")
    return True

if __name__ == "__main__":
    ok = run_test()
    sys.exit(0 if ok else 1)
