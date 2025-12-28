#pragma once

#include <string>
#include <string_view>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <array>

namespace forma::http {

// Simple JSON builder for responses
struct JsonBuilder {
    std::string json;
    
    void start_object() { json += "{"; }
    void end_object() { json += "}"; }
    void start_array() { json += "["; }
    void end_array() { json += "]"; }
    
    void add_string(std::string_view key, std::string_view value) {
        if (json.back() != '{' && json.back() != '[') json += ",";
        json += "\"" + std::string(key) + "\":\"" + std::string(value) + "\"";
    }
    
    void add_number(std::string_view key, int value) {
        if (json.back() != '{' && json.back() != '[') json += ",";
        json += "\"" + std::string(key) + "\":" + std::to_string(value);
    }
    
    void add_bool(std::string_view key, bool value) {
        if (json.back() != '{' && json.back() != '[') json += ",";
        json += "\"" + std::string(key) + "\":" + (value ? "true" : "false");
    }
    
    void add_object_start(std::string_view key) {
        if (json.back() != '{' && json.back() != '[') json += ",";
        json += "\"" + std::string(key) + "\":{";
    }
    
    void add_array_start(std::string_view key) {
        if (json.back() != '{' && json.back() != '[') json += ",";
        json += "\"" + std::string(key) + "\":[";
    }
};

// Simple JSON parser for extracting values
inline std::string_view extract_string(std::string_view json, std::string_view key) {
    std::string search = "\"" + std::string(key) + "\":\"";
    size_t pos = json.find(search);
    if (pos == std::string_view::npos) return "";
    
    pos += search.size();
    size_t end = json.find("\"", pos);
    if (end == std::string_view::npos) return "";
    
    return json.substr(pos, end - pos);
}

inline int extract_number(std::string_view json, std::string_view key) {
    std::string search = "\"" + std::string(key) + "\":";
    size_t pos = json.find(search);
    if (pos == std::string_view::npos) return 0;
    
    pos += search.size();
    return std::atoi(json.data() + pos);
}

inline std::string_view extract_object(std::string_view json, std::string_view key) {
    std::string search = "\"" + std::string(key) + "\":";
    size_t pos = json.find(search);
    if (pos == std::string_view::npos) return "";
    
    pos += search.size();
    if (pos >= json.size()) return "";
    
    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n')) pos++;
    
    if (json[pos] == '{') {
        size_t start = pos;
        int depth = 0;
        while (pos < json.size()) {
            if (json[pos] == '{') depth++;
            else if (json[pos] == '}') {
                depth--;
                if (depth == 0) return json.substr(start, pos - start + 1);
            }
            pos++;
        }
    }
    
    return "";
}

// Simple HTTP server for LSP
template<typename LSPManager>
class HttpServer {
    int server_fd = -1;
    int port;
    LSPManager& lsp_manager;
    std::string document_storage; // Store document text since string_view needs backing storage
    
public:
    HttpServer(int p, LSPManager& manager) : port(p), lsp_manager(manager) {}
    
    ~HttpServer() {
        if (server_fd >= 0) close(server_fd);
    }
    
    bool start() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) return false;
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) return false;
        if (listen(server_fd, 3) < 0) return false;
        
        return true;
    }
    
    void handle_request(int client_socket) {
        std::array<char, 4096> buffer;
        ssize_t bytes_read = read(client_socket, buffer.data(), buffer.size() - 1);
        if (bytes_read <= 0) return;
        
        buffer[bytes_read] = '\0';
        std::string_view request(buffer.data(), bytes_read);
        
        // Find the JSON body (after \r\n\r\n)
        size_t body_start = request.find("\r\n\r\n");
        if (body_start == std::string_view::npos) {
            send_error(client_socket, "Invalid request");
            return;
        }
        
        body_start += 4;
        std::string_view json_body = request.substr(body_start);
        
        // Extract JSON-RPC fields
        std::string_view method = extract_string(json_body, "method");
        int id = extract_number(json_body, "id");
        std::string_view params_str = extract_object(json_body, "params");
        
        // Route to appropriate LSP method
        std::string response;
        if (method == "initialize") {
            response = handle_initialize(id, params_str);
        } else if (method == "textDocument/didOpen") {
            response = handle_did_open(id, params_str);
        } else if (method == "textDocument/didChange") {
            response = handle_did_change(id, params_str);
        } else if (method == "textDocument/didClose") {
            response = handle_did_close(id, params_str);
        } else if (method == "textDocument/diagnostic") {
            response = handle_diagnostic(id, params_str);
        } else {
            response = build_error_response(id, -32601, "Method not found");
        }
        
        send_http_response(client_socket, response);
    }
    
    void run() {
        while (true) {
            sockaddr_in address{};
            socklen_t addrlen = sizeof(address);
            int client_socket = accept(server_fd, (sockaddr*)&address, &addrlen);
            
            if (client_socket < 0) continue;
            
            handle_request(client_socket);
            close(client_socket);
        }
    }
    
