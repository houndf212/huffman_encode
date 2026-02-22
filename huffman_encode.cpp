#include "huffman_encode.h"
#include <limits>
#include <string.h>
#include <queue>
#include <bit>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <assert.h>

namespace
{

using uchar = unsigned char;
static constexpr size_t g_charSize = 256;

struct stHuffNode
{
    const uchar  m_uchar;
    const size_t m_freq;
    const uint64_t m_bitRep;
    const size_t   m_bitNum;
    const stHuffNode * const m_left;
    const stHuffNode * const m_right;
private:
    stHuffNode(uchar uc, size_t freq, const stHuffNode *left, const stHuffNode *right)
        :m_uchar(uc)
        ,m_freq(freq)
        ,m_bitRep(0)
        ,m_bitNum(0)
        ,m_left(left)
        ,m_right(right)
    {}
public:

    void init_rep_bit(uint64_t rep, size_t num) const
    {
        assert(num >= 1);
        assert(0 == m_bitRep);
        assert(0 == m_bitNum);
        const_cast<uint64_t&>(m_bitRep) = rep;
        const_cast<uint64_t&>(m_bitNum) = num;
    }

    bool is_leaf() const
    {
        return nullptr == m_left && nullptr == m_right;
    }

    static const stHuffNode *
    create_node(uchar ch, size_t freq, const stHuffNode *left, const stHuffNode *right)
    {
        return new stHuffNode(ch, freq, left, right);
    }

    static void free_node_r(const stHuffNode *pNode)
    {
        if (nullptr != pNode)
        {
            free_node_r(pNode->m_left);
            free_node_r(pNode->m_right);
            delete pNode;
        }
    }
};

struct stHuffNodeCMP
{
    bool operator()(const stHuffNode *n1, const stHuffNode *n2) const noexcept
    {
        return n1->m_freq > n2->m_freq;
    }
};

struct stEncodeInfoImpl
{
    size_t m_origCharSize = 0;
    size_t m_encBitSize = 0;
    const stHuffNode *m_root = nullptr;
    size_t     m_encBuffSize = 0;
    uint64_t  *m_encBuff = nullptr;

};

static void
_set_rep_r(const stHuffNode *pNode, uint64_t bitRep, size_t nBitNum)
{
    if (nullptr == pNode)
    {
        return;
    }

    if (pNode->is_leaf())
    {
        assert(nBitNum >= 1);

        pNode->init_rep_bit(bitRep, nBitNum);
        return;
    }

    bitRep <<= 1;
    ++nBitNum;

    _set_rep_r(pNode->m_left,  bitRep,       nBitNum);
    _set_rep_r(pNode->m_right, bitRep | 0x1, nBitNum);
}

static void
_count_freq(size_t *arr, const uchar *pData, size_t nLen)
{
    const uchar *beg = pData;
    const uchar *end = beg + nLen;

    for (; beg != end; ++beg)
    {
        ++arr[*beg];
    }
}

static void
_count_all_enc_bit_r(size_t *nCount, const stHuffNode *pNode)
{
    if (nullptr == pNode)
    {
        return;
    }

    if (pNode->is_leaf())
    {
        *nCount += pNode->m_bitNum * pNode->m_freq;
        return;
    }

    _count_all_enc_bit_r(nCount, pNode->m_left);
    _count_all_enc_bit_r(nCount, pNode->m_right);
}

static const stHuffNode *
_build_huffman_tree(const uchar *pData, size_t nLen)
{
    assert(nLen >= 1);

    size_t freq_array[g_charSize] = {0};
    _count_freq(freq_array, pData, nLen);

    using MinHeap = std::priority_queue<const stHuffNode*, std::vector<const stHuffNode*>, stHuffNodeCMP>;
    MinHeap heap;
    for (size_t ch=0; ch<g_charSize; ++ch)
    {
        size_t freq = freq_array[ch];
        if (0 == freq)
        {
            continue;
        }

        auto pNode = stHuffNode::create_node(ch, freq, nullptr, nullptr);
        heap.push(pNode);
    }

    assert(heap.size() >= 1);

    constexpr uchar g_parentChar = 0;

    if (heap.size() == 1)
    {
        auto pNode = heap.top();
        heap.pop();
        auto pParent = stHuffNode::create_node(g_parentChar, pNode->m_freq, pNode, nullptr);
        heap.push(pParent);
    }
    else
    {
        while (heap.size() != 1)
        {
            assert(heap.size() >= 2);

            auto pLeft = heap.top();
            heap.pop();
            auto pRight = heap.top();
            heap.pop();

            size_t freq = pLeft->m_freq + pRight->m_freq;

            auto pParent = stHuffNode::create_node(g_parentChar, freq, pLeft, pRight);

            heap.push(pParent);
        }
    }

    assert(heap.size() == 1);

    auto pRoot = heap.top();

    assert(false == pRoot->is_leaf());

    _set_rep_r(pRoot, 0, 0);

    return pRoot;
}

static void
_tree_to_array_r(const stHuffNode **arr, const stHuffNode *pNode)
{
    if (nullptr == pNode)
    {
        return;
    }

    if (pNode->is_leaf())
    {
        assert(nullptr == arr[pNode->m_uchar]);

        arr[pNode->m_uchar] = pNode;
        return;
    }

    _tree_to_array_r(arr, pNode->m_left);
    _tree_to_array_r(arr, pNode->m_right);
}

struct stBitVector
{
    uint64_t *m_buff;
    size_t    m_leftBitCount;
    uint64_t  m_curBitFlag;

