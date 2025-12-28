#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

std::string send_request(const std::string& body) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "Socket creation failed";
    
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    
    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return "Connection failed";
    }
    
    std::ostringstream request;
    request << "POST / HTTP/1.1\r\n";
    request << "Host: localhost:8080\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << body.size() << "\r\n";
    request << "\r\n";
    request << body;
    
    std::string req = request.str();
    send(sock, req.c_str(), req.size(), 0);
    
    char buffer[4096] = {0};
    ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);
    close(sock);
    
    if (bytes > 0) {
        std::string response(buffer, bytes);
        size_t body_pos = response.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            return response.substr(body_pos + 4);
        }
        return response;
    }
    
    return "No response";
}

int main() {
    std::cout << "Testing Forma LSP Server\n";
    std::cout << "========================\n\n";
    
    // Test 1: Initialize
    std::cout << "1. Initialize:\n";
    std::string init_resp = send_request(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"processId":1234,"rootUri":"file:///workspace"}})");
    std::cout << init_resp << "\n\n";
    
    // Test 2: Open valid document
    std::cout << "2. Open valid document:\n";
    std::string open_resp = send_request(R"({"jsonrpc":"2.0","id":2,"method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///test.fml","languageId":"forma","version":1,"text":"Point { property x: int property y: int }"}}})");
    std::cout << open_resp << "\n\n";
    
    // Test 3: Get diagnostics
    std::cout << "3. Get diagnostics (should be empty):\n";
    std::string diag_resp = send_request(R"({"jsonrpc":"2.0","id":3,"method":"textDocument/diagnostic","params":{"textDocument":{"uri":"file:///test.fml"}}})");
    std::cout << diag_resp << "\n\n";
    
    // Test 4: Open document with error
    std::cout << "4. Open document with unknown type:\n";
    std::string error_resp = send_request(R"({"jsonrpc":"2.0","id":4,"method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///error.fml","languageId":"forma","version":1,"text":"MyRect { property pos: UnknownType }"}}})");
    std::cout << error_resp << "\n\n";
    
    // Test 5: Get diagnostics for error
    std::cout << "5. Get diagnostics (should have error):\n";
    std::string error_diag = send_request(R"({"jsonrpc":"2.0","id":5,"method":"textDocument/diagnostic","params":{"textDocument":{"uri":"file:///error.fml"}}})");
    std::cout << error_diag << "\n\n";
    
    std::cout << "Tests completed!\n";
    return 0;
}
