
#pragma once
#include "base_node.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>

class Node_Factory {
public:
    using Creator = std::function<std::shared_ptr<base_node>()>;

    // Registers a creator function with a string key
    static void registerType(const std::string& typeName, Creator creator) {
        auto& map = getMap();
        if (map.find(typeName) != map.end()) {
            throw std::runtime_error("Type already registered: " + typeName);
        }
        map[typeName] = creator;
    }

    // Creates an object based on the string key
    static std::shared_ptr<base_node> create_shared(const std::string& typeName) {
        auto& map = getMap();
        auto it = map.find(typeName);
        if (it != map.end()) {
            return (it->second)();
        }
        throw std::runtime_error("Type not registered: " + typeName);
    }

private:
    // Returns the static map
    static std::unordered_map<std::string, Creator>& getMap() {
        static std::unordered_map<std::string, Creator> map;
        return map;
    }
};

// Template class to register a type with the factory
template <typename T>
class Node_Registrar {
public:
    Node_Registrar(const std::string& typeName) {
        Node_Factory::registerType(typeName, []() -> std::shared_ptr<base_node> {
            return std::make_shared<T>();
        });
    }
};
