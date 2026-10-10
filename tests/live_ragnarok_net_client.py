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

def unpack_pos2(p):
    x0 = (p[0] << 2) | (p[1] >> 6)
    y0 = ((p[1] & 0x3F) << 4) | (p[2] >> 4)
    x1 = ((p[2] & 0x0F) << 6) | (p[3] >> 2)
    y1 = ((p[3] & 0x03) << 8) | p[4]
    sx0 = (p[5] & 0xF0) >> 4
    sy0 = p[5] & 0x0F
    return x0, y0, x1, y1, sx0, sy0

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
    ca_login = struct.pack("<HI24s24sB", 0x0064, 20250402, USERNAME.encode('ascii').ljust(24, b'\x00'), PASSWORD.encode('ascii').ljust(24, b'\x00'), 0)
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
    # 4. MOVIMENTO & VALIDAÇÃO WASD (CZ_REQUEST_MOVE 0x035F)
    # -------------------------------------------------------------------------
    print("\n=== [4] Teste de Movimento & Validação WASD (CZ_REQUEST_MOVE 0x035F) ===")
    orig_x, orig_y = cur_x, cur_y
    target_5_x, target_5_y = cur_x + 5, cur_y
    cz_move = struct.pack("<H", 0x035F) + pack_pos(target_5_x, target_5_y)
    s_map.sendall(cz_move)
    print(f"[Passo 1] Enviado CZ_REQUEST_MOVE para 5 celulas a frente: ({target_5_x}, {target_5_y})")

    # Aguarda 300ms simulando tempo de caminhada
    time.sleep(0.3)
    
    # Drena primeiro ZC_NOTIFY_PLAYERMOVE
    s_map.settimeout(0.5)
    try:
        chunk = s_map.recv(4096)
        for i in range(len(chunk) - 5):
            if chunk[i:i+2] == b'\x87\x00':
                m_tick, = struct.unpack_from("<I", chunk, i + 2)
                md = chunk[i + 6 : i + 12]
                x0, y0, x1, y1, sx0, sy0 = unpack_pos2(md)
                print(f"  << ZC_NOTIFY_PLAYERMOVE (0x0087) recebido: ({x0},{y0}) -> ({x1},{y1}), sub=({sx0},{sy0}), Tick: {m_tick}")
            elif chunk[i:i+2] == b'\x88\x00':
                aid, stop_x, stop_y = struct.unpack_from("<IHH", chunk, i + 2)
                print(f"  << ZC_STOPMOVE (0x0088) recebido! AID={aid}, Pos=({stop_x},{stop_y})")
    except socket.timeout:
        pass

    # [Passo 2] Envia pedido para CELULA DE ORIGEM (o bug 'anda e volta')
    print(f"[Passo 2 - Teste Anda e Volta] Enviando CZ_REQUEST_MOVE para CELULA DE ORIGEM ({orig_x}, {orig_y})")
    cz_move_back = struct.pack("<H", 0x035F) + pack_pos(orig_x, orig_y)
    s_map.sendall(cz_move_back)

    time.sleep(0.5)
    s_map.settimeout(1.0)
    got_rebound = False
    try:
        chunk = s_map.recv(4096)
        for i in range(len(chunk) - 5):
            if chunk[i:i+2] == b'\x87\x00':
                m_tick, = struct.unpack_from("<I", chunk, i + 2)
                md = chunk[i + 6 : i + 12]
                x0, y0, x1, y1, sx0, sy0 = unpack_pos2(md)
                print(f"  << [REBOTE CONFIRMADO] ZC_NOTIFY_PLAYERMOVE (0x0087) de volta: ({x0},{y0}) -> ({x1},{y1}), Tick: {m_tick}")
                got_rebound = True
            elif chunk[i:i+2] == b'\x88\x00':
                aid, stop_x, stop_y = struct.unpack_from("<IHH", chunk, i + 2)
                print(f"  << ZC_STOPMOVE (0x0088) recebido! AID={aid}, Pos=({stop_x},{stop_y})")
    except socket.timeout:
        pass

    # Aguarda personagem terminar de voltar para a origem
    time.sleep(1.0)

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

    # Desativa auto-battle
    time.sleep(0.1)
    send_chat_message(s_map, char_name, "@autobattle off")

    # -------------------------------------------------------------------------
    # 8. EFEITOS GERAIS DO SERVIDOR (ZC_NOTIFY_EFFECT 0x019b)
    # -------------------------------------------------------------------------
    print("\n=== [8] Teste de Efeito Geral (ZC_NOTIFY_EFFECT 0x019b) ===")
    send_chat_message(s_map, char_name, "@blvl -5")
    time.sleep(0.1)
    send_chat_message(s_map, char_name, "@blvl 1")
    print("Comando '@blvl 1' enviado para disparar efeito de level up")

    got_notify_effect = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_notify_effect:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x9b\x01' in chunk: # 0x019b (ZC_NOTIFY_EFFECT)
                idx = chunk.find(b'\x9b\x01')
                if idx + 10 <= len(chunk):
                    eff_aid, = struct.unpack_from("<I", chunk, idx + 2)
                    eff_type, = struct.unpack_from("<i", chunk, idx + 6)
                    print(f">>> ZC_NOTIFY_EFFECT (0x019b) recebido! AID={eff_aid}, EffectType={eff_type} (0=BaseLevelUp)")
                    got_notify_effect = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    # -------------------------------------------------------------------------
    # 9. BUFFS / STATUS CHANGE (ZC_MSG_STATE_CHANGE3 0x0983 & 0x0196)
    # -------------------------------------------------------------------------
    print("\n=== [9] Teste de Buffs / Status Change (ZC_MSG_STATE_CHANGE3 0x0983 & 0x0196) ===")
    send_chat_message(s_map, char_name, "@job 4") # Noviço
    time.sleep(0.1)
    send_chat_message(s_map, char_name, "@allskill")
    time.sleep(0.1)

    # Usa Bênção (skill 34, lv 10) no próprio personagem (account_id)
    # CZ_USE_SKILL 0x089b (10 bytes: [0x089b][lv 10][skill 34][target_id])
    cz_use_bless = struct.pack("<HHHI", 0x089b, 10, 34, account_id)
    s_map.sendall(cz_use_bless)
    print("Enviado CZ_USE_SKILL para Bênção (AL_BLESSING nv 10)")

    got_state_change3 = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_state_change3:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x83\x09' in chunk: # 0x0983 (ZC_MSG_STATE_CHANGE3)
                idx = chunk.find(b'\x83\x09')
                if idx + 29 <= len(chunk):
                    efst_id, = struct.unpack_from("<H", chunk, idx + 2)
                    st_aid, = struct.unpack_from("<I", chunk, idx + 4)
                    st_flag = chunk[idx + 8]
                    total_ms, = struct.unpack_from("<I", chunk, idx + 9)
                    remain_ms, = struct.unpack_from("<I", chunk, idx + 13)
                    print(f">>> ZC_MSG_STATE_CHANGE3 (0x0983) recebido! EfstID={efst_id}, AID={st_aid}, Flag={st_flag}, TotalMs={total_ms}, RemainMs={remain_ms}")
                    got_state_change3 = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    # Aplica um status curto de 1 segundo (1000ms) para observar a desativação natural (0x0196)
    send_chat_message(s_map, char_name, "@sc_start 10 1000 1")
    got_state_change_off = False
    start_wait = time.time()
    while time.time() - start_wait < 4.0 and not got_state_change_off:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x96\x01' in chunk: # 0x0196 (ZC_MSG_STATE_CHANGE)
                idx = chunk.find(b'\x96\x01')
                if idx + 9 <= len(chunk):
                    efst_id, = struct.unpack_from("<H", chunk, idx + 2)
                    st_aid, = struct.unpack_from("<I", chunk, idx + 4)
                    st_flag = chunk[idx + 8]
                    print(f">>> ZC_MSG_STATE_CHANGE (0x0196) recebido! EfstID={efst_id}, AID={st_aid}, Flag={st_flag} (0=Desativado)")
                    got_state_change_off = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    # -------------------------------------------------------------------------
    # 10. SKILLS DE CHÃO (ZC_SKILL_ENTRY5 0x09ca & ZC_SKILL_DISAPPEAR 0x0120)
    # -------------------------------------------------------------------------
    print("\n=== [10] Teste de Skills de Chão (ZC_SKILL_ENTRY5 0x09ca & ZC_SKILL_DISAPPEAR 0x0120) ===")
    send_chat_message(s_map, char_name, "@job 2") # Mago
    time.sleep(0.1)
    send_chat_message(s_map, char_name, "@allskill")
    time.sleep(0.1)

    # Conjura Parede de Fogo (WZ_FIREWALL, id 18, lv 1) na posição à frente
    # CZ_USE_SKILL2 0x0959 (10 bytes: [0x0959][lv 1][skill 18][x][y])
    fw_x, fw_y = cur_x + 1, cur_y + 1
    cz_use_pos = struct.pack("<HHHHH", 0x0959, 1, 18, fw_x, fw_y)
    s_map.sendall(cz_use_pos)
    print(f"Enviado CZ_USE_SKILL2 para Parede de Fogo (WZ_FIREWALL nv 1) em ({fw_x}, {fw_y})")

    got_skill_entry = False
    fw_unit_id = 0
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_skill_entry:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\xca\x09' in chunk: # 0x09ca (ZC_SKILL_ENTRY5)
                idx = chunk.find(b'\xca\x09')
                if idx + 23 <= len(chunk):
                    p_len, = struct.unpack_from("<H", chunk, idx + 2)
                    fw_unit_id, = struct.unpack_from("<I", chunk, idx + 4)
                    fw_owner, = struct.unpack_from("<I", chunk, idx + 8)
                    sx, = struct.unpack_from("<h", chunk, idx + 12)
                    sy, = struct.unpack_from("<h", chunk, idx + 14)
                    u_type, = struct.unpack_from("<I", chunk, idx + 16)
                    print(f">>> ZC_SKILL_ENTRY5 (0x09ca) recebido! UnitID={fw_unit_id}, Owner={fw_owner}, Pos=({sx},{sy}), UnitType={u_type}")
                    got_skill_entry = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    # Aguarda ou força sumiço da unidade de chão para verificar 0x0120
    got_skill_disappear = False
    start_wait = time.time()
    while time.time() - start_wait < 6.0 and not got_skill_disappear:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x20\x01' in chunk: # 0x0120 (ZC_SKILL_DISAPPEAR)
                idx = chunk.find(b'\x20\x01')
                if idx + 6 <= len(chunk):
                    dis_id, = struct.unpack_from("<I", chunk, idx + 2)
                    print(f">>> ZC_SKILL_DISAPPEAR (0x0120) recebido! UnitID={dis_id}")
                    got_skill_disappear = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    # -------------------------------------------------------------------------
    # 11. EQUIPAR E DESEQUIPAR (0x0998, 0x00ab, 0x0999, 0x099a)
    # -------------------------------------------------------------------------
    print("\n=== [11] Teste de Equipar e Desequipar (CZ_REQ_WEAR_EQUIP / CZ_REQ_TAKEOFF_EQUIP) ===")
    send_chat_message(s_map, char_name, "@item 2301 1") # Cotton Shirt
    time.sleep(0.1)
    send_chat_message(s_map, char_name, "@item 1201 1") # Knife
    time.sleep(0.1)

    shirt_idx = None
    shirt_loc = 16 # EQP_ARMOR
    knife_idx = None
    knife_loc = 2  # EQP_HAND_R

    start_wait = time.time()
    while time.time() - start_wait < 3.0 and (shirt_idx is None or knife_idx is None):
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            idx = 0
            while idx < len(chunk):
                if chunk[idx:idx+2] == b'\x41\x0b': # 0x0b41 ZC_ITEM_PICKUP_ACK (34 bytes min)
                    if idx + 34 <= len(chunk):
                        p_idx, = struct.unpack_from("<H", chunk, idx + 2)
                        p_nameid, = struct.unpack_from("<I", chunk, idx + 6)
                        p_loc, = struct.unpack_from("<I", chunk, idx + 28)
                        if p_nameid == 2301:
                            shirt_idx = p_idx
                            shirt_loc = p_loc
                            print(f">>> Item 2301 (Cotton Shirt) adicionado: Index={shirt_idx}, Location=0x{shirt_loc:x}")
                        elif p_nameid == 1201:
                            knife_idx = p_idx
                            knife_loc = p_loc
                            print(f">>> Item 1201 (Knife) adicionado: Index={knife_idx}, Location=0x{knife_loc:x}")
                    idx += 34
                else:
                    idx += 1
        except (socket.timeout, BlockingIOError):
            break

    if shirt_idx is None:
        shirt_idx = 2
    if knife_idx is None:
        knife_idx = 3

    # [11.1] Equipa Cotton Shirt (CZ_REQ_WEAR_EQUIP 0x0998: [0x0998][idx:2B][wearLoc:4B])
    pkt_wear_shirt = struct.pack("<HHI", 0x0998, shirt_idx, shirt_loc)
    s_map.sendall(pkt_wear_shirt)
    print(f"Enviado CZ_REQ_WEAR_EQUIP para Cotton Shirt Index={shirt_idx} Loc=0x{shirt_loc:x}")

    got_wear_shirt = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_wear_shirt:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x99\x09' in chunk: # 0x0999 (ZC_REQ_WEAR_EQUIP_ACK: 11 bytes)
                pos = chunk.find(b'\x99\x09')
                if pos + 11 <= len(chunk):
                    p_type, ack_idx, ack_loc, ack_spr, ack_res = struct.unpack_from("<HHIHB", chunk, pos)
                    print(f">>> ZC_REQ_WEAR_EQUIP_ACK (0x0999) recebido! Index={ack_idx}, Loc=0x{ack_loc:x}, Sprite={ack_spr}, Result={ack_res} (0=OK)")
                    if ack_idx == shirt_idx and ack_res == 0:
                        got_wear_shirt = True
                        break
        except (socket.timeout, BlockingIOError):
            break

    # [11.2] Equipa Knife
    pkt_wear_knife = struct.pack("<HHI", 0x0998, knife_idx, knife_loc)
    s_map.sendall(pkt_wear_knife)
    print(f"Enviado CZ_REQ_WEAR_EQUIP para Knife Index={knife_idx} Loc=0x{knife_loc:x}")

    got_wear_knife = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_wear_knife:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x99\x09' in chunk: # 0x0999
                pos = chunk.find(b'\x99\x09')
                if pos + 11 <= len(chunk):
                    p_type, ack_idx, ack_loc, ack_spr, ack_res = struct.unpack_from("<HHIHB", chunk, pos)
                    print(f">>> ZC_REQ_WEAR_EQUIP_ACK (0x0999) recebido! Index={ack_idx}, Loc=0x{ack_loc:x}, Sprite={ack_spr}, Result={ack_res} (0=OK)")
                    if ack_idx == knife_idx and ack_res == 0:
                        got_wear_knife = True
                        break
        except (socket.timeout, BlockingIOError):
            break

    # [11.3] Desequipa Cotton Shirt (CZ_REQ_TAKEOFF_EQUIP 0x00ab: [0x00ab][idx:2B])
    pkt_takeoff_shirt = struct.pack("<HH", 0x00ab, shirt_idx)
    s_map.sendall(pkt_takeoff_shirt)
    print(f"Enviado CZ_REQ_TAKEOFF_EQUIP para Cotton Shirt Index={shirt_idx}")

    got_takeoff_shirt = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_takeoff_shirt:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x9a\x09' in chunk: # 0x099a (ZC_REQ_TAKEOFF_EQUIP_ACK: 9 bytes)
                pos = chunk.find(b'\x9a\x09')
                if pos + 9 <= len(chunk):
                    p_type, ack_idx, ack_loc, ack_flag = struct.unpack_from("<HHIB", chunk, pos)
                    print(f">>> ZC_REQ_TAKEOFF_EQUIP_ACK (0x099a) recebido! Index={ack_idx}, Loc=0x{ack_loc:x}, Flag={ack_flag} (0=OK)")
                    if ack_idx == shirt_idx and ack_flag == 0:
                        got_takeoff_shirt = True
                        break
        except (socket.timeout, BlockingIOError):
            break

    # [11.4] Desequipa Knife
    pkt_takeoff_knife = struct.pack("<HH", 0x00ab, knife_idx)
    s_map.sendall(pkt_takeoff_knife)
    print(f"Enviado CZ_REQ_TAKEOFF_EQUIP para Knife Index={knife_idx}")

    got_takeoff_knife = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_takeoff_knife:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x9a\x09' in chunk: # 0x099a
                pos = chunk.find(b'\x9a\x09')
                if pos + 9 <= len(chunk):
                    p_type, ack_idx, ack_loc, ack_flag = struct.unpack_from("<HHIB", chunk, pos)
                    print(f">>> ZC_REQ_TAKEOFF_EQUIP_ACK (0x099a) recebido! Index={ack_idx}, Loc=0x{ack_loc:x}, Flag={ack_flag} (0=OK)")
                    if ack_idx == knife_idx and ack_flag == 0:
                        got_takeoff_knife = True
                        break
        except (socket.timeout, BlockingIOError):
            break

    if not (got_wear_shirt and got_wear_knife and got_takeoff_shirt and got_takeoff_knife):
        print("\nFalha na confirmação de algum dos pacotes de equipar/desequipar!")
        s_map.close()
        return False

    # -------------------------------------------------------------------------
    # 12. PONTOS DE ATRIBUTO (CZ_STATUS_CHANGE 0x00bb -> ZC_STATUS_CHANGE_ACK 0x00bc)
    # -------------------------------------------------------------------------
    print("\n=== [12] Teste de Distribuição de Atributos (CZ_STATUS_CHANGE 0x00bb) ===")
    send_chat_message(s_map, char_name, "@stpoint 10")
    time.sleep(0.1)

    # CZ_STATUS_CHANGE (0x00bb) - 5 bytes: [0x00bb][StatusType: uint16][Amount: uint8]
    # StatusType: 13 = SP_STR
    pkt_status_up = struct.pack("<HHB", 0x00bb, 13, 1)
    s_map.sendall(pkt_status_up)
    print("Enviado CZ_STATUS_CHANGE para SP_STR (13) +1")

    got_status_ack = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_status_ack:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\xbc\x00' in chunk: # 0x00bc (ZC_STATUS_CHANGE_ACK: 6 bytes)
                pos = chunk.find(b'\xbc\x00')
                if pos + 6 <= len(chunk):
                    p_type, ack_sp, ack_ok, ack_val = struct.unpack_from("<HHBB", chunk, pos)
                    print(f">>> ZC_STATUS_CHANGE_ACK (0x00bc) recebido! SP={ack_sp}, OK={ack_ok}, NovoValor={ack_val}")
                    if ack_sp == 13 and ack_ok == 1:
                        got_status_ack = True
                        break
        except (socket.timeout, BlockingIOError):
            break

    if not got_status_ack:
        print("Falha ao receber ZC_STATUS_CHANGE_ACK (0x00bc)!")
        s_map.close()
        return False

    # -------------------------------------------------------------------------
    # 13. PONTOS DE HABILIDADE (CZ_UPGRADE_SKILLLEVEL 0x0112 -> 0x010e / 0x0b33)
    # -------------------------------------------------------------------------
    print("\n=== [13] Teste de Subir Habilidade (CZ_UPGRADE_SKILLLEVEL 0x0112) ===")
    send_chat_message(s_map, char_name, "@job 0") # Aprendiz
    time.sleep(0.1)
    send_chat_message(s_map, char_name, "@resetskill") # Reseta habilidades
    time.sleep(0.1)
    send_chat_message(s_map, char_name, "@skpoint 10") # Concede pontos para subir
    time.sleep(0.2)

    # CZ_UPGRADE_SKILLLEVEL (0x0112) - 4 bytes: [0x0112][SkillID: uint16]
    pkt_skill_up = struct.pack("<HH", 0x0112, 1)
    s_map.sendall(pkt_skill_up)
    print("Enviado CZ_UPGRADE_SKILLLEVEL para Habilidade Básica (Skill 1)")

    got_skill_up_ack = False
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and not got_skill_up_ack:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            if b'\x0e\x01' in chunk: # 0x010e (ZC_SKILLINFO_UPDATE: 11 bytes)
                pos = chunk.find(b'\x0e\x01')
                if pos + 11 <= len(chunk):
                    p_type, s_id, s_lv, s_sp, s_rng, s_up = struct.unpack_from("<HHHHHB", chunk, pos)
                    print(f">>> ZC_SKILLINFO_UPDATE (0x010e) recebido! SkillID={s_id}, Lv={s_lv}, SP={s_sp}, Range={s_rng}, Up={s_up}")
                    got_skill_up_ack = True
                    break
            elif b'\x33\x0b' in chunk: # 0x0b33 (ZC_SKILLINFO_UPDATE2: 17 bytes)
                pos = chunk.find(b'\x33\x0b')
                if pos + 17 <= len(chunk):
                    p_type, s_id, s_inf, s_lv, s_sp, s_rng, s_up = struct.unpack_from("<HIHHHHB", chunk, pos)
                    print(f">>> ZC_SKILLINFO_UPDATE2 (0x0b33) recebido! SkillID={s_id}, Lv={s_lv}, SP={s_sp}, Range={s_rng}")
                    got_skill_up_ack = True
                    break
        except (socket.timeout, BlockingIOError):
            break

    if not got_skill_up_ack:
        print("Falha ao receber atualização de skill (0x010e ou 0x0b33)!")
        s_map.close()
        return False

    # -------------------------------------------------------------------------
    # 14. ATAQUE FORA DE ALCANCE (0x0139 ZC_ATTACK_FAILURE_FOR_DISTANCE)
    # -------------------------------------------------------------------------
    print("\n=== [14] Teste de Ataque Fora de Alcance (ZC_ATTACK_FAILURE_FOR_DISTANCE 0x0139) ===")
    send_chat_message(s_map, char_name, "@monster poring 1")
    time.sleep(0.2)

    poring_aid = None
    poring_x, poring_y = cur_x + 5, cur_y + 5
    start_wait = time.time()
    while time.time() - start_wait < 3.0 and poring_aid is None:
        try:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            idx = 0
            while idx < len(chunk):
                p_cmd, = struct.unpack_from("<H", chunk, idx)
                if p_cmd == 0x09fe and idx + 66 <= len(chunk):
                    m_aid, = struct.unpack_from("<I", chunk, idx + 5)
                    if m_aid != account_id:
                        poring_aid = m_aid
                        px, py, pdir = unpack_pos(chunk[idx + 63 : idx + 66])
                        poring_x, poring_y = px, py
                        print(f"Poring detectado: AID={poring_aid} em ({poring_x}, {poring_y})")
                        break
                    idx += 107
                elif p_cmd == 0x09ff and idx + 66 <= len(chunk):
                    m_aid, = struct.unpack_from("<I", chunk, idx + 5)
                    if m_aid != account_id:
                        poring_aid = m_aid
                        px, py, pdir = unpack_pos(chunk[idx + 63 : idx + 66])
                        poring_x, poring_y = px, py
                        print(f"Poring detectado: AID={poring_aid} em ({poring_x}, {poring_y})")
                        break
                    idx += 108
                else:
                    idx += 1
        except (socket.timeout, BlockingIOError):
            break

    got_attack_failure_distance = False
    if poring_aid:
        # Se o Poring nasceu adjacente (dist <= 1), movemos o jogador para longe
        dist_x = abs(cur_x - poring_x)
        dist_y = abs(cur_y - poring_y)
        print(f"Distância até o Poring: dx={dist_x}, dy={dist_y} (Alcance Desarmado=1)")

        if dist_x <= 1 and dist_y <= 1:
            far_x, far_y = cur_x + 4, cur_y + 4
            cz_move = struct.pack("<H", 0x035f) + pack_pos(far_x, far_y)
            s_map.sendall(cz_move)
            print(f"Movendo jogador para ({far_x}, {far_y}) para afastar do Poring...")
            time.sleep(1.5) # Aguarda o jogador parar de andar

        # Drena o socket antes de pedir ataque
        try:
            while True:
                junk = s_map.recv(4096)
                if not junk:
                    break
        except (socket.timeout, BlockingIOError):
            pass

        # Envia CZ_REQUEST_ACT (0x088e) atacando o Poring de longe
        # [0x088e: 2B][TargetAID: uint32 = 4B][ActionType: uint8 = 0 (ataque)] Total: 7 bytes
        pkt_act = struct.pack("<HIB", 0x088e, poring_aid, 0)
        s_map.sendall(pkt_act)
        print(f"Enviado CZ_REQUEST_ACT (0x088e) contra Poring AID={poring_aid}")

        start_wait = time.time()
        while time.time() - start_wait < 3.0 and not got_attack_failure_distance:
            try:
                chunk = s_map.recv(4096)
                if not chunk:
                    break
                if b'\x39\x01' in chunk: # 0x0139 (ZC_ATTACK_FAILURE_FOR_DISTANCE: 16 bytes)
                    pos = chunk.find(b'\x39\x01')
                    if pos + 16 <= len(chunk):
                        p_type, t_aid, tx, ty, px, py, rng = struct.unpack_from("<HIhhhhh", chunk, pos)
                        print(f">>> ZC_ATTACK_FAILURE_FOR_DISTANCE (0x0139) recebido! TargetAID={t_aid}, TargetPos=({tx},{ty}), Pos=({px},{py}), Range={rng}")
                        if t_aid == poring_aid:
                            got_attack_failure_distance = True
                            break
            except (socket.timeout, BlockingIOError):
                break

    if not got_attack_failure_distance:
        print("Falha ao receber ZC_ATTACK_FAILURE_FOR_DISTANCE (0x0139)!")
        s_map.close()
        return False

    print("\n==================================================================")
    print("  >>> TODOS OS TESTES DE REDE E FLUXO FORAM CONCLUÍDOS COM SUCESSO! <<<")
    print("==================================================================")
    return True

if __name__ == "__main__":
    ok = run_test()
    sys.exit(0 if ok else 1)