private:
    std::string handle_initialize(int id, std::string_view params) {
        int process_id = extract_number(params, "processId");
        std::string_view root_uri = extract_string(params, "rootUri");
        
        auto result = lsp_manager.initialize(process_id, root_uri);
        
        JsonBuilder json;
        json.start_object();
        json.add_number("jsonrpc", 2);
        json.add_number("id", id);
        json.add_object_start("result");
        json.add_object_start("capabilities");
        json.add_object_start("textDocumentSync");
        json.add_bool("openClose", result.capabilities.text_document_sync.open_close);
        json.add_number("change", result.capabilities.text_document_sync.change);
        json.end_object(); // textDocumentSync
        json.add_bool("diagnosticProvider", result.capabilities.diagnostic_provider);
        json.end_object(); // capabilities
        json.add_object_start("serverInfo");
        json.add_string("name", result.server_name);
        json.add_string("version", result.server_version);
        json.end_object(); // serverInfo
        json.end_object(); // result
        json.end_object();
        
        return json.json;
    }
    
    std::string handle_did_open(int id, std::string_view params) {
        std::string_view text_doc = extract_object(params, "textDocument");
        std::string_view uri = extract_string(text_doc, "uri");
        std::string_view language_id = extract_string(text_doc, "languageId");
        int version = extract_number(text_doc, "version");
        std::string_view text = extract_string(text_doc, "text");
        
        // Store text in our storage
        document_storage = std::string(text);
        
        forma::lsp::TextDocumentItem item;
        item.uri = uri;
        item.language_id = language_id;
        item.version = version;
        item.text = document_storage;
        
        lsp_manager.did_open(item);
        
        // didOpen is a notification, no response needed
        return build_empty_response(id);
    }
    
    std::string handle_did_change(int id, std::string_view params) {
        std::string_view text_doc = extract_object(params, "textDocument");
        std::string_view uri = extract_string(text_doc, "uri");
        int version = extract_number(text_doc, "version");
        
        // Extract the new text from contentChanges array
        // Simplified: assumes full document sync
        size_t changes_pos = params.find("contentChanges");
        if (changes_pos != std::string_view::npos) {
            std::string_view remaining = params.substr(changes_pos);
            std::string_view text = extract_string(remaining, "text");
            document_storage = std::string(text);
            
            forma::lsp::VersionedTextDocumentIdentifier id_obj;
            id_obj.uri = uri;
            id_obj.version = version;
            
            lsp_manager.did_change(id_obj, document_storage);
        }
        
        return build_empty_response(id);
    }
    
    std::string handle_did_close(int id, std::string_view params) {
        std::string_view text_doc = extract_object(params, "textDocument");
        std::string_view uri = extract_string(text_doc, "uri");
        
        forma::lsp::TextDocumentIdentifier id_obj;
        id_obj.uri = uri;
        
        lsp_manager.did_close(id_obj);
        
        return build_empty_response(id);
    }
    
    std::string handle_diagnostic(int id, std::string_view params) {
        std::string_view text_doc = extract_object(params, "textDocument");
        std::string_view uri = extract_string(text_doc, "uri");
        
        auto* doc = lsp_manager.find_document(uri);
        
        JsonBuilder json;
        json.start_object();
        json.add_string("jsonrpc", "2.0");
        json.add_number("id", id);
        json.add_object_start("result");
        json.add_string("kind", "full");
        json.add_array_start("items");
        
        if (doc) {
            for (size_t i = 0; i < doc->diagnostic_count; ++i) {
                const auto& diag = doc->diagnostics[i];
                if (i > 0) json.json += ",";
                json.start_object();
                json.add_object_start("range");
                json.add_object_start("start");
                json.add_number("line", diag.range.start.line);
                json.add_number("character", diag.range.start.character);
                json.end_object(); // start
                json.add_object_start("end");
                json.add_number("line", diag.range.end.line);
                json.add_number("character", diag.range.end.character);
                json.end_object(); // end
                json.end_object(); // range
                json.add_number("severity", static_cast<int>(diag.severity));
                json.add_string("code", diag.code);
                json.add_string("message", diag.message);
                json.end_object();
            }
        }
        
        json.end_array(); // items
        json.end_object(); // result
        json.end_object();
        
        return json.json;
    }
    
    std::string build_empty_response(int id) {
        return "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(id) + ",\"result\":null}";
    }
    
    std::string build_error_response(int id, int code, std::string_view message) {
        JsonBuilder json;
        json.start_object();
        json.add_string("jsonrpc", "2.0");
        json.add_number("id", id);
        json.add_object_start("error");
        json.add_number("code", code);
        json.add_string("message", message);
        json.end_object();
        json.end_object();
        return json.json;
    }
    
    void send_http_response(int socket, const std::string& body) {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "\r\n";
        response << body;
        
        std::string resp = response.str();
        write(socket, resp.c_str(), resp.size());
    }
    
    void send_error(int socket, std::string_view message) {
        std::string body = "{\"error\":\"" + std::string(message) + "\"}";
        send_http_response(socket, body);
    }
};

} // namespace forma::http
