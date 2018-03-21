/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef HELPER_H
#define HELPER_H

#include <string>

namespace ns3 {

char *IntToStr(int num);

// can handle 0x 0b or normal digit
unsigned int StrToInt(const std::string str);

int HexcharToInt(char c);

std::string HexstrToBytes(const std::string str);

std::string HexstrToBytes(const std::string str, unsigned int bitWidth);

std::string UintToString(unsigned int num);

std::string Uint32ipToHex(unsigned int ip);
}
#endif /* HELPER_H */
