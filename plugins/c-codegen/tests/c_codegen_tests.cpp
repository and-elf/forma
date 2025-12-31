#include "c_codegen.hpp"
#include "ir.hpp"
#include <bugspray/bugspray.hpp>

using namespace forma;
using namespace forma::codegen;

TEST_CASE("C Codegen - Basic class generation")
{
    SECTION("Class with properties")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        // Define a Counter class
        TypeDecl counter_class;
        counter_class.name = "Counter";
        PropertyDecl value_prop;
        value_prop.name = "value";
        value_prop.type = TypeRef{"int"};
        value_prop.reactive = false;
        counter_class.properties[0] = value_prop;
        counter_class.prop_count = 1;
        
        // Add a method to make it a class
        MethodDecl increment_method;
        increment_method.name = "increment";
        increment_method.return_type = TypeRef{"void"};
        increment_method.param_count = 0;
        counter_class.methods[0] = increment_method;
        counter_class.method_count = 1;
        
        doc.types[0] = counter_class;
        doc.type_count = 1;
        
        CCodeGenerator<4096> generator;
        generator.generate(doc);
        
        auto output = generator.get_output();
        
        // Check includes
        CHECK(output.find("#include <stdint.h>") != std::string_view::npos);
        CHECK(output.find("#include <stdbool.h>") != std::string_view::npos);
        
        // Check struct definition
        CHECK(output.find("typedef struct {") != std::string_view::npos);
        CHECK(output.find("int32_t value;") != std::string_view::npos);
        CHECK(output.find("} Counter;") != std::string_view::npos);
        
        // Check global instance
        CHECK(output.find("Counter counter") != std::string_view::npos);
        CHECK(output.find(".value = 0") != std::string_view::npos);
    }
    
    SECTION("Multiple classes")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        // Define Counter class
        TypeDecl counter_class;
        counter_class.name = "Counter";
        PropertyDecl value_prop;
        value_prop.name = "value";
        value_prop.type = TypeRef{"int"};
        counter_class.properties[0] = value_prop;
        counter_class.prop_count = 1;
        MethodDecl inc_method;
        inc_method.name = "increment";
        inc_method.return_type = TypeRef{"void"};
        counter_class.methods[0] = inc_method;
        counter_class.method_count = 1;
        doc.types[0] = counter_class;
        
        // Define Timer class
        TypeDecl timer_class;
        timer_class.name = "Timer";
        PropertyDecl elapsed_prop;
        elapsed_prop.name = "elapsed";
        elapsed_prop.type = TypeRef{"f32"};
        timer_class.properties[0] = elapsed_prop;
        timer_class.prop_count = 1;
        MethodDecl start_method;
        start_method.name = "start";
        start_method.return_type = TypeRef{"void"};
        timer_class.methods[0] = start_method;
        timer_class.method_count = 1;
        doc.types[1] = timer_class;
        
        doc.type_count = 2;
        
        CCodeGenerator<4096> generator;
        generator.generate(doc);
        
        auto output = generator.get_output();
        
        // Check both classes generated
        CHECK(output.find("} Counter;") != std::string_view::npos);
        CHECK(output.find("} Timer;") != std::string_view::npos);
        CHECK(output.find("Counter counter") != std::string_view::npos);
        CHECK(output.find("Timer timer") != std::string_view::npos);
    }
    
    SECTION("Type mapping")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        TypeDecl test_class;
        test_class.name = "TestTypes";
        
        PropertyDecl props[5];
        props[0].name = "int_val";
        props[0].type = TypeRef{"int"};
        props[1].name = "bool_val";
        props[1].type = TypeRef{"bool"};
        props[2].name = "float_val";
        props[2].type = TypeRef{"float"};
        props[3].name = "str_val";
        props[3].type = TypeRef{"string"};
        props[4].name = "i64_val";
        props[4].type = TypeRef{"i64"};
        
        for (size_t i = 0; i < 5; ++i) {
            test_class.properties[i] = props[i];
        }
        test_class.prop_count = 5;
        
        MethodDecl method;
        method.name = "test";
        method.return_type = TypeRef{"void"};
        test_class.methods[0] = method;
        test_class.method_count = 1;
        
        doc.types[0] = test_class;
        doc.type_count = 1;
        
        CCodeGenerator<4096> generator;
        generator.generate(doc);
        
        auto output = generator.get_output();
        
        // Check type mappings
        CHECK(output.find("int32_t int_val;") != std::string_view::npos);
        CHECK(output.find("bool bool_val;") != std::string_view::npos);
        CHECK(output.find("float float_val;") != std::string_view::npos);
        CHECK(output.find("const char* str_val;") != std::string_view::npos);
        CHECK(output.find("int64_t i64_val;") != std::string_view::npos);
    }
}
