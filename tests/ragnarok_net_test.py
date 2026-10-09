#!/usr/bin/env python3
"""
Ragnarok Reborn / Myth of Thor - Headless Network Protocol & Systems Test Suite
-------------------------------------------------------------------------------
Valida automaticamente a integridade do protocolo de rede,
a disponibilidade das portas do rAthena e a lógica matemática do Auto-Battle (3h),
incluindo regras de morte com penalidade de 5% de EXP e retorno ao ponto de save.

Uso:
    python tests/ragnarok_net_test.py [--host 127.0.0.1]
"""

import sys
import socket
import struct
import argparse
from typing import Tuple

DEFAULT_HOST = "127.0.0.1"
LOGIN_PORT = 6900
CHAR_PORT = 6121
MAP_PORT = 5121
DB_PORT = 3311

PACKET_CA_LOGIN = 0x0064
PACKET_AC_ACCEPT_LOGIN = 0x0069
PACKET_CH_ENTER = 0x0065
PACKET_CZ_ENTER = 0x0072
PACKET_CZ_REQUEST_MOVE = 0x035F
PACKET_CZ_REQUEST_ACT = 0x0360
PACKET_CZ_USE_SKILL = 0x0113

GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"


class TestReporter:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.skipped = 0

    def record_pass(self, name: str, detail: str = ""):
        self.passed += 1
        msg = f"{GREEN}[PASS]{RESET} {name}"
        if detail:
            msg += f" {CYAN}({detail}){RESET}"
        print(msg)

    def record_fail(self, name: str, reason: str):
        self.failed += 1
        print(f"{RED}[FAIL]{RESET} {name} -> {YELLOW}{reason}{RESET}")

    def record_skip(self, name: str, reason: str):
        self.skipped += 1
        print(f"{YELLOW}[SKIP]{RESET} {name} -> {reason}")

    def summary(self):
        print("\n" + "=" * 60)
        total = self.passed + self.failed + self.skipped
        print(f"{BOLD}RESUMO DOS TESTES:{RESET} Total: {total} | {GREEN}Passou: {self.passed}{RESET} | {RED}Falhou: {self.failed}{RESET} | {YELLOW}Ignorado: {self.skipped}{RESET}")
        print("=" * 60)
        return self.failed == 0


def test_port(host: str, port: int, timeout: float = 1.0) -> bool:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    try:
        s.connect((host, port))
        s.close()
        return True
    except (socket.timeout, ConnectionRefusedError, OSError):
        return False


