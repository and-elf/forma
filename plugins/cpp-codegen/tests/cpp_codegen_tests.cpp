#include "cpp_codegen.hpp"
#include "ir.hpp"
#include <bugspray/bugspray.hpp>

using namespace forma;
using namespace forma::codegen;

TEST_CASE("C++ Codegen - Basic class generation")
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
        
        CppCodeGenerator<4096> generator;
        generator.generate(doc);
        
        auto output = generator.get_output();
        
        // Check header guard
        CHECK(output.find("#pragma once") != std::string_view::npos);
        
        // Check includes
        CHECK(output.find("#include <cstdint>") != std::string_view::npos);
        CHECK(output.find("#include <string>") != std::string_view::npos);
        
        // Check class definition
        CHECK(output.find("class Counter {") != std::string_view::npos);
        CHECK(output.find("public:") != std::string_view::npos);
        
        // Check constructor with member initializer
        CHECK(output.find("Counter()") != std::string_view::npos);
        CHECK(output.find(": value(0)") != std::string_view::npos);
        
        // Check method declaration
        CHECK(output.find("void increment();") != std::string_view::npos);
        
        // Check private members
        CHECK(output.find("private:") != std::string_view::npos);
        CHECK(output.find("int32_t value;") != std::string_view::npos);
        
        // Check global instance (inline variable)
        CHECK(output.find("inline Counter counter;") != std::string_view::npos);
    }
    
    SECTION("Class with inheritance")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        // Define a derived class
        TypeDecl derived_class;
        derived_class.name = "AdvancedCounter";
        derived_class.base_type = "Counter";
        
        PropertyDecl step_prop;
        step_prop.name = "step";
        step_prop.type = TypeRef{"int"};
        derived_class.properties[0] = step_prop;
        derived_class.prop_count = 1;
        
        MethodDecl increment_by_method;
        increment_by_method.name = "incrementBy";
        increment_by_method.return_type = TypeRef{"void"};
        increment_by_method.params[0] = MethodParam{"amount", TypeRef{"int"}};
        increment_by_method.param_count = 1;
        derived_class.methods[0] = increment_by_method;
        derived_class.method_count = 1;
        
        doc.types[0] = derived_class;
        doc.type_count = 1;
        
        CppCodeGenerator<4096> generator;
        generator.generate(doc);
        
        auto output = generator.get_output();
        
        // Check inheritance
        CHECK(output.find("class AdvancedCounter : public Counter {") != std::string_view::npos);
        CHECK(output.find("void incrementBy(int32_t amount);") != std::string_view::npos);
    }
    
    SECTION("Multiple classes with different types")
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
        
        // Define Timer class with different types
        TypeDecl timer_class;
        timer_class.name = "Timer";
        
        PropertyDecl name_prop;
        name_prop.name = "name";
        name_prop.type = TypeRef{"string"};
        timer_class.properties[0] = name_prop;
        
        PropertyDecl elapsed_prop;
        elapsed_prop.name = "elapsed";
        elapsed_prop.type = TypeRef{"f64"};
        timer_class.properties[1] = elapsed_prop;
        
        PropertyDecl running_prop;
        running_prop.name = "running";
        running_prop.type = TypeRef{"bool"};
        timer_class.properties[2] = running_prop;
        
        timer_class.prop_count = 3;
        
        MethodDecl start_method;
        start_method.name = "start";
        start_method.return_type = TypeRef{"void"};
        timer_class.methods[0] = start_method;
        timer_class.method_count = 1;
        doc.types[1] = timer_class;
        
        doc.type_count = 2;
        
        CppCodeGenerator<4096> generator;
        generator.generate(doc);
        
        auto output = generator.get_output();
        
        // Check both classes generated
        CHECK(output.find("class Counter {") != std::string_view::npos);
        CHECK(output.find("class Timer {") != std::string_view::npos);
        
        // Check type mappings
        CHECK(output.find("std::string name;") != std::string_view::npos);
        CHECK(output.find("double elapsed;") != std::string_view::npos);
        CHECK(output.find("bool running;") != std::string_view::npos);
        
        // Check default values in constructor
        CHECK(output.find(": name(\"\")") != std::string_view::npos);
        CHECK(output.find("elapsed(0.0)") != std::string_view::npos);
        CHECK(output.find("running(false)") != std::string_view::npos);
        
        // Check global instances
        CHECK(output.find("inline Counter counter;") != std::string_view::npos);
        CHECK(output.find("inline Timer timer;") != std::string_view::npos);
    }
    
    SECTION("Method with return type and parameters")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        TypeDecl calc_class;
        calc_class.name = "Calculator";
        
        PropertyDecl result_prop;
        result_prop.name = "result";
        result_prop.type = TypeRef{"int"};
        calc_class.properties[0] = result_prop;
        calc_class.prop_count = 1;
        
        MethodDecl add_method;
        add_method.name = "add";
        add_method.return_type = TypeRef{"int"};
        add_method.params[0] = MethodParam{"a", TypeRef{"int"}};
        add_method.params[1] = MethodParam{"b", TypeRef{"int"}};
        add_method.param_count = 2;
        calc_class.methods[0] = add_method;
        calc_class.method_count = 1;
        
        doc.types[0] = calc_class;
        doc.type_count = 1;
        
        CppCodeGenerator<4096> generator;
        generator.generate(doc);
        
        auto output = generator.get_output();
        
        // Check method with return type and parameters
        CHECK(output.find("int32_t add(int32_t a, int32_t b);") != std::string_view::npos);
    }
}
