
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

void otTestHexToVector(std::string &aHex, std::vector<uint8_t> &aOutBytes)
{
    std::istringstream ss( aHex );
    std::string word;

    while( ss >> word )
    {
        uint8_t n = strtol(word.data(), NULL, 16);
	aOutBytes.push_back( n );
    }
}

void otTestPrintHex(uint8_t *aBuffer, int aLength)
{
    int i;
    for (i = 0; i < aLength; i++) {
        printf("%02x ", aBuffer[i]);
	if (i % 16 == 7) printf(" ");
	if (i % 16 == 15) printf("\n");
    }
    printf("\n");
}

void otTestPrintHex(std::string &aString)
{
    otTestPrintHex((uint8_t*)aString.data(), aString.size());
}

void otTestPrintHex(std::vector<uint8_t> &aBytes)
{
    otTestPrintHex((uint8_t*)&aBytes[0], aBytes.size());
}
        

