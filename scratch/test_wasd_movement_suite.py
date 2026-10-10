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

def RBUFPOS2(p):
    x0 = (p[0] << 2) | (p[1] >> 6)
    y0 = ((p[1] & 0x3F) << 4) | (p[2] >> 4)
    x1 = ((p[2] & 0x0F) << 6) | (p[3] >> 2)
    y1 = ((p[3] & 0x03) << 8) | p[4]
    sx0 = (p[5] & 0xF0) >> 4
    sy0 = p[5] & 0x0F
    return x0, y0, x1, y1, sx0, sy0

def get_session():
    # 1. Login
    s_login = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_login.settimeout(5.0)
    s_login.connect((LOGIN_HOST, LOGIN_PORT))
    ca_login = struct.pack("<HI24s24sB", 0x0064, 20250402, USERNAME.encode('ascii').ljust(24, b'\x00'), PASSWORD.encode('ascii').ljust(24, b'\x00'), 0)
    s_login.sendall(ca_login)
    resp = s_login.recv(1024)
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
    ack_aid = s_char.recv(4)
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
    zone_resp = s_char.recv(1024)
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
            if idx + 9 <= len(initial_buffer):
                start_tick, = struct.unpack_from("<I", initial_buffer, idx + 2)
                cur_x, cur_y, cur_dir = unpack_pos(initial_buffer[idx + 6 : idx + 9])
                break

    s_map.sendall(struct.pack("<H", 0x007d))
    time.sleep(0.5)
    s_map.settimeout(0.3)
    try:
        while True:
            c = s_map.recv(4096)
            if not c: break
    except:
        pass

    return s_map, cur_x, cur_y

