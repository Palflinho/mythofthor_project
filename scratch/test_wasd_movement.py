import socket
import struct
import time
import sys

LOGIN_HOST = "127.0.0.1"
LOGIN_PORT = 6900
USERNAME = "mythbot"
PASSWORD = "mythbot"

def recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("Socket closed prematurely")
        buf += chunk
    return buf

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

def connect_to_map():
    # 1. Login
    s_login = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_login.settimeout(5.0)
    s_login.connect((LOGIN_HOST, LOGIN_PORT))
    ca_login = struct.pack("<HI24s24sB", 0x0064, 20250402, USERNAME.encode('ascii').ljust(24, b'\x00'), PASSWORD.encode('ascii').ljust(24, b'\x00'), 0)
    s_login.sendall(ca_login)
    
    header = recv_exact(s_login, 4)
    cmd, plen = struct.unpack("<HH", header)
    rest = recv_exact(s_login, plen - 4)
    resp = header + rest
    
    packet_type, packet_len, login_id1, account_id, login_id2, last_ip = struct.unpack_from("<HHIIII", resp, 0)
    sex = resp[46]
    char_ip_raw, char_port, char_name_server = struct.unpack_from("<IH20s", resp, 64)
    s_login.close()

    # 2. Char
    s_char = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_char.settimeout(5.0)
    s_char.connect((LOGIN_HOST, char_port if char_port != 0 else 6121))
    ch_enter = struct.pack("<HIIIHB", 0x0065, account_id, login_id1, login_id2, 0, sex)
    s_char.sendall(ch_enter)
    ack_aid = recv_exact(s_char, 4)
    
    char_data = s_char.recv(4096)
    char_gid = None
    offset = 0
    while offset < len(char_data):
        p_cmd, = struct.unpack_from("<H", char_data, offset)
        if p_cmd == 0x006b:
            char_info_raw = char_data[offset + 27 : offset + 27 + 175]
            char_gid, = struct.unpack_from("<I", char_info_raw, 0)
            break
        elif p_cmd == 0x082d:
            offset += 29
        else:
            break

    cz_select = struct.pack("<HB", 0x0066, 0)
    s_char.sendall(cz_select)
    zone_resp = recv_exact(s_char, 28)
    map_gid, map_name_b, map_ip_raw, map_port = struct.unpack_from("<I16sIH", zone_resp, 2)
    s_char.close()

    # 3. Map
    s_map = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_map.settimeout(5.0)
    s_map.connect((LOGIN_HOST, map_port))
    cz_enter = struct.pack("<IIIIIB", 0x0436, char_gid, login_id1, account_id, login_id2, sex)
    s_map.sendall(cz_enter)
    
    initial_buffer = b""
    cur_x, cur_y = 0, 0
    while True:
        chunk = s_map.recv(4096)
        if not chunk: break
        initial_buffer += chunk
        if b'\xeb\x02' in initial_buffer:
            idx = initial_buffer.find(b'\xeb\x02')
            if idx + 13 <= len(initial_buffer):
                cur_x, cur_y, _ = unpack_pos(initial_buffer[idx + 6 : idx + 9])
                break

    s_map.sendall(struct.pack("<H", 0x007d))
    time.sleep(0.3)
    s_map.settimeout(0.5)
    try:
        while True:
            c = s_map.recv(4096)
            if not c: break
    except:
        pass
    s_map.settimeout(3.0)
    return s_map, cur_x, cur_y

