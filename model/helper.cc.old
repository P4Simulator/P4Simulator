#include "helper.h"
#include <iostream>
#include <math.h>

char * int_to_str(int num)
{
    char *ss=new char[10];
    int pos = 0;

    if (num == 0)
        ss[0] = '0';
    while (num)
    {
        ss[pos++] = num % 10 + '0';
        num = num / 10;
    }
    char temp;
    for (int i = 0; i < pos / 2; i++)
    {
        temp = ss[i];
        ss[i] = ss[pos - 1 - i];
        ss[pos - 1 - i] = temp;
    }
    ss[pos] = '\0';
    return ss;
}

unsigned int str_to_int(const std::string str)
{

    unsigned int res = 0;
    if (str.find("0x") < str.size())//16
    {
        for (size_t i = 2; i < str.size(); i++)
        {
            if (str[i] >= '0'&&str[i] <= '9')
            {
                res = res * 16 + str[i] - '0';
            }
            else
            {
                if (str[i] >= 'a'&&str[i] <= 'f')
                {
                    res = res * 16 + str[i] - 'a'+10;
                }
                else
                {
                    if(str[i]>='A'&&str[i]<='F')
                        res=res*16+str[i]-'A'+10;
                    else
                    {
                        std::cout << "in P4Model::str_to_int, action data error!" << std::endl;
                    }
                }
            }
        }
    }
    else
    {
        if (str.find("0b") < str.size())//2
        {
            for (size_t i = 2; i < str.size(); i++)
            {
                if (str[i] >= '0'&&str[i] <= '1')
                {
                    res = res * 2 + str[i] - '0';
                }
                else
                {
                    std::cout << "in P4Model::str_to_int, action data error!" << std::endl;
                }
            }
        }
        else //10
        {
            for (size_t i = 0; i < str.size(); i++)
            {
                if (str[i] >= '0'&&str[i] <= '9')
                {
                    res = res * 10 + str[i] - '0';
                }
                else
                {
                    std::cout << "in P4Model::str_to_int, action data error!" << std::endl;
                }
            }
        }
    }
    return res;
}

int hexchar_to_int(char c)
{

    int temp=0;
    if (c >= '0'&&c <= '9')
        temp = c - '0';
    else
    {
        if (c >= 'a'&&c <= 'f')
            temp = c - 'a' + 10;
        else
        {
            if(c>='A'&&c<='F')
                temp=c-'A'+10;
            else
            {
                std::cout << "str_to_bytes error" << std::endl;
            }
        }
    }
    return temp;
}

std::string hexstr_to_bytes(const std::string str)
{

    std::string hex_str;
    if (str.find("0x") < str.size())
    {
        hex_str = str.substr(2);
    }
    else
    {
        hex_str = str;
    }
    std::string res;
    res.resize(hex_str.size() / 2);
    for (size_t i = 0,j=0; i < hex_str.size(); i += 2,j++)
    {
        res[j] = hexchar_to_int(hex_str[i]) * 16 + hexchar_to_int(hex_str[i + 1]);
    }
    return res;
}

std::string hexstr_to_bytes(const std::string str, unsigned int bit_width)
{

    std::string hex_str;
    if (str.find("0x") < str.size())
    {
        hex_str = str.substr(2);
    }
    else
    {
        hex_str = str;
    }
    std::string res;
    res.resize(ceil(double(bit_width) / 8));
    std::cout<<res.size()<<std::endl;
    for (size_t i = 0, j = 0, w = 0; i<hex_str.size(); i += 2, j++, w += 8)
    {
        if (w + 8<bit_width)
        {
            res[j] = hexchar_to_int(hex_str[i]) * 16 + hexchar_to_int(hex_str[i + 1]);
        }
        else
        {
            if (w + 4 >= bit_width)
            {
                std::string binary_str(4, '0');
                size_t k = 3;
                int value = hexchar_to_int(hex_str[i]);
                while (value)
                {
                    binary_str[k--] = value % 2 + '0';
                    value /= 2;
                }
                res[j] = 0;
                unsigned int left_len = bit_width - w;
                for (size_t t = 0; t<left_len; t++)
                {
                    res[j] = res[j] * 2 + binary_str[t] - '0';
                }
                for (size_t t = left_len; t < 4; t++)
                    res[j] *= 2;
                res[j] *= 16;
            }
            else
            {
                res[j] = hexchar_to_int(hex_str[i]) * 16;
                std::string binary_str(4, '0');
                size_t k = 3;
                int value = hexchar_to_int(hex_str[i+1]);
                while (value)
                {
                    binary_str[k--] = value % 2 + '0';
                    value /= 2;
                }
                int temp = 0;
                unsigned int left_len = bit_width - w - 4;
                for (size_t t = 0; t<left_len; t++)
                {
                    temp = temp * 2 + binary_str[t] - '0';
                }
                for (size_t t = left_len; t < 4; t++)
                    temp *= 2;
                res[j] += temp;
            }
            break;
        }
    }
    return res;
}