def test_packet_serialization(reporter: TestReporter):
    print(f"\n{BOLD}[1] Teste de Serialização de Pacotes (Memória & Bytes){RESET}")

    # 1.1 Teste CA_LOGIN
    try:
        packet_id = PACKET_CA_LOGIN
        version = 20211103
        username = b"testuser".ljust(24, b"\x00")
        password = b"secret123".ljust(24, b"\x00")
        client_type = 0

        ca_login_data = struct.pack("<HI24s24sB", packet_id, version, username, password, client_type)
        expected_len = 2 + 4 + 24 + 24 + 1

        if len(ca_login_data) == expected_len:
            reporter.record_pass("Pacote CA_LOGIN (Estrutura de Login)", f"{len(ca_login_data)} bytes corretos")
        else:
            reporter.record_fail("Pacote CA_LOGIN", f"Tamanho incorreto: {len(ca_login_data)} != {expected_len}")
    except Exception as e:
        reporter.record_fail("Pacote CA_LOGIN", str(e))

    # 1.2 Teste CZ_REQUEST_MOVE (Packing canônico WBUFPOS / RBUFPOS do rAthena)
    try:
        x, y = 156, 180
        b0 = (x >> 2) & 0xFF
        b1 = ((x << 6) & 0xC0) | ((y >> 4) & 0x3F)
        b2 = (y << 4) & 0xF0
        move_packet = struct.pack("<H", PACKET_CZ_REQUEST_MOVE) + bytes([b0, b1, b2])

        if len(move_packet) == 5:
            p = move_packet[2:]
            unpacked_x = (p[0] << 2) | (p[1] >> 6)
            unpacked_y = ((p[1] & 0x3F) << 4) | (p[2] >> 4)

            if (unpacked_x, unpacked_y) == (x, y):
                reporter.record_pass("Pacote CZ_REQUEST_MOVE (Coordenadas de Prontera)", f"X={unpacked_x}, Y={unpacked_y}")
            else:
                reporter.record_fail("Pacote CZ_REQUEST_MOVE", f"Desempacotamento falhou: ({unpacked_x}, {unpacked_y}) != ({x}, {y})")
        else:
            reporter.record_fail("Pacote CZ_REQUEST_MOVE", f"Tamanho incorreto: {len(move_packet)} != 5")
    except Exception as e:
        reporter.record_fail("Pacote CZ_REQUEST_MOVE", str(e))

    # 1.3 Teste CZ_REQUEST_ACT (Ataque Normal)
    try:
        target_id = 110023
        action_type = 0
        act_packet = struct.pack("<HIB", PACKET_CZ_REQUEST_ACT, target_id, action_type)
        if len(act_packet) == 7:
            p_id, t_id, a_type = struct.unpack("<HIB", act_packet)
            if p_id == PACKET_CZ_REQUEST_ACT and t_id == target_id and a_type == 0:
                reporter.record_pass("Pacote CZ_REQUEST_ACT (Ataque Básico)", f"7 bytes corretos, TargetID={t_id}")
            else:
                reporter.record_fail("Pacote CZ_REQUEST_ACT", "Campos desempacotados incorretos")
        else:
            reporter.record_fail("Pacote CZ_REQUEST_ACT", f"Tamanho incorreto: {len(act_packet)} != 7")
    except Exception as e:
        reporter.record_fail("Pacote CZ_REQUEST_ACT", str(e))

    # 1.4 Teste CZ_USE_SKILL (Conjuração de Habilidade)
    try:
        skill_id = 8   # Bash / Golpe Fatal
        skill_lvl = 10
        target_id = 110023
        skill_packet = struct.pack("<HHHI", PACKET_CZ_USE_SKILL, skill_lvl, skill_id, target_id)
        if len(skill_packet) == 10:
            p_id, s_lvl, s_id, t_id = struct.unpack("<HHHI", skill_packet)
            if s_id == skill_id and s_lvl == skill_lvl and t_id == target_id:
                reporter.record_pass("Pacote CZ_USE_SKILL (Conjuração de Bash Lvl 10)", f"10 bytes corretos, Skill={s_id}, Lvl={s_lvl}")
            else:
                reporter.record_fail("Pacote CZ_USE_SKILL", "Campos desempacotados incorretos")
        else:
            reporter.record_fail("Pacote CZ_USE_SKILL", f"Tamanho incorreto: {len(skill_packet)} != 10")
    except Exception as e:
        reporter.record_fail("Pacote CZ_USE_SKILL", str(e))


