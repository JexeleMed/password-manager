#include <algorithm>
#include <iostream>
#include <random>
#include <array>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>


using Block = std::array<uint8_t, 16>;
using Blocks = std::vector<Block>;

//S-box array
const std::array<uint8_t, 256> sbox = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};
//Rcon array
const std::array<uint8_t, 10> rcon = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36
};


// Key generator
std::array<uint8_t, 16> generateKey() {
    std::array<uint8_t, 16> key = {};
    std::random_device rd;

    for(auto& byte : key) {
        byte = rd() % 256;
    }

    return key;
}


// BYTES to HEX
template <std::size_t N>
std::string toHex(const std::array<uint8_t, N>& key) {
    std::ostringstream oss;
    for (uint8_t byte : key) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

void subBytes(Blocks& text) {
    for (auto& block : text) {
        for (auto& byte : block) {
            byte = sbox[byte];
        }
    }
}

void shiftRowsHelp(Block& block) {
    std::rotate(block.begin() + 4, block.begin() + 5, block.begin() + 8);

    std::rotate(block.begin() + 8, block.begin() + 10, block.begin() + 12);

    std::rotate(block.begin() + 12, block.begin() + 15, block.begin() + 16);
}

void shiftRows(Blocks& blocks) {
    for(auto& block : blocks) {
        shiftRowsHelp(block);
    }
}

uint8_t gfMul(uint8_t a, uint8_t b) {
    uint8_t res = 0;
    while (b) {
        if (b & 1) res ^= a;        // If the youngest bit is set add a
        bool carry = a & 0x80;     // Check highest bit
        a <<= 1;                   // Shift left
        if (carry) a ^= 0x1B;      // XOR
        b >>= 1;                   // Shift b right
    }
    return res;
}

void mixColumnsHelp(Block& block) {
    for (int col = 0; col < 4; ++col) {
        uint8_t b0 = block[col];
        uint8_t b1 = block[col + 4];
        uint8_t b2 = block[col + 8];
        uint8_t b3 = block[col + 12];

        block[col]      = gfMul(0x02, b0) ^ gfMul(0x03, b1) ^ b2 ^ b3;
        block[col + 4]  = b0 ^ gfMul(0x02, b1) ^ gfMul(0x03, b2) ^ b3;
        block[col + 8]  = b0 ^ b1 ^ gfMul(0x02, b2) ^ gfMul(0x03, b3);
        block[col + 12] = gfMul(0x03, b0) ^ b1 ^ b2 ^ gfMul(0x02, b3);
    }
}

void mixColumns(Blocks& blocks) {
    for (auto& block : blocks) {
        mixColumnsHelp(block); // Mix columns for each block
    }
}



// Load file to buffer and apply padding
Blocks load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return {};
    }

    Blocks blocks;


        while (!file.eof()) {
            constexpr size_t chunkSize = 16;
            Block block = {};
            file.read(reinterpret_cast<char*>(block.data()), chunkSize);
            size_t bytesRead = file.gcount();

            // Padding
            if (bytesRead < chunkSize) {
                std::fill(block.begin() + bytesRead, block.end(), static_cast<uint8_t>(chunkSize - bytesRead));
            }

            blocks.push_back(block);
        }

    return blocks;
}

std::vector<Block> keyExpansion(const Block& key) {
    std::vector<Block> roundKeys(11);
    roundKeys[0] = key;

    uint32_t rconIndex = 0;
    for (size_t round = 1; round <= 10; ++round) {
        std::array<uint8_t, 4> temp = {
            roundKeys[round - 1][12],
            roundKeys[round - 1][13],
            roundKeys[round - 1][14],
            roundKeys[round - 1][15],
        };

        std::rotate(temp.begin(), temp.begin() + 1, temp.end());

        for (auto& byte : temp) {
            byte = sbox[byte];
        }

        temp[0] ^= rcon[rconIndex++];

        for (size_t i = 0; i < 4; ++i) {
            roundKeys[round][i] = roundKeys[round - 1][i] ^ temp[i];
        }

        for (size_t i = 4; i < 16; ++i) {
            roundKeys[round][i] = roundKeys[round - 1][i] ^ roundKeys[round][i - 4];
        }
    }

    return roundKeys;
}


// Plain text XORing with RoundKey to add
void addRoundKey(Blocks& blocks, const std::vector<Block>& roundKeys, size_t round) {
    for (auto& block : blocks) {
        for (size_t j = 0; j < block.size(); ++j) {
            block[j] ^= roundKeys[round][j];
        }
    }
}

void aesEncrypt(Blocks& blocks, const std::array<uint8_t, 16>& key) {
    auto roundKeys = keyExpansion(key);
    addRoundKey(blocks, roundKeys, 0);
    for(size_t round = 1; round < 10; ++round) {
        subBytes(blocks);
        for (auto& block : blocks) {
            // Apply ShiftRows and MixColumns
            shiftRowsHelp(block);
            mixColumnsHelp(block);
        }
        addRoundKey(blocks, roundKeys, round);
    }
}

void printBlocks(const Blocks& blocks) {
    for (const auto& block : blocks) {
        std::cout << toHex(block) << std::endl;
    }
}

int main() {
    auto key = generateKey();
    auto iv = generateKey();

    // Key printing for debugging
    std::string hexKey = toHex(key);
    std::string hexIV = toHex(iv);
    std::cout << "Hex Key: " << hexKey << std::endl;
    std::cout << "Hex IV: " << hexIV << std::endl;

    std::string path = "sample.txt";
    Blocks data = load(path);

    aesEncrypt(data, key);

    std::cout << "Encrypted Data:" << std::endl;
    printBlocks(data);
    return 0;
}