    void init_flag()
    {
        m_leftBitCount = 64;
        m_curBitFlag   = 0;
    }

    void push(uint64_t flag)
    {
        assert(0 == flag || 1 == flag);
        assert(m_leftBitCount > 0);

        --m_leftBitCount;

        m_curBitFlag |= flag << m_leftBitCount;

        if (0 == m_leftBitCount)
        {
            *m_buff++ = m_curBitFlag;
            init_flag();
        }
    }

    void finish_flag()
    {
        if (64 != m_leftBitCount)
        {
            *m_buff++ = m_curBitFlag;
            init_flag();
        }
    }
};

static void
_enc_to_vec(uint64_t *buff,
            const stHuffNode *pRoot,
            const uchar *pData,
            size_t nLen)
{
    const stHuffNode *nodeArr[g_charSize] = {nullptr};
    _tree_to_array_r(nodeArr, pRoot);

    stBitVector bitVec;
    bitVec.m_buff = buff;
    bitVec.init_flag();

    auto beg = pData;
    auto end = beg + nLen;
    for (; beg != end; ++beg)
    {
        auto pNode = nodeArr[*beg];

        assert(pNode);

        for (int flagNum=pNode->m_bitNum-1; flagNum>=0; --flagNum)
        {
            uint64_t flag = (pNode->m_bitRep >> flagNum) & 0x1;
            bitVec.push(flag);
        }
    }

    bitVec.finish_flag();
}

static bool
_dec_to_buff(char *outIter,
             const stEncodeInfoImpl *pImpl)
{
    char *oldOutIter = outIter;
    size_t nBitCount = pImpl->m_encBitSize;
    auto pNode = pImpl->m_root;

    const uint64_t *beg = pImpl->m_encBuff;
    const uint64_t *end = beg + pImpl->m_encBuffSize;
    for (; beg != end; ++beg)
    {
        uint64_t flag = *beg;
        for (int i=63; i>=0; --i)
        {
            if (0 == nBitCount)
            {
                return false;
            }

            --nBitCount;

            bool isOne = (flag >> i) & 0x1;
            if (isOne)
            {
                pNode = pNode->m_right;
            }
            else
            {
                pNode = pNode->m_left;
            }

            if (nullptr == pNode)
            {
                return false;
            }

            if (pNode->is_leaf())
            {
                *outIter++ = pNode->m_uchar;
                pNode = pImpl->m_root;

                if (0 == nBitCount)
                {
                    return outIter - oldOutIter == pImpl->m_origCharSize;
                }
            }
        }
    }

    return false;
}

#ifndef NDEBUG

static void
_print_Node(const stHuffNode *pNode)
{
    if (isprint(pNode->m_uchar))
    {
        printf("\'%c\' %3d : b = \'", pNode->m_uchar, pNode->m_uchar);
    }
    else
    {
        printf("\'%c\' %3d : b = \'", ' ', pNode->m_uchar);
    }

    uint64_t c = pNode->m_bitRep;
    if (0 == pNode->m_bitNum)
    {
        printf("0");
    }
    else
    {
        for (int i = pNode->m_bitNum - 1; i >= 0; --i)
        {
            bool b = c & (uint64_t(1) << i);
            int n = b ? 1 : 0;
            printf("%d", n);
        }
    }
    printf("\' n = %d f = %d\n", pNode->m_bitNum, pNode->m_freq);
}

static void
_print_tree(const stHuffNode *pNode)
{
    if (nullptr == pNode)
    {
        return;
    }

    if (pNode->is_leaf())
    {
        _print_Node(pNode);
        return;
    }
    _print_tree(pNode->m_left);
    _print_tree(pNode->m_right);
}
#endif

}

