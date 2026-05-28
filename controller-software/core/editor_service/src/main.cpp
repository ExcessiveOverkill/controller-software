#include <arpa/inet.h>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;

namespace {

struct HttpRequest {
    std::string method;
    std::string target;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct NodePort {
    std::string name;
    std::string type;
};

struct NodeConfigField {
    std::string key;
    std::string widget;
    std::string type;
    json options;
};

struct NodeTypeDefinition {
    std::string type;
    std::vector<NodePort> inputs;
    std::vector<NodePort> outputs;
    std::vector<NodeConfigField> config_fields;
};

struct GraphNode {
    std::string name;
    std::string type;
    json config;
};

struct GraphConnection {
    std::string source_node;
    std::string source_port;
    std::string target_node;
    std::string target_port;
};

struct FileInfo {
    std::string path;   // relative to allowed root
    uintmax_t   size = 0;
    std::time_t mtime = 0;
};

class SandboxGraph {
public:
    SandboxGraph() {
        initialize_node_types();
        reset();
    }

    void reset() {
        nodes_.clear();
        connections_.clear();
    }

    json get_node_types_json() const {
        json out = json::array();
        for (const auto& [name, def] : node_types_) {
            (void)name;
            json n;
            n["type"] = def.type;
            n["inputs"] = json::array();
            n["outputs"] = json::array();
            n["config_fields"] = json::array();

            for (const auto& in : def.inputs) {
                n["inputs"].push_back({{"name", in.name}, {"type", in.type}});
            }
            for (const auto& outp : def.outputs) {
                n["outputs"].push_back({{"name", outp.name}, {"type", outp.type}});
            }
            for (const auto& field : def.config_fields) {
                n["config_fields"].push_back({
                    {"key", field.key},
                    {"widget", field.widget},
                    {"type", field.type},
                    {"options", field.options}
                });
            }

            out.push_back(n);
        }
        return out;
    }

    json get_graph_json() const {
        json out;
        out["format"] = "sandbox_graph_v1";
        out["nodes"] = json::array();
        out["connections"] = json::array();

        for (const auto& [name, node] : nodes_) {
            (void)name;
            out["nodes"].push_back({
                {"name", node.name},
                {"type", node.type},
                {"config", node.config}
            });
        }

        for (const auto& c : connections_) {
            out["connections"].push_back({
                {"src_node", c.source_node},
                {"src_port", c.source_port},
                {"dst_node", c.target_node},
                {"dst_port", c.target_port}
            });
        }
        return out;
    }

    bool add_node(const json& body, std::string& error) {
        if (!body.contains("name") || !body["name"].is_string()) {
            error = "missing or invalid 'name'";
            return false;
        }
        if (!body.contains("type") || !body["type"].is_string()) {
            error = "missing or invalid 'type'";
            return false;
        }

        const std::string name = body["name"].get<std::string>();
        const std::string type = body["type"].get<std::string>();

        if (nodes_.find(name) != nodes_.end()) {
            error = "node name already exists";
            return false;
        }

        if (node_types_.find(type) == node_types_.end()) {
            error = "unsupported node type";
            return false;
        }

        GraphNode node;
        node.name = name;
        node.type = type;
        node.config = body.contains("config") ? body["config"] : json::object();
        nodes_.emplace(name, node);
        return true;
    }

