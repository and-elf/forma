// Test suite for Tracer Plugin

#include "../src/tracer_plugin.hpp"
#include <cassert>
#include <sstream>
#include <iostream>

using namespace forma::tracer;

void test_tracer_levels() {
    auto& tracer = get_tracer();
    
    // Test level configuration
    tracer.set_level(TraceLevel::Silent);
    assert(tracer.get_level() == TraceLevel::Silent);
    
    tracer.set_level(TraceLevel::Normal);
    assert(tracer.get_level() == TraceLevel::Normal);
    
    tracer.set_level(TraceLevel::Verbose);
    assert(tracer.get_level() == TraceLevel::Verbose);
    
    tracer.set_level(TraceLevel::Debug);
    assert(tracer.get_level() == TraceLevel::Debug);
    
    std::cout << "✓ Tracer level tests passed\n";
}

void test_tracer_output() {
    auto& tracer = get_tracer();
    tracer.set_level(TraceLevel::Debug);
    
    std::cout << "\nTesting tracer output methods:\n";
    std::cout << "===============================\n";
    
    tracer.begin_stage("Test Stage");
    tracer.info("This is an info message");
    tracer.verbose("This is a verbose message");
    tracer.debug("This is a debug message");
    tracer.warning("This is a warning");
    tracer.error("This is an error");
    tracer.stat("Count", 42);
    tracer.stat("Name", "TestValue");
    tracer.end_stage();
    
    tracer.success("Test completed");
    
    std::cout << "\n✓ Tracer output tests completed\n";
}

void test_nested_stages() {
    auto& tracer = get_tracer();
    tracer.set_level(TraceLevel::Verbose);
    
    std::cout << "\nTesting nested stages:\n";
    std::cout << "======================\n";
    
    tracer.begin_stage("Outer Stage");
    tracer.info("Outer stage message");
    
    tracer.begin_stage("Inner Stage");
    tracer.info("Inner stage message");
    tracer.end_stage();
    
    tracer.info("Back to outer");
    tracer.end_stage();
    
    std::cout << "\n✓ Nested stage tests passed\n";
}

void test_plugin_metadata() {
    assert(std::string(get_plugin_name()) == "tracer");
    assert(std::string(get_plugin_version()) == "0.1.0");
    
    std::cout << "\n✓ Plugin metadata tests passed\n";
}

int main() {
    std::cout << "Tracer Plugin Test Suite\n";
    std::cout << "========================\n\n";
    
    test_tracer_levels();
    test_tracer_output();
    test_nested_stages();
    test_plugin_metadata();
    
    std::cout << "\n🎉 All tests passed!\n";
    
    return 0;
}
