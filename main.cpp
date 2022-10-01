#include "aes.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cerrno>
#include <fstream>
#include <filesystem>

bool optionExists(const std::vector<std::string>& args, const std::string& option)
{
    return std::find(args.begin(), args.end(), option) != args.end();
}

bool optionValue(const std::vector<std::string>& args, const std::string& option, std::string& value)
{
    std::string tempValue;
    auto it = std::find(args.begin(), args.end(), option);
    if (it == args.end() || it == args.end()-1)
    {
        return false;
    }
    it++;
    tempValue = *it;
    if (tempValue.front() == '-')
    {
        // The next arg is another flag, not a value
        return false;
    }
    value = tempValue;
    return true;
}

int main(int argc, char** argv)
{
    std::vector<std::string> args(argv, argv+argc);

    // Check for whether we're testing
    if (optionExists(args, "-t"))
    {
        AES::test();
        return 0;
    }

    bool verbose = optionExists(args, "-v");

    // Get filenames
    if (args.size() == 1 || args[1].front() == '-')
    {
        std::cerr << "Error: Include file to encrypt/decrypt.\n";
        return EINVAL;
    }
    std::string inputFilename = args[1];

    std::string keyFilename;
    if (!optionValue(args, "-k", keyFilename))
    {
        std::cerr << "Error: Use -k flag to indicate file containing AES key.\n";
        return EINVAL;
    }
    
    std::string outputFilename = "aes-output.bin";
    optionValue(args, "-o", outputFilename);

    // Get mode: encrypt or decrypt
    bool eFlag = optionExists(args, "-e");
    bool dFlag = optionExists(args, "-d");
    if (eFlag && dFlag)
    {
        std::cerr << "Error: Specify either encrypt mode or decrypt mode, not both.\n";
        return EINVAL;
    }
    if (!eFlag && !dFlag)
    {
        std::cerr << "Error: Specify either encrypt or decrypt mode with -e or -d flags.\n";
        return EINVAL;
    }

    // Open keyfile and load key
    std::ifstream keyfile(keyFilename, std::ios::in | std::ios::binary);
    if (!keyfile)
    {
        std::cerr << "Error: Failed to open key file: " << keyFilename << "\n";
        return EBADF;
    }
    std::vector<uint8_t> key;
    key.resize(32); // max keysize is 32 bytes, AES-256
    keyfile.read(reinterpret_cast<char*>(key.data()), 32);
    key.resize(keyfile.gcount());
    if (keyfile.gcount() < 16)
    {
        std::cerr << "Error: Failed to read 16-byte AES key from key file: " << keyFilename << "\n";
        return EBADF;
    }
    if (keyfile.peek() != EOF)
    {
        std::cerr << "Error: Key is larger than 32 bytes. The AES maximum keysize is 32 bytes.\n";
        return EINVAL;
    }

    // Open plaintext and ciphertext files
    std::ifstream inputFile(inputFilename, std::ios::in | std::ios::binary);
    if (!inputFile)
    {
        std::cerr << "Error: Failed to open input file: " << inputFilename << "\n";
        return EBADF;
    }

    if (std::filesystem::exists(outputFilename) && !optionExists(args, "-f"))
    {
        std::cerr << "Error: Force option (-f) isn't used, and output file already exists: " << outputFilename << "\n";
        return EINVAL;
    }
    std::ofstream outputFile(outputFilename, std::ios::out | std::ios::binary);
    if (!outputFile)
    {
        std::cerr << "Error: Failed to open output file: " << outputFilename << "\n";
        return EBADF;
    }

    // Call encrypt/decrypt
    try
    {
        AES aes(key);
        if (eFlag)
        {
            if (verbose)
            {
                std::cout << "Calling encrypt\n";
                std::cout << "Plaintext file: " << inputFilename << "\n";
                std::cout << "Ciphertext file: " << outputFilename << "\n";
                std::cout << "Key file: " << keyFilename << "\n";
            }
            aes.encrypt(inputFile, outputFile);
        }
        else
        {
            if (verbose)
            {
                std::cout << "Calling decrypt\n";
                std::cout << "Ciphertext file: " << inputFilename << "\n";
                std::cout << "Plaintext file: " << outputFilename << "\n";
                std::cout << "Key file: " << keyFilename << "\n";
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
