#include "aes.h"
#include <cassert>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <ctime>

// All of the test functions for AES are implemented here, rather than in aes.cpp.

void AES::test()
{
    std::cout << "Testing AES...\n";
    srand(time(NULL));
    cleanup();
    std::cout << "Running testSteps()...\n";
    testSteps();
    cleanup();
    std::cout << "Running testEncyptDecrypt()...\n";
    testEncryptDecrypt();
    cleanup();
    std::cout << "Running testEndToEnd()...\n";
    testEndToEnd(".\\testFiles\\texttest.txt");
    cleanup();
    testEndToEnd(".\\testFiles\\largetest.txt");
    cleanup();
    std::cout << "Running testMalformedCiphertext()...\n";
    testMalformedCiphertext(".\\testFiles\\ciphertext1.test"); // test malformed padding
    cleanup();
    testMalformedCiphertext(".\\testFiles\\texttest.txt"); // test non-integer number of blocks
    std::cout << "Done testing AES!\n";
}

void getRandomKey(std::vector<uint8_t>& key, int numBytes)
{
    key.resize(numBytes);
    for (int i = 0; i < numBytes; i++)
    {
        key[i] = rand() % 256;
    }
}

void AES::testSteps()
{
    const std::vector<uint8_t> key1 =
    {0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00};
    const uint8_t roundkeys1[11][16] =
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
     {0x62,0x63,0x63,0x63,0x62,0x63,0x63,0x63,0x62,0x63,0x63,0x63,0x62,0x63,0x63,0x63}, 
     {0x9b,0x98,0x98,0xc9,0xf9,0xfb,0xfb,0xaa,0x9b,0x98,0x98,0xc9,0xf9,0xfb,0xfb,0xaa}, 
     {0x90,0x97,0x34,0x50,0x69,0x6c,0xcf,0xfa,0xf2,0xf4,0x57,0x33,0x0b,0x0f,0xac,0x99}, 
     {0xee,0x06,0xda,0x7b,0x87,0x6a,0x15,0x81,0x75,0x9e,0x42,0xb2,0x7e,0x91,0xee,0x2b}, 
     {0x7f,0x2e,0x2b,0x88,0xf8,0x44,0x3e,0x09,0x8d,0xda,0x7c,0xbb,0xf3,0x4b,0x92,0x90}, 
     {0xec,0x61,0x4b,0x85,0x14,0x25,0x75,0x8c,0x99,0xff,0x09,0x37,0x6a,0xb4,0x9b,0xa7}, 
     {0x21,0x75,0x17,0x87,0x35,0x50,0x62,0x0b,0xac,0xaf,0x6b,0x3c,0xc6,0x1b,0xf0,0x9b}, 
     {0x0e,0xf9,0x03,0x33,0x3b,0xa9,0x61,0x38,0x97,0x06,0x0a,0x04,0x51,0x1d,0xfa,0x9f}, 
     {0xb1,0xd4,0xd8,0xe2,0x8a,0x7d,0xb9,0xda,0x1d,0x7b,0xb3,0xde,0x4c,0x66,0x49,0x41}, 
     {0xb4,0xef,0x5b,0xcb,0x3e,0x92,0xe2,0x11,0x23,0xe9,0x51,0xcf,0x6f,0x8f,0x18,0x8e}};
    const std::vector<uint8_t> key2 =
    {0xff,0xff,0xff,0xff,
     0xff,0xff,0xff,0xff,
     0xff,0xff,0xff,0xff,
     0xff,0xff,0xff,0xff};
    const uint8_t roundkeys2[11][16] =
    {{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
     {0xe8,0xe9,0xe9,0xe9,0x17,0x16,0x16,0x16,0xe8,0xe9,0xe9,0xe9,0x17,0x16,0x16,0x16}, 
     {0xad,0xae,0xae,0x19,0xba,0xb8,0xb8,0x0f,0x52,0x51,0x51,0xe6,0x45,0x47,0x47,0xf0}, 
     {0x09,0x0e,0x22,0x77,0xb3,0xb6,0x9a,0x78,0xe1,0xe7,0xcb,0x9e,0xa4,0xa0,0x8c,0x6e}, 
     {0xe1,0x6a,0xbd,0x3e,0x52,0xdc,0x27,0x46,0xb3,0x3b,0xec,0xd8,0x17,0x9b,0x60,0xb6}, 
     {0xe5,0xba,0xf3,0xce,0xb7,0x66,0xd4,0x88,0x04,0x5d,0x38,0x50,0x13,0xc6,0x58,0xe6}, 
     {0x71,0xd0,0x7d,0xb3,0xc6,0xb6,0xa9,0x3b,0xc2,0xeb,0x91,0x6b,0xd1,0x2d,0xc9,0x8d}, 
     {0xe9,0x0d,0x20,0x8d,0x2f,0xbb,0x89,0xb6,0xed,0x50,0x18,0xdd,0x3c,0x7d,0xd1,0x50}, 
     {0x96,0x33,0x73,0x66,0xb9,0x88,0xfa,0xd0,0x54,0xd8,0xe2,0x0d,0x68,0xa5,0x33,0x5d}, 
     {0x8b,0xf0,0x3f,0x23,0x32,0x78,0xc5,0xf3,0x66,0xa0,0x27,0xfe,0x0e,0x05,0x14,0xa3}, 
     {0xd6,0x0a,0x35,0x88,0xe4,0x72,0xf0,0x7b,0x82,0xd2,0xd7,0x85,0x8c,0xd7,0xc3,0x26}};
    const std::vector<uint8_t> key3 =
    {0x00,0x01,0x02,0x03,
     0x04,0x05,0x06,0x07,
     0x08,0x09,0x0a,0x0b,
     0x0c,0x0d,0x0e,0x0f};
    const uint8_t roundkeys3[11][16] =
    {{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
     {0xd6,0xaa,0x74,0xfd,0xd2,0xaf,0x72,0xfa,0xda,0xa6,0x78,0xf1,0xd6,0xab,0x76,0xfe}, 
     {0xb6,0x92,0xcf,0x0b,0x64,0x3d,0xbd,0xf1,0xbe,0x9b,0xc5,0x00,0x68,0x30,0xb3,0xfe}, 
     {0xb6,0xff,0x74,0x4e,0xd2,0xc2,0xc9,0xbf,0x6c,0x59,0x0c,0xbf,0x04,0x69,0xbf,0x41}, 
     {0x47,0xf7,0xf7,0xbc,0x95,0x35,0x3e,0x03,0xf9,0x6c,0x32,0xbc,0xfd,0x05,0x8d,0xfd}, 
     {0x3c,0xaa,0xa3,0xe8,0xa9,0x9f,0x9d,0xeb,0x50,0xf3,0xaf,0x57,0xad,0xf6,0x22,0xaa}, 
     {0x5e,0x39,0x0f,0x7d,0xf7,0xa6,0x92,0x96,0xa7,0x55,0x3d,0xc1,0x0a,0xa3,0x1f,0x6b}, 
     {0x14,0xf9,0x70,0x1a,0xe3,0x5f,0xe2,0x8c,0x44,0x0a,0xdf,0x4d,0x4e,0xa9,0xc0,0x26}, 
     {0x47,0x43,0x87,0x35,0xa4,0x1c,0x65,0xb9,0xe0,0x16,0xba,0xf4,0xae,0xbf,0x7a,0xd2}, 
     {0x54,0x99,0x32,0xd1,0xf0,0x85,0x57,0x68,0x10,0x93,0xed,0x9c,0xbe,0x2c,0x97,0x4e}, 
     {0x13,0x11,0x1d,0x7f,0xe3,0x94,0x4a,0x17,0xf3,0x07,0xa7,0x8b,0x4d,0x2b,0x30,0xc5}};
    const std::vector<uint8_t> key4 =
    {0x49,0x20,0xe2,0x99,
     0xa5,0x20,0x52,0x61,
      0x64,0x69,0x6f,0x47,
      0x61,0x74,0x75,0x6e};
    const uint8_t roundkeys4[11][16] =
    {{0x49,0x20,0xe2,0x99,0xa5,0x20,0x52,0x61,0x64,0x69,0x6f,0x47,0x61,0x74,0x75,0x6e},
     {0xda,0xbd,0x7d,0x76,0x7f,0x9d,0x2f,0x17,0x1b,0xf4,0x40,0x50,0x7a,0x80,0x35,0x3e}, 
     {0x15,0x2b,0xcf,0xac,0x6a,0xb6,0xe0,0xbb,0x71,0x42,0xa0,0xeb,0x0b,0xc2,0x95,0xd5}, 
     {0x34,0x01,0xcc,0x87,0x5e,0xb7,0x2c,0x3c,0x2f,0xf5,0x8c,0xd7,0x24,0x37,0x19,0x02}, 
     {0xa6,0xd5,0xbb,0xb1,0xf8,0x62,0x97,0x8d,0xd7,0x97,0x1b,0x5a,0xf3,0xa0,0x02,0x58}, 
     {0x56,0xa2,0xd1,0xbc,0xae,0xc0,0x46,0x31,0x79,0x57,0x5d,0x6b,0x8a,0xf7,0x5f,0x33}, 
     {0x1e,0x6d,0x12,0xc2,0xb0,0xad,0x54,0xf3,0xc9,0xfa,0x09,0x98,0x43,0x0d,0x56,0xab}, 
     {0x89,0xdc,0x70,0xd8,0x39,0x71,0x24,0x2b,0xf0,0x8b,0x2d,0xb3,0xb3,0x86,0x7b,0x18}, 
     {0x4d,0xfd,0xdd,0xb5,0x74,0x8c,0xf9,0x9e,0x84,0x07,0xd4,0x2d,0x37,0x81,0xaf,0x35}, 
     {0x5a,0x84,0x4b,0x2f,0x2e,0x08,0xb2,0xb1,0xaa,0x0f,0x66,0x9c,0x9d,0x8e,0xc9,0xa9}, 
     {0x75,0x59,0x98,0x71,0x5b,0x51,0x2a,0xc0,0xf1,0x5e,0x4c,0x5c,0x6c,0xd0,0x85,0xf5}};

    const uint8_t state1[16] =
    {0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00};
    const uint8_t state2[16] =
    {0x49, 0x20, 0xe2, 0x99,
     0xa5, 0x20, 0x52, 0x61,
     0x64, 0x69, 0x6f, 0x47,
     0x61, 0x74, 0x75, 0x6e};
    const uint8_t state3[16] =
    {0x3c, 0x5a, 0x4e, 0xd7,
     0x5b, 0x03, 0x41, 0x8c,
     0x65, 0x2b, 0xfc, 0x8f,
     0x18, 0x10, 0x75, 0xea};
    const uint8_t state4[16] =
    {0xdb, 0x13, 0x53, 0x45,
     0xf2, 0x0a, 0x22, 0x5c,
     0x01, 0x01, 0x01, 0x01,
     0x2d, 0x26, 0x31, 0x4c};

    const uint8_t subbedState1[16] =
    {0x63, 0x63, 0x63, 0x63,
     0x63, 0x63, 0x63, 0x63,
     0x63, 0x63, 0x63, 0x63,
     0x63, 0x63, 0x63, 0x63};
    const uint8_t subbedState2[16] =
    {0x3b, 0xb7, 0x98, 0xee,
     0x06, 0xb7, 0x00, 0xef,
     0x43, 0xf9, 0xa8, 0xa0,
     0xef, 0x92, 0x9d, 0x9f};

    const uint8_t shiftState1[16] =
    {0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00};
    const uint8_t shiftState2[16] =
    {0x49, 0x20, 0x6f, 0x6e,
     0xa5, 0x69, 0x75, 0x99,
     0x64, 0x74, 0xe2, 0x61,
     0x61, 0x20, 0x52, 0x47};
    const uint8_t shiftState3[16] =
    {0x3c, 0x03, 0xfc, 0xea,
     0x5b, 0x2b, 0x75, 0xd7,
     0x65, 0x10, 0x4e, 0x8c,
     0x18, 0x5a, 0x41, 0x8f};
    
    const uint8_t mixColState4[16] =
    {0x8e, 0x4d, 0xa1, 0xbc,
     0x9f, 0xdc, 0x58, 0x9d,	
     0x01, 0x01, 0x01, 0x01,
     0x4d, 0x7e, 0xbd, 0xf8};

    /////////////// TEST KEY EXPANSION ///////////////
    keyExpansion(key1);
    for (int i = 0; i < 11; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            assert(m_roundKeys[i][j] == roundkeys1[i][j]);
        }
    }
    keyExpansion(key2);
    for (int i = 0; i < 11; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            assert(m_roundKeys[i][j] == roundkeys2[i][j]);
        }
    }
    keyExpansion(key3);
    for (int i = 0; i < 11; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            assert(m_roundKeys[i][j] == roundkeys3[i][j]);
        }
    }
    keyExpansion(key4);
    for (int i = 0; i < 11; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            assert(m_roundKeys[i][j] == roundkeys4[i][j]);
        }
    }

    /////////////// TEST ADDROUNDKEY ///////////////
    
    for (int i = 0; i < 16; i++) m_state[i] = state1[i];
    addRoundKey(0);
    assert(m_state[0] == 0x49);
    assert(m_state[1] == 0x20);
    assert(m_state[2] == 0xe2);
    assert(m_state[3] == 0x99);
    assert(m_state[4] == 0xa5);
    assert(m_state[5] == 0x20);
    assert(m_state[6] == 0x52);
    assert(m_state[7] == 0x61);
    assert(m_state[8] == 0x64);
    assert(m_state[9] == 0x69);
    assert(m_state[10] == 0x6f);
    assert(m_state[11] == 0x47);
    assert(m_state[12] == 0x61);
    assert(m_state[13] == 0x74);
    assert(m_state[14] == 0x75);
    assert(m_state[15] == 0x6e);
    addRoundKey(1);
    assert(m_state[0] == (0x49 ^ 0xda));
    assert(m_state[1] == (0x20 ^ 0xbd));
    assert(m_state[2] == (0xe2 ^ 0x7d));
    assert(m_state[3] == (0x99 ^ 0x76));
    assert(m_state[4] == (0xa5 ^ 0x7f));
    assert(m_state[5] == (0x20 ^ 0x9d));
    assert(m_state[6] == (0x52 ^ 0x2f));
    assert(m_state[7] == (0x61 ^ 0x17));
    assert(m_state[8] == (0x64 ^ 0x1b));
    assert(m_state[9] == (0x69 ^ 0xf4));
    assert(m_state[10] == (0x6f ^ 0x40));
    assert(m_state[11] == (0x47 ^ 0x50));
    assert(m_state[12] == (0x61 ^ 0x7a));
    assert(m_state[13] == (0x74 ^ 0x80));
    assert(m_state[14] == (0x75 ^ 0x35));
    assert(m_state[15] == (0x6e ^ 0x3e));

    /////////////// TEST SUBBYTES ///////////////

    for (int i = 0; i < 16; i++) m_state[i] = state1[i];
    subBytes();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == subbedState1[i]);
    }
    invSubBytes();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == state1[i]);
    }

    for (int i = 0; i < 16; i++) m_state[i] = state2[i];
    subBytes();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == subbedState2[i]);
    }
    invSubBytes();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == state2[i]);
    }

    /////////////// TEST SHIFTROWS ///////////////

    for (int i = 0; i < 16; i++) m_state[i] = state1[i];
    shiftRows();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == shiftState1[i]);
    }
    invShiftRows();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == state1[i]);
    }

    for (int i = 0; i < 16; i++) m_state[i] = state2[i];
    shiftRows();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == shiftState2[i]);
    }
    invShiftRows();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == state2[i]);
    }

    for (int i = 0; i < 16; i++) m_state[i] = state3[i];
    shiftRows();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == shiftState3[i]);
    }
    invShiftRows();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == state3[i]);
    }

    /////////////// TEST MIXCOLUMNS ///////////////

    for (int i = 0; i < 16; i++) m_state[i] = state4[i];
    mixColumns();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == mixColState4[i]);
    }
    invMixColumns();
    for (int i = 0; i < 16; i++)
    {
        assert(m_state[i] == state4[i]);
    }
}

