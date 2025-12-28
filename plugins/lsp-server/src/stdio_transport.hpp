#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>

namespace forma::lsp {

// Simple JSON-RPC 2.0 reader/writer for stdio transport
class StdioTransport {
public:
    // Read a JSON-RPC message from stdin
    static std::string read_message() {
        std::string content;
        size_t content_length = 0;
        
        // Read headers
        while (true) {
            std::string line;
            std::getline(std::cin, line);
            
            // Remove \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            // Empty line marks end of headers
            if (line.empty()) {
                break;
            }
            
            // Parse Content-Length header
            const char* prefix = "Content-Length: ";
            if (line.compare(0, strlen(prefix), prefix) == 0) {
                content_length = std::stoul(line.substr(strlen(prefix)));
            }
        }
        
        // Read content
        if (content_length > 0) {
            content.resize(content_length);
            std::cin.read(&content[0], static_cast<std::streamsize>(content_length));
        }
        
        return content;
    }
    
    // Write a JSON-RPC message to stdout
    static void write_message(const std::string& content) {
        std::ostringstream header;
        header << "Content-Length: " << content.length() << "\r\n\r\n";
        
        std::cout << header.str() << content;
        std::cout.flush();
    }
    
    // Simple JSON response builder
    static std::string make_response(int id, const std::string& result) {
        std::ostringstream json;
        json << "{\"jsonrpc\":\"2.0\",\"id\":" << id 
             << ",\"result\":" << result << "}";
        return json.str();
    }
    
    static std::string make_error_response(int id, int code, const std::string& message) {
        std::ostringstream json;
        json << "{\"jsonrpc\":\"2.0\",\"id\":" << id 
             << ",\"error\":{\"code\":" << code 
             << ",\"message\":\"" << message << "\"}}";
        return json.str();
    }
    
    static std::string make_notification(const std::string& method, const std::string& params) {
        std::ostringstream json;
        json << "{\"jsonrpc\":\"2.0\",\"method\":\"" << method 
             << "\",\"params\":" << params << "}";
        return json.str();
    }
    
    // Parse simple JSON fields (minimal parser for LSP)
    static std::string get_string_field(const std::string& json, const std::string& field) {
        std::string search = "\"" + field + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        
        pos += search.length();
        
        // Find the closing quote, handling escape sequences
        std::string result;
        bool escaped = false;
        for (size_t i = pos; i < json.length(); ++i) {
            char c = json[i];
            
            if (escaped) {
                // Handle escape sequences
                switch (c) {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    case '/': result += '/'; break;
                    default: result += c; break;  // Unknown escape, keep as-is
                }
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                // Found unescaped closing quote
                return result;
            } else {
                result += c;
            }
        }
        
        return result;  // No closing quote found, return what we have
    }
    
    static int get_int_field(const std::string& json, const std::string& field) {
        std::string search = "\"" + field + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return -1;
        
        pos += search.length();
        size_t end = json.find_first_of(",}", pos);
        if (end == std::string::npos) return -1;
        
        std::string value = json.substr(pos, end - pos);
        try {
            return std::stoi(value);
        } catch (...) {
            return -1;
        }
    }
    
    static std::string get_object_field(const std::string& json, const std::string& field) {
        std::string search = "\"" + field + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "{}";
        
        pos += search.length();
        
        // Skip whitespace
        while (pos < json.length() && std::isspace(json[pos])) pos++;
        
        if (pos >= json.length() || json[pos] != '{') return "{}";
        
        // Find matching closing brace
        int depth = 0;
        size_t start = pos;
        while (pos < json.length()) {
            if (json[pos] == '{') depth++;
            else if (json[pos] == '}') {
                depth--;
                if (depth == 0) {
                    return json.substr(start, pos - start + 1);
                }
            }
            pos++;
        }
        
        return "{}";
    }
};

} // namespace forma::lsp
