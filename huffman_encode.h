#ifndef HUFFMAN_ENCODE_H
#define HUFFMAN_ENCODE_H

#include <vector>

namespace huffman_encode
{

struct huffman_encode_info;

const huffman_encode_info *
encode(const char *pData, size_t nLen);

bool
decode_to(char *buff, const huffman_encode_info* info);

bool
decode_to_vec(std::vector<char> *outVec, const huffman_encode_info* info);

size_t
get_orig_len(const huffman_encode_info* info);

void
destory_info(const huffman_encode_info *info);

}

#endif // HUFFMAN_ENCODE_H
