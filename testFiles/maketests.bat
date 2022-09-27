@echo off
setlocal ENABLEDELAYEDEXPANSION

:: Use this script for turning sets of test vectors in vectors.txt, formatted as
:: hex literals, into binary files for use in tests.
:: Each test vector in vectors.txt should be a different line.
:: Each line contains 3 things: the key, the plaintext, and the ciphertext; these
:: are each separated by a space.

:: This produces the following files:
::   keyN.test
::   plaintextN.test
::   ciphertextN.test
:: where N is an integer representing the test number.

set /a t=1

FOR /f "tokens=1,2,3" %%a in (vectors.txt) do (
    set kTemp=key!t!.temp
    set pTemp=plaintext!t!.temp
    set cTemp=ciphertext!t!.temp
    set k=key!t!.test
    set p=plaintext!t!.test
    set c=ciphertext!t!.test

    echo %%a > !kTemp!
    echo %%b > !pTemp!
    echo %%c > !cTemp!
    certutil /f /decodehex !kTemp! !k! >nul
    certutil /f /decodehex !pTemp! !p! >nul
    certutil /f /decodehex !cTemp! !c! >nul

    set /a t=t+1
)
del *.temp
endlocal
