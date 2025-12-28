#include "../src/lsp.hpp"
#include "lsp_tests.hpp"

int main() {
    // Run LSP tests
    forma::lsp::tests::run_all_tests();
    return 0;
}
