// Sorry, used huge comments because I didn't want to split this into multiple
// files.

#include <iostream>
#include <string>
#include <vector>

// basic functions

// stole this from stackoverflow idk, tostring method that removes trailing dot
// and zeros when to_string(double)
template <typename T>
std::string to_string(const T& t) {
    std::string str{std::to_string(t)};
    int offset{1};
    if (str.find_last_not_of('0') == str.find('.')) {
        offset = 0;
    }
    str.erase(str.find_last_not_of('0') + offset, std::string::npos);
    return str;
}

std::string remParen(std::string x) {
    if (x.front() == '(' && x.back() == ')') {
        return x.substr(1, x.size() - 2);
    } else
        return x;
}

////////////////////////////////////////////
////////////////////////////////////////////
// LEXICAL ANALYSIS
////////////////////////////////////////////
////////////////////////////////////////////

struct Token {
    std::string str;
    enum types {
        CONST,
        VAR,
        KEYWORD,
        POW,
        PLUS,
        MINUS,
        MUL,
        DIV,
        LPAREN,
        RPAREN
    } type;
    size_t start, end;

    Token() = default;
    Token(types _type, std::string _str, size_t _start = 0, size_t _end = 0) {
        start = _start;
        end = _end;
        str = _str;
        type = _type;
    }
};

#define BASICTOK(type, str)                               \
    toks.push_back(Token(Token::types::type, str, i, i)); \
    break

std::vector<Token> lex(std::string source) {
    std::vector<Token> toks{};
    for (size_t i = 0; i < source.length(); i++) {
        if (isalpha(source[i])) {
            size_t start = i;
            while (isalnum(source[i])) i++;
            if (i - start == 1 && tolower(source[start]) == 'x')
                toks.push_back(Token(Token::types::VAR, "x", start, start));
            else
                toks.push_back(Token(Token::types::KEYWORD,
                                     source.substr(start, i - start), start,
                                     i));
            i--;
        } else if (isdigit(source[i])) {
            size_t start = i;
            while (isdigit(source[i])) i++;
            if (source[i] == '.') i++;
            while (isdigit(source[i])) i++;

            toks.push_back(Token(Token::types::CONST,
                                 source.substr(start, i - start), start, i));
            i--;
        } else {
            switch (source[i]) {
                case '+':
                    BASICTOK(PLUS, "+");
                case '-':
                    BASICTOK(MINUS, "-");
                case '*':
                    BASICTOK(MUL, "*");
                case '/':
                    BASICTOK(DIV, "/");
                case '^':
                    BASICTOK(POW, "^");
            }
        }
    }

    return toks;
}

////////////////////////////////////////////
////////////////////////////////////////////
// PARSING
////////////////////////////////////////////
////////////////////////////////////////////

template <typename T>
bool tContains(std::vector<T> tbl, T obj) {
    for (size_t i = 0; i < tbl.size(); i++) {
        if (tbl[i] == obj) return true;
    }

    return false;
}

struct ASTNode {
    Token t1;
    Token t2;

    ASTNode* a1;
    ASTNode* a2;

    virtual std::string name() = 0;
    virtual std::string derive(bool self = false) = 0;
    virtual std::string read() = 0;

    ASTNode() = default;
    ASTNode(Token _t1) : t1(_t1) {}
    ASTNode(ASTNode* _a1) : a1(_a1) {}
    ASTNode(ASTNode* _a1, ASTNode* _a2) : a1(_a1), a2(_a2) {}
    ASTNode(ASTNode* _a1, Token _t1, ASTNode* _a2)
        : a1(_a1), t1(_t1), a2(_a2) {}
};

struct BinopNode : ASTNode {
    std::string name() override { return "BinopNode"; }
    BinopNode(ASTNode* _a1, Token _t1, ASTNode* _a2) : ASTNode(_a1, _t1, _a2) {}

    std::string read() override {
        return "(" + a1->read() + " " + t1.str + " " + a2->read() + ")";
    }