class AutoBattleManager:
    MAX_DAILY_SECONDS = 3 * 3600  # 3 horas
    DOWNED_WAIT_SECONDS = 120.0   # 2 minutos caído antes do respawn

    def __init__(self):
        self.remaining_seconds = self.MAX_DAILY_SECONDS
        self.is_active = False
        self.inventory_weight_percent = 30.0
        self.current_hp = 500
        self.max_hp = 500
        self.base_exp_percent = 50.0  # 50% de EXP base
        self.is_downed = False
        self.downed_timer = 0.0
        self.location = "prt_fild08"
        self.save_point = "prontera"

    def toggle(self, state: bool) -> Tuple[bool, str]:
        if state:
            if self.is_downed or self.current_hp <= 0:
                return False, "Não é possível ativar Auto-Battle enquanto estiver morto."
            if self.remaining_seconds <= 0:
                return False, "Vigor diário esgotado (0h restante)."
            if self.inventory_weight_percent >= 90.0:
                return False, "Inventário acima de 90% de sobrepeso."
            self.is_active = True
            return True, "Auto-Battle ATIVADO."
        else:
            self.is_active = False
            return True, "Auto-Battle DESATIVADO."

    def on_player_death(self):
        """Disparado quando o HP do jogador atinge zero."""
        self.current_hp = 0
        self.is_downed = True
        self.downed_timer = self.DOWNED_WAIT_SECONDS

        if self.is_active:
            # Penalidade agravada para morte no Auto-Battle: 5% de EXP
            self.is_active = False
            self.base_exp_percent = max(0.0, self.base_exp_percent - 5.0)
            return 5.0
        else:
            # Penalidade normal de morte manual: 1% de EXP
            self.base_exp_percent = max(0.0, self.base_exp_percent - 1.0)
            return 1.0

    def tick(self, elapsed_seconds: float):
        if self.is_active:
            self.remaining_seconds = max(0.0, self.remaining_seconds - elapsed_seconds)
            if self.remaining_seconds == 0:
                self.is_active = False

        if self.is_downed:
            self.downed_timer -= elapsed_seconds
            if self.downed_timer <= 0:
                # Retorna ao ponto de save (Prontera) com HP recuperado
                self.is_downed = False
                self.downed_timer = 0.0
                self.location = self.save_point
                self.current_hp = self.max_hp
                # Auto-Battle permanece RIGOROSAMENTE desligado após respawn
                self.is_active = False

    def reset_daily(self):
        self.remaining_seconds = self.MAX_DAILY_SECONDS


def test_autobattle_logic(reporter: TestReporter):
    print(f"\n{BOLD}[2] Teste da Mecânica de Auto-Battle (3h Diárias, Morte 5% & Retorno){RESET}")

    ab = AutoBattleManager()

    # 2.1 Ativação normal
    ok, msg = ab.toggle(True)
    if ok and ab.is_active:
        reporter.record_pass("Ativação de Auto-Battle", msg)
    else:
        reporter.record_fail("Ativação de Auto-Battle", msg)

    # 2.2 Consumo de 1h
    ab.tick(3600)
    if ab.remaining_seconds == 7200 and ab.is_active:
        reporter.record_pass("Consumo de Vigor (1h decorrida)", f"Restam {ab.remaining_seconds/3600:.1f}h")
    else:
        reporter.record_fail("Consumo de Vigor", f"Tempo incorreto: {ab.remaining_seconds}")

    # 2.3 Esgotamento de 3h
    ab.tick(7200)
    if ab.remaining_seconds == 0 and not ab.is_active:
        reporter.record_pass("Desativação Automática ao zerar 3h", "Auto-Battle desligou com sucesso")
    else:
        reporter.record_fail("Desativação Automática", f"Deveria estar inativo: active={ab.is_active}")

    # 2.4 Bloqueio de sobrepeso
    ab.reset_daily()
    ab.inventory_weight_percent = 92.0
    ok, msg = ab.toggle(True)
    if not ok and not ab.is_active:
        reporter.record_pass("Bloqueio de Sobrecarga (Peso > 90%)", "Impediu início com inventário cheio")
    else:
        reporter.record_fail("Bloqueio de Sobrecarga", "Permitiu ativar mesmo com 92% de peso")

    # 2.5 Teste de Morte no Auto-Battle: Penalidade de 5% de EXP
    ab.inventory_weight_percent = 30.0
    ab.toggle(True)
    exp_before = ab.base_exp_percent
    penalty = ab.on_player_death()

    if penalty == 5.0 and ab.base_exp_percent == (exp_before - 5.0) and not ab.is_active and ab.is_downed:
        reporter.record_pass("Penalidade Agravada de Morte no Auto-Battle", f"Perdeu {penalty}% de EXP e Auto-Battle desligou")
    else:
        reporter.record_fail("Penalidade Agravada de Morte", f"Penalidade incorreta: {penalty}%")

    # 2.6 Teste de Tempo Caído (2 minutos) e Retorno ao Ponto de Save
    ab.tick(60)  # Passa 1 minuto caído
    if ab.is_downed and ab.location == "prt_fild08":
        reporter.record_pass("Estado Caído no Campo (1 min)", "Permanece no local para possível suporte")
    else:
        reporter.record_fail("Estado Caído no Campo", "Não deveria ter teleportado ainda")

    ab.tick(70)  # Passa mais 70 segundos (completando mais de 2 min)
    if not ab.is_downed and ab.location == "prontera" and not ab.is_active:
        reporter.record_pass("Retorno ao Ponto de Save (Prontera)", "Teleportou para cidade com Auto-Battle pausado")
    else:
        reporter.record_fail("Retorno ao Ponto de Save", f"Local: {ab.location}, Downed: {ab.is_downed}, Active: {ab.is_active}")