void AES::testEncryptDecrypt()
{
    // Setup output file, which will hold the output of encrypt or decrypt
    std::string outputFilename = "testOutput.tmp";
    std::fstream outputFile;
    std::array<char, 16> output;
    outputFile.open(outputFilename, std::ios::out);
    assert(outputFile);
    outputFile.close();
    outputFile.open(outputFilename, std::ios::in | std::ios::out | std::ios::binary);
    assert(outputFile);

    // Iterate over the 284 tests
    for (int t = 1; t <= 284; t++)
    {
        std::string keyFilename = ".\\testFiles\\key" + std::to_string(t) + ".test";
        std::ifstream keyFile;
        std::vector<uint8_t> key;
        key.resize(16);

        std::string plaintextFilename = ".\\testFiles\\plaintext" + std::to_string(t) + ".test";
        std::ifstream plaintextFile;
        std::array<char, 16> plaintext;

        std::string ciphertextFilename = ".\\testFiles\\ciphertext" + std::to_string(t) + ".test";
        std::ifstream ciphertextFile;
        std::array<char, 16> ciphertext;

        // Get key
        keyFile.open(keyFilename, std::ios::in | std::ios::binary);
        assert(keyFile);
        keyFile.read(reinterpret_cast<char*>(key.data()), 16);
        keyFile.clear(); keyFile.seekg(0);

        // Get plaintext
        plaintextFile.open(plaintextFilename, std::ios::in | std::ios::binary);
        assert(plaintextFile);
        plaintextFile.read(plaintext.data(), 16);
        plaintextFile.clear(); plaintextFile.seekg(0);

        // Get ciphertext
        ciphertextFile.open(ciphertextFilename, std::ios::in | std::ios::binary);
        assert(ciphertextFile);
        ciphertextFile.read(ciphertext.data(), 16);
        ciphertextFile.clear(); ciphertextFile.seekg(0);

        // Encrypt
        keyExpansion(key);
        encrypt(plaintextFile, outputFile);
        plaintextFile.clear(); plaintextFile.seekg(0);
        outputFile.clear(); outputFile.seekg(0);

        // Verify that output == ciphertext that we expected
        outputFile.read(output.data(), 16);
        for (int i = 0; i < 16; i++)
        {
            assert(output[i] == ciphertext[i]);
        }
        outputFile.clear(); outputFile.seekg(0);

        // Decrypt
        // Don't use padding for decryption, since the NIST test vectors are 16 bytes
        decrypt(ciphertextFile, outputFile, false);
        ciphertextFile.clear(); ciphertextFile.seekg(0);
        outputFile.clear(); outputFile.seekg(0);

        // Verify that output == plaintext that we expected
        outputFile.read(output.data(), 16);
        for (int i = 0; i < 16; i++)
        {
            assert(output[i] == plaintext[i]);
        }
        outputFile.clear(); outputFile.seekg(0);

        // This test is complete, so close the test files
        keyFile.close();
        plaintextFile.close();
        ciphertextFile.close();
    }
    // All of the tests are complete, so close and delete the output file
    outputFile.close();
    std::filesystem::remove(outputFilename);
}