    std::string derive(bool self = false) override {
        if (t1.type == Token::types::PLUS || t1.type == Token::types::MINUS) {
            if (a2->derive(true) == "(0)") return a1->derive();
            return "(" + remParen(a1->derive(true)) + " " + t1.str + " " +
                   remParen(a2->derive(true)) + ")";
        } else if (t1.type == Token::types::POW) {
            if (a1->name() == "XNode") {
                double val = std::stod(a2->t1.str);
                std::string pval1 = to_string(val);
                std::string pval2 = to_string(val - 1);

                if (pval2 == "1") return "(" + pval1 + "x)";
                return "(" + pval1 + "x^(" + pval2 + "))";
            } else {
                return "(0)";  // its just a constant then (TODO: NOT true, need
                               // to do pow checks later)
            }
        } else if (t1.type == Token::types::MUL) {
            if (a1->name() == "ConstNode" && a2->name() == "XNode") {
                return "(" + a1->t1.str + ")";
            } else if (a1->name() == "ConstNode" && a2->name() == "BinopNode") {
                return a1->t1.str + a2->derive();
            } else if (a1->name() == "ConstNode" && a2->name() == "ConstNode") {
                return "(0)";
            } else {
                std::cout << "Unknown multiply case: '" + a1->name() + "' * '" +
                                 a2->name() + "'."
                          << std::endl;
                return "(ERR)";
            }
        } else {
            std::cout << "BinopNode: Not implemented (Mul, Div)" << std::endl;
            return "(ERR)";
        }
    }
};

struct XNode : ASTNode {
    std::string name() override { return "XNode"; }
    XNode() = default;

    std::string derive(bool self = false) override { return "(1)"; }

    std::string read() override { return "x"; }
};

struct ConstNode : ASTNode {
    std::string name() override { return "ConstNode"; }
    ConstNode(Token _t1) : ASTNode(_t1) {}

    std::string derive(bool self = false) override {
        if (self) return "(0)";
        return "(" + t1.str + ")";
    }

    std::string read() override { return t1.str; }
};

struct ParserEnv {
    size_t pos = 0;
    std::vector<Token> toks;

    ParserEnv(std::vector<Token> _toks) { toks = _toks; }
};

ASTNode* pow(ParserEnv& env);

ASTNode* val(ParserEnv& env) {
    Token t = env.toks[env.pos];
    env.pos++;
    if (t.type == Token::types::CONST) {
        if (env.toks[env.pos].type == Token::types::VAR &&
            env.toks[env.pos + 1].type != Token::types::POW) {
            env.pos++;
            return new BinopNode(
                new ConstNode(t),
                Token(Token::types::MUL, "*", t.end + 1, t.end + 1),
                new XNode());
        } else if (env.toks[env.pos].type == Token::types::VAR) {
            return new BinopNode(
                new ConstNode(t),
                Token(Token::types::MUL, "*", t.end + 1, t.end + 1), pow(env));
        }

        return new ConstNode(t);
    } else if (t.type == Token::types::VAR) {
        return new XNode();
    } else {
        std::cout << "Unknown token type: '" << t.type << "' (" << t.str << ")"
                  << std::endl;
        return nullptr;
    }
}

ASTNode* binop(ASTNode* (*f)(ParserEnv&), ParserEnv& env,
               std::vector<Token::types> check) {
    ASTNode* left = f(env);

    while (tContains(check, env.toks[env.pos].type)) {
        Token t = env.toks[env.pos];
        env.pos++;
        ASTNode* right = f(env);

        left = new BinopNode(left, t, right);
    }

    return left;
}

ASTNode* pow(ParserEnv& env) { return binop(val, env, {Token::types::POW}); }

ASTNode* atom(ParserEnv& env) {
    return binop(pow, env, {Token::types::MUL, Token::types::DIV});
}

ASTNode* expr(ParserEnv& env) {
    return binop(atom, env, {Token::types::PLUS, Token::types::MINUS});
}

ASTNode* parse(std::vector<Token> toks) {
    ParserEnv env(toks);
    return expr(env);
}

////////////////////////////////////////////
////////////////////////////////////////////
// MAIN
////////////////////////////////////////////
////////////////////////////////////////////

int main(int argc, const char** argv) {
    if (argc == 1) {
        std::cout << "please provide function in quotes after the executable."
                  << std::endl;
        return 1;
    }

    if (argc > 2) {
        std::cout << "Make sure you provide the derivative in quotes."
                  << std::endl;
        return 1;
    }

    const char* deriv = argv[1];
    std::vector<Token> lexed = lex(deriv);
    ASTNode* parsed = parse(lexed);

    std::cout << "deriving: '" << parsed->read() << "':" << std::endl;
    std::cout << parsed->derive() << std::endl;

    return 0;
}