    bool add_connection(const json& body, std::string& error) {
        if (!body.contains("src_node") || !body["src_node"].is_string() ||
            !body.contains("src_port") || !body["src_port"].is_string() ||
            !body.contains("dst_node") || !body["dst_node"].is_string() ||
            !body.contains("dst_port") || !body["dst_port"].is_string()) {
            error = "connection body must include src_node, src_port, dst_node, dst_port";
            return false;
        }

        const std::string src_node = body["src_node"].get<std::string>();
        const std::string src_port = body["src_port"].get<std::string>();
        const std::string dst_node = body["dst_node"].get<std::string>();
        const std::string dst_port = body["dst_port"].get<std::string>();

        const auto src_it = nodes_.find(src_node);
        const auto dst_it = nodes_.find(dst_node);
        if (src_it == nodes_.end() || dst_it == nodes_.end()) {
            error = "source or target node not found";
            return false;
        }

        const auto src_type_it = node_types_.find(src_it->second.type);
        const auto dst_type_it = node_types_.find(dst_it->second.type);
        if (src_type_it == node_types_.end() || dst_type_it == node_types_.end()) {
            error = "node type definition missing";
            return false;
        }

        std::string src_port_type;
        std::string dst_port_type;

        bool src_found = false;
        for (const auto& p : src_type_it->second.outputs) {
            if (p.name == src_port) {
                src_port_type = p.type;
                src_found = true;
                break;
            }
        }

        bool dst_found = false;
        for (const auto& p : dst_type_it->second.inputs) {
            if (p.name == dst_port) {
                dst_port_type = p.type;
                dst_found = true;
                break;
            }
        }

        if (!src_found || !dst_found) {
            error = "source or target port not found";
            return false;
        }

        if (src_port_type != dst_port_type) {
            error = "port type mismatch";
            return false;
        }

        for (const auto& c : connections_) {
            if (c.target_node == dst_node && c.target_port == dst_port) {
                error = "target input already connected";
                return false;
            }
        }

        connections_.push_back({src_node, src_port, dst_node, dst_port});
        return true;
    }

private:
    void initialize_node_types() {
        NodeTypeDefinition constant;
        constant.type = "constant";
        constant.outputs = {{"output", "dynamic"}};
        constant.config_fields = {
            {"type", "dropdown", "string", json::array({"bool", "uint32", "int32", "float", "double"})},
            {"value", "number", "dynamic", json::object()}
        };
        node_types_.emplace(constant.type, constant);

        NodeTypeDefinition logic_gate;
        logic_gate.type = "logic_gate";
        logic_gate.outputs = {{"output", "bool"}};
        logic_gate.inputs = {{"input_0", "bool"}, {"input_1", "bool"}};
        logic_gate.config_fields = {
            {"gate_type", "dropdown", "string", json::array({"and", "or", "not", "nand", "nor", "xor", "xnor"})},
            {"input_count", "number", "uint8", json({{"min", 1}, {"max", 32}})}
        };
        node_types_.emplace(logic_gate.type, logic_gate);

        NodeTypeDefinition print;
        print.type = "print";
        print.inputs = {{"input", "dynamic"}, {"enable", "bool"}};
        print.config_fields = {
            {"type", "dropdown", "string", json::array({"bool", "uint32", "int32", "float", "double"})},
            {"name", "text", "string", json::object()}
        };
        node_types_.emplace(print.type, print);

        NodeTypeDefinition edge_delay;
        edge_delay.type = "edge_delay";
        edge_delay.inputs = {{"input", "bool"}};
        edge_delay.outputs = {{"output", "bool"}};
        edge_delay.config_fields = {
            {"rising_edge", "radio", "bool", json::array({true, false})},
            {"cycles", "number", "uint32", json({{"min", 0}})}
        };
        node_types_.emplace(edge_delay.type, edge_delay);
    }

public:

    // --- file format serialisation ---

    // Wrap the current graph in the persistent editor_graph_v1 envelope.
    json to_file_json(const std::string& name, const std::string& description) const {
        std::time_t now = std::time(nullptr);
        char ts[32];
        std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));

        const json graph_inner = get_graph_json();
        json graph_body;
        graph_body["nodes"]       = graph_inner["nodes"];
        graph_body["connections"] = graph_inner["connections"];

        json out;
        out["format"]  = "editor_graph_v1";
        out["version"] = 1;
        out["metadata"]["name"]        = name.empty() ? "untitled" : name;
        out["metadata"]["description"] = description;
        out["metadata"]["created_at"]  = std::string(ts);
        out["metadata"]["modified_at"] = std::string(ts);
        out["graph"] = graph_body;
        return out;
    }

