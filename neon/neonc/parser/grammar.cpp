#include "grammar.h"

#define CHECK_NEWLINE_OR_SEMICOLON if (!accept(pack, TokenId::NEWLINE, std::nullopt).has_value()) { \
    if (!accept(pack, TokenId::SEMICOLON, std::nullopt)) { throw_parse_error(pack, "expected new line or semicolon"); } }

#define PRECEDENCE_SORT(ops) for (unsigned long int i = 2; i < nodes.size(); i += 2) { \
    auto & left = nodes[i - 2]; std::shared_ptr<Operator> op = std::dynamic_pointer_cast<Operator>(nodes[i - 1]); auto & right = nodes[i]; \
    if (ops) { auto result = std::make_shared<Expression>(); \
    result->nodes.push_back(left); result->nodes.push_back(op); result->nodes.push_back(right); \
    nodes[i - 2] = result; nodes.erase(nodes.begin() + i - 1, nodes.begin() + i + 1); i -= 2; } }

namespace neonc { 
    const std::optional<Token> accept(Pack * pack, const TokenId to_find, const std::optional<TokenId> ignore, bool progress = true) {
        for (std::size_t i = pack->index; i < std::size(pack->tokens); i++) {
            Token tok = pack->tokens[i];

            if (tok.token == to_find) {
                if (progress)
                    pack->index += i - pack->index + 1;

                return tok;
            }

            if (ignore.has_value())
                if (ignore.value() == tok.token)
                    continue;

            return {};
        }

        return {};
    }

    const std::optional<Token> expect(Pack * pack, const TokenId to_find, const std::optional<TokenId> ignore, const char * message, bool progress = true) {
        auto result = accept(pack, to_find, ignore, progress);

        if (!result.has_value()) {
            throw_parse_error(pack, message);

            return {};
        }

        return result;
    }

    inline static bool parse_body(Pack * pack, Node * node) {
        if (pack->get().token == TokenId::ENDOFFILE) {
            return false; 
        } else if (accept(pack, TokenId::NEWLINE, {}, false) || accept(pack, TokenId::SEMICOLON, {}, false)) {
            pack->next();
        } else if (pack->get().token == TokenId::RBRACE) {
            return false;
        } else if (accept(pack, TokenId::VAR, TokenId::NEWLINE, false)) {
            if (!parse_variable(pack, node)) return false;
        } else if (accept(pack, TokenId::RET, TokenId::NEWLINE, false)) {
            if (!parse_return(pack, node)) return false;
        } else if (parse_standalone_expression(pack, node)) {
            // nothing to do
        } else {
            throw_parse_error(pack, "unexpected");
        } 

        return true;
    }

    inline static bool __parse(Pack * pack, Node * node) {
        if (pack->get().token == TokenId::ENDOFFILE) {
            return false;
        } else if (accept(pack, TokenId::NEWLINE, {}, false) || accept(pack, TokenId::SEMICOLON, {}, false)) {
            pack->next();
        } else if (pack->get().token == TokenId::RBRACE) {
            return false;
        } else if (
            accept(pack, TokenId::FN, TokenId::NEWLINE, false)
            || accept(pack, TokenId::PUB, TokenId::NEWLINE, false)
        ) {
            if (!parse_function(pack, node)) return false;
        } else {
            throw_parse_error(pack, "unexpected");
        }

        return true;
    }

    void evaluate_expression(Node * node) {
        std::vector<std::shared_ptr<Node>> & nodes = node->nodes;

        PRECEDENCE_SORT(op->op == op::Operator::PERCENT || op->op == op::Operator::SLASH || op->op == op::Operator::ASTERISK);
        PRECEDENCE_SORT(op->op == op::Operator::PLUS || op->op == op::Operator::MINUS);

        PRECEDENCE_SORT(
            op->op == op::Operator::EQUAL
            || op->op == op::Operator::NOT_EQUAL
            || op->op == op::Operator::GREATER_THAN
            || op->op == op::Operator::LESS_THAN
            || op->op == op::Operator::GREATER_THAN_OR_EQUAL
            || op->op == op::Operator::LESS_THAN_OR_EQUAL
        );

        PRECEDENCE_SORT(op->op == op::Operator::NOT);
        PRECEDENCE_SORT(op->op == op::Operator::B_LEFT_SHIFT || op->op == op::Operator::B_RIGHT_SHIFT);
        PRECEDENCE_SORT(op->op == op::Operator::B_AND);
        PRECEDENCE_SORT(op->op == op::Operator::B_XOR);
        PRECEDENCE_SORT(op->op == op::Operator::B_OR);
        PRECEDENCE_SORT(op->op == op::Operator::AND);
        PRECEDENCE_SORT(op->op == op::Operator::OR);
    }

