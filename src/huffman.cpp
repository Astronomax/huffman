#include "huffman.h"

huffman_archiver::huffman_stat huffman_archiver::compress(std::ifstream &from, std::ofstream &to) {
    auto frequency = calc_frequency(from);
    node* tree = build_tree(frequency);
    auto codes = codes_builder::build(tree);
    auto extra_bits = calc_extra_bits(from, codes);
    unsigned long long in_size = 0, out_size = 0, table_size = 1025;
    write_extra_info(to, {frequency, extra_bits});

    uint8_t current_byte = 0;
    std::size_t pos_in_byte = 0;

    from.clear();
    from.seekg(0);
    uint8_t read_byte;
    from.read((char *) &read_byte, 1);

    while (!from.eof()) {
        for (std::size_t i = 0; i < codes[read_byte].size(); i++) {
            if (pos_in_byte == BITS_IN_BYTE) {
                to.write((char *) &current_byte, 1);
                current_byte = 0;
                pos_in_byte = 0;
                ++out_size;
            }
            if (codes[read_byte].get_bit(i))
                current_byte |= (1u << pos_in_byte);
            ++pos_in_byte;
        }
        from.read((char *) &read_byte, 1);
        ++in_size;
    }
    if (pos_in_byte) {
        to.write((char *) &current_byte, 1);
        ++out_size;
    }
    delete tree;
    return {in_size, out_size, table_size};
}

huffman_archiver::huffman_stat huffman_archiver::decompress(std::ifstream &from, std::ofstream &to) {
    extra_info info = read_extra_info(from);
    node *tree = build_tree(info.frequency), *cur_pos = tree;
    auto codes = codes_builder::build(tree);
    unsigned long long in_size = 0, out_size = 0, table_size = 1025;

    uint8_t read_byte;
    from.read((char *) &read_byte, 1);

    while (!from.eof()) {
        uint8_t next_read_byte;
        from.read((char *) &next_read_byte, 1);

	std::size_t bits_to_read = BITS_IN_BYTE - (from.eof() ? info.extra_bits : 0);
        for (std::size_t i = 0; i < bits_to_read; i++) {
            if (cur_pos->is_leaf()) {
                uint8_t s = cur_pos->get_c();
                to.write((char *) &s, 1);
                cur_pos = tree;
                ++out_size;
            }
            if (read_byte & (1u << i))
                cur_pos = cur_pos->get_child(1);
            else cur_pos = cur_pos->get_child(0);
        }
        read_byte = next_read_byte;
        ++in_size;
    }

    if (cur_pos->is_leaf()) {
        uint8_t s = cur_pos->get_c();
        to.write((char *) &s, 1);
        ++out_size;
    }
    delete tree;
    return {in_size, out_size, table_size};
}

huffman_archiver::code::code(std::bitset<CHARS> c, std::size_t s) noexcept : _size(s), _code(c) {}

std::size_t huffman_archiver::code::size() const {
    return _size;
}

bool huffman_archiver::code::get_bit(std::size_t bit) const {
    return _code[bit];
}

huffman_archiver::node::~node() {
    if(!is_leaf()){
        delete get_child(0);
        delete get_child(1);
    }
}

huffman_archiver::node::node(uint8_t c) : c(c) {}

bool huffman_archiver::node::is_leaf() const {
    return (children[0] == nullptr && children[1] == nullptr);
}

uint8_t huffman_archiver::node::get_c() const {
    return c;
}

huffman_archiver::node* huffman_archiver::node::get_child(std::size_t to) const {
    return children[to];
}

void huffman_archiver::node::set_child(std::size_t to, node *v) {
    children[to] = v;
}

std::vector<huffman_archiver::code> huffman_archiver::codes_builder::build(node *tree) {
    _build(tree);
    return std::vector<code>(codes, codes + CHARS);
}

void huffman_archiver::codes_builder::_build(node *tree) {
    if (tree->is_leaf())
        codes[tree->get_c()] = code(pref, len);
    else {
        ++len;
        pref[len - 1] = false;
        _build(tree->get_child(0));
        pref[len - 1] = true;
        _build(tree->get_child(1));
        --len;
    }
}

huffman_archiver::node* huffman_archiver::build_tree(const std::vector<float> &frequency) {
    struct comp {
        bool operator()(std::pair<float, node *> a, std::pair<float, node *> b) const {
            return std::make_pair(a.first, a.second->get_c()) < std::make_pair(b.first, b.second->get_c());
        }
    };
    std::set<std::pair<float, node *>, comp> order;
    for (std::size_t i = 0; i < CHARS; i++)
        order.insert({frequency[i], new node(i)});

    while (order.size() >= 2) {
        auto a = order.begin(), b = ++order.begin();
        if (a->second->get_c() > b->second->get_c())
            std::swap(a, b);
        node *v = new node(a->second->get_c());
        v->set_child(0, a->second);
        v->set_child(1, b->second);        
        std::pair<float, node*> parent = {a->first + b->first, v};
        order.erase(a);
        order.erase(b);
        order.insert(parent);
    }
    return order.begin()->second;
}

std::vector<float> huffman_archiver::calc_frequency(std::ifstream &file) {
    file.clear();
    file.seekg(0);
    long long total = 0;
    std::vector<long long> counter(CHARS);

    uint8_t c;
    file.read((char *) &c, 1);
    while (!file.eof()) {
        ++counter[c];
        ++total;
        file.read((char *) &c, 1);
    }

    std::vector<float> frequency(CHARS);
    for (std::size_t i = 0; i < CHARS; i++)
        frequency[i] = (float) counter[i] / (float) total;
    return frequency;
}

uint8_t huffman_archiver::calc_extra_bits(std::ifstream &file, const std::vector<code> &codes)  {
    file.clear();
    file.seekg(0);
    uint8_t c, res = 0;
    file.read((char *) &c, 1);
    while (!file.eof()) {
        res = (res + codes[c].size() % BITS_IN_BYTE) % BITS_IN_BYTE;
        file.read((char *) &c, 1);
    }
    return (BITS_IN_BYTE - res) % BITS_IN_BYTE;
}

huffman_archiver::extra_info huffman_archiver::read_extra_info(std::ifstream &file) {
    file.clear();
    file.seekg(0);
    std::vector<float> frequency(CHARS);
    std::for_each(frequency.begin(), frequency.end(), [&file](float &f) { file.read((char *) &f, sizeof(float)); });
    uint8_t extra_bits;
    file.read((char *) &extra_bits, 1);
    return {frequency, extra_bits};
}

void huffman_archiver::write_extra_info(std::ofstream &file, const extra_info &info) {
    std::for_each(info.frequency.begin(), info.frequency.end(), [&file](float f) { file.write((char *) &f, sizeof(float)); });
    file.write((char *) &info.extra_bits, 1);
}

std::size_t huffman_archiver::codes_builder::len = 0;
std::bitset<huffman_archiver::CHARS> huffman_archiver::codes_builder::pref;
huffman_archiver::code huffman_archiver::codes_builder::codes[huffman_archiver::CHARS];

