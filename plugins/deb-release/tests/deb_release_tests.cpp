#include <bugspray/bugspray.hpp>
#include "../src/deb_release.hpp"
#include <fstream>

using namespace forma::deb;

TEST_CASE("Debian Package - Config Parser")
{
    SECTION("Parse basic metadata")
    {
        PackageMetadata meta;
        std::string test_config = "/tmp/test_package.cfg";
        
        std::ofstream config(test_config);
        config << "name = myapp\n";
        config << "version = 1.2.3\n";
        config << "maintainer = John Doe <john@example.com>\n";
        config << "description = My awesome application\n";
        config << "architecture = amd64\n";
        config.close();
        
        CHECK(parse_package_config(test_config, meta));
        CHECK(meta.name == "myapp");
        CHECK(meta.version == "1.2.3");
        CHECK(meta.maintainer == "John Doe <john@example.com>");
        CHECK(meta.architecture == "amd64");
    }
    
    SECTION("Parse dependencies")
    {
        PackageMetadata meta;
        std::string test_config = "/tmp/test_deps.cfg";
        
        std::ofstream config(test_config);
        config << "name = testpkg\n";
        config << "depends = libc6, libssl3, python3\n";
        config.close();
        
        CHECK(parse_package_config(test_config, meta));
        CHECK(meta.dependencies.size() == static_cast<size_t>(3));
        CHECK(meta.dependencies[0] == "libc6");
        CHECK(meta.dependencies[1] == "libssl3");
        CHECK(meta.dependencies[2] == "python3");
    }
    
    SECTION("Parse postinst script")
    {
        PackageMetadata meta;
        std::string test_config = "/tmp/test_script.cfg";
        
        std::ofstream config(test_config);
        config << "name = testpkg\n";
        config << "[postinst]\n";
        config << "systemctl enable myapp.service\n";
        config << "systemctl start myapp.service\n";
        config.close();
        
        CHECK(parse_package_config(test_config, meta));
        CHECK(meta.postinst_script.find("systemctl enable") != std::string::npos);
        CHECK(meta.postinst_script.find("systemctl start") != std::string::npos);
    }
}

TEST_CASE("Debian Package - Builder")
{
    SECTION("Generate control file")
    {
        PackageMetadata meta;
        meta.name = "testapp";
        meta.version = "2.0.0";
        meta.description = "Test application";
        
        DebianPackageBuilder builder("/tmp/deb_test_build", "/tmp/deb_test_src");
        builder.set_metadata(meta);
        
        CHECK(builder.get_metadata().name == "testapp");
        CHECK(builder.get_metadata().version == "2.0.0");
    }
    
    SECTION("Package filename generation")
    {
        PackageMetadata meta;
        meta.name = "myapp";
        meta.version = "1.5.2";
        meta.architecture = "arm64";
        
        DebianPackageBuilder builder("/tmp/build", "/tmp/src");
        builder.set_metadata(meta);
        
        CHECK(builder.get_package_filename() == "myapp_1.5.2_arm64.deb");
    }
}
