import struct
import zlib

mask = [0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01]

initialPermutationTable = [
  58, 50, 42, 34, 26, 18, 10,  2,
  60, 52, 44, 36, 28, 20, 12,  4,
  62, 54, 46, 38, 30, 22, 14,  6,
  64, 56, 48, 40, 32, 24, 16,  8,
  57, 49, 41, 33, 25, 17,  9,  1,
  59, 51, 43, 35, 27, 19, 11,  3,
  61, 53, 45, 37, 29, 21, 13,  5,
  63, 55, 47, 39, 31, 23, 15,  7
]

finalPermutationTable = [
  40,  8, 48, 16, 56, 24, 64, 32,
  39,  7, 47, 15, 55, 23, 63, 31,
  38,  6, 46, 14, 54, 22, 62, 30,
  37,  5, 45, 13, 53, 21, 61, 29,
  36,  4, 44, 12, 52, 20, 60, 28,
  35,  3, 43, 11, 51, 19, 59, 27,
  34,  2, 42, 10, 50, 18, 58, 26,
  33,  1, 41,  9, 49, 17, 57, 25
]

transpositionTable = [
  16,  7, 20, 21,
  29, 12, 28, 17,
   1, 15, 23, 26,
   5, 18, 31, 10,
   2,  8, 24, 14,
  32, 27,  3,  9,
  19, 13, 30,  6,
  22, 11,  4, 25
]

substitutionBoxTable = [
  [
    0xef, 0x03, 0x41, 0xfd, 0xd8, 0x74, 0x1e, 0x47,  0x26, 0xef, 0xfb, 0x22, 0xb3, 0xd8, 0x84, 0x1e,
    0x39, 0xac, 0xa7, 0x60, 0x62, 0xc1, 0xcd, 0xba,  0x5c, 0x96, 0x90, 0x59, 0x05, 0x3b, 0x7a, 0x85,
    0x40, 0xfd, 0x1e, 0xc8, 0xe7, 0x8a, 0x8b, 0x21,  0xda, 0x43, 0x64, 0x9f, 0x2d, 0x14, 0xb1, 0x72,
    0xf5, 0x5b, 0xc8, 0xb6, 0x9c, 0x37, 0x76, 0xec,  0x39, 0xa0, 0xa3, 0x05, 0x52, 0x6e, 0x0f, 0xd9 
  ],
  [
    0xa7, 0xdd, 0x0d, 0x78, 0x9e, 0x0b, 0xe3, 0x95,  0x60, 0x36, 0x36, 0x4f, 0xf9, 0x60, 0x5a, 0xa3,
    0x11, 0x24, 0xd2, 0x87, 0xc8, 0x52, 0x75, 0xec,  0xbb, 0xc1, 0x4c, 0xba, 0x24, 0xfe, 0x8f, 0x19,
    0xda, 0x13, 0x66, 0xaf, 0x49, 0xd0, 0x90, 0x06,  0x8c, 0x6a, 0xfb, 0x91, 0x37, 0x8d, 0x0d, 0x78,
    0xbf, 0x49, 0x11, 0xf4, 0x23, 0xe5, 0xce, 0x3b,  0x55, 0xbc, 0xa2, 0x57, 0xe8, 0x22, 0x74, 0xce
  ],
  [
    0x2c, 0xea, 0xc1, 0xbf, 0x4a, 0x24, 0x1f, 0xc2,  0x79, 0x47, 0xa2, 0x7c, 0xb6, 0xd9, 0x68, 0x15,
    0x80, 0x56, 0x5d, 0x01, 0x33, 0xfd, 0xf4, 0xae,  0xde, 0x30, 0x07, 0x9b, 0xe5, 0x83, 0x9b, 0x68,
    0x49, 0xb4, 0x2e, 0x83, 0x1f, 0xc2, 0xb5, 0x7c,  0xa2, 0x19, 0xd8, 0xe5, 0x7c, 0x2f, 0x83, 0xda,
    0xf7, 0x6b, 0x90, 0xfe, 0xc4, 0x01, 0x5a, 0x97,  0x61, 0xa6, 0x3d, 0x40, 0x0b, 0x58, 0xe6, 0x3d
  ],
  [
    0x4d, 0xd1, 0xb2, 0x0f, 0x28, 0xbd, 0xe4, 0x78,  0xf6, 0x4a, 0x0f, 0x93, 0x8b, 0x17, 0xd1, 0xa4,
    0x3a, 0xec, 0xc9, 0x35, 0x93, 0x56, 0x7e, 0xcb,  0x55, 0x20, 0xa0, 0xfe, 0x6c, 0x89, 0x17, 0x62,
    0x17, 0x62, 0x4b, 0xb1, 0xb4, 0xde, 0xd1, 0x87,  0xc9, 0x14, 0x3c, 0x4a, 0x7e, 0xa8, 0xe2, 0x7d,
    0xa0, 0x9f, 0xf6, 0x5c, 0x6a, 0x09, 0x8d, 0xf0,  0x0f, 0xe3, 0x53, 0x25, 0x95, 0x36, 0x28, 0xcb
  ]
]

