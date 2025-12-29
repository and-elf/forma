#include <bugspray/bugspray.hpp>
#include "example_plugin.hpp"

using namespace forma::example;

TEST_CASE("Example Plugin - Basic Functionality")
{
    SECTION("Enum processing")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        EnumDecl color_enum;
        color_enum.name = "Color";
        color_enum.values[0] = "Red";
        color_enum.values[1] = "Green";
        color_enum.values[2] = "Blue";
        color_enum.value_count = 3;
        doc.enums[doc.enum_count++] = color_enum;
        
        ExamplePlugin<1024> plugin;
        plugin.process(doc);
        
        auto output = plugin.get_output();
        CHECK(output.find("enum Color") != std::string_view::npos);
    }
}