    //

    const std::optional<Type> parse_type(Pack * pack) {
        auto _type = accept(pack, TokenId::IDENT, TokenId::NEWLINE);

        std::string __type = "";
        
        if (_type) {
            __type += _type->value;
        } else {
            return std::nullopt;
        }

        return Type(__type, _type->position);
    }

    bool parse_number(Pack * pack, Node * node) {
        auto neg = accept(pack, TokenId::MINUS, std::nullopt);

        if (auto num = accept(pack, TokenId::NUMBER, TokenId::NEWLINE); num) {
            node->add_node<Number>((neg ? "-" : "") + num->value, false, num->position);

            return true;
        }

        if (auto fnum = accept(pack, TokenId::FLOATING_NUMBER, TokenId::NEWLINE); fnum) {
            node->add_node<Number>((neg ? "-" : "") + fnum->value, true, fnum->position);
            
            return true;
        }

        return false;
    }

    bool parse_boolean(Pack * pack, Node * node) {
        if (auto _true = accept(pack, TokenId::TRUE, TokenId::NEWLINE); _true) {
            node->add_node<Boolean>(true, _true->position);

            return true;
        }

        if (auto _false = accept(pack, TokenId::FALSE, TokenId::NEWLINE); _false) {
            node->add_node<Boolean>(false, _false->position);

            return true;
        }

        return false;
    }

    bool parse_ident(Pack * pack, Node * node) {
        auto ident = accept(pack, TokenId::IDENT, TokenId::NEWLINE);

        if (!ident)
            return false;


        if (accept(pack, TokenId::LPAREN, TokenId::NEWLINE)) {
            auto call = node->add_node<Call>(ident->value, ident->position);

            while (true) {
                if (!parse_expression(pack, call.get()))
                    break;

                if (!accept(pack, TokenId::COMMA, TokenId::NEWLINE))
                    break;
            }

            expect(pack, TokenId::RPAREN, TokenId::NEWLINE, "expected ')'");
        } else {
            node->add_node<Identifier>(ident->value, ident->position);
        }

        return true;
    }

    bool parse_string(Pack * pack, Node * node) {
        if (auto str = accept(pack, TokenId::STRING, TokenId::NEWLINE); str) {
            std::string _str = str->value;

            node->add_node<String>(str->value, str->position);

            return true;
        }

        return false;
    }

