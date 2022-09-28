# Crypto

Let's implement some cryptographic algorithms for fun!  **This project is just for fun. Obviously don't use any of this code in any security-critical application!**

For now, only AES-128 in ECB mode is implemented.

## Compiling
The supplied makefile is for the Windows NMAKE utility. Running `nmake` will produce aes.exe.

## Running
Run aes.exe from the command line, with the first argument being the file to encrypt or decrypt:
```
aes.exe [plaintext/ciphertext file] [options]
```
Other required arguments include:

* `-k [key file]`
    Provides the file containing AES key. For now, only AES-128 is supported, so the file must contain exactly 16 bytes.
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

## Roadmap
As my leisure time permits, here are my future tasks for this project:
- Implement AES-192 and AES-256
- Implement AES modes other than the ECB mode, which, while simple, should never be used. Start with CBC mode.
- Derive AES keys from passwords rather than using a key read from a file.
