import gdb

gdb.execute("start")
gdb.execute("call packetdb_readdb()")

packet_db = gdb.parse_and_eval("packet_db")
for i in range(3328):
    fn = str(packet_db[i]['func'])
    if 'WantToConnection' in fn or 'LoadEndAck' in fn or 'TickSend' in fn or 'WalkToXY' in fn or 'ActionRequest' in fn or 'UseSkill' in fn:
        pos = [int(packet_db[i]['pos'][j]) for j in range(6)]
        print(f"Packet 0x{i:04x}: len={int(packet_db[i]['len'])}, handler={fn}, pos={pos}")

gdb.execute("quit")
