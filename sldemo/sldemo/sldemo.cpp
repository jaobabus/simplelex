
#include <iostream>
#include <string>

#include <simplelex.hpp>
#include <slinplace.hpp>



void print_hex(const void* data, size_t size) {
    for (int i = 0; i < size; i++) {
        if (i % 16 == 0) {
            if (i < 16)
                printf("\033[38;5;2m  %02X\033[38;5;250m: ", i);
            else
                printf("\033[38;5;2m%+4X\033[38;5;250m: ", i);
        }
        printf("%02X ", ((uint8_t*)data)[i]);
        if ((i + 1) % 16 == 0 || i == size - 1) {
            printf("\033[0m\n");
        }
    }
}

int main()
{
    char buffer[128];
    while (true) {
        std::string line;
        getline(std::cin, line);
        std::memcpy(buffer, line.c_str(), line.size() + 1);
        auto res = simplelexer::parse_repack(buffer, sizeof(buffer), buffer, line.size() + 1);
        if (res.error != simplelexer::Error::NoError) {
            std::cout << "Error: " << str(res.error) << std::endl;
        }
        else {
            std::cout << "Line size: " << line.size() << "(" << line.size() + 1 << ")" << std::endl;
            std::cout << "Pack size: " << res.accessor.buffer_size() << std::endl;
            print_hex(res.accessor.buffer(), res.accessor.buffer_size() + 1);
            for (size_t i = 0; i < res.accessor.size(); i++) {
                auto token = res.accessor[i];
                std::cout << str(token.type) << ": " << std::string(token.data, token.data + token.size) << std::endl;
            }
        }
    }

}

