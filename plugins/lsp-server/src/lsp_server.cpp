#include "lsp.hpp"
#include "http_server.hpp"
#include <iostream>
#include <signal.h>

bool running = true;

void signal_handler(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    forma::lsp::LSPDocumentManager<16> lsp_manager;
    forma::http::HttpServer server(port, lsp_manager);
    
    if (!server.start()) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return 1;
    }
    
    std::cout << "Forma LSP Server running on http://localhost:" << port << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;
    
    server.run();
    
    return 0;
}
