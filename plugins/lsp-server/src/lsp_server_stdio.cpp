#include "lsp.hpp"
#include "stdio_transport.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <memory>

using namespace forma::lsp;

int main() {
    // Allocate on heap to avoid stack overflow
    // Use 64 document slots to handle larger workspaces
    auto lsp_manager = std::make_unique<LSPDocumentManager<64>>();
    bool running = true;
    
    // Write to stderr for logging (stdout is for LSP messages)
    std::cerr << "Forma LSP Server (stdio) started" << std::endl;
    
    while (running && std::cin.good()) {
        try {
            // Read incoming message
            std::string message = StdioTransport::read_message();
            if (message.empty()) {
                break; // EOF or error
            }
            
            std::cerr << "Received: " << message.substr(0, 100) << "..." << std::endl;
            
            // Parse method and id
            std::string method = StdioTransport::get_string_field(message, "method");
            int id = StdioTransport::get_int_field(message, "id");
            
            std::cerr << "Method: " << method << ", ID: " << id << std::endl;
            
            // Handle requests
            if (method == "initialize") {
                std::string response = StdioTransport::make_response(id,
                    "{"
                    "\"capabilities\":{"
                        "\"textDocumentSync\":{"
                            "\"openClose\":true,"
                            "\"change\":1"
                        "},"
                        "\"diagnosticProvider\":true,"
                        "\"completionProvider\":{"
                            "\"triggerCharacters\":[\".\",\":\"]"
                        "},"
                        "\"hoverProvider\":true,"
                        "\"definitionProvider\":true"
                    "},"
                    "\"serverInfo\":{"
                        "\"name\":\"forma-lsp\","
                        "\"version\":\"0.1.0\""
                    "}"
                    "}"
                );
                StdioTransport::write_message(response);
                std::cerr << "Sent initialize response" << std::endl;
                
            } else if (method == "initialized") {
                // Notification, no response needed
                std::cerr << "Client initialized" << std::endl;
                
            } else if (method == "shutdown") {
                std::string response = StdioTransport::make_response(id, "null");
                StdioTransport::write_message(response);
                std::cerr << "Shutdown requested" << std::endl;
                
            } else if (method == "exit") {
                running = false;
                std::cerr << "Exiting" << std::endl;
                
            } else if (method == "textDocument/didOpen") {
                // Extract document info
                std::string params = StdioTransport::get_object_field(message, "params");
                std::string textDocument = StdioTransport::get_object_field(params, "textDocument");
                std::string uri = StdioTransport::get_string_field(textDocument, "uri");
                std::string text = StdioTransport::get_string_field(textDocument, "text");
                int version = StdioTransport::get_int_field(textDocument, "version");
                
                std::cerr << "Document opened: " << uri << " (" << text.length() << " bytes)" << std::endl;
                std::cerr << "Debug: First 200 chars of text: " << text.substr(0, std::min(size_t(200), text.size())) << std::endl;
                
                // Parse document and get diagnostics
                TextDocumentItem item;
                item.uri = uri;
                item.text = text;
                item.version = version;
                item.language_id = "forma";
                
                std::cerr << "Debug: Calling did_open..." << std::endl;
                lsp_manager->did_open(item);
                std::cerr << "Debug: did_open complete" << std::endl;
                
                // Get diagnostics from the document
                const auto* doc = lsp_manager->find_document(uri);
                
                // Send diagnostics
                std::ostringstream diag_json;
                diag_json << "{\"uri\":\"" << uri << "\",\"diagnostics\":[";
                
                if (doc) {
                    bool first = true;
                    for (size_t i = 0; i < doc->diagnostic_count; ++i) {
                        if (!first) diag_json << ",";
                        first = false;
                        
                        const auto& diag = doc->diagnostics[i];
                        diag_json << "{"
                            << "\"range\":{"
                                << "\"start\":{\"line\":" << diag.range.start.line << ",\"character\":" << diag.range.start.character << "},"
                                << "\"end\":{\"line\":" << diag.range.end.line << ",\"character\":" << diag.range.end.character << "}"
                            << "},"
                            << "\"severity\":" << static_cast<int>(diag.severity) << ","
                            << "\"message\":\"" << diag.message << "\""
                            << "}";
                    }
                    std::cerr << "Sent " << doc->diagnostic_count << " diagnostics" << std::endl;
                }
                
                diag_json << "]}";
                
                std::string notification = StdioTransport::make_notification(
                    "textDocument/publishDiagnostics",
                    diag_json.str()
                );
                StdioTransport::write_message(notification);
                
            } else if (method == "textDocument/didChange") {
                std::string params = StdioTransport::get_object_field(message, "params");
                std::string textDocument = StdioTransport::get_object_field(params, "textDocument");
                std::string uri = StdioTransport::get_string_field(textDocument, "uri");
                
                // For simplicity, get the full text from contentChanges[0].text
                // This assumes full document sync
                std::string contentChanges = StdioTransport::get_object_field(params, "contentChanges");
                
                std::cerr << "Document changed: " << uri << std::endl;
                // Note: Full implementation would parse contentChanges array and extract text
                
            } else if (method == "textDocument/didClose") {
                std::string params = StdioTransport::get_object_field(message, "params");
                std::string textDocument = StdioTransport::get_object_field(params, "textDocument");
                std::string uri = StdioTransport::get_string_field(textDocument, "uri");
                
                TextDocumentIdentifier id;
                id.uri = uri;
                lsp_manager->did_close(id);
                std::cerr << "Document closed: " << uri << std::endl;
                
            } else if (method == "textDocument/definition") {
                std::string params = StdioTransport::get_object_field(message, "params");
                std::string textDocument = StdioTransport::get_object_field(params, "textDocument");
                std::string uri = StdioTransport::get_string_field(textDocument, "uri");
                std::string position_str = StdioTransport::get_object_field(params, "position");
                
                Position pos;
                pos.line = StdioTransport::get_int_field(position_str, "line");
                pos.character = StdioTransport::get_int_field(position_str, "character");
                
                std::cerr << "Definition request at " << uri << " (" << pos.line << ":" << pos.character << ")" << std::endl;
                
                Location def_location;
                if (lsp_manager->find_definition(uri, pos, def_location)) {
                    std::ostringstream result;
                    result << "{"
                        << "\"uri\":\"" << def_location.uri << "\","
                        << "\"range\":{"
                            << "\"start\":{\"line\":" << def_location.range.start.line << ",\"character\":" << def_location.range.start.character << "},"
                            << "\"end\":{\"line\":" << def_location.range.end.line << ",\"character\":" << def_location.range.end.character << "}"
                        << "}"
                        << "}";
                    
                    std::string response = StdioTransport::make_response(id, result.str());
                    StdioTransport::write_message(response);
                    std::cerr << "Found definition at line " << def_location.range.start.line << std::endl;
                } else {
                    // No definition found - return null
                    std::string response = StdioTransport::make_response(id, "null");
                    StdioTransport::write_message(response);
                    std::cerr << "No definition found" << std::endl;
                }
                
            } else {
                // Unknown method
                if (id >= 0) {
                    std::string response = StdioTransport::make_error_response(
                        id, -32601, "Method not found: " + method
                    );
                    StdioTransport::write_message(response);
                }
                std::cerr << "Unknown method: " << method << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    
    std::cerr << "Forma LSP Server (stdio) stopped" << std::endl;
    return 0;
}