    // Populate the sandbox from an editor_graph_v1 file JSON.
    bool load_from_file(const json& file_json, std::string& error) {
        if (!file_json.contains("format") || file_json["format"] != "editor_graph_v1") {
            error = "invalid or missing 'format', expected 'editor_graph_v1'";
            return false;
        }
        if (!file_json.contains("version") || file_json["version"] != 1) {
            error = "unsupported file version";
            return false;
        }
        if (!file_json.contains("graph")) {
            error = "missing 'graph' section";
            return false;
        }

        const json& graph = file_json["graph"];
        reset();

        if (graph.contains("nodes") && graph["nodes"].is_array()) {
            for (const auto& n : graph["nodes"]) {
                std::string node_error;
                if (!add_node(n, node_error)) {
                    error = "failed to load node: " + node_error;
                    return false;
                }
            }
        }

        if (graph.contains("connections") && graph["connections"].is_array()) {
            for (const auto& c : graph["connections"]) {
                std::string conn_error;
                if (!add_connection(c, conn_error)) {
                    error = "failed to load connection: " + conn_error;
                    return false;
                }
            }
        }

        return true;
    }

private:

    std::map<std::string, NodeTypeDefinition> node_types_;
    std::map<std::string, GraphNode> nodes_;
    std::vector<GraphConnection> connections_;
};

// ---------------------------------------------------------------------------
// FileService — secure filesystem operations restricted to an allowed root
// ---------------------------------------------------------------------------
class FileService {
public:
    void set_root(const std::string& root) { allowed_root_ = root; }
    const std::string& root() const { return allowed_root_; }

    bool init(std::string& error) {
        try {
            std::filesystem::create_directories(allowed_root_);
        } catch (const std::exception& e) {
            error = std::string("failed to create file root: ") + e.what();
            return false;
        }
        std::cout << "[editor_service] file root: " << allowed_root_ << std::endl;
        return true;
    }

    // Returns true when rel is safe to use under allowed_root_.
    // Checks: no traversal, no absolute path, .json extension only,
    // no control characters, max 256 chars.
    bool is_safe_path(const std::string& rel, std::string& error) const {
        if (rel.empty()) {
            error = "path must not be empty";
            return false;
        }
        if (rel.size() > 256) {
            error = "path too long";
            return false;
        }
        if (rel[0] == '/') {
            error = "absolute paths not allowed";
            return false;
        }
        if (rel.find("..") != std::string::npos) {
            error = "path traversal not allowed";
            return false;
        }
        for (unsigned char c : rel) {
            if (c < 0x20 || c == 0x7f) {
                error = "invalid character in path";
                return false;
            }
        }
        if (std::filesystem::path(rel).extension() != ".json") {
            error = "only .json files are allowed";
            return false;
        }
        return true;
    }

    std::vector<FileInfo> list_files() const {
        std::vector<FileInfo> result;
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(allowed_root_, ec)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".json") continue;

            FileInfo info;
            info.path = std::filesystem::relative(entry.path(), allowed_root_).string();
            info.size = entry.file_size(ec);

