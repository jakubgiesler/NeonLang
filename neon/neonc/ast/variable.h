#pragma once

#include "node.h"
#include <neonc.h>
#include "../llvm/util/str_to_type.h"
#include "expression.h"
#include "ast.h"

namespace neonc {
    struct Variable : public Node {
        Variable(): Node(std::nullopt) {
            type = "i32"; // TODO: REMOVE
            declare = false;
        }

        Variable(
            const std::string identifier,
            const std::optional<std::string> type,
            const std::optional<Position> position
        ): identifier(identifier), type(type), Node(position) {}
       
        virtual NodeId id() const {
            return NodeId::Variable;
        }
        
        virtual void dump(const uint32_t indentation) const {
            std::cout << cli::indent(indentation) << (declare ? cli::colorize("let ", indentation) : "_") << (declare ? identifier : "");
            std::cout << ": " << (type.has_value() ? type.value() : ColorRed + std::string("unknown") + ColorReset);

            if (!nodes.empty()) {
                std::cout << " = ";

                for (auto & n : nodes)
                    n->dump(indentation);
            }

            std::cout << std::endl;
        }

        void * build(Module & module) {
            auto _type = str_to_type(module, type.value());
            
            if (declare) {
                auto alloca = module.get_builder()->CreateAlloca(_type);

                module.local_variables[identifier] = alloca;

                if (!nodes.empty()) {
                    auto expr = std::dynamic_pointer_cast<Expression>(nodes.back());
        
                    if (expr) {
                        auto built = expr->build(module, _type);

                        if (built == nullptr)
                            throw std::invalid_argument("ICE: expression returned nullptr");

                        module.get_builder()->CreateStore(built, alloca);
                    }
                } else {
                    // bool
                    if (_type == llvm::Type::getInt1Ty(*module.context)) module.get_builder()->CreateStore(module.get_builder()->getInt1(0), alloca);
                    // integer types
                    else if (_type == llvm::Type::getInt8Ty(*module.context)) module.get_builder()->CreateStore(module.get_builder()->getInt8(0), alloca);
                    else if (_type == llvm::Type::getInt16Ty(*module.context)) module.get_builder()->CreateStore(module.get_builder()->getInt16(0), alloca);
                    else if (_type == llvm::Type::getInt32Ty(*module.context)) module.get_builder()->CreateStore(module.get_builder()->getInt32(0), alloca);
                    else if (_type == llvm::Type::getInt64Ty(*module.context)) module.get_builder()->CreateStore(module.get_builder()->getInt64(0), alloca);
                    // floating point types
                    else if (_type == llvm::Type::getFloatTy(*module.context))
                        module.get_builder()->CreateStore(llvm::ConstantFP::get(llvm::Type::getFloatTy(*module.context), 0), alloca);
                    else if (_type == llvm::Type::getDoubleTy(*module.context))
                        module.get_builder()->CreateStore(llvm::ConstantFP::get(llvm::Type::getDoubleTy(*module.context), 0), alloca);
                    //
                    else throw std::invalid_argument("ICE: unknown variable type to zero");
                }
            } else {
                if (!nodes.empty()) {
                    auto expr = std::dynamic_pointer_cast<Expression>(nodes.back());

                    if (expr) {
                        expr->build(module, _type);
                    }
                }
            }

            return nullptr;
        }

        const std::string identifier;
        std::optional<std::string> type;
    private:
        bool declare = true;
    };
}
