#include <bugspray/bugspray.hpp>
#include "forma.hpp"
#include "ir.hpp"

using namespace forma;

TEST_CASE("Tokenizer - Basic Punctuation")
{
    constexpr std::string_view source = "{ } : . , ( )";
    Lexer lexer{source, 0};
    
    CHECK(next_token(lexer).kind == TokenKind::LBrace);
    CHECK(next_token(lexer).kind == TokenKind::RBrace);
    CHECK(next_token(lexer).kind == TokenKind::Colon);
    CHECK(next_token(lexer).kind == TokenKind::Dot);
    CHECK(next_token(lexer).kind == TokenKind::Comma);
    CHECK(next_token(lexer).kind == TokenKind::LParen);
    CHECK(next_token(lexer).kind == TokenKind::RParen);
    CHECK(next_token(lexer).kind == TokenKind::EndOfFile);
}

TEST_CASE("Tokenizer - String Literals")
{
    SECTION("Basic string")
    {
        constexpr std::string_view source = R"("hello world")";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::StringLiteral);
        CHECK(tok.text == "hello world");
    }
    
    SECTION("Empty string")
    {
        constexpr std::string_view source = R"("")";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::StringLiteral);
        CHECK(tok.text == "");
    }
}

TEST_CASE("Tokenizer - Identifiers")
{
    SECTION("Simple identifier")
    {
        constexpr std::string_view source = "myVariable";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::Identifier);
        CHECK(tok.text == "myVariable");
    }
    
    SECTION("Identifier with underscores")
    {
        constexpr std::string_view source = "_my_var_123";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::Identifier);
        CHECK(tok.text == "_my_var_123");
    }
}

TEST_CASE("Tokenizer - Keywords")
{
    SECTION("property keyword")
    {
        constexpr std::string_view source = "property";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::Property);
        CHECK(tok.text == "property");
    }
    
    SECTION("when keyword")
    {
        constexpr std::string_view source = "when";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::When);
        CHECK(tok.text == "when");
    }
    
    SECTION("enum keyword")
    {
        constexpr std::string_view source = "enum";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::Enum);
        CHECK(tok.text == "enum");
    }
}

TEST_CASE("Tokenizer - Numbers")
{
    SECTION("Simple integer")
    {
        constexpr std::string_view source = "42";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::IntegerLiteral);
        CHECK(tok.text == "42");
    }
    
    SECTION("Zero")
    {
        constexpr std::string_view source = "0";
        Lexer lexer{source, 0};
        auto tok = next_token(lexer);
        
        CHECK(tok.kind == TokenKind::IntegerLiteral);
        CHECK(tok.text == "0");
    }
}

TEST_CASE("Tokenizer - Whitespace Handling")
{
    SECTION("Spaces between tokens")
    {
        constexpr std::string_view source = "  id1   id2  ";
        Lexer lexer{source, 0};
        
        auto tok1 = next_token(lexer);
        CHECK(tok1.kind == TokenKind::Identifier);
        CHECK(tok1.text == "id1");
        
        auto tok2 = next_token(lexer);
        CHECK(tok2.kind == TokenKind::Identifier);
        CHECK(tok2.text == "id2");
    }
}

TEST_CASE("Tokenizer - Complex Sequences")
{
    SECTION("Property declaration")
    {
        constexpr std::string_view source = R"(property name: "value")";
        Lexer lexer{source, 0};
        
        CHECK(next_token(lexer).kind == TokenKind::Property);
        CHECK(next_token(lexer).kind == TokenKind::Identifier);
        CHECK(next_token(lexer).kind == TokenKind::Colon);
        CHECK(next_token(lexer).kind == TokenKind::StringLiteral);
    }
    
    SECTION("Object with braces")
    {
        constexpr std::string_view source = "{ id1, id2 }";
        Lexer lexer{source, 0};
        
        CHECK(next_token(lexer).kind == TokenKind::LBrace);
        CHECK(next_token(lexer).kind == TokenKind::Identifier);
        CHECK(next_token(lexer).kind == TokenKind::Comma);
        CHECK(next_token(lexer).kind == TokenKind::Identifier);
        CHECK(next_token(lexer).kind == TokenKind::RBrace);
    }
}

// Compile-time evaluation tests (commented out - tokenizer not yet fully constexpr)
// EVAL_TEST_CASE("Tokenizer - Basic Punctuation");
// EVAL_TEST_CASE("Tokenizer - String Literals");
// EVAL_TEST_CASE("Tokenizer - Identifiers");
// EVAL_TEST_CASE("Tokenizer - Keywords");
// EVAL_TEST_CASE("Tokenizer - Numbers");
// EVAL_TEST_CASE("Tokenizer - Whitespace Handling");
// EVAL_TEST_CASE("Tokenizer - Complex Sequences");
