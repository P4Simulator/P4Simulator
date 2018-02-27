/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef HELPER_H
#define HELPER_H

#include <string>

namespace ns3 {

char *IntToStr(int num);

unsigned int StrToInt(const std::string str);

int HexcharToInt(char c);

std::string HexstrToBytes(const std::string str);

std::string HexstrToBytes(const std::string str, unsigned int bitWidth);

}
#endif /* HELPER_H */