def test_movement_case(case_name, second_dest_offset):
    print(f"\n========================================================")
    print(f"TESTANDO: {case_name}")
    print(f"========================================================")
    s_map, start_x, start_y = connect_to_map()
    print(f"Posicao inicial no mapa: ({start_x}, {start_y})")

    # Pedido a 5 celulas no eixo X (ex: start_x + 5, start_y)
    dest_5_x = start_x + 5
    dest_5_y = start_y
    cz_move_1 = struct.pack("<H", 0x035F) + pack_pos(dest_5_x, dest_5_y)
    print(f"[Acao 1] Enviando CZ_REQUEST_MOVE para (+5, 0) -> ({dest_5_x}, {dest_5_y})")
    s_map.sendall(cz_move_1)

    # Espera 300 ms
    time.sleep(0.3)
    
    # Drena pacotes gerados no intervalo
    try:
        chunk = s_map.recv(4096)
        for i in range(len(chunk) - 5):
            if chunk[i:i+2] == b'\x87\x00':
                m_tick, = struct.unpack_from("<I", chunk, i + 2)
                md = chunk[i + 6 : i + 12]
                x0, y0, d0 = unpack_pos(md[:3])
                x1, y1, d1 = unpack_pos(md[3:6])
                print(f"  << Servidor aceitou movimento 1: ZC_NOTIFY_PLAYERMOVE 0x0087: ({x0},{y0}) -> ({x1},{y1}) tick {m_tick}")
            elif chunk[i:i+2] == b'\x88\x00':
                aid, stop_x, stop_y = struct.unpack_from("<IHH", chunk, i + 2)
                print(f"  << ZC_STOPMOVE 0x0088: AID={aid}, pos=({stop_x},{stop_y})")
    except Exception as e:
        pass

    # Agora executa o Passo 2 conforme o caso
    if second_dest_offset == 'origem':
        target_x, target_y = start_x, start_y
        print(f"[Acao 2 apos 300ms] Enviando CZ_REQUEST_MOVE para CELULA DE ORIGEM ({target_x}, {target_y})")
        s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(target_x, target_y))
    elif second_dest_offset == 'atual':
        target_x, target_y = start_x + 2, start_y
        print(f"[Acao 2 apos 300ms] Enviando CZ_REQUEST_MOVE para CELULA ATUAL ESTIMADA (+2) ({target_x}, {target_y})")
        s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(target_x, target_y))
    elif second_dest_offset == 'um_a_frente':
        target_x, target_y = start_x + 3, start_y
        print(f"[Acao 2 apos 300ms] Enviando CZ_REQUEST_MOVE para +1 CELULA A FRENTE (+3) ({target_x}, {target_y})")
        s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(target_x, target_y))
    elif second_dest_offset == 'nada':
        print(f"[Acao 2 apos 300ms] SOLTOU A TECLA: Nenhum pacote enviado!")

    # Monitora respostas pelos proximos 2 segundos
    t_end = time.time() + 2.0
    got_87 = 0
    got_88 = 0
    while time.time() < t_end:
        try:
            chunk = s_map.recv(4096)
            if not chunk: break
            for i in range(len(chunk) - 5):
                if chunk[i:i+2] == b'\x87\x00':
                    got_87 += 1
                    m_tick, = struct.unpack_from("<I", chunk, i + 2)
                    md = chunk[i + 6 : i + 12]
                    x0, y0, d0 = unpack_pos(md[:3])
                    x1, y1, d1 = unpack_pos(md[3:6])
                    print(f"  << [RESPOSTA] 0x0087 ZC_NOTIFY_PLAYERMOVE: ({x0},{y0}) -> ({x1},{y1}) tick {m_tick}")
                elif chunk[i:i+2] == b'\x88\x00':
                    got_88 += 1
                    aid, stop_x, stop_y = struct.unpack_from("<IHH", chunk, i + 2)
                    print(f"  << [RESPOSTA] 0x0088 ZC_STOPMOVE: AID={aid}, pos=({stop_x},{stop_y})")
        except socket.timeout:
            pass
        except Exception as e:
            break

    print(f"Sumario do caso '{case_name}': Recebidos {got_87}x 0x0087, {got_88}x 0x0088")
    s_map.close()
    time.sleep(0.5)

if __name__ == '__main__':
    test_movement_case("Cenario A: Destino = Celula de Origem (o bug antigo)", "origem")
    test_movement_case("Cenario B: Destino = Celula Atual Estimada", "atual")
    test_movement_case("Cenario C: Destino = 1 Celula a Frente", "um_a_frente")
    test_movement_case("Cenario D: Nao Enviar Nada ao Soltar Tecla (Buffer de 1 passo)", "nada")
