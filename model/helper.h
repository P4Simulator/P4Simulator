/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef HELPER_H
#define HELPER_H

#include <string>

char * int_to_str(int num);

unsigned int str_to_int(const std::string str);

int hexchar_to_int(char c);

std::string hexstr_to_bytes(const std::string str);

std::string hexstr_to_bytes(const std::string str, unsigned int bit_width);

#endif /* HELPER_H */