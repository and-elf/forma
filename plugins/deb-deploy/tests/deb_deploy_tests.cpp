#include <bugspray/bugspray.hpp>
#include "../src/deb_deploy.hpp"
#include <fstream>

using namespace forma::deb;

TEST_CASE("Debian Package - Config Parser")
{
    SECTION("Parse basic metadata from TOML")
    {
        PackageMetadata meta;
        std::string test_config = "/tmp/test_package.toml";
        
        std::ofstream config(test_config);
        config << "[package]\n";
        config << "name = \"myapp\"\n";
        config << "version = \"1.2.3\"\n";
        config << "maintainer = \"John Doe <john@example.com>\"\n";
        config << "description = \"My awesome application\"\n";
        config << "architecture = \"amd64\"\n";
        config.close();
        
        CHECK(load_package_metadata(test_config, meta));
        CHECK(meta.name == "myapp");
        CHECK(meta.version == "1.2.3");
        CHECK(meta.maintainer == "John Doe <john@example.com>");
        CHECK(meta.architecture == "amd64");
    }
    
    SECTION("Parse deploy table overrides")
    {
        PackageMetadata meta;
        std::string test_config = "/tmp/test_deploy.toml";
        
        std::ofstream config(test_config);
        config << "[package]\n";
        config << "name = \"myapp\"\n";
        config << "version = \"1.0.0\"\n";
        config << "\n";
        config << "[deploy]\n";
        config << "version = \"1.2.3\"\n";  // Deploy overrides package version
        config << "architecture = \"arm64\"\n";
        config.close();
        
        CHECK(load_package_metadata(test_config, meta));
        CHECK(meta.name == "myapp");
        CHECK(meta.version == "1.2.3");  // Should use deploy value
        CHECK(meta.architecture == "arm64");
    }
    
    SECTION("Parse postinst script from deploy table")
    {
        PackageMetadata meta;
        std::string test_config = "/tmp/test_script.toml";
        
        std::ofstream config(test_config);
        config << "[package]\n";
        config << "name = \"testpkg\"\n";
        config << "\n";
        config << "[deploy]\n";
        config << "postinst = \"systemctl enable myapp.service\\nsystemctl start myapp.service\"\n";
        config.close();
        
        CHECK(load_package_metadata(test_config, meta));
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
