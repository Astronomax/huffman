#ifndef HUFFMAN_TEST_H_
#define HUFFMAN_TEST_H_

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
#include <numeric>
#include <filesystem>
#include "doctest.h"

class huffman_archiver {
public:
    struct huffman_stat {
        const unsigned long long in_size, out_size, table_size;
    };
public:
    static huffman_stat compress(std::ifstream &from, std::ofstream &to);
    static huffman_stat decompress(std::ifstream &from, std::ofstream &to);

private:
    static const int BITS_IN_BYTE = 8;
    static const int CHARS = 256;
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
    
#ifdef HUFFMAN_TESTING
    static void compare_files(std::ifstream &a, std::ifstream &b) {
        while(!a.eof() && !b.eof()) {
            char c1, c2;
            a.read(&c1, 1);
            b.read(&c2, 1);
            CHECK_EQ(c1, c2);
        }
        CHECK((a.eof() && b.eof()));
    }

    TEST_CASE_CLASS("test_build_tree") {
        std::vector<long long> counter(CHARS);
        counter[0] = 15;
        counter[1] = 7;
        counter[2] = 6;
        counter[3] = 6;
        counter[4] = 5;
        std::vector<float> frequency(CHARS);
        long long total = accumulate(counter.begin(), counter.end(), 0LL);
        for (std::size_t i = 0; i < CHARS; i++)
            frequency[i] = (float) counter[i] / (float) total;
        node* tree = build_tree(frequency);
        CHECK(!tree->is_leaf());
        CHECK_EQ(tree->get_c(), 0);
        CHECK(tree->get_child(0)->is_leaf());
        CHECK_EQ(tree->get_child(0)->get_c(), 0);
        CHECK(!tree->get_child(1)->is_leaf());
        CHECK_EQ(tree->get_child(1)->get_c(), 1);
        CHECK(!tree->get_child(1)->get_child(0)->is_leaf());
        CHECK_EQ(tree->get_child(1)->get_child(0)->get_c(), 1);
        CHECK(!tree->get_child(1)->get_child(1)->is_leaf());
        CHECK_EQ(tree->get_child(1)->get_child(1)->get_c(), 2);
        CHECK(tree->get_child(1)->get_child(0)->get_child(0)->is_leaf());
        CHECK_EQ(tree->get_child(1)->get_child(0)->get_child(0)->get_c(), 1);
        CHECK(tree->get_child(1)->get_child(0)->get_child(1)->is_leaf());
        CHECK_EQ(tree->get_child(1)->get_child(0)->get_child(1)->get_c(), 3);
        CHECK(tree->get_child(1)->get_child(1)->get_child(0)->is_leaf());
        CHECK_EQ(tree->get_child(1)->get_child(1)->get_child(0)->get_c(), 2);
        CHECK(!tree->get_child(1)->get_child(1)->get_child(1)->is_leaf());
        CHECK_EQ(tree->get_child(1)->get_child(1)->get_child(1)->get_c(), 4);
        delete tree;
    }

    TEST_CASE_CLASS("test_calc_extra_bits") {
        std::vector<std::string> test_files = {"test1.bmp", "test2.txt", "test3.bmp"},
        answer_files = {"ans1.bin", "ans2.bin", "ans3.bin"},
        code_files = {"code1.bin", "code2.bin", "code3.bin"};

        for (int t = 0; t < 3; t++) {
            std::vector<code> codes(CHARS);
            std::ifstream f;
            f.open("./calc_extra_bits_tests/codes/" + code_files[t], std::ios_base::binary);
            for (int j = 0; j < CHARS; j++) {
                uint8_t len;
                f.read((char *) &len, 1);
                std::size_t num_of_bytes = (len + BITS_IN_BYTE - 1) / BITS_IN_BYTE;
                auto *code_array = new uint8_t[num_of_bytes];
                f.read((char *) code_array, num_of_bytes);
                std::bitset<CHARS> cur_code;
                for (std::size_t q = 0; q < len; q++)
                    cur_code[q] = (code_array[q / BITS_IN_BYTE] & (1u << (q % BITS_IN_BYTE)));
                codes[j] = code(cur_code, len);
                delete[] code_array;
            }
            f.close();
            f.open("./calc_extra_bits_tests/pool/" + test_files[t], std::ios_base::binary);
            uint8_t res = calc_extra_bits(f, codes);
            f.close();

            f.open("./calc_extra_bits_tests/answers/" + answer_files[t], std::ios::binary);
            uint8_t ans;
            f.read((char*)&ans, 1);
            f.close();
            CHECK_EQ(res, ans);
        }
    }