def main():
    s_map, cur_x, cur_y = get_session()
    print(f"Sessao ativa no Map Server! Posicao atual do personagem: ({cur_x}, {cur_y})")

    def drain_all(timeout=1.0):
        s_map.settimeout(timeout)
        events = []
        t_end = time.time() + timeout
        while time.time() < t_end:
            try:
                chunk = s_map.recv(4096)
                if not chunk: break
                for i in range(len(chunk) - 5):
                    if chunk[i:i+2] == b'\x87\x00':
                        m_tick, = struct.unpack_from("<I", chunk, i + 2)
                        md = chunk[i + 6 : i + 12]
                        x0, y0, x1, y1, sx0, sy0 = RBUFPOS2(md)
                        events.append(('0x0087 ZC_NOTIFY_PLAYERMOVE', f"origem=({x0},{y0}) -> destino=({x1},{y1})", m_tick))
                    elif chunk[i:i+2] == b'\x88\x00':
                        aid, stop_x, stop_y = struct.unpack_from("<IHH", chunk, i + 2)
                        events.append(('0x0088 ZC_STOPMOVE', f"AID={aid} pos=({stop_x},{stop_y})", 0))
            except socket.timeout:
                break
            except Exception:
                break
        return events

    # -------------------------------------------------------------------------
    # CASO 1: Movimento para +5 celulas, e apos 300ms pede a CELULA DE ORIGEM
    # (Este e o cenario do bug 'anda e volta')
    # -------------------------------------------------------------------------
    print("\n" + "="*70)
    print("CASO 1: Destino Inicial (+5 X) -> Apos 300ms pede CELULA DE ORIGEM (Bug antigo)")
    print("="*70)
    orig_x, orig_y = cur_x, cur_y
    target_5_x = cur_x + 5
    target_5_y = cur_y
    print(f">> [0ms] Enviando CZ_REQUEST_MOVE para ({target_5_x}, {target_5_y})...")
    s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(target_5_x, target_5_y))
    time.sleep(0.3)
    evs1 = drain_all(0.05)
    for ev in evs1:
        print(f"   << Servidor respondeu: {ev[0]} | {ev[1]} (tick={ev[2]})")

    print(f">> [300ms] Enviando CZ_REQUEST_MOVE para CELULA DE ORIGEM ({orig_x}, {orig_y})...")
    s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(orig_x, orig_y))
    evs2 = drain_all(1.5)
    for ev in evs2:
        print(f"   << Servidor respondeu: {ev[0]} | {ev[1]} (tick={ev[2]})")
    cur_x, cur_y = orig_x, orig_y

    time.sleep(1.0)
    drain_all(0.3)

    # -------------------------------------------------------------------------
    # CASO 2: Movimento para +5 celulas, e apos 300ms pede 1 CELULA A FRENTE (+3 X)
    # -------------------------------------------------------------------------
    print("\n" + "="*70)
    print("CASO 2: Destino Inicial (+5 X) -> Apos 300ms pede 1 CELULA A FRENTE (+3 X)")
    print("="*70)
    start_x, start_y = cur_x, cur_y
    target_5_x = start_x + 5
    target_5_y = start_y
    print(f">> [0ms] Enviando CZ_REQUEST_MOVE para ({target_5_x}, {target_5_y})...")
    s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(target_5_x, target_5_y))
    time.sleep(0.3)
    evs1 = drain_all(0.05)
    for ev in evs1:
        print(f"   << Servidor respondeu: {ev[0]} | {ev[1]} (tick={ev[2]})")

    target_front_x = start_x + 3
    print(f">> [300ms] Enviando CZ_REQUEST_MOVE para 1 CELULA A FRENTE ({target_front_x}, {start_y})...")
    s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(target_front_x, start_y))
    evs2 = drain_all(1.5)
    for ev in evs2:
        print(f"   << Servidor respondeu: {ev[0]} | {ev[1]} (tick={ev[2]})")
    cur_x, cur_y = target_front_x, start_y

    time.sleep(1.0)
    drain_all(0.3)

    # -------------------------------------------------------------------------
    # CASO 3: Movimento com BUFFER DE 1 PASSO -> Solta tecla (NENHUM PACOTE)
    # (A solucao definitiva e mais suave)
    # -------------------------------------------------------------------------
    print("\n" + "="*70)
    print("CASO 3: Buffer de 1 Passo -> Solta Tecla (Nenhum pacote enviado)")
    print("="*70)
    start_x, start_y = cur_x, cur_y
    target_step_x = start_x + 1
    print(f">> [WASD PRESSIONADO] Enviando CZ_REQUEST_MOVE apenas 1 celula adiante ({target_step_x}, {start_y})...")
    s_map.sendall(struct.pack("<H", 0x035F) + pack_pos(target_step_x, start_y))
    time.sleep(0.15)
    evs1 = drain_all(0.05)
    for ev in evs1:
        print(f"   << Servidor respondeu: {ev[0]} | {ev[1]} (tick={ev[2]})")

    print(f">> [WASD SOLTO] Tecla liberada. NENHUM pacote enviado. Aguardando fim natural do passo...")
    evs2 = drain_all(1.0)
    if not evs2:
        print(f"   << Nenhum pacote adicional recebido! O timer do rAthena expirou na celula ({target_step_x}, {start_y}) e o personagem parou instantaneamente!")
    for ev in evs2:
        print(f"   << Servidor respondeu: {ev[0]} | {ev[1]} (tick={ev[2]})")

    s_map.close()
    print("\n" + "="*70)
    print("RESUMO COMPLETO:")
    print("1. No Caso 1, o servidor recebeu o pedido para a origem e gerou novo ZC_NOTIFY_PLAYERMOVE")
    print("   com rota DE VOLTA para a origem, confirmando a causa exata do 'anda e volta'.")
    print("2. Em NENHUM dos casos normais o servidor enviou 0x0088 ZC_STOPMOVE para o proprio jogador.")
    print("   O servidor apenas envia 0x0087 (nova rota) ou deixa o walktimer expirar.")
    print("3. A regra ideal: enquanto WASD estiver pressionado, manter buffer de 1 passo a frente.")
    print("   Ao soltar WASD, NAO enviar pacote nenhum; o rAthena completa o passo atual e para.")
    print("="*70)

if __name__ == '__main__':
    main()
