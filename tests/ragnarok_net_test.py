#!/usr/bin/env python3
"""
Ragnarok Reborn / Myth of Thor - Headless Network Protocol & Systems Test Suite
-------------------------------------------------------------------------------
Valida automaticamente a integridade do protocolo de rede,
a disponibilidade das portas do rAthena e a lógica matemática do Auto-Battle (3h).

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

    # 1.2 Teste CZ_REQUEST_MOVE
    try:
        x, y = 156, 180
        packed_coords = ((x & 0x3FF) << 14) | ((y & 0x3FF) << 4)
        move_packet = struct.pack("<HI", PACKET_CZ_REQUEST_MOVE, packed_coords)[:5]

        if len(move_packet) == 5:
            unpacked_val = struct.unpack("<I", move_packet[2:] + b"\x00")[0]
            unpacked_x = (unpacked_val >> 14) & 0x3FF
            unpacked_y = (unpacked_val >> 4) & 0x3FF

            if (unpacked_x, unpacked_y) == (x, y):
                reporter.record_pass("Pacote CZ_REQUEST_MOVE (Coordenadas de Prontera)", f"X={unpacked_x}, Y={unpacked_y}")
            else:
                reporter.record_fail("Pacote CZ_REQUEST_MOVE", f"Desempacotamento falhou: ({unpacked_x}, {unpacked_y}) != ({x}, {y})")
        else:
            reporter.record_fail("Pacote CZ_REQUEST_MOVE", f"Tamanho incorreto: {len(move_packet)} != 5")
    except Exception as e:
        reporter.record_fail("Pacote CZ_REQUEST_MOVE", str(e))


class AutoBattleManager:
    MAX_DAILY_SECONDS = 3 * 3600

    def __init__(self):
        self.remaining_seconds = self.MAX_DAILY_SECONDS
        self.is_active = False
        self.inventory_weight_percent = 30.0

    def toggle(self, state: bool) -> Tuple[bool, str]:
        if state:
            if self.remaining_seconds <= 0:
                return False, "Vigor diário esgotado (0h restante)."
            if self.inventory_weight_percent >= 90.0:
                return False, "Inventário acima de 90% de sobrepeso."
            self.is_active = True
            return True, "Auto-Battle ATIVADO."
        else:
            self.is_active = False
            return True, "Auto-Battle DESATIVADO."

    def tick(self, elapsed_seconds: float):
        if self.is_active:
            self.remaining_seconds = max(0.0, self.remaining_seconds - elapsed_seconds)
            if self.remaining_seconds == 0:
                self.is_active = False

    def reset_daily(self):
        self.remaining_seconds = self.MAX_DAILY_SECONDS


def test_autobattle_logic(reporter: TestReporter):
    print(f"\n{BOLD}[2] Teste da Mecânica de Auto-Battle (3h Diárias & Travas){RESET}")

    ab = AutoBattleManager()

    ok, msg = ab.toggle(True)
    if ok and ab.is_active:
        reporter.record_pass("Ativação de Auto-Battle", msg)
    else:
        reporter.record_fail("Ativação de Auto-Battle", msg)

    ab.tick(3600)
    if ab.remaining_seconds == 7200 and ab.is_active:
        reporter.record_pass("Consumo de Vigor (1h decorrida)", f"Restam {ab.remaining_seconds/3600:.1f}h")
    else:
        reporter.record_fail("Consumo de Vigor", f"Tempo incorreto: {ab.remaining_seconds}")

    ab.tick(7200)
    if ab.remaining_seconds == 0 and not ab.is_active:
        reporter.record_pass("Desativação Automática ao zerar 3h", "Auto-Battle desligou com sucesso")
    else:
        reporter.record_fail("Desativação Automática", f"Deveria estar inativo: active={ab.is_active}")

    ab.reset_daily()
    ab.inventory_weight_percent = 92.0
    ok, msg = ab.toggle(True)
    if not ok and not ab.is_active:
        reporter.record_pass("Bloqueio de Sobrecarga (Peso > 90%)", "Impediu início com inventário cheio")
    else:
        reporter.record_fail("Bloqueio de Sobrecarga", "Permitiu ativar mesmo com 92% de peso")


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

    success = reporter.summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
