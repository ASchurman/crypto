#pragma once
#include <iostream>
#include <cstdint>
#include <vector>
#include <array>
#include <string>

/* Alexander Schurman
 *
 * Implements the AES algorithm.
 * For now, only AES-128 in ECB mode is implemented.
 * This was done for fun. Obviously don't use any of this code in any security-critical application.
 * 
 * TODO
 * 1. More gracefully handle incorrectly padded ciphertext
 * 2. Test looking for incorrectly padded ciphertext
 * 3. Key derivation from password
 * 4. Tests for key derivation from password
 * 5. Write main to actually encrypt/decrypt files, not just run the tests
 * 6. Implement larger key sizes
 * 7. CBC mode, not just ECB
 */

class AES
{
public:
    AES(const std::vector<uint8_t>& key);
    AES() { }
    ~AES() { cleanup(); }

    void encrypt(std::istream& plaintext, std::ostream& ciphertext);

    // Decrypt ciphertext into plaintext. 'usePadding' indicates whether PKCS7 padding is used, or
    // whether no padding is used at all; usePadding=false is used for the sake of testing with
    // NIST test vectors, which are 16 bytes
    void decrypt(std::istream& ciphertext, std::ostream& plaintext, bool usePadding = true);

    // Run my collection of tests
    void test();
    
private:
    // A collection of 16-byte round keys derived from the AES key: one per round, plus one extra.
    // For 128-bit key, 10 rounds, 11 round keys;
    //     192-bit key, 12 rounds, 13 round keys;
    //     256-bit key, 14 rounds, 15 round keys.
    // Each round key's array represents a matrix. Indices 0-3 are column 1, 4-7 are column 2, etc.
    std::vector<std::array<uint8_t, 16>> m_roundKeys;

    // The 16-byte state
    std::array<uint8_t, 16> m_state;

    // Expands the key to m_roundKeys using the AES key schedule
    // For now, we only support AES-128, so key is expected to contain 4 32-bit words
    void keyExpansion(const std::vector<uint8_t>& key);

    // Each of these functions apply a step of the AES algorithm to m_state.
    void addRoundKey(int round);
    void subBytes();
    void invSubBytes();
    void shiftRows();
    void invShiftRows();
    void mixColumns();
    void invMixColumns();

    void cleanup();

    // Test Functions //
    void testSteps(); // tests that check individual steps of the algorithm
    void testEncryptDecrypt(); // tests that use NIST's test vectors
    void testEndToEnd(const std::string& plaintextFilename); // tests that encrypt and decrypt a file and check it's not changed

    static const std::array<uint8_t, 256> SBOX;
    static const std::array<uint8_t, 256> INV_SBOX;
    static const std::array<uint8_t, 256> GMUL2;
    static const std::array<uint8_t, 256> GMUL3;
    static const std::array<uint8_t, 256> GMUL9;
    static const std::array<uint8_t, 256> GMUL11;
    static const std::array<uint8_t, 256> GMUL13;
    static const std::array<uint8_t, 256> GMUL14;
    static const std::array<uint8_t,  10> RC;
};
