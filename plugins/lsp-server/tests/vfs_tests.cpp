#include <bugspray/bugspray.hpp>
#include "../src/lsp.hpp"
#include "../src/virtual_fs.hpp"

using namespace forma::lsp;
using namespace forma::vfs;

TEST_CASE("VirtualFS - Basic File Operations")
{
    VirtualFS<32> fs;
    
    SECTION("Write and read file")
    {
        REQUIRE(fs.write_file("file:///test.fml", "Point { property x: int }"));
        CHECK(fs.exists("file:///test.fml"));
        CHECK(fs.count() == static_cast<size_t>(1));
        
        auto content = fs.read_file("file:///test.fml");
        REQUIRE(content.has_value());
        CHECK(content.value() == "Point { property x: int }");
    }
    
    SECTION("Update file")
    {
        fs.write_file("file:///test.fml", "Point { property x: int }");
        
        REQUIRE(fs.write_file("file:///test.fml", "Point { property x: int property y: int }", 2));
        auto content = fs.read_file("file:///test.fml");
        CHECK(content.value() == "Point { property x: int property y: int }");
    }
    
    SECTION("Delete file")
    {
        fs.write_file("file:///test.fml", "Point { property x: int }");
        
        REQUIRE(fs.remove_file("file:///test.fml"));
        CHECK(!fs.exists("file:///test.fml"));
        CHECK(fs.count() == static_cast<size_t>(0));
    }
}