            struct stat st{};
            if (::stat(entry.path().c_str(), &st) == 0) {
                info.mtime = st.st_mtime;
            }
            result.push_back(info);
        }
        return result;
    }

    bool read_file(const std::string& rel, std::string& content, std::string& error) const {
        std::string path_err;
        if (!is_safe_path(rel, path_err)) { error = path_err; return false; }

        const std::filesystem::path full = std::filesystem::path(allowed_root_) / rel;
        if (!std::filesystem::exists(full) || !std::filesystem::is_regular_file(full)) {
            error = "file not found";
            return false;
        }
        std::error_code ec;
        if (std::filesystem::file_size(full, ec) > k_max_file_size) {
            error = "file exceeds 1 MB size limit";
            return false;
        }

        std::ifstream f(full, std::ios::binary);
        if (!f.is_open()) { error = "failed to open file"; return false; }
        std::ostringstream buf;
        buf << f.rdbuf();
        content = buf.str();
        return true;
    }

    enum class WriteResult { ok, conflict, error };

    WriteResult write_file_atomic(const std::string& rel, const std::string& content,
                                  std::time_t expected_mtime, std::time_t& out_mtime,
                                  std::string& error) {
        std::string path_err;
        if (!is_safe_path(rel, path_err)) { error = path_err; return WriteResult::error; }

        if (content.size() > k_max_file_size) {
            error = "content exceeds 1 MB size limit";
            return WriteResult::error;
        }

        const std::filesystem::path full = std::filesystem::path(allowed_root_) / rel;

        std::error_code ec;
        std::filesystem::create_directories(full.parent_path(), ec);
        if (ec) {
            error = "failed to create directory: " + ec.message();
            return WriteResult::error;
        }

        // optional conflict check: if the file already exists and the caller
        // supplied an expected mtime, reject if the on-disk mtime differs.
        if (expected_mtime != 0 && std::filesystem::exists(full)) {
            struct stat st{};
            if (::stat(full.c_str(), &st) == 0 && st.st_mtime != expected_mtime) {
                error = "file has been modified since last read (mtime conflict)";
                return WriteResult::conflict;
            }
        }

        // write to <path>.tmp, then rename atomically
        const std::filesystem::path tmp_path = std::filesystem::path(full).replace_extension(".tmp");
        {
            std::ofstream tmp(tmp_path, std::ios::binary | std::ios::trunc);
            if (!tmp.is_open()) { error = "failed to open temp file for write"; return WriteResult::error; }
            tmp.write(content.c_str(), static_cast<std::streamsize>(content.size()));
            if (!tmp) {
                error = "write failed";
                tmp.close();
                std::filesystem::remove(tmp_path, ec);
                return WriteResult::error;
            }
        }

        std::filesystem::rename(tmp_path, full, ec);
        if (ec) {
            error = "rename failed: " + ec.message();
            std::filesystem::remove(tmp_path, ec);
            return WriteResult::error;
        }

        struct stat st{};
        if (::stat(full.c_str(), &st) == 0) {
            out_mtime = st.st_mtime;
        }
        return WriteResult::ok;
    }

    bool delete_file(const std::string& rel, std::string& error) {
        std::string path_err;
        if (!is_safe_path(rel, path_err)) { error = path_err; return false; }

        const std::filesystem::path full = std::filesystem::path(allowed_root_) / rel;
        if (!std::filesystem::exists(full)) { error = "file not found"; return false; }

        std::error_code ec;
        std::filesystem::remove(full, ec);
        if (ec) { error = "delete failed: " + ec.message(); return false; }
        return true;
    }

private:
    std::string allowed_root_{"controller/config/user/editor"};
    static constexpr size_t k_max_file_size = 1024UL * 1024UL; // 1 MB
};

static constexpr const char* k_pid_file = "/tmp/controller_editor_service.pid";

volatile std::sig_atomic_t g_keep_running = 1;
SandboxGraph g_sandbox;
FileService  g_file_service;

void write_pid_file() {
    std::ofstream f(k_pid_file);
    if (f.is_open()) {
        f << getpid() << "\n";
    }
}

void remove_pid_file() {
    std::remove(k_pid_file);
}

void handle_signal(int) {
    g_keep_running = 0;
    remove_pid_file();
}

std::string get_env_or_default(const char* key, const std::string& fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }
    return std::string(value);
}

uint16_t get_port() {
    const std::string port_str = get_env_or_default("EDITOR_SERVICE_PORT", "8080");
    int port = 8080;
    try {
        port = std::stoi(port_str);
    } catch (...) {
        port = 8080;
    }

    if (port < 1 || port > 65535) {
        port = 8080;
    }

    return static_cast<uint16_t>(port);
}