void AES::testEndToEnd(const std::string& plaintextFilename)
{
    std::ifstream plaintextFile;
    plaintextFile.open(plaintextFilename, std::ios::in | std::ios::binary);
    assert(plaintextFile);

    std::string ciphertextFilename = "ciphertext.tmp";
    std::fstream ciphertextFile;
    ciphertextFile.open(ciphertextFilename, std::ios::out);
    ciphertextFile.close();
    ciphertextFile.open(ciphertextFilename, std::ios::in | std::ios::out | std::ios::binary);
    assert(ciphertextFile);

    std::string finalPlaintextFilename = "plaintext.tmp";
    std::fstream finalPlaintextFile;
    finalPlaintextFile.open(finalPlaintextFilename, std::ios::out);
    finalPlaintextFile.close();
    finalPlaintextFile.open(finalPlaintextFilename, std::ios::in | std::ios::out | std::ios::binary);
    assert(finalPlaintextFile);

    std::vector<uint8_t> key;
    getRandomKey(key, 16);
    keyExpansion(key);

    encrypt(plaintextFile, ciphertextFile);
    plaintextFile.clear(); plaintextFile.seekg(0);
    ciphertextFile.clear(); ciphertextFile.seekg(0);

    decrypt(ciphertextFile, finalPlaintextFile);
    ciphertextFile.clear(); ciphertextFile.seekg(0);
    finalPlaintextFile.clear(); finalPlaintextFile.seekg(0);

    // Compare plaintextFile and finalPlaintextFile
    for (;;)
    {
        char expected = plaintextFile.get();
        char actual = finalPlaintextFile.get();
        assert(expected == actual);

        if (plaintextFile.eof() || finalPlaintextFile.eof())
        {
            break;
        }
    }

    plaintextFile.close();
    ciphertextFile.close();
    finalPlaintextFile.close();
    std::filesystem::remove(ciphertextFilename);
    std::filesystem::remove(finalPlaintextFilename);
}

void AES::testMalformedCiphertext(const std::string& ciphertextFilename)
{
    std::string plaintextFilename = "plaintext.tmp";
    std::ofstream plaintextFile;
    plaintextFile.open(plaintextFilename, std::ios::out | std::ios::binary);
    assert(plaintextFile);

    std::ifstream ciphertextFile;
    ciphertextFile.open(ciphertextFilename, std::ios::in | std::ios::binary);
    assert(ciphertextFile);

    std::vector<uint8_t> key;
    getRandomKey(key, 16);
    keyExpansion(key);

    bool foundException = false;
    try
    {
        decrypt(ciphertextFile, plaintextFile);
    }
    catch(const std::invalid_argument& e)
    {
        foundException = true;
    }
    assert(foundException);
    
    plaintextFile.close();
    ciphertextFile.close();
    std::filesystem::remove(plaintextFilename);
}
