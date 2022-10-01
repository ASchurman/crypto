# Crypto

Let's implement some cryptographic algorithms for fun!  **This project is just for fun. Obviously don't use any of this code in any security-critical application!**

For now, only AES in ECB mode is implemented.

## Compiling
The supplied makefile is for the Windows NMAKE utility. Running `nmake` will produce aes.exe.

## Running
Run aes.exe from the command line, with the first argument being the file to encrypt or decrypt:
```
aes.exe [plaintext/ciphertext file] [options]
```
Other required arguments include:

* `-k [key file]`
    Provides the file containing AES key. The file must contain exactly 16, 24, or 32 bytes.
* `-e` or `-d`
    Indicates whether to encrypt (-e) or decrypt (-d) the input file.

Optional arguments include:

* `-o [output file]`
    Indicates where the output of the encrypt/decrypt operation should be written. Defaults to aes-output.bin. Will not overwrite existing file unless the -f option is used.
* `-f`
    Force. Overwrites output file if it already exists.
* `-v`
    Verbose. More about the status of the encryption/decryption will be written to cout.
* `-t`
    Test. Instead of encrypting/decrypting a file, run tests to verify that aes.exe is working correctly.

For example, to encrypt a file secrets.txt to a file encryptedSecrets.bin, using an AES key in the file key.bin, run:
```
aes.exe secrets.txt -e -k key.bin -o encryptedSecrets.bin
```

## Testing
Before running the tests with `aes.exe -t`, the test files must be generated in the testFiles folder. This folder contains vectors.txt, which contains AES test vectors [published by NIST](https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/aes/AESAVS.pdf). To expand these test vectors into files used by `aes.exe -t`, run `maketests.bat` in the testFiles folder.

## Roadmap
As my leisure time permits, here are my future tasks for this project:
- Implement AES modes other than the ECB mode, which, while simple, should never be used. Start with CBC mode.
- Derive AES keys from passwords rather than using a key read from a file.