std::string mime_type_for_path(const std::filesystem::path& path) {
    static const std::map<std::string, std::string> k_types = {
        {".html", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".json", "application/json; charset=utf-8"},
        {".svg", "image/svg+xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".ico", "image/x-icon"},
        {".txt", "text/plain; charset=utf-8"},
        {".map", "application/json; charset=utf-8"},
        {".wasm", "application/wasm"},
    };

    const auto ext = path.extension().string();
    const auto it = k_types.find(ext);
    if (it != k_types.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

std::string to_lower(std::string value) {
    for (char& c : value) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return value;
}

// Returns the path portion of a request target, stripping any query string.
std::string target_path(const std::string& target) {
    const auto q = target.find('?');
    return (q == std::string::npos) ? target : target.substr(0, q);
}

// Decodes a percent-encoded URL string.
std::string url_decode(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            auto hex_val = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            const int h = hex_val(s[i + 1]);
            const int l = hex_val(s[i + 2]);
            if (h >= 0 && l >= 0) {
                result += static_cast<char>((h << 4) | l);
                i += 2;
                continue;
            }
        }
        result += (s[i] == '+') ? ' ' : s[i];
    }
    return result;
}

// Parses key=value pairs from the query string of a request target.
std::map<std::string, std::string> parse_query_string(const std::string& target) {
    std::map<std::string, std::string> result;
    const auto q = target.find('?');
    if (q == std::string::npos) return result;
    std::istringstream ss(target.substr(q + 1));
    std::string token;
    while (std::getline(ss, token, '&')) {
        const auto eq = token.find('=');
        if (eq == std::string::npos) {
            result[url_decode(token)] = "";
        } else {
            result[url_decode(token.substr(0, eq))] = url_decode(token.substr(eq + 1));
        }
    }
    return result;
}

void send_response(int fd, int status, const std::string& status_text, const std::string& content_type, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << status_text << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    const std::string out = response.str();
    const char* data = out.c_str();
    size_t total = 0;

    while (total < out.size()) {
        const ssize_t sent = send(fd, data + total, out.size() - total, 0);
        if (sent <= 0) {
            return;
        }
        total += static_cast<size_t>(sent);
    }
}

void send_json_response(int fd, int status, const std::string& status_text, const json& payload) {
    send_response(fd, status, status_text, "application/json; charset=utf-8", payload.dump() + "\n");
}

std::string read_file_to_string(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool safe_path_requested(const std::string& path) {
    if (path.empty() || path[0] != '/') {
        return false;
    }

    if (path.find("..") != std::string::npos) {
        return false;
    }

    return true;
}

bool parse_request(const std::string& raw, HttpRequest& req) {
    const auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false;
    }

    std::string header_text = raw.substr(0, header_end);
    req.body = raw.substr(header_end + 4);

    std::istringstream lines(header_text);
    std::string first_line;
    if (!std::getline(lines, first_line)) {
        return false;
    }
    if (!first_line.empty() && first_line.back() == '\r') {
        first_line.pop_back();
    }

    std::istringstream request_line(first_line);
    request_line >> req.method >> req.target >> req.version;
    if (req.method.empty() || req.target.empty()) {
        return false;
    }

    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        const auto sep = line.find(':');
        if (sep == std::string::npos) {
            continue;
        }

        std::string key = to_lower(line.substr(0, sep));
        std::string value = line.substr(sep + 1);
        while (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }
        req.headers[key] = value;
    }

    return true;
}

bool read_http_request(int client_fd, HttpRequest& req) {
    constexpr size_t k_buf_size = 4096;
    std::string raw;
    raw.reserve(16384);

    while (raw.find("\r\n\r\n") == std::string::npos && raw.size() < (1024 * 1024)) {
        char buf[k_buf_size];
        const ssize_t r = recv(client_fd, buf, sizeof(buf), 0);
        if (r <= 0) {
            return false;
        }
        raw.append(buf, static_cast<size_t>(r));
    }

    if (!parse_request(raw, req)) {
        return false;
    }

    size_t expected = req.body.size();
    auto it = req.headers.find("content-length");
    if (it != req.headers.end()) {
        try {
            expected = static_cast<size_t>(std::stoul(it->second));
        } catch (...) {
            return false;
        }
    }

    while (req.body.size() < expected) {
        char buf[k_buf_size];
        const ssize_t r = recv(client_fd, buf, sizeof(buf), 0);
        if (r <= 0) {
            return false;
        }
        req.body.append(buf, static_cast<size_t>(r));
    }

    if (req.body.size() > expected) {
        req.body.resize(expected);
    }

    return true;
}

