#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>

#pragma once
#include "api_objects.h"

class controller_api {
    private:
        std::vector<std::shared_ptr<Base_API>> calls;
        shared_mem shared_mem_obj;
        Base_API default_call_obj;
        api_call_id_t api_call_id = api_call_id_t::DEFAULT;

        uint32_t get_next_api_call_id_from_shared_mem();

        uint32_t get_last_web_to_controller_call();

        std::atomic<bool>* pause = nullptr;
        
    public:
        static std::mutex controller_api_mtx;

        controller_api();

        uint32_t get_new_call();

        uint32_t run_calls();

        void set_pause_flag(std::atomic<bool>* pause_);

        ~controller_api();
};

