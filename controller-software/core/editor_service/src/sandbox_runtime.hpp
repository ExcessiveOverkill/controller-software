#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "json.hpp"

class SandboxRuntimeManager {
public:
    using json = nlohmann::json;

    explicit SandboxRuntimeManager(json node_types)
        : node_types_json_(std::move(node_types)) {
        parse_node_types();
        graph_["format"] = "sandbox_graph_v1";
        graph_["nodes"] = json::array();
        graph_["connections"] = json::array();
    }

    ~SandboxRuntimeManager() {
        shutdown();
    }

    void set_update_callback(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        update_callback_ = std::move(callback);
    }

    void start_worker() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (worker_started_) {
            return;
        }
        stop_worker_ = false;
        worker_started_ = true;
        worker_ = std::thread([this]() { worker_loop(); });
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!worker_started_) {
                return;
            }
            stop_worker_ = true;
            run_state_ = RunState::Idle;
        }
        if (worker_.joinable()) {
            worker_.join();
        }
        std::lock_guard<std::mutex> lock(mutex_);
        worker_started_ = false;
    }

    void sync_from_graph(const json& graph, bool clear_state) {
        std::lock_guard<std::mutex> lock(mutex_);
        graph_ = graph;
        graph_revision_ = graph.value("revision", graph_revision_ + 1);
        rebuild_node_index_locked(clear_state);
        topology_dirty_ = true;
        last_rebuild_error_.clear();
        if (clear_state) {
            tick_ = 0;
            run_state_ = RunState::Idle;
            reset_runtime_locked(false);
        }
    }

    bool run(std::string& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (topology_dirty_ && !rebuild_locked(error)) {
            return false;
        }
        run_state_ = RunState::Running;
        return true;
    }

    bool pause(std::string& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (run_state_ != RunState::Running) {
            error = "runtime is not running";
            return false;
        }
        run_state_ = RunState::Paused;
        return true;
    }

    bool reset(std::string& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (run_state_ == RunState::Running) {
            error = "pause runtime before reset";
            return false;
        }
        reset_runtime_locked(true);
        run_state_ = RunState::Idle;
        return true;
    }

    bool rebuild(std::string& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (run_state_ == RunState::Running) {
            error = "pause runtime before rebuilding execution order";
            return false;
        }
        return rebuild_locked(error);
    }

    bool step(std::string& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (run_state_ == RunState::Running) {
            error = "pause runtime before single-step";
            return false;
        }
        if (topology_dirty_ && !rebuild_locked(error)) {
            return false;
        }
        step_locked();
        run_state_ = RunState::Paused;
        return true;
    }

    json status() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return build_status_locked();
    }

    json build_stream_payload(const std::set<std::string>& active_keys,
                              const std::set<std::string>& promoted_keys) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::set<std::string> effective = active_keys.empty() ? default_active_keys_locked() : active_keys;

        json payload;
        payload["type"] = "runtime_update";
        payload["graph"] = graph_;
        payload["runtime"] = runtime_summary_locked();
        payload["runtime"]["nodes"] = json::array();
        payload["runtime"]["active_keys"] = json::array();
        payload["runtime"]["promoted_keys"] = json::array();
        payload["runtime"]["promoted_values"] = json::array();

        for (const auto& key : effective) {
            payload["runtime"]["active_keys"].push_back(key);
        }

        for (const auto& [name, node] : nodes_) {
            json node_payload = base_node_payload_locked(node);
            node_payload["live_values"] = json::object();
            const auto runtime_it = runtime_nodes_.find(name);
            if (runtime_it != runtime_nodes_.end()) {
                for (const auto& [key, value] : runtime_it->second.values) {
                    const std::string full_key = full_live_key(name, key);
                    if (effective.find(full_key) == effective.end()) {
                        continue;
                    }
                    node_payload["live_values"][full_key] = serialize_live_value_locked(name, value);
                }
            }
            payload["runtime"]["nodes"].push_back(node_payload);
        }

        for (const auto& full_key : promoted_keys) {
            auto split = split_live_key(full_key);
            if (!split.has_value()) {
                continue;
            }
            auto node_it = runtime_nodes_.find(split->first);
            if (node_it == runtime_nodes_.end()) {
                continue;
            }
            auto value_it = node_it->second.values.find(split->second);
            if (value_it == node_it->second.values.end()) {
                continue;
            }
            payload["runtime"]["promoted_keys"].push_back(full_key);
            payload["runtime"]["promoted_values"].push_back(serialize_live_value_locked(split->first, value_it->second));
        }

        return payload;
    }

