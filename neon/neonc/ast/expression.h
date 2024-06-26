#pragma once

#include "node.h"
#include <neonc.h>
#include "operator.h"
#include "number.h"
#include "identifier.h"
#include "boolean.h"
#include "call.h"
#include "string.h"

namespace neonc {
    struct Expression : public Node {
        Expression(): Node(std::nullopt) {}

        virtual NodeId id() const {
            return NodeId::Expression;
        }

        virtual void dump(const uint32_t indentation) const {
            std::cout << ColorGreen << BoldFont << "( " << ColorReset;

            for (auto & n : nodes)
                n->dump(indentation);

            std::cout << ColorGreen << BoldFont << " )" << ColorReset;
        }

        llvm::Value * build(Module & module, llvm::Type * type) {
            if (type == nullptr) {
                std::cerr << "ICE: expression.h type is nullptr" << std::endl;
                exit(0);
            }

            llvm::Value * value = nullptr;
            std::optional<std::shared_ptr<Operator>> op = std::nullopt;

            for (auto & n : nodes) {
                if (auto expr = std::dynamic_pointer_cast<Expression>(n); expr) {
                    auto _value = expr->build(module, type);
                    value = op.has_value() ? op->get()->build(module, value, _value, type) : _value;

                    continue;
                }

                if (auto _op = std::dynamic_pointer_cast<Operator>(n); _op) {
                    if (type == llvm::Type::getVoidTy(*module.context)) {
                        std::cerr << "ICE: operation on void type" << std::endl;
                        exit(0);
                    }

                    op = _op;

                    continue;
                }

                if (auto num = std::dynamic_pointer_cast<Number>(n); num) {
                    auto _value = num->build(module, type);
                    value = op.has_value() ? op->get()->build(module, value, _value, type) : _value;
                    
                    continue;
                }

                if (auto boolean = std::dynamic_pointer_cast<Boolean>(n); boolean) {
                    auto _value = (llvm::Value *)boolean->build(module);
                    value = op.has_value() ? op->get()->build(module, value, _value, type) : _value;

                    continue;
                }

                if (auto identifier = std::dynamic_pointer_cast<Identifier>(n); identifier) {
                    if (module.local_variables.contains(identifier->identifier)) {
                        auto _value = module.get_builder()->CreateLoad(type, module.local_variables[identifier->identifier]);
                        value = op.has_value() ? op->get()->build(module, value, _value, type) : _value;
                    } else if (module.get_arguments().contains(identifier->identifier)) {
                        auto _value = module.get_arguments()[identifier->identifier];
                        value = op.has_value() ? op->get()->build(module, value, _value, type) : _value;
                    } else {
                        throw std::invalid_argument("ICE: unknown identifier");
                    }

                    continue;
                }

                if (auto call = std::dynamic_pointer_cast<Call>(n); call) {
                    std::vector<llvm::Value *> args;

                    for (uint32_t i = 0; i < call->nodes.size(); i++) {
                        if (auto expr = std::dynamic_pointer_cast<Expression>(call->nodes[i]); expr) {
                            llvm::Type * _t = nullptr;

                            if (i < module.module->getFunction(call->identifier)->arg_size()) {
                                _t = module.module->getFunction(call->identifier)->getArg(i)->getType();
                            } else {
                                // TODO: get actual type of vaarg
                                _t = llvm::Type::getInt32Ty(*module.context);
                            }

                            if (_t == nullptr) {
                                std::cerr << "ICE: call arg type is nullptr" << std::endl;
                                exit(0);
                            }

                            args.push_back(expr->build(module, _t));
                        }
                    }

                    auto _value = (llvm::Value *)call->build(module, args);
                    value = op.has_value() ? op->get()->build(module, value, _value, type) : _value;

                    continue;
                }

                if (auto string = std::dynamic_pointer_cast<String>(n); string) {
                    value = (llvm::Value *)string->build(module);

                    continue;
                }
            }

            return value;
        }
    };
}
