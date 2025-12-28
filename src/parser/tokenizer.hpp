#pragma once
#include <array>
#include <string_view>

namespace forma {

// ============================================================================
// Tokenizer / Lexer
// ============================================================================

enum class TokenKind {
    // identifiers & literals
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    BoolLiteral,

    // structure
    Colon,
    Comma,
    Dot,
    LBrace,
    RBrace,
    LParen,
    RParen,
    At,  // @ for annotations

    // operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    // comparisons
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    // keywords
    Property,
    Method,
    When,
    On,
    Import,
    Preview,
    Enum,
    Event,
    Or,
    Class,
    Animate,
    Requires,  // @requires annotation

    // misc
    EndOfFile,
    Invalid
};

struct Tok {
    TokenKind kind;
    std::string_view text;
    size_t pos;
};

template <size_t N>
struct TokSeq {
    std::array<Tok, N> toks;
    constexpr size_t size() const { return N; }
};

struct Lexer {
    std::string_view src;
    size_t pos = 0;

    constexpr char peek() const {
        return pos < src.size() ? src[pos] : '\0';
    }

    constexpr char advance() {
        return pos < src.size() ? src[pos++] : '\0';
    }
};

constexpr bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || c == '_';
}

constexpr bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

constexpr Tok next_token(Lexer& l) {
    // Skip whitespace and comments
    while (true) {
        while (l.peek() == ' ' || l.peek() == '\n' || l.peek() == '\t' || l.peek() == '\r')
            l.advance();
        
        // Skip line comments (// ...)
        if (l.peek() == '/' && l.pos + 1 < l.src.size() && l.src[l.pos + 1] == '/') {
            l.advance(); // skip first /
            l.advance(); // skip second /
            while (l.peek() && l.peek() != '\n')
                l.advance();
            if (l.peek() == '\n')
                l.advance();
            continue; // Check for more whitespace/comments
        }
        
        // Skip block comments (/* ... */)
        if (l.peek() == '/' && l.pos + 1 < l.src.size() && l.src[l.pos + 1] == '*') {
            l.advance(); // skip /
            l.advance(); // skip *
            while (l.peek()) {
                if (l.peek() == '*' && l.pos + 1 < l.src.size() && l.src[l.pos + 1] == '/') {
                    l.advance(); // skip *
                    l.advance(); // skip /
                    break;
                }
                l.advance();
            }
            continue; // Check for more whitespace/comments
        }
        
        break; // No more whitespace or comments
    }

    size_t start = l.pos;
    char c = l.advance();

    if (c == '\0')
        return {TokenKind::EndOfFile, "", l.pos};

    switch (c) {
        case '{': return {TokenKind::LBrace, "{", start};
        case '}': return {TokenKind::RBrace, "}", start};
        case ':': return {TokenKind::Colon, ":", start};
        case '.': return {TokenKind::Dot, ".", start};
        case ',': return {TokenKind::Comma, ",", start};
        case '(': return {TokenKind::LParen, "(", start};
        case ')': return {TokenKind::RParen, ")", start};
        case '@': return {TokenKind::At, "@", start};
        case '"': {
            while (l.peek() && l.peek() != '"') l.advance();
            l.advance(); // closing "
            return {
                TokenKind::StringLiteral,
                l.src.substr(start + 1, l.pos - start - 2),
                start
            };
        }
    }

    if (is_alpha(c)) {
        while (is_alpha(l.peek()) || is_digit(l.peek()))
            l.advance();

        auto text = l.src.substr(start, l.pos - start);
        if (text == "true" || text == "false")
            return {TokenKind::BoolLiteral, text, start};
        if (text == "property")
            return {TokenKind::Property, text, start};
        if (text == "method")
            return {TokenKind::Method, text, start};
        if (text == "when")
            return {TokenKind::When, text, start};
        if (text == "on")
            return {TokenKind::On, text, start};
        if (text == "preview")
            return {TokenKind::Preview, text, start};
        if (text == "enum")
            return {TokenKind::Enum, text, start};
        if (text == "event")
            return {TokenKind::Event, text, start};
        if (text == "import")
            return {TokenKind::Import, text, start};
        if (text == "or")
            return {TokenKind::Or, text, start};
        if (text == "class")
            return {TokenKind::Class, text, start};
        if (text == "animate")
            return {TokenKind::Animate, text, start};
        if (text == "requires")
            return {TokenKind::Requires, text, start};
        return {TokenKind::Identifier, text, start};
    }

    if (is_digit(c)) {
        while (is_digit(l.peek())) l.advance();
        return {
            TokenKind::IntegerLiteral,
            l.src.substr(start, l.pos - start),
            start
        };
    }

    return {TokenKind::Invalid, l.src.substr(start, 1), start};
}

} // namespace forma