private:
    struct NodePortSchema {
        std::string name;
        std::string type;
    };

    struct NodeTypeSchema {
        std::string type;
        std::vector<NodePortSchema> inputs;
        std::vector<NodePortSchema> outputs;
    };

    struct GraphNodeSnapshot {
        std::string name;
        std::string type;
        json config;
    };

    struct GraphConnectionSnapshot {
        std::string source_node;
        std::string source_port;
        std::string target_node;
        std::string target_port;
    };

    struct RuntimeLiveValue {
        std::string key;
        std::string label;
        std::string category;
        json current_value;
        json previous_value;
        bool changed_this_tick = false;
        uint64_t last_change_tick = 0;
        json pulse_history = json::array();
    };

    struct NodeRuntimeState {
        std::map<std::string, RuntimeLiveValue> values;
        bool edge_last_input = false;
        std::deque<uint32_t> edge_pending_cycles;
        json print_last_value;
    };

    enum class RunState {
        Idle,
        Running,
        Paused,
    };

    mutable std::mutex mutex_;
    json node_types_json_;
    json graph_;
    std::map<std::string, NodeTypeSchema> node_types_;
    std::map<std::string, GraphNodeSnapshot> nodes_;
    std::vector<GraphConnectionSnapshot> connections_;
    std::map<std::string, NodeRuntimeState> runtime_nodes_;
    std::vector<std::string> execution_order_;
    uint64_t tick_ = 0;
    uint64_t graph_revision_ = 0;
    bool topology_dirty_ = true;
    RunState run_state_ = RunState::Idle;
    std::string last_rebuild_error_;
    std::function<void()> update_callback_;
    std::thread worker_;
    bool worker_started_ = false;
    bool stop_worker_ = false;

    static std::string full_live_key(const std::string& node_name, const std::string& local_key) {
        return node_name + "|" + local_key;
    }

    static std::optional<std::pair<std::string, std::string>> split_live_key(const std::string& full_key) {
        const auto sep = full_key.find('|');
        if (sep == std::string::npos || sep == 0 || sep + 1 >= full_key.size()) {
            return std::nullopt;
        }
        return std::make_pair(full_key.substr(0, sep), full_key.substr(sep + 1));
    }

    static std::string lower_copy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            if (c >= 'A' && c <= 'Z') {
                return static_cast<char>(c - 'A' + 'a');
            }
            return static_cast<char>(c);
        });
        return value;
    }

    static json default_value_for_type(const std::string& type, const json& config = json::object()) {
        std::string resolved = type;
        if (resolved == "dynamic" && config.contains("type") && config["type"].is_string()) {
            resolved = config["type"].get<std::string>();
        }
        if (resolved == "bool") {
            return false;
        }
        if (resolved == "float" || resolved == "double") {
            return 0.0;
        }
        if (resolved == "uint32" || resolved == "uint8" || resolved == "int32") {
            return 0;
        }
        return 0;
    }

    static bool json_as_bool(const json& value) {
        if (value.is_boolean()) {
            return value.get<bool>();
        }
        if (value.is_number_integer()) {
            return value.get<int64_t>() != 0;
        }
        if (value.is_number_unsigned()) {
            return value.get<uint64_t>() != 0;
        }
        if (value.is_number_float()) {
            return value.get<double>() != 0.0;
        }
        if (value.is_string()) {
            const std::string text = lower_copy(value.get<std::string>());
            return text == "true" || text == "1" || text == "yes" || text == "on";
        }
        return false;
    }

    static json normalize_dynamic_value(const json& value, const std::string& declared_type) {
        if (declared_type == "bool") {
            return json_as_bool(value);
        }
        if (declared_type == "float" || declared_type == "double") {
            return value.is_number() ? json(value.get<double>()) : json(0.0);
        }
        if (declared_type == "uint32" || declared_type == "uint8") {
            return value.is_number() ? json(static_cast<uint32_t>(value.get<double>())) : json(0);
        }
        if (declared_type == "int32") {
            return value.is_number() ? json(static_cast<int32_t>(value.get<double>())) : json(0);
        }
        return value;
    }

    static std::vector<std::tuple<std::string, std::string, std::string>> live_catalog_for_node(const GraphNodeSnapshot& node,
                                                                                                  const NodeTypeSchema& schema) {
        std::vector<std::tuple<std::string, std::string, std::string>> catalog;
        for (const auto& input : schema.inputs) {
            catalog.emplace_back("in:" + input.name, input.name, "input");
        }
        for (const auto& output : schema.outputs) {
            catalog.emplace_back("out:" + output.name, output.name, "output");
        }
        if (node.type == "edge_delay") {
            catalog.emplace_back("state:pending_count", "pending_count", "state");
            catalog.emplace_back("state:last_edge_match", "last_edge_match", "state");
        }
        if (node.type == "print") {
            catalog.emplace_back("state:last_print", "last_print", "state");
        }
        return catalog;
    }

    static std::set<std::string> default_live_keys_for_node(const GraphNodeSnapshot& node) {
        if (node.type == "constant") {
            return {"out:output"};
        }
        if (node.type == "logic_gate") {
            return {"out:output"};
        }
        if (node.type == "edge_delay") {
            return {"out:output", "state:pending_count"};
        }
        if (node.type == "print") {
            return {"in:input", "state:last_print"};
        }
        return {};
    }

    static std::string run_state_string(RunState state) {
        switch (state) {
            case RunState::Idle: return "idle";
            case RunState::Running: return "running";
            case RunState::Paused: return "paused";
        }
        return "idle";
    }

    void parse_node_types() {
        for (const auto& entry : node_types_json_) {
            if (!entry.contains("type") || !entry["type"].is_string()) {
                continue;
            }
            NodeTypeSchema schema;
            schema.type = entry["type"].get<std::string>();
            if (entry.contains("inputs") && entry["inputs"].is_array()) {
                for (const auto& input : entry["inputs"]) {
                    schema.inputs.push_back({input.value("name", ""), input.value("type", "dynamic")});
                }
            }
            if (entry.contains("outputs") && entry["outputs"].is_array()) {
                for (const auto& output : entry["outputs"]) {
                    schema.outputs.push_back({output.value("name", ""), output.value("type", "dynamic")});
                }
            }
            node_types_[schema.type] = schema;
        }
    }

    void rebuild_node_index_locked(bool clear_state) {
        std::map<std::string, GraphNodeSnapshot> next_nodes;
        std::vector<GraphConnectionSnapshot> next_connections;

        if (graph_.contains("nodes") && graph_["nodes"].is_array()) {
            for (const auto& node : graph_["nodes"]) {
                GraphNodeSnapshot item;
                item.name = node.value("name", "");
                item.type = node.value("type", "");
                item.config = node.contains("config") ? node["config"] : json::object();
                if (!item.name.empty()) {
                    next_nodes[item.name] = item;
                }
            }
        }

        if (graph_.contains("connections") && graph_["connections"].is_array()) {
            for (const auto& connection : graph_["connections"]) {
                GraphConnectionSnapshot item;
                item.source_node = connection.value("src_node", "");
                item.source_port = connection.value("src_port", "");
                item.target_node = connection.value("dst_node", "");
                item.target_port = connection.value("dst_port", "");
                if (!item.source_node.empty() && !item.target_node.empty()) {
                    next_connections.push_back(item);
                }
            }
        }

        nodes_ = std::move(next_nodes);
        connections_ = std::move(next_connections);

        if (clear_state) {
            runtime_nodes_.clear();
        }
        sync_runtime_shapes_locked();
    }

    void sync_runtime_shapes_locked() {
        std::set<std::string> valid_nodes;
        for (const auto& [name, node] : nodes_) {
            valid_nodes.insert(name);
            auto& runtime = runtime_nodes_[name];
            auto schema_it = node_types_.find(node.type);
            if (schema_it == node_types_.end()) {
                continue;
            }
            std::set<std::string> valid_keys;
            for (const auto& [key, label, category] : live_catalog_for_node(node, schema_it->second)) {
                valid_keys.insert(key);
                ensure_live_value_locked(runtime, node, key, label, category);
            }
            for (auto it = runtime.values.begin(); it != runtime.values.end();) {
                if (valid_keys.find(it->first) == valid_keys.end()) {
                    it = runtime.values.erase(it);
                } else {
                    ++it;
                }
            }
        }
        for (auto it = runtime_nodes_.begin(); it != runtime_nodes_.end();) {
            if (valid_nodes.find(it->first) == valid_nodes.end()) {
                it = runtime_nodes_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ensure_live_value_locked(NodeRuntimeState& runtime,
                                  const GraphNodeSnapshot& node,
                                  const std::string& key,
                                  const std::string& label,
                                  const std::string& category) {
        json default_value = default_live_value_locked(node, key);
        auto it = runtime.values.find(key);
        if (it == runtime.values.end()) {
            RuntimeLiveValue value;
            value.key = key;
            value.label = label;
            value.category = category;
            value.current_value = default_value;
            value.previous_value = default_value;
            runtime.values[key] = value;
            return;
        }
        it->second.label = label;
        it->second.category = category;
    }

    json default_live_value_locked(const GraphNodeSnapshot& node, const std::string& key) const {
        auto schema_it = node_types_.find(node.type);
        if (schema_it != node_types_.end()) {
            for (const auto& input : schema_it->second.inputs) {
                if (key == "in:" + input.name) {
                    if (node.type == "print" && input.name == "enable") {
                        return true;
                    }
                    return default_value_for_type(input.type, node.config);
                }
            }
            for (const auto& output : schema_it->second.outputs) {
                if (key == "out:" + output.name) {
                    return default_value_for_type(output.type, node.config);
                }
            }
        }
        if (key == "state:pending_count") {
            return 0;
        }
        if (key == "state:last_edge_match") {
            return false;
        }
        return json();
    }

    bool rebuild_locked(std::string& error) {
        std::map<std::string, size_t> indegree;
        std::map<std::string, std::vector<std::string>> adjacency;
        for (const auto& [name, node] : nodes_) {
            (void)node;
            indegree[name] = 0;
        }
        for (const auto& connection : connections_) {
            adjacency[connection.source_node].push_back(connection.target_node);
            indegree[connection.target_node] += 1;
        }

        std::deque<std::string> ready;
        for (const auto& [name, degree] : indegree) {
            if (degree == 0) {
                ready.push_back(name);
            }
        }

        std::vector<std::string> ordered;
        while (!ready.empty()) {
            std::string current = ready.front();
            ready.pop_front();
            ordered.push_back(current);
            for (const auto& next : adjacency[current]) {
                auto it = indegree.find(next);
                if (it == indegree.end()) {
                    continue;
                }
                if (it->second > 0) {
                    it->second -= 1;
                }
                if (it->second == 0) {
                    ready.push_back(next);
                }
            }
        }

        if (ordered.size() != nodes_.size()) {
            error = "circular dependency detected during execution order calculation";
            last_rebuild_error_ = error;
            return false;
        }

        execution_order_ = std::move(ordered);
        topology_dirty_ = false;
        last_rebuild_error_.clear();
        return true;
    }

    void reset_runtime_locked(bool reset_tick) {
        sync_runtime_shapes_locked();
        if (reset_tick) {
            tick_ = 0;
        }
        for (const auto& [name, node] : nodes_) {
            auto runtime_it = runtime_nodes_.find(name);
            if (runtime_it == runtime_nodes_.end()) {
                continue;
            }
            auto& runtime = runtime_it->second;
            runtime.edge_last_input = false;
            runtime.edge_pending_cycles.clear();
            runtime.print_last_value = json();
            for (auto& [key, value] : runtime.values) {
                value.current_value = default_live_value_locked(node, key);
                value.previous_value = value.current_value;
                value.changed_this_tick = false;
                value.last_change_tick = 0;
                value.pulse_history = json::array();
            }
        }
    }

    void worker_loop() {
        while (true) {
            bool did_step = false;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (stop_worker_) {
                    return;
                }
                if (run_state_ == RunState::Running) {
                    std::string error;
                    if (topology_dirty_ && !rebuild_locked(error)) {
                        run_state_ = RunState::Paused;
                    } else {
                        step_locked();
                        did_step = true;
                    }
                }
            }
            if (did_step) {
                emit_update();
                std::this_thread::sleep_for(std::chrono::milliseconds(60));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    }

    void emit_update() {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback = update_callback_;
        }
        if (callback) {
            callback();
        }
    }

    RuntimeLiveValue* live_value_locked(const std::string& node_name, const std::string& key) {
        auto node_it = runtime_nodes_.find(node_name);
        if (node_it == runtime_nodes_.end()) {
            return nullptr;
        }
        auto value_it = node_it->second.values.find(key);
        if (value_it == node_it->second.values.end()) {
            return nullptr;
        }
        return &value_it->second;
    }

    void set_live_value_locked(const std::string& node_name, const std::string& key, const json& value, bool pulse = false) {
        RuntimeLiveValue* live = live_value_locked(node_name, key);
        if (live == nullptr) {
            return;
        }
        live->changed_this_tick = (live->current_value != value);
        if (live->changed_this_tick) {
            live->previous_value = live->current_value;
            live->current_value = value;
            live->last_change_tick = tick_;
            if (pulse) {
                live->pulse_history.push_back({{"tick", tick_}, {"value", value}});
                while (live->pulse_history.size() > 12) {
                    live->pulse_history.erase(live->pulse_history.begin());
                }
            }
        } else {
            live->current_value = value;
        }
    }

    json resolve_input_locked(const GraphNodeSnapshot& node, const std::string& input_name) const {
        for (const auto& connection : connections_) {
            if (connection.target_node == node.name && connection.target_port == input_name) {
                auto runtime_it = runtime_nodes_.find(connection.source_node);
                if (runtime_it == runtime_nodes_.end()) {
                    break;
                }
                auto value_it = runtime_it->second.values.find("out:" + connection.source_port);
                if (value_it != runtime_it->second.values.end()) {
                    return value_it->second.current_value;
                }
            }
        }
        if (node.type == "print" && input_name == "enable") {
            return true;
        }
        auto schema_it = node_types_.find(node.type);
        if (schema_it != node_types_.end()) {
            for (const auto& input : schema_it->second.inputs) {
                if (input.name == input_name) {
                    return default_value_for_type(input.type, node.config);
                }
            }
        }
        return false;
    }

    void clear_tick_flags_locked() {
        for (auto& [name, runtime] : runtime_nodes_) {
            (void)name;
            for (auto& [key, value] : runtime.values) {
                (void)key;
                value.changed_this_tick = false;
            }
        }
    }

    void step_locked() {
        clear_tick_flags_locked();
        ++tick_;

        for (const auto& node_name : execution_order_) {
            auto node_it = nodes_.find(node_name);
            if (node_it == nodes_.end()) {
                continue;
            }
            auto runtime_it = runtime_nodes_.find(node_name);
            if (runtime_it == runtime_nodes_.end()) {
                continue;
            }

            const auto& node = node_it->second;
            auto& runtime = runtime_it->second;

            if (node.type == "constant") {
                const std::string declared_type = node.config.value("type", std::string("double"));
                const json raw_value = node.config.contains("value") ? node.config["value"] : default_value_for_type(declared_type, node.config);
                set_live_value_locked(node_name, "out:output", normalize_dynamic_value(raw_value, declared_type));
                continue;
            }

            if (node.type == "logic_gate") {
                const bool a = json_as_bool(resolve_input_locked(node, "input_0"));
                const bool b = json_as_bool(resolve_input_locked(node, "input_1"));
                set_live_value_locked(node_name, "in:input_0", a);
                set_live_value_locked(node_name, "in:input_1", b);
                const std::string gate = lower_copy(node.config.value("gate_type", std::string("and")));
                bool result = false;
                if (gate == "or") result = a || b;
                else if (gate == "not") result = !a;
                else if (gate == "nand") result = !(a && b);
                else if (gate == "nor") result = !(a || b);
                else if (gate == "xor") result = (a != b);
                else if (gate == "xnor") result = (a == b);
                else result = a && b;
                set_live_value_locked(node_name, "out:output", result);
                continue;
            }

            if (node.type == "edge_delay") {
                const bool input_value = json_as_bool(resolve_input_locked(node, "input"));
                const bool rising_edge = node.config.value("rising_edge", true);
                const bool edge_match = rising_edge ? (!runtime.edge_last_input && input_value)
                                                    : (runtime.edge_last_input && !input_value);
                runtime.edge_last_input = input_value;
                set_live_value_locked(node_name, "in:input", input_value);
                set_live_value_locked(node_name, "state:last_edge_match", edge_match);
                if (edge_match) {
                    runtime.edge_pending_cycles.push_back(static_cast<uint32_t>(node.config.value("cycles", 0)));
                }
                bool pulse = false;
                for (auto it = runtime.edge_pending_cycles.begin(); it != runtime.edge_pending_cycles.end();) {
                    if (*it == 0) {
                        pulse = true;
                        it = runtime.edge_pending_cycles.erase(it);
                    } else {
                        *it -= 1;
                        ++it;
                    }
                }
                set_live_value_locked(node_name, "state:pending_count", static_cast<uint32_t>(runtime.edge_pending_cycles.size()));
                set_live_value_locked(node_name, "out:output", pulse, pulse);
                continue;
            }

            if (node.type == "print") {
                const json input_value = resolve_input_locked(node, "input");
                const bool enabled = json_as_bool(resolve_input_locked(node, "enable"));
                set_live_value_locked(node_name, "in:input", input_value);
                set_live_value_locked(node_name, "in:enable", enabled);
                if (enabled) {
                    runtime.print_last_value = input_value;
                    set_live_value_locked(node_name, "state:last_print", input_value);
                }
                continue;
            }
        }
    }

    json runtime_summary_locked() const {
        json runtime;
        runtime["state"] = run_state_string(run_state_);
        runtime["tick"] = tick_;
        runtime["topology_dirty"] = topology_dirty_;
        runtime["execution_order"] = execution_order_;
        runtime["last_rebuild_error"] = last_rebuild_error_;
        runtime["stream_path"] = "/ws/runtime";
        runtime["graph_revision"] = graph_revision_;
        return runtime;
    }

    json base_node_payload_locked(const GraphNodeSnapshot& node) const {
        json payload;
        payload["name"] = node.name;
        payload["type"] = node.type;
        payload["config"] = node.config;
        payload["default_live_keys"] = json::array();
        payload["available_live"] = json::array();
        auto schema_it = node_types_.find(node.type);
        if (schema_it != node_types_.end()) {
            for (const auto& [key, label, category] : live_catalog_for_node(node, schema_it->second)) {
                payload["available_live"].push_back({
                    {"key", full_live_key(node.name, key)},
                    {"local_key", key},
                    {"label", label},
                    {"category", category}
                });
            }
        }
        for (const auto& local_key : default_live_keys_for_node(node)) {
            payload["default_live_keys"].push_back(full_live_key(node.name, local_key));
        }
        return payload;
    }

    json serialize_live_value_locked(const std::string& node_name, const RuntimeLiveValue& value) const {
        return {
            {"node", node_name},
            {"key", value.key},
            {"label", value.label},
            {"category", value.category},
            {"current", value.current_value},
            {"previous", value.previous_value},
            {"changed_this_tick", value.changed_this_tick},
            {"last_change_tick", value.last_change_tick},
            {"pulse_history", value.pulse_history}
        };
    }

    std::set<std::string> default_active_keys_locked() const {
        std::set<std::string> keys;
        for (const auto& [name, node] : nodes_) {
            for (const auto& key : default_live_keys_for_node(node)) {
                keys.insert(full_live_key(name, key));
            }
        }
        return keys;
    }

    json build_status_locked() const {
        json out;
        out["graph"] = graph_;
        out["runtime"] = runtime_summary_locked();
        out["runtime"]["nodes"] = json::array();
        for (const auto& [name, node] : nodes_) {
            json payload = base_node_payload_locked(node);
            payload["live_values"] = json::object();
            auto runtime_it = runtime_nodes_.find(name);
            if (runtime_it != runtime_nodes_.end()) {
                for (const auto& [key, value] : runtime_it->second.values) {
                    payload["live_values"][full_live_key(name, key)] = serialize_live_value_locked(name, value);
                }
            }
            out["runtime"]["nodes"].push_back(payload);
        }
        return out;
    }
};