bool handle_api_request(int client_fd, const HttpRequest& req, const std::filesystem::path& static_root) {
    (void)static_root;

    if (req.target == "/health" && req.method == "GET") {
        send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"service", "editor_service"}});
        return true;
    }

    if (req.target == "/ready" && req.method == "GET") {
        const bool static_exists = std::filesystem::exists(static_root);
        if (static_exists) {
            send_json_response(client_fd, 200, "OK", {{"status", "ready"}, {"static_root", true}});
        } else {
            send_json_response(client_fd, 503, "Service Unavailable", {{"status", "not_ready"}, {"static_root", false}});
        }
        return true;
    }

    if (req.target == "/api/v1/capabilities" && req.method == "GET") {
        send_json_response(client_fd, 200, "OK", {
            {"mode", "sandbox"},
            {"supported_node_types", {"constant", "logic_gate", "print", "edge_delay"}},
            {"supports_live_apply", false},
            {"supports_file_io", true},
            {"file_root", g_file_service.root()}
        });
        return true;
    }

    if (req.target == "/api/v1/node-types" && req.method == "GET") {
        send_json_response(client_fd, 200, "OK", {{"node_types", g_sandbox.get_node_types_json()}});
        return true;
    }

    if (req.target == "/api/v1/graph" && req.method == "GET") {
        send_json_response(client_fd, 200, "OK", g_sandbox.get_graph_json());
        return true;
    }

    if (req.target == "/api/v1/graph/new" && req.method == "POST") {
        g_sandbox.reset();
        send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"graph", g_sandbox.get_graph_json()}});
        return true;
    }

    if (req.target == "/api/v1/graph/nodes" && req.method == "POST") {
        try {
            const json body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string error;
            if (!g_sandbox.add_node(body, error)) {
                send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", error}});
                return true;
            }
            send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"graph", g_sandbox.get_graph_json()}});
            return true;
        } catch (const std::exception& e) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", std::string("invalid json: ") + e.what()}});
            return true;
        }
    }

    if (req.target == "/api/v1/graph/connections" && req.method == "POST") {
        try {
            const json body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string error;
            if (!g_sandbox.add_connection(body, error)) {
                send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", error}});
                return true;
            }
            send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"graph", g_sandbox.get_graph_json()}});
            return true;
        } catch (const std::exception& e) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", std::string("invalid json: ") + e.what()}});
            return true;
        }
    }

    // -----------------------------------------------------------------------
    // File service endpoints (M3)
    // -----------------------------------------------------------------------

    const std::string req_path   = target_path(req.target);
    const auto        req_params = parse_query_string(req.target);

    // GET /api/v1/files — list all .json files under file root
    if (req_path == "/api/v1/files" && req.method == "GET") {
        const auto files = g_file_service.list_files();
        json arr = json::array();
        for (const auto& fi : files) {
            arr.push_back({{"path", fi.path}, {"size", fi.size}, {"mtime", fi.mtime}});
        }
        send_json_response(client_fd, 200, "OK", {{"root", g_file_service.root()}, {"files", arr}});
        return true;
    }

    // GET /api/v1/files/content?path=<rel> — read file, return parsed JSON
    if (req_path == "/api/v1/files/content" && req.method == "GET") {
        const auto it = req_params.find("path");
        if (it == req_params.end() || it->second.empty()) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "missing 'path' query param"}});
            return true;
        }
        std::string content, error;
        if (!g_file_service.read_file(it->second, content, error)) {
            const int code = (error == "file not found") ? 404 : 400;
            send_json_response(client_fd, code, (code == 404 ? "Not Found" : "Bad Request"),
                               {{"status", "error"}, {"message", error}});
            return true;
        }
        try {
            send_json_response(client_fd, 200, "OK", json::parse(content));
        } catch (...) {
            send_response(client_fd, 200, "OK", "application/json; charset=utf-8", content);
        }
        return true;
    }

    // POST /api/v1/files/save — atomic write with optional conflict check
    if (req_path == "/api/v1/files/save" && req.method == "POST") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "invalid JSON body"}});
            return true;
        }
        if (!body.contains("path") || !body["path"].is_string() || !body.contains("content")) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "body must include 'path' and 'content'"}});
            return true;
        }
        const std::time_t exp_mtime = body.value("expected_mtime", static_cast<std::time_t>(0));
        std::time_t out_mtime = 0;
        std::string error;
        const auto res = g_file_service.write_file_atomic(
            body["path"].get<std::string>(), body["content"].dump(),
            exp_mtime, out_mtime, error);
        if (res == FileService::WriteResult::conflict) {
            send_json_response(client_fd, 409, "Conflict", {{"status", "error"}, {"message", error}});
            return true;
        }
        if (res == FileService::WriteResult::error) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", error}});
            return true;
        }
        send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"path", body["path"]}, {"mtime", out_mtime}});
        return true;
    }

    // DELETE /api/v1/files?path=<rel>
    if (req_path == "/api/v1/files" && req.method == "DELETE") {
        const auto it = req_params.find("path");
        if (it == req_params.end() || it->second.empty()) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "missing 'path' query param"}});
            return true;
        }
        std::string error;
        if (!g_file_service.delete_file(it->second, error)) {
            const int code = (error == "file not found") ? 404 : 400;
            send_json_response(client_fd, code, (code == 404 ? "Not Found" : "Bad Request"),
                               {{"status", "error"}, {"message", error}});
            return true;
        }
        send_json_response(client_fd, 200, "OK", {{"status", "ok"}});
        return true;
    }

    // GET /api/v1/files/download?path=<rel> — file download with attachment header
    if (req_path == "/api/v1/files/download" && req.method == "GET") {
        const auto it = req_params.find("path");
        if (it == req_params.end() || it->second.empty()) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "missing 'path' query param"}});
            return true;
        }
        std::string content, error;
        if (!g_file_service.read_file(it->second, content, error)) {
            const int code = (error == "file not found") ? 404 : 400;
            send_json_response(client_fd, code, (code == 404 ? "Not Found" : "Bad Request"),
                               {{"status", "error"}, {"message", error}});
            return true;
        }
        const std::string filename = std::filesystem::path(it->second).filename().string();
        std::ostringstream resp;
        resp << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: application/json; charset=utf-8\r\n"
             << "Content-Length: " << content.size() << "\r\n"
             << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n"
             << "Connection: close\r\n\r\n"
             << content;
        const std::string out = resp.str();
        size_t total = 0;
        while (total < out.size()) {
            const ssize_t s = send(client_fd, out.c_str() + total, out.size() - total, 0);
            if (s <= 0) break;
            total += static_cast<size_t>(s);
        }
        return true;
    }

    // POST /api/v1/files/upload — JSON-body upload: {path, content}
    if (req_path == "/api/v1/files/upload" && req.method == "POST") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "invalid JSON body"}});
            return true;
        }
        if (!body.contains("path") || !body["path"].is_string() || !body.contains("content")) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "body must include 'path' and 'content'"}});
            return true;
        }
        std::time_t out_mtime = 0;
        std::string error;
        const auto res = g_file_service.write_file_atomic(
            body["path"].get<std::string>(), body["content"].dump(),
            0, out_mtime, error);
        if (res != FileService::WriteResult::ok) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", error}});
            return true;
        }
        send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"path", body["path"]}, {"mtime", out_mtime}});
        return true;
    }

    // POST /api/v1/graph/open — load a file into the sandbox
    if (req_path == "/api/v1/graph/open" && req.method == "POST") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "invalid JSON body"}});
            return true;
        }
        if (!body.contains("path") || !body["path"].is_string()) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "missing 'path'"}});
            return true;
        }
        std::string content, read_err;
        if (!g_file_service.read_file(body["path"].get<std::string>(), content, read_err)) {
            const int code = (read_err == "file not found") ? 404 : 400;
            send_json_response(client_fd, code, (code == 404 ? "Not Found" : "Bad Request"),
                               {{"status", "error"}, {"message", read_err}});
            return true;
        }
        json file_json;
        try { file_json = json::parse(content); }
        catch (...) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "file is not valid JSON"}});
            return true;
        }
        std::string load_err;
        if (!g_sandbox.load_from_file(file_json, load_err)) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", load_err}});
            return true;
        }
        send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"graph", g_sandbox.get_graph_json()}});
        return true;
    }

    // POST /api/v1/graph/save — persist current sandbox to a file
    if (req_path == "/api/v1/graph/save" && req.method == "POST") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "invalid JSON body"}});
            return true;
        }
        if (!body.contains("path") || !body["path"].is_string()) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", "missing 'path'"}});
            return true;
        }
        const std::string rel  = body["path"].get<std::string>();
        const std::string name = body.value("name", std::filesystem::path(rel).stem().string());
        const std::string desc = body.value("description", std::string(""));
        const std::time_t exp_mtime = body.value("expected_mtime", static_cast<std::time_t>(0));

        const std::string content = g_sandbox.to_file_json(name, desc).dump(4);
        std::time_t out_mtime = 0;
        std::string error;
        const auto res = g_file_service.write_file_atomic(rel, content, exp_mtime, out_mtime, error);
        if (res == FileService::WriteResult::conflict) {
            send_json_response(client_fd, 409, "Conflict", {{"status", "error"}, {"message", error}});
            return true;
        }
        if (res == FileService::WriteResult::error) {
            send_json_response(client_fd, 400, "Bad Request", {{"status", "error"}, {"message", error}});
            return true;
        }
        send_json_response(client_fd, 200, "OK", {{"status", "ok"}, {"path", rel}, {"mtime", out_mtime}});
        return true;
    }

    return false;
}

