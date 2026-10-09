import struct
import json
import sys

# Read maps
with open('/proc/7/maps') as f:
    for line in f:
        if '/rathena/map-server' in line and 'r--p 00000000' in line:
            base_str = line.split('-')[0]
            base_addr = int(base_str, 16)
            break
    else:
        print("Could not find base address", file=sys.stderr)
        sys.exit(1)

print(f"Base address: 0x{base_addr:x}", file=sys.stderr)

packet_db_offset = 0x01f83e40
packet_db_addr = base_addr + packet_db_offset
print(f"packet_db address: 0x{packet_db_addr:x}", file=sys.stderr)

# struct s_packet_db:
# int16 len (2 bytes)
# padding (6 bytes)
# void* func (8 bytes)
# int16 pos[20] (40 bytes)
# Total size: 56 bytes.
STRUCT_SIZE = 56

with open('/proc/7/mem', 'rb', 0) as mem:
    mem.seek(packet_db_addr)
    data = mem.read(3328 * STRUCT_SIZE)

packets = {}
for i in range(3328):
    chunk = data[i * STRUCT_SIZE : (i + 1) * STRUCT_SIZE]
    length, = struct.unpack('<h', chunk[:2])
    if length != 0:
        packets[f"0x{i:04x}"] = length

print(f"Found {len(packets)} active packets in packet_db!", file=sys.stderr)

for test_id in ["0x0064", "0x0065", "0x0072", "0x0069", "0x035f", "0x0360", "0x0113"]:
    print(f"Packet {test_id}: len = {packets.get(test_id, 'NOT FOUND')}", file=sys.stderr)

with open('/rathena/packet_lengths_20211103.json', 'w') as out:
    json.dump(packets, out, indent=2)

print("Saved to /rathena/packet_lengths_20211103.json", file=sys.stderr)
