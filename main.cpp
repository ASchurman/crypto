#include <iostream>
#include "aes.h"

int main()
{
    std::cout << "Testing AES...\n";
    AES aes;
    aes.test();
    std::cout << "Done testing AES!\n";
}