    TEST_CASE_CLASS("test_calc_frequency") {
         std::vector<std::string> test_files = {"test1.bmp", "test2.txt", "test3.bmp"},
         answer_files = {"res1.bin", "res2.bin", "res3.bin"},
         result_files = {"res1.bin", "res2.bin", "res3.bin"};

         for (int t = 0; t < 3; t++) {
             std::ifstream f;
             std::ofstream o;
             f.open("./calc_frequency_tests/pool/" + test_files[t], std::ios_base::binary);
             o.open("./calc_frequency_tests/results/" + result_files[t], std::ios_base::binary);
             auto frequency = calc_frequency(f);
             std::for_each(frequency.begin(), frequency.end(), [&o](float f) { o.write((char *) &f, sizeof(float)); });
             f.close();
             o.close();

             std::ifstream a, b;
             a.open("./calc_frequency_tests/results/" + result_files[t], std::ios_base::binary);
             b.open("./calc_frequency_tests/answers/" + answer_files[t], std::ios_base::binary);
             compare_files(a, b);
             a.close();
             b.close();
         }
    }

    TEST_CASE_CLASS("test_compress_decompress") {
        std::vector<std::string> test_files = {"test1.bmp", "test2.txt", "test3.bmp"},
        compressed_files = {"res1.bin", "res2.bin", "res3.bin"},
        decompressed_files = {"res1.bmp", "res2.txt", "res3.bmp"};

        for (int t = 0; t < 3; t++) {
            std::ifstream f;
            std::ofstream o;
            f.open("./compress_decompress_tests/pool/" + test_files[t], std::ios_base::binary);
            o.open("./compress_decompress_tests/compressed/" + compressed_files[t], std::ios_base::binary);
            compress(f, o);
            f.close();
            o.close();

            f.open("./compress_decompress_tests/compressed/" + compressed_files[t], std::ios_base::binary);
            o.open("./compress_decompress_tests/decompressed/" + decompressed_files[t], std::ios_base::binary);
            decompress(f, o);
            f.close();
            o.close();

            std::ifstream a, b;
            a.open("./compress_decompress_tests/pool/" + test_files[t], std::ios_base::binary);
            b.open("./compress_decompress_tests/decompressed/" + decompressed_files[t], std::ios_base::binary);
            compare_files(a, b);
            a.close();
            b.close();
        }
    }

    TEST_CASE_CLASS("test_read_write_extra_info") {
        std::vector<std::string> test_files = {"test1.bin", "test2.bin", "test3.bin"},
        result_files = {"res1.bin", "res2.bin", "res3.bin"};

        for (int t = 0; t < 3; t++) {
            std::ifstream f;
            f.open("./read_write_extra_info_tests/pool/" + test_files[t], std::ios_base::binary);
            auto res = read_extra_info(f);
            std::ofstream o;
            o.open("./read_write_extra_info_tests/results/" + result_files[t], std::ios_base::binary);
            write_extra_info(o, res);
            f.close();
            o.close();

            std::ifstream a, b;
            a.open("./read_write_extra_info_tests/pool/" + test_files[t], std::ios_base::binary);
            b.open("./read_write_extra_info_tests/results/" + result_files[t], std::ios_base::binary);
            compare_files(a, b);
            a.close();
            b.close();
        }
    }
#endif
};

#endif
