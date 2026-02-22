#include <iostream>
#include <string>
#include <assert.h>
#include <random>
#include <string.h>
#include <fstream>
#include "huffman_encode.h"

using namespace std;

static void
_rand_test(size_t nLen, int k)
{
    std::random_device rng;
    string str;
    for (size_t i=0; i<nLen; ++i)
    {
        int ch = rng() % 26;
        str.push_back(ch + 'A');
    }
#if 0
    std::ofstream outFile;
    outFile.open("rand.txt");
    outFile.write(str.c_str(), str.size());
#endif

    while (k-- > 0)
    {
        auto info = huffman_encode::encode(str.c_str(), str.size());
        std::vector<char> outVec;
        bool b = huffman_encode::decode(&outVec, info);
        assert(b);
        assert(outVec.size() == str.size());
        assert(0 == ::strncmp(str.c_str(), outVec.data(), str.size()));
        huffman_encode::destory_info(info);
    }
}

static void
test_huffman()
{
    size_t from = 0;
    size_t to   = 500;
    int k = 1;
    for (size_t i=from; i<to; ++i)
    {
        _rand_test(i, k);
        cout << "done: " << i << "*" << k << endl;
    }
}

int main()
{
    cout << "Hello World!" << endl;
    {
        string str = "";
        auto info = huffman_encode::encode(str.c_str(), str.size());
        huffman_encode::destory_info(info);
    }

    {
        string str = "a";
        auto info = huffman_encode::encode(str.c_str(), str.size());
        huffman_encode::destory_info(info);
    }

    {
        string str = "aaa";
        auto info = huffman_encode::encode(str.c_str(), str.size());
        huffman_encode::destory_info(info);
    }

    {
        string str = "aabbbcccc";
        auto info = huffman_encode::encode(str.c_str(), str.size());
        huffman_encode::destory_info(info);
    }

    {
        std::string str;
        str.insert(str.size(), 5, 'a');
        str.insert(str.size(), 9, 'b');
        str.insert(str.size(), 12, 'c');
        str.insert(str.size(), 13, 'd');
        str.insert(str.size(), 16, 'e');
        str.insert(str.size(), 45, 'f');
        cout << str << endl;
        auto info = huffman_encode::encode(str.c_str(), str.size());
        huffman_encode::destory_info(info);
    }
#if 1
    {
        string str;
        for (int i=0; i<500; ++i)
        {
            str.push_back(i);
        }

        //assert(str.size() == 256);

        auto info = huffman_encode::encode(str.c_str(), str.size());
        huffman_encode::destory_info(info);
    }
#endif

    test_huffman();

    return 0;
}
