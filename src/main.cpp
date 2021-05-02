#include <iostream>
#include <vector>
#include <string>
#include "huffman.h"

class arguments_exception : public std::exception {
public:
    explicit arguments_exception(std::string what) : what_(what) {}
    const char *what() const noexcept override {
        return what_.c_str();
    }
private:
    std::string what_;
};

int main(int argc, char* argv[]) {
    std::ifstream input_file;
    std::ofstream output_file;
    try {
        std::vector<std::string> arguments;
        for (int i = 1; i < argc; i++)
            arguments.emplace_back(argv[i]);

        std::string mode, input_file_name, output_file_name;
        for (std::size_t i = 0; i < arguments.size(); i++) {
            if (arguments[i] == "-c" || arguments[i] == "-u")
                mode = arguments[i];
            else if (arguments[i] == "-f" || arguments[i] == "--file") {
                if (i + 1 == arguments.size())
                    throw arguments_exception("Invalid arguments");
                input_file_name = arguments[++i];
            } else if (arguments[i] == "-o" || arguments[i] == "--output") {
                if (i + 1 == arguments.size())
                    throw arguments_exception("Invalid arguments");
                output_file_name = arguments[++i];
            } else throw arguments_exception("Invalid arguments");
        }
        input_file.open(input_file_name, std::ios_base::binary);
        output_file.open(output_file_name, std::ios_base::binary);
        if(!input_file || !output_file)
            throw arguments_exception("File opening error");

        if (mode == "-c") {
            auto res = huffman_archiver::compress(input_file, output_file);
            std::cout << res.in_size << std::endl << res.out_size << std::endl << res.table_size << std::endl;
        } else if (mode == "-u") {
            auto res = huffman_archiver::decompress(input_file, output_file);
            std::cout << res.in_size << std::endl << res.out_size << std::endl << res.table_size << std::endl;
        }
        else throw arguments_exception("Invalid arguments");
    }
    catch(arguments_exception &e) {
        std::cout << e.what();
    }
    input_file.close();
    output_file.close();
    return 0;
}
