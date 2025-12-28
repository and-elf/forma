#include "src/parser/ir.hpp"
#include "src/core/assets.hpp"
#include "plugins/lvgl-renderer/src/lvgl_renderer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace forma;
using namespace forma::lvgl;

std::string read_file(const char* path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.forma>\n";
        return 1;
    }
    
    std::string source = read_file(argv[1]);
    std::cout << "Parsing: " << argv[1] << "\n";
    
    // Parse the document
    auto doc = parse_document(source);
    
    // Collect assets
    auto bundler = collect_assets(doc);
    
    std::cout << "\nFound " << bundler.asset_count << " asset(s):\n";
    for (size_t i = 0; i < bundler.asset_count; ++i) {
        const auto& asset = bundler.assets[i];
        std::cout << "  [" << i << "] " 
                  << std::string_view(asset.uri.data(), asset.uri.size())
                  << "\n      Type: ";
        
        switch (asset.type) {
            case AssetDecl::Type::Image: std::cout << "Image"; break;
            case AssetDecl::Type::Font: std::cout << "Font"; break;
            case AssetDecl::Type::Binary: std::cout << "Binary"; break;
        }
        
        std::cout << "\n      Path: " 
                  << std::string_view(asset.file_path.data(), asset.file_path.size())
                  << "\n      Symbol: "
                  << std::string_view(asset.symbol_name.data(), asset.symbol_name.size())
                  << "\n";
    }
    
    // Copy assets to document for code generation
    for (size_t i = 0; i < bundler.asset_count && i < doc.assets.size(); ++i) {
        doc.assets[i] = bundler.assets[i];
        doc.asset_count = bundler.asset_count;
    }
    
    // Generate LVGL code
    LVGLRenderer renderer;
    renderer.generate(doc);
    
    std::cout << "\nGenerated LVGL Code:\n";
    std::cout << "====================\n";
    std::cout << renderer.get_output() << "\n";
    
    return 0;
}