def test_services_connectivity(host: str, reporter: TestReporter):
    print(f"\n{BOLD}[3] Teste de Conexão com Servidores rAthena ({host}){RESET}")

    services = [
        ("MariaDB Database", DB_PORT),
        ("Login-Server", LOGIN_PORT),
        ("Char-Server", CHAR_PORT),
        ("Map-Server", MAP_PORT),
    ]

    for name, port in services:
        if test_port(host, port):
            reporter.record_pass(f"Porta {port} [{name}]", "Conexão TCP aceita")
        else:
            reporter.record_skip(f"Porta {port} [{name}]", f"Serviço offline no momento em {host}:{port}")


def test_live_server_flow(host: str, reporter: TestReporter):
    print(f"\n{BOLD}[4] Teste End-to-End no Servidor Real ({host}) - Login -> Char -> Map{RESET}")

    if not (test_port(host, LOGIN_PORT) and test_port(host, CHAR_PORT) and test_port(host, MAP_PORT)):
        reporter.record_skip("Fluxo End-to-End", "Um ou mais servidores offline")
        return

    import time
    username = "mythtest"
    password = "mythtest123"

    try:
        # 4.1 Login Server (6900)
        s_login = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_login.settimeout(4.0)
        s_login.connect((host, LOGIN_PORT))

        ca_login = struct.pack("<HI24s24sB", 0x0064, 20211103, username.encode('ascii').ljust(24, b'\x00'), password.encode('ascii').ljust(24, b'\x00'), 0)
        s_login.sendall(ca_login)

        resp = s_login.recv(1024)
        if len(resp) < 2:
            reporter.record_fail("Login Server (0x0064)", "Resposta vazia")
            s_login.close()
            return

        cmd, = struct.unpack_from("<H", resp, 0)
        if cmd == 0x083e:
            err, = struct.unpack_from("<I", resp, 2)
            reporter.record_fail("Login Server (0x0064)", f"Login recusado: código {err}")
            s_login.close()
            return
        elif cmd == 0x0081:
            reporter.record_skip("Login Server (0x0064)", "Usuário já conectado na sessão anterior (aguardando timeout)")
            s_login.close()
            return
        elif cmd != 0x0ac4:
            reporter.record_fail("Login Server (0x0064)", f"Packet inesperado: 0x{cmd:04x}")
            s_login.close()
            return

        packet_type, packet_len, login_id1, account_id, login_id2, last_ip = struct.unpack_from("<HHIIII", resp, 0)
        sex = resp[46]
        char_ip_raw, char_port, char_name = struct.unpack_from("<IH20s", resp, 64)
        s_login.close()
        reporter.record_pass("Login-Server Autenticação (0x0ac4)", f"AID={account_id}, LoginID1={login_id1}")

        # 4.2 Char Server (6121)
        s_char = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_char.settimeout(4.0)
        s_char.connect((host, char_port if char_port != 0 else CHAR_PORT))

        ch_enter = struct.pack("<HIIIHB", 0x0065, account_id, login_id1, login_id2, 0, sex)
        s_char.sendall(ch_enter)

        ack_aid = s_char.recv(4)
        char_data = s_char.recv(4096)
        ch_select = struct.pack("<HB", 0x0066, 0)
        s_char.sendall(ch_select)

        zone_data = s_char.recv(1024)
        z_cmd, z_cid = struct.unpack_from("<HI", zone_data, 0)
        z_map = zone_data[6:22].decode('ascii', errors='ignore').strip(chr(0))
        z_ip_raw, z_port = struct.unpack_from("<IH", zone_data, 22)
        s_char.close()
        reporter.record_pass("Char-Server Seleção (0x0066 -> 0x0ac5)", f"CID={z_cid}, Mapa='{z_map}'")

        # 4.3 Map Server (5121)
        s_map = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_map.settimeout(4.0)
        s_map.connect((host, z_port if z_port != 0 else MAP_PORT))

        client_tick = int(time.time() * 1000) & 0xFFFFFFFF
        cz_enter = struct.pack("<HIIIIB", 0x0888, account_id, z_cid, login_id1, client_tick, sex)
        s_map.sendall(cz_enter)

        # Aguarda ZC_ACCEPT_ENTER (0x02eb) do Map Server
        initial_buf = b""
        got_accept = False
        t_wait = time.time()
        while time.time() - t_wait < 4.0 and not got_accept:
            chunk = s_map.recv(4096)
            if not chunk:
                break
            initial_buf += chunk
            if b'\xeb\x02' in initial_buf:
                got_accept = True
                break

        if not got_accept:
            reporter.record_fail("Map-Server Entrada", "Timeout aguardando ZC_ACCEPT_ENTER (0x02eb)")
            s_map.close()
            return

        # Envia CZ_NOTIFY_ACTORINIT (0x007d)
        cz_load = struct.pack("<H", 0x007d)
        s_map.sendall(cz_load)

        time.sleep(0.3)
        # Drain any pending world packets first
        world_data = b""
        s_map.settimeout(1.0)
        try:
            while True:
                chunk = s_map.recv(4096)
                if not chunk:
                    break
                world_data += chunk
                if len(chunk) < 4096:
                    break
        except (socket.timeout, BlockingIOError):
            pass

        # 4.4 Test Move in Map Server (WBUFPOS / RBUFPOS)
        target_x, target_y = 158, 182
        b0 = (target_x >> 2) & 0xFF
        b1 = ((target_x << 6) & 0xC0) | ((target_y >> 4) & 0x3F)
        b2 = (target_y << 4) & 0xF0
        cz_move = struct.pack("<H", PACKET_CZ_REQUEST_MOVE) + bytes([b0, b1, b2])
        s_map.sendall(cz_move)

        move_resp = b""
        got_move = False
        t_start = time.time()
        while time.time() - t_start < 4.0 and not got_move:
            try:
                chunk = s_map.recv(4096)
                if not chunk:
                    break
                move_resp += chunk
                if b'\x87\x00' in move_resp:
                    got_move = True
                    break
            except (socket.timeout, BlockingIOError):
                break

        s_map.close()

        reporter.record_pass("Map-Server Entrada e Pacotes do Mundo", f"Recebidos {len(world_data)} bytes iniciais")
        if got_move:
            reporter.record_pass("Map-Server Movimentação (0x035F -> ZC_NOTIFY_PLAYERMOVE)", f"Recebido 0x0087 com confirmação de rota para ({target_x}, {target_y})")
        else:
            reporter.record_fail("Map-Server Movimentação (0x035F)", f"0x0087 não retornado pelo servidor ({len(move_resp)} bytes recebidos)")

    except Exception as e:
        reporter.record_fail("Fluxo End-to-End Real", str(e))


def main():
    parser = argparse.ArgumentParser(description="Myth of Thor Automated Test Suite")
    parser.add_argument("--host", default=DEFAULT_HOST, help="IP do servidor rAthena")
    args = parser.parse_args()

    print("=" * 60)
    print(f"{BOLD}{CYAN}MYTH OF THOR - TEST RUNNER AUTOMÁTICO{RESET}")
    print("=" * 60)

    reporter = TestReporter()
    test_packet_serialization(reporter)
    test_autobattle_logic(reporter)
    test_services_connectivity(args.host, reporter)
    test_live_server_flow(args.host, reporter)

    success = reporter.summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()

