#include "aes.h"
#include "argparse.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cerrno>
#include <fstream>
#include <filesystem>
#include <string>

using argparse::Argument;
using argparse::ArgumentParser;
using std::string;

int main(int argc, char** argv)
{
    ArgumentParser ap;

    Argument inArg("input");
    inArg.help = "The file to encrypt/decrypt";
    ap.addArgument(inArg);
    string input;

    Argument outArg("output");
    outArg.help = "Indicates where the output of the encrypt/decrypt operation should be written. Will not overwrite existing file unless the -f option is used.";
    ap.addArgument(outArg);
    string output;

    Argument keyArg("--key");
    keyArg.shortName = "-k";
    keyArg.required = true;
    keyArg.help = "The file containing AES key. The file must contain exactly 16, 24, or 32 bytes.";
    keyArg.metavar = "KeyFilepath";
    ap.addArgument(keyArg);
    string key;

    Argument encryptArg("--encrypt");
    encryptArg.shortName = "-e";
    encryptArg.nargs = 0;
    encryptArg.help = "Encrypt the input file. (Mutually exclusive with --decrypt.)";
    ap.addArgument(encryptArg);
    bool encrypt;

    Argument decryptArg("--decrypt");
    decryptArg.shortName = "-d";
    decryptArg.nargs = 0;
    decryptArg.help = "Decrypt the input file. (Mutually exclusive with --encrypt.)";
    ap.addArgument(decryptArg);
    bool decrypt;

    Argument modeArg("--mode");
    modeArg.shortName = "-m";
    modeArg.help = "The mode of operation to use for AES encryption. Valid modes are cbc and ecb, with the default being cbc. The mode is specified in the header of an encrypted file, so this option is ignored when -d is specified.";
    modeArg.defaultValue = "cbc";
    ap.addArgument(modeArg);
    string mode;

    Argument forceArg("--force");
    forceArg.shortName = "-f";
    forceArg.help = "Overwrites output file if it already exists.";
    forceArg.nargs = 0;
    ap.addArgument(forceArg);
    bool force;

    Argument verboseArg("--verbose");
    verboseArg.shortName = "-v";
    verboseArg.help = "Writes more about the status of encryption/decryption to cout.";
    verboseArg.nargs = 0;
    ap.addArgument(verboseArg);
    bool verbose;

    Argument testArg("--test");
    testArg.shortName = "-t";
    testArg.help = "Instead of encrypting/decrypting a file, run tests to verify that AES is working correctly.";
    testArg.nargs = 0;
    ap.addArgument(testArg);
    bool test;

    try
    {
        ap.parse(argc, argv);
        input = ap.get<string>("input");
        output = ap.get<string>("output");
        key = ap.get<string>("--key");
        encrypt = ap.get<bool>("--encrypt");
        decrypt = ap.get<bool>("--decrypt");
        if ((!encrypt && !decrypt) || (encrypt && decrypt))
        {
            throw std::invalid_argument("Specify exactly 1 of --encrypt and --decrypt.");
        }
        mode = ap.get<string>("--mode");
        if (mode != "ecb" && mode != "cbc")
        {
            throw std::invalid_argument("--mode must be cbc or ecb");
        }
        force = ap.get<bool>("--force");
        verbose = ap.get<bool>("--verbose");
        test = ap.get<bool>("--test");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    // Check for whether we're testing
    if (test)
    {
        AES::test();
        return 0;
    }

    AES::Mode modeOpt;
    if (mode == "ecb")
    {
        modeOpt = AES::Mode::ECB;
    }
    else if (mode == "cbc")
    {
        modeOpt = AES::Mode::CBC;
    }

    // Open keyfile and load key
    std::ifstream keyfile(key, std::ios::in | std::ios::binary);
    if (!keyfile)
    {
        std::cerr << "Error: Failed to open key file: " << key << "\n";
        return EBADF;
    }
    std::vector<uint8_t> keyValue;
    keyValue.resize(32); // max keysize is 32 bytes, AES-256
    keyfile.read(reinterpret_cast<char*>(keyValue.data()), 32);
    keyValue.resize(keyfile.gcount());
    if (keyfile.gcount() < 16)
    {
        std::cerr << "Error: Failed to read 16-byte AES key from key file: " << key << "\n";
        return EBADF;
    }
    if (keyfile.peek() != EOF)
    {
        std::cerr << "Error: Key is larger than 32 bytes. The AES maximum keysize is 32 bytes.\n";
        return EINVAL;
    }

    // Open plaintext and ciphertext files
    std::ifstream inputFile(input, std::ios::in | std::ios::binary);
    if (!inputFile)
    {
        std::cerr << "Error: Failed to open input file: " << input << "\n";
        return EBADF;
    }

    if (std::filesystem::exists(output) && !force)
    {
        std::cerr << "Error: Force option (-f) isn't used, and output file already exists: " << output << "\n";
        return EINVAL;
    }
    std::ofstream outputFile(output, std::ios::out | std::ios::binary);
    if (!outputFile)
    {
        std::cerr << "Error: Failed to open output file: " << output << "\n";
        return EBADF;
    }

    // Call encrypt/decrypt
    try
    {
        AES aes(keyValue);
        if (encrypt)
        {
            if (verbose)
            {
                std::cout << "Calling encrypt\n";
                std::cout << "Plaintext file: " << input << "\n";
                std::cout << "Ciphertext file: " << output << "\n";
                std::cout << "Key file: " << key << "\n";
                std::cout << "Mode: " << mode << " (" << static_cast<int>(modeOpt) << ")\n";
            }
            aes.encrypt(inputFile, outputFile, modeOpt);
        }
        else
        {
            if (verbose)
            {
                std::cout << "Calling decrypt\n";
                std::cout << "Ciphertext file: " << input << "\n";
                std::cout << "Plaintext file: " << output << "\n";
                std::cout << "Key file: " << key << "\n";
            }
            aes.decrypt(inputFile, outputFile);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return EINVAL;
    }
    return 0;
}
