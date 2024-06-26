#pragma once

#include "node.h"
#include <neonc.h>

namespace neonc {
    struct Boolean : public Node {
        Boolean(bool value, std::optional<Position> position): value(value), Node(position) {}

        virtual NodeId id() const {
            return NodeId::Boolean;
        }

        virtual void dump(const uint32_t indentation) const {
            (void)indentation;

            std::cout << (value ? "true" : "false");
        }
 
        void * build(Module & module) {
            return module.get_builder()->getInt1(value);
        }

        bool value;
    };
}
