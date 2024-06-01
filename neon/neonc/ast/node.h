#pragma once

#include "../types/position.h"
#include <neonc.h>
#include "../util/clicolor.h"

#define DEF_INDENT_MUL 4

namespace neonc {
    struct Node {
        Node(const std::optional<Position> position): position(position) {}
        virtual ~Node() = default;

        virtual void dump(const uint32_t indent) const = 0;

        template<typename T, typename ... Args>
        std::shared_ptr<T> add_node(Args ... args) {
            nodes.push_back(std::make_shared<T>(args...));
            
            return std::dynamic_pointer_cast<T>(nodes.back());
        }

        void add_node(std::shared_ptr<Node> node) {
            nodes.push_back(node);
        }

        std::vector<std::shared_ptr<Node>> nodes;

        std::optional<Position> position;
    };

    namespace cli {
        constexpr const std::string colorize(const std::string input, const uint32_t indentation) {
            constexpr uint8_t len = 5;

            constexpr char * array[len] = {
                (char *)ColorCyan,
                (char *)ColorYellow,
                (char *)ColorBlue,
                (char *)ColorGreen,
                (char *)ColorMagenta,
            };

            return array[indentation % len] + input + ColorReset;
        }

        constexpr const std::string indent(const uint32_t indentation) {
            return std::string(indentation * DEF_INDENT_MUL, ' ');
        }
    }
}
