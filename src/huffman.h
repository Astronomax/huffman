#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <iostream>
#include <fstream>
#include <utility>
#include <set>
#include <vector>
#include <string>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <algorithm>

class huffman_archiver {
public:
    struct huffman_stat {
        const unsigned long long in_size, out_size, table_size;
    };
public:
    static huffman_stat compress(std::ifstream &from, std::ofstream &to);
    static huffman_stat decompress(std::ifstream &from, std::ofstream &to);

private:
    static const std::size_t BITS_IN_BYTE = 8, CHARS = 256;
private:
    struct extra_info {
        std::vector<float> frequency;
        uint8_t extra_bits;
    };
    class code {
    public:
        code() = default;
        explicit code(std::bitset<CHARS> c, std::size_t s) noexcept;
        std::size_t size() const;
        bool get_bit(std::size_t bit) const;
        
    private:
        std::size_t _size = 0;
        std::bitset<CHARS> _code;
    };

    class node {
    public:
        ~node();
        explicit node(uint8_t c);
        bool is_leaf() const;
        uint8_t get_c() const;
        node *get_child(std::size_t to) const;
        void set_child(std::size_t to, node *v);

    private:
        uint8_t c = 0;
        node *children[2] = {nullptr, nullptr};
    };

    class codes_builder {
    public:
        static std::vector<code> build(node *tree);

    private:
        static void _build(node *tree);

    private:
        static std::size_t len;
        static std::bitset<CHARS> pref;
        static code codes[CHARS];
    };

private:
    static node *build_tree(const std::vector<float> &frequency);
    static std::vector<float> calc_frequency(std::ifstream &file);
    static uint8_t calc_extra_bits(std::ifstream &file, const std::vector<code> &codes);
    static extra_info read_extra_info(std::ifstream &file);
    static void write_extra_info(std::ofstream &file, const extra_info &info);
};

#endif
