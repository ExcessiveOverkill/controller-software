#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>
#include "json.hpp"

#pragma once
#include "api_objects.h"

using json = nlohmann::json;

class controller_api {
    private:
        std::vector<std::shared_ptr<Base_API>> calls;
        shared_mem shared_mem_obj;
        Base_API default_call_obj;
        api_call_id_t api_call_id = api_call_id_t::DEFAULT;

        uint32_t get_next_api_call_id_from_shared_mem();

        uint32_t get_last_web_to_controller_call();

        
        static std::mutex data_mutex;
        static std::condition_variable data_ready;
        static std::string* json_to_parse;
        static json* parsed_json;
        static bool* json_parsed;
        static std::atomic<bool> parsing_complete;
        static void json_parser_thread();

        void setup_json_parser();

        Node_Core* node_core = nullptr;
        
    public:
        controller_api();

        void setup(Node_Core* node_core_);

        uint32_t get_new_call();

        uint32_t run_calls();

        ~controller_api();
};