void handle_static_request(int client_fd, const HttpRequest& req, const std::filesystem::path& static_root) {
    if (req.method != "GET") {
        send_response(client_fd, 405, "Method Not Allowed", "text/plain; charset=utf-8", "Method Not Allowed\n");
        return;
    }

    std::string target = req.target;
    const auto query_pos = target.find('?');
    if (query_pos != std::string::npos) {
        target = target.substr(0, query_pos);
    }

    if (!safe_path_requested(target)) {
        send_response(client_fd, 400, "Bad Request", "text/plain; charset=utf-8", "Bad Request\n");
        return;
    }

    std::filesystem::path relative = target.substr(1);
    if (relative.empty()) {
        relative = "index.html";
    }

    std::filesystem::path requested_path = static_root / relative;

    if (std::filesystem::is_directory(requested_path)) {
        requested_path /= "index.html";
    }

    if (!std::filesystem::exists(requested_path) || !std::filesystem::is_regular_file(requested_path)) {
        const std::filesystem::path fallback = static_root / "index.html";
        if (std::filesystem::exists(fallback) && std::filesystem::is_regular_file(fallback)) {
            requested_path = fallback;
        } else {
            send_response(client_fd, 404, "Not Found", "text/plain; charset=utf-8", "Not Found\n");
            return;
        }
    }

    const std::string body = read_file_to_string(requested_path);
    if (body.empty() && std::filesystem::file_size(requested_path) > 0) {
        send_response(client_fd, 500, "Internal Server Error", "text/plain; charset=utf-8", "Failed to read file\n");
        return;
    }

    send_response(client_fd, 200, "OK", mime_type_for_path(requested_path), body);
}