    bool parse_operator(Pack * pack, Node * node) {
        // +
        if (accept(pack, TokenId::PLUS, TokenId::NEWLINE)) {
            node->add_node<Operator>(op::Operator::PLUS);
            return true;
        }
        // -
        if (accept(pack, TokenId::MINUS, TokenId::NEWLINE)) {
            node->add_node<Operator>(op::Operator::MINUS);
            return true;
        }
        // /
        if (accept(pack, TokenId::SLASH, TokenId::NEWLINE)) {
            node->add_node<Operator>(op::Operator::SLASH);
            return true;
        }
        // *
        if (accept(pack, TokenId::ASTERISK, TokenId::NEWLINE)) {
            node->add_node<Operator>(op::Operator::ASTERISK);
            return true;
        }
        // %
        if (accept(pack, TokenId::PERCENT, TokenId::NEWLINE)) {
            node->add_node<Operator>(op::Operator::PERCENT);
            return true;
        }
        // ==
        if (accept(pack, TokenId::EQUALS, TokenId::NEWLINE)) {
            expect(pack, TokenId::EQUALS, {}, "expected '='");

            node->add_node<Operator>(op::Operator::EQUAL);
            return true;
        }
        // !
        // !=
        if (accept(pack, TokenId::EXCLAMATION, TokenId::NEWLINE)) {
            if (accept(pack, TokenId::EQUALS, {})) {
                node->add_node<Operator>(op::Operator::NOT_EQUAL);
                return true;
            }

            node->add_node<Operator>(op::Operator::NOT);
            return true;
        }
        // >
        // >=
        // >>
        if (accept(pack, TokenId::GREATER_THAN, TokenId::NEWLINE)) {
            if (accept(pack, TokenId::EQUALS, {})) {
                node->add_node<Operator>(op::Operator::GREATER_THAN_OR_EQUAL);
                return true;
            }

            if (accept(pack, TokenId::GREATER_THAN, {})) {
                node->add_node<Operator>(op::Operator::B_RIGHT_SHIFT);
                return true;
            }

            node->add_node<Operator>(op::Operator::GREATER_THAN);
            return true;
        }
        // <
        // <=
        // <<
        if (accept(pack, TokenId::LESS_THAN, TokenId::NEWLINE)) {
            if (accept(pack, TokenId::EQUALS, {})) {
                node->add_node<Operator>(op::Operator::LESS_THAN_OR_EQUAL);
                return true;
            }

            if (accept(pack, TokenId::LESS_THAN, {})) {
                node->add_node<Operator>(op::Operator::B_LEFT_SHIFT);
                return true;
            }

            node->add_node<Operator>(op::Operator::LESS_THAN);
            return true;
        }
        // &
        // &&
        if (accept(pack, TokenId::AND, TokenId::NEWLINE)) {
            if (accept(pack, TokenId::AND, {})) {
                node->add_node<Operator>(op::Operator::AND);
                return true;
            }

            node->add_node<Operator>(op::Operator::B_AND);
            return true;
        }
        // |
        // ||
        if (accept(pack, TokenId::OR, TokenId::NEWLINE)) {
            if (accept(pack, TokenId::OR, {})) {
                node->add_node<Operator>(op::Operator::OR);
                return true;
            }

            node->add_node<Operator>(op::Operator::B_OR);
            return true;
        }
        // ^
        if (accept(pack, TokenId::CIRC, TokenId::NEWLINE)) {
            node->add_node<Operator>(op::Operator::B_XOR);
            return true;
        }

        return false;
    }

    bool parse_expression(Pack * pack, Node * node) {
        auto expr = std::make_shared<Expression>();

    __parse_expr:
        if (accept(pack, TokenId::LPAREN, TokenId::NEWLINE)) {
            if (!parse_expression(pack, expr.get()))
                return false;

            expect(pack, TokenId::RPAREN, TokenId::NEWLINE, "expected ')'");

            if (parse_operator(pack, expr.get()))
                goto __parse_expr;
            else {
                evaluate_expression(expr.get());

                node->add_node(expr);

                return true;
            }
        }

        if (!(
            parse_boolean(pack, expr.get())
            || parse_number(pack, expr.get())
            || parse_ident(pack, expr.get())
            || parse_string(pack, expr.get())
        )) {
            return false;
        }

        if (parse_operator(pack, expr.get()))
            goto __parse_expr;

        evaluate_expression(expr.get());

        node->add_node(expr);

        return true;
    }

    bool parse_standalone_expression(Pack * pack, Node * node) {
        auto var = std::make_shared<Variable>();

        if (parse_expression(pack, var.get()))
            node->add_node(var);

        CHECK_NEWLINE_OR_SEMICOLON;

        return true;
    }

    bool parse_return(Pack * pack, Node * node) {
        auto ret = node->add_node<Return>(accept(pack, TokenId::RET, TokenId::NEWLINE)->position);

        parse_expression(pack, ret.get());

        CHECK_NEWLINE_OR_SEMICOLON;

        return true;
    }

