import gdb
import json

gdb.execute("start")
print("Calling packetdb_readdb()...")
gdb.execute("call packetdb_readdb()")

packet_db = gdb.parse_and_eval("packet_db")
print(f"packet_db size: {packet_db.type.sizeof}")

packets = {}
for i in range(3328):
    length = int(packet_db[i]['len'])
    if length != 0:
        packets[f"0x{i:04x}"] = length

print(f"Total packets dumped: {len(packets)}")
output_path = "/rathena/packet_lengths_20211103.json"
with open(output_path, "w") as f:
    json.dump(packets, f, indent=2)

print(f"Saved {len(packets)} packets to {output_path} successfully!")
gdb.execute("quit")
