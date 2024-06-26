#pragma once

#include "../lexer/token.h"
#include <neonc.h>

namespace neonc {
    struct Pack {
        Pack(const std::string file_name, std::vector<Token> tokens) : file_name(file_name), tokens(tokens) {}

        const std::string file_name;
        Token get() const;
        Token get_next() const;
        Token get_previous() const;
        Token get_offset(const int64_t offset) const;
        bool is_at_end() const;
        Token next();

        uint32_t index = 0;
        const std::vector<Token> tokens;
    };
}