# Shuffle table
shuffleDecTable = list(range(256))
_pairs = [
  0x00, 0x2b, 0x6c, 0x80, 0x01, 0x68, 0x48,
  0x77, 0x60, 0xff, 0xb9, 0xc0, 0xfe, 0xeb
]
for i in range(0, len(_pairs), 2):
    a = _pairs[i]
    b = _pairs[i+1]
    shuffleDecTable[a] = b
    shuffleDecTable[b] = a

def initialPermutation(src: bytearray, index: int):
    tmp = [0] * 8
    for i in range(64):
        j = initialPermutationTable[i] - 1
        if src[index + ((j >> 3) & 7)] & mask[j & 7]:
            tmp[(i >> 3) & 7] |= mask[i & 7]
    src[index:index+8] = tmp

def finalPermutation(src: bytearray, index: int):
    tmp = [0] * 8
    for i in range(64):
        j = finalPermutationTable[i] - 1
        if src[index + ((j >> 3) & 7)] & mask[j & 7]:
            tmp[(i >> 3) & 7] |= mask[i & 7]
    src[index:index+8] = tmp

def transposition(src: bytearray, index: int, tmp: list):
    for i in range(32):
        j = transpositionTable[i] - 1
        if src[index + (j >> 3)] & mask[j & 7]:
            tmp[(i >> 3) + 4] |= mask[i & 7]

def expansion(src: bytearray, index: int) -> list:
    return [
      ((src[index + 7] << 5) | (src[index + 4] >> 3)) & 0x3f,
      ((src[index + 4] << 1) | (src[index + 5] >> 7)) & 0x3f,
      ((src[index + 4] << 5) | (src[index + 5] >> 3)) & 0x3f,
      ((src[index + 5] << 1) | (src[index + 6] >> 7)) & 0x3f,
      ((src[index + 5] << 5) | (src[index + 6] >> 3)) & 0x3f,
      ((src[index + 6] << 1) | (src[index + 7] >> 7)) & 0x3f,
      ((src[index + 6] << 5) | (src[index + 7] >> 3)) & 0x3f,
      ((src[index + 7] << 1) | (src[index + 4] >> 7)) & 0x3f,
    ]

def substitutionBox(exp_data: list) -> list:
    res = [0] * 8
    for i in range(4):
        res[i] = (substitutionBoxTable[i][exp_data[i * 2]] & 0xf0) | (substitutionBoxTable[i][exp_data[i * 2 + 1]] & 0x0f)
    return res

def roundFunction(src: bytearray, index: int):
    tmp2 = src[index:index+8]
    exp_data = expansion(tmp2, 0)
    sbox_data = substitutionBox(exp_data)
    tmp_tp = [0] * 8
    for i in range(32):
        j = transpositionTable[i] - 1
        if sbox_data[j >> 3] & mask[j & 7]:
            tmp_tp[(i >> 3) + 4] |= mask[i & 7]
    src[index + 0] ^= tmp_tp[4]
    src[index + 1] ^= tmp_tp[5]
    src[index + 2] ^= tmp_tp[6]
    src[index + 3] ^= tmp_tp[7]

def decryptBlock(src: bytearray, index: int):
    initialPermutation(src, index)
    roundFunction(src, index)
    finalPermutation(src, index)

def shuffleDec(src: bytearray, index: int):
    t0 = src[index + 3]
    t1 = src[index + 4]
    t2 = src[index + 6]
    t3 = src[index + 0]
    t4 = src[index + 1]
    t5 = src[index + 2]
    t6 = src[index + 5]
    t7 = shuffleDecTable[src[index + 7]]
    src[index:index+8] = [t0, t1, t2, t3, t4, t5, t6, t7]

def decodeHeader(src: bytearray, length: int):
    count = length >> 3
    limit = min(20, count)
    for i in range(limit):
        decryptBlock(src, i * 8)

def decodeFull(src: bytearray, length: int, entryLength: int):
    digits = len(str(entryLength))
    if digits < 3:
        cycle = 1
    elif digits < 5:
        cycle = digits + 1
    elif digits < 7:
        cycle = digits + 9
    else:
        cycle = digits + 15

    nblocks = length >> 3
    limit = min(20, nblocks)
    for i in range(limit):
        decryptBlock(src, i * 8)

    j = -1
    for i in range(20, nblocks):
        if i % cycle == 0:
            decryptBlock(src, i * 8)
            continue
        j += 1
        if j and (j % 7 == 0):
            shuffleDec(src, i * 8)
