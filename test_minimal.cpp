#include <array>
#include <string_view>

namespace forma::lvgl {

enum class Platform {
    Linux
};

template <size_t MaxOutput = 16384>
class LVGLRenderer {
private:
    std::array<char, MaxOutput> output_buffer{};
    size_t output_pos = 0;

public:
    constexpr std::string_view get_output() const {
        return std::string_view(output_buffer.data(), output_pos);
    }
};

}

int main() {
    forma::lvgl::LVGLRenderer<1024> r;
    auto output = r.get_output();
    return 0;
}