    bool parse_variable(Pack * pack, Node * node) {
        auto _var = expect(pack, TokenId::VAR, TokenId::NEWLINE, "expected 'let'");
        auto ident = expect(pack, TokenId::IDENT, TokenId::NEWLINE, "expected identifier");

        if (accept(pack, TokenId::COLON, TokenId::NEWLINE)) {
            auto _type = parse_type(pack);

            if (!_type) {
                throw_parse_error(pack, "expected type");

                return false;
            }

            auto var = node->add_node<Variable>(ident->value, _type, _var->position);

            if (accept(pack, TokenId::EQUALS, TokenId::NEWLINE)) {
                if (!parse_expression(pack, var.get())) {
                    throw_parse_error(pack, "expected expression");

                    return false;
                }
            }
        } else {
            expect(pack, TokenId::EQUALS, TokenId::NEWLINE, "expected ':' or '='");

            auto var = node->add_node<Variable>(ident->value, std::nullopt, _var->position);

            if (!parse_expression(pack, var.get())) {
                throw_parse_error(pack, "expected expression");

                return false;
            }
        }

        CHECK_NEWLINE_OR_SEMICOLON;

        return true;
    }

    bool parse_function_arguments(Pack * pack, std::shared_ptr<Function> func) {
        std::vector<Argument> args;

        while (true) {
            auto ident = accept(pack, TokenId::IDENT, TokenId::NEWLINE);

            if (!ident)
                break;

            if (accept(pack, TokenId::COLON, TokenId::NEWLINE, "expected ':'")) {
                if (accept(pack, TokenId::DOT, TokenId::NEWLINE)) {
                    expect(pack, TokenId::DOT, TokenId::NEWLINE, "expected '...'");
                    expect(pack, TokenId::DOT, TokenId::NEWLINE, "expected '...'");

                    auto _type = parse_type(pack);
                    
                    if (!_type) {
                        throw_parse_error(pack, "expected type");
                    
                        return false;
                    }
    
                    auto vaarg = Argument(ident->value, _type, _type->position);
                    vaarg.set_variadic(true);
                    args.push_back(vaarg);

                    accept(pack, TokenId::COMMA, TokenId::NEWLINE);

                    break;
                }

                auto _type = parse_type(pack);

                if (!_type) {
                    throw_parse_error(pack, "expected type");
                
                    return false;
                }

                args.push_back(Argument(ident->value, _type, ident->position));
            } else {
                args.push_back(Argument(ident->value, std::nullopt, ident->position));
            }

            if (!accept(pack, TokenId::COMMA, TokenId::NEWLINE))
                break;
        }

        for (int32_t i = args.size(); i >= 0; i--) { // propagate types from right to left
            if (i == int32_t(args.size()) - 1 && !args[i].get_type().has_value()) {
            __no_type:
                throw_parse_error_at_position(pack, args[i].get_position().value(), "argument has no type");
            } else if (i < int32_t(args.size())) {
                if (args[i + 1].get_variadic() && !args[i].get_type()) {
                    goto __no_type;
                }

                if (!args[i].get_type()) {
                    args[i].set_type(args[i + 1].get_type());
                }
            }
        }

        func->add_arguments(args);

        return true;
    }

    bool parse_function(Pack * pack, Node * node) {
        auto pub = accept(pack, TokenId::PUB, TokenId::NEWLINE);
        auto fntok = expect(pack, TokenId::FN, TokenId::NEWLINE, "expected 'fn'");
        auto ident = expect(pack, TokenId::IDENT, TokenId::NEWLINE, "expected identifier");
 
        auto func = node->add_node<Function>(ident->value, fntok->position);

        if (pub)
            func->set_public(true);

        expect(pack, TokenId::LPAREN, TokenId::NEWLINE, "expected '('");

        parse_function_arguments(pack, func);

        expect(pack, TokenId::RPAREN, TokenId::NEWLINE, "expected ')'");

        if (auto ret_type = parse_type(pack); ret_type)
            func->set_return_type(ret_type);

        if (accept(pack, TokenId::SEMICOLON, TokenId::NEWLINE)) {
            func->set_is_declaration(true);

            return true;
        }

        expect(pack, TokenId::LBRACE, TokenId::NEWLINE, "expected '{'");

        while (parse_body(pack, func.get()));

        expect(pack, TokenId::RBRACE, TokenId::NEWLINE, "expected '}'");

        return true;
    }
 
    void parse(Pack * pack, std::shared_ptr<Node> node) {
        while (true) {
            if (!__parse(pack, node.get())) {
                break;
            }
        }
    }
}