void handle_client(int client_fd, const std::filesystem::path& static_root) {
    HttpRequest req;
    if (!read_http_request(client_fd, req)) {
        send_response(client_fd, 400, "Bad Request", "text/plain; charset=utf-8", "Bad Request\n");
        return;
    }

    if (handle_api_request(client_fd, req, static_root)) {
        return;
    }

    handle_static_request(client_fd, req, static_root);
}

}  // namespace

int main() {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    const uint16_t port = get_port();
    const std::filesystem::path static_root = get_env_or_default("EDITOR_STATIC_ROOT", "controller/web/client");

    g_file_service.set_root(get_env_or_default("EDITOR_FILE_ROOT", "controller/config/user/editor"));
    {
        std::string init_err;
        if (!g_file_service.init(init_err)) {
            std::cerr << "[editor_service] " << init_err << std::endl;
            return 1;
        }
    }

    std::cout << "[editor_service] static root: " << static_root << std::endl;
    std::cout << "[editor_service] listening on 0.0.0.0:" << port << std::endl;

    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[editor_service] socket creation failed" << std::endl;
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[editor_service] setsockopt failed" << std::endl;
        close(server_fd);
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[editor_service] bind failed" << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 16) < 0) {
        std::cerr << "[editor_service] listen failed" << std::endl;
        close(server_fd);
        return 1;
    }

    // Only publish PID once the socket is bound and listening.
    write_pid_file();

    while (g_keep_running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        if (client_fd < 0) {
            if (g_keep_running) {
                std::cerr << "[editor_service] accept failed" << std::endl;
            }
            continue;
        }

        handle_client(client_fd, static_root);
        close(client_fd);
    }

    close(server_fd);
    remove_pid_file();
    std::cout << "[editor_service] shutting down" << std::endl;

    return 0;
}
