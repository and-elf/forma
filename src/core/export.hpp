#pragma once
#include <string>
enum class Language {
    C,
    CPP
};

struct RendererConfig {
    Language output_language;
    std::string output_dir;
    bool enable_preview; // optional for tooling
    ThemeId theme;
};
