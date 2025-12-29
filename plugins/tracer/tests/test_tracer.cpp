#include <bugspray/bugspray.hpp>
#include "../src/tracer_plugin.hpp"

using namespace forma::tracer;

TEST_CASE("Tracer - Level Configuration")
{
    auto& tracer = get_tracer();
    
    SECTION("Set and get Silent level")
    {
        tracer.set_level(TraceLevel::Silent);
        CHECK(tracer.get_level() == TraceLevel::Silent);
    }
    
    SECTION("Set and get Normal level")
    {
        tracer.set_level(TraceLevel::Normal);
        CHECK(tracer.get_level() == TraceLevel::Normal);
    }
    
    SECTION("Set and get Verbose level")
    {
        tracer.set_level(TraceLevel::Verbose);
        CHECK(tracer.get_level() == TraceLevel::Verbose);
    }
    
    SECTION("Set and get Debug level")
    {
        tracer.set_level(TraceLevel::Debug);
        CHECK(tracer.get_level() == TraceLevel::Debug);
    }
}

TEST_CASE("Tracer - Output Methods")
{
    auto& tracer = get_tracer();
    tracer.set_level(TraceLevel::Debug);
    
    SECTION("Stage management")
    {
        tracer.begin_stage("Test Stage");
        tracer.info("Info message");
        tracer.end_stage();
        // No assertion - just verify no crash
        REQUIRE(true);
    }
    
    SECTION("Message types")
    {
        tracer.info("This is an info message");
        tracer.verbose("This is a verbose message");
        tracer.debug("This is a debug message");
        tracer.warning("This is a warning");
        tracer.error("This is an error");
        // No assertion - just verify no crash
        REQUIRE(true);
    }
    
    SECTION("Statistics")
    {
        tracer.stat("Count", 42);
        tracer.stat("Name", "TestValue");
        tracer.success("Test completed");
        // No assertion - just verify no crash
        REQUIRE(true);
    }
}

TEST_CASE("Tracer - Nested Stages")
{
    auto& tracer = get_tracer();
    tracer.set_level(TraceLevel::Verbose);
    
    SECTION("Two level nesting")
    {
        tracer.begin_stage("Outer Stage");
        tracer.info("Outer stage message");
        
        tracer.begin_stage("Inner Stage");
        tracer.info("Inner stage message");
        tracer.end_stage();
        
        tracer.info("Back to outer");
        tracer.end_stage();
        
        // No assertion - just verify no crash
        REQUIRE(true);
    }
}

TEST_CASE("Tracer - Plugin Metadata")
{
    SECTION("Plugin name")
    {
        CHECK(std::string(get_plugin_name()) == "tracer");
    }
    
    SECTION("Plugin version")
    {
        CHECK(std::string(get_plugin_version()) == "0.1.0");
    }
}