namespace huffman_encode
{

const huffman_encode_info *
encode(const char *pData, size_t nLen)
{
    if (0 == nLen)
    {
        return nullptr;
    }

    const uchar *pUChar = reinterpret_cast<const uchar *>(pData);
    auto pRoot = _build_huffman_tree(pUChar, nLen);

#ifndef NDEBUG
    _print_tree(pRoot);
#endif

    auto pImpl = new stEncodeInfoImpl;

    size_t nEncBitCount = 0;
    _count_all_enc_bit_r(&nEncBitCount, pRoot);

    size_t nEncBuffSize = (nEncBitCount + 63) / 64;

    uint64_t *pBuff = reinterpret_cast<uint64_t*>(::malloc(nEncBuffSize * sizeof (uint64_t)));

    _enc_to_vec(pBuff, pRoot, pUChar, nLen);

    pImpl->m_origCharSize = nLen;
    pImpl->m_encBitSize   = nEncBitCount;
    pImpl->m_root         = pRoot;
    pImpl->m_encBuff      = pBuff;
    pImpl->m_encBuffSize  = nEncBuffSize;

    huffman_encode_info *pInfo = reinterpret_cast<huffman_encode_info*>(pImpl);

#ifndef NDEBUG
    printf("orig = %d, enc = %d\n", nLen, (pImpl->m_encBitSize + 7) / 8);
    {
        std::vector<char> outVec;
        bool b = decode_to_vec(&outVec, pInfo);
        assert(b);
        assert(outVec.size() == nLen);
        assert(0 == strncmp(pData, outVec.data(), nLen));
    }
#endif

    return pInfo;
}

bool
decode_to(char *buff, const huffman_encode_info *info)
{
    if (nullptr == info)
    {
        return true;
    }

    auto pImpl = reinterpret_cast<const stEncodeInfoImpl*>(info);

    return _dec_to_buff(buff, pImpl);
}

bool
decode_to_vec(std::vector<char> *outVec, const huffman_encode_info *info)
{
    if (nullptr == info)
    {
        return true;
    }

    auto pImpl = reinterpret_cast<const stEncodeInfoImpl*>(info);

    //make enough space
    auto oldSize = outVec->size();
    outVec->resize(oldSize + pImpl->m_origCharSize);

    return _dec_to_buff(outVec->data() + oldSize, pImpl);
}

void
destory_info(const huffman_encode_info *info)
{
    if (nullptr == info)
    {
        return;
    }

    auto pImpl = reinterpret_cast<const stEncodeInfoImpl*>(info);

    assert(pImpl->m_root);
    assert(pImpl->m_encBuff);

    stHuffNode::free_node_r(pImpl->m_root);
    ::free(pImpl->m_encBuff);
    delete pImpl;
}

size_t
get_orig_len(const huffman_encode_info *info)
{
    if (nullptr == info)
    {
        return 0;
    }

    auto pImpl = reinterpret_cast<const stEncodeInfoImpl*>(info);
    return pImpl->m_origCharSize;
}

}
