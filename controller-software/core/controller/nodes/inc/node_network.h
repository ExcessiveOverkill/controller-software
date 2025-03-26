#include <pthread.h>
#include <sched.h>
#include <ctime>
#include <cerrno>

//#include "logic_not.h"
//#include "bool_constant.h"
//#include "logic_and.h"
//#include "bool_print_cout.h"
//#include "nothing_delay.h"

#ifndef NODE_NETWORK_H
#define NODE_NETWORK_H

//#include "print_float.h"

// #include "api_cmd_float.h"
// #include "convert_float_to_bool.h"
// #include "set_global_variable.h"
// #include "em_serial_device.h"
// #include "float_print_cout.h"

#include "node_factory.h"
#include "base_node.h"

class node_network {
    public:
        enum update_type{
            SYNC,
            ASYNC
        };
    private:
        std::map<std::string, std::shared_ptr<global_variable>>* global_variables = nullptr;    // pointer to global variables stored in node_core

        std::vector<std::shared_ptr<base_node>> execution_order;

        bool execution_order_update_required = true;

        uint32_t next_update_cycle = 0;    // last cycle the network was updated

        pthread_t thread;
        uint32_t thread_return_code = 0;

        int sync_thread_priority = 20;   // should be set just below the main thread
        int async_thread_priority = 19;  // should be dynamically set based on how soon the network needs to complete (lower frerquency networks can have lower priority)

        uint32_t late_cycles = 0;
        
        bool thread_running = false;

        struct timespec ts;

        struct config_settings {
            bool enable = false;

            uint32_t update_cycle_trigger_count = 0;    // number of cycles before update is triggered (0 = every cycle)
            
            update_type type = ASYNC;   // sync or async

            uint32_t timeout_s = 0; // timeout in seconds
            uint32_t timeout_ns = 0;    // timeout in nanoseconds

            uint32_t async_allowed_late_cycles = 0;   // number of cycles the async thread is allowed to be late before forcefully stopped and restarted
        
            uint32_t execution_order = 0;   // order of network, not used internally
        } config;

        struct thread_args {
            uint32_t* return_code;
            std::vector<std::shared_ptr<base_node>>* execution_order_;
        } thread_args_;


        static void* sync_run(void* args_);

        // for now this is the same as sync_run(), but may later be modified separately
        static void* async_run(void* args_);

        void set_thread_priority(pthread_t* thread, int priority, bool ignore_no_such_process = false);
        
    public:
        std::map<std::string, std::shared_ptr<base_node>> nodes;
        
        node_network(std::map<std::string, std::shared_ptr<global_variable>>* global_variables_);

        uint32_t get_node_count();

        uint32_t set_type(update_type type_);

        uint32_t set_update_cycle_trigger_count(uint32_t count);

        uint32_t set_timeout_sec(uint32_t seconds);

        uint32_t set_timeout_msec(uint32_t milliseconds);

        uint32_t set_timeout_usec(uint32_t microseconds);

        uint32_t set_sync_thread_priority(int priority);

        uint32_t set_async_thread_priority(int priority);

        uint32_t set_async_allowed_late_cycles(uint32_t cycles);

        uint32_t set_enable(bool enable);

        uint32_t set_execution_order(uint32_t order);

        uint32_t get_execution_order();
        
        bool config_allowed();

        uint32_t add_node(std::string type, std::string name);

        uint32_t remove_node(std::string name);

        uint32_t configure_node(std::string node_name, json* j);

        uint32_t connect_nodes(std::string source_node_name, std::string source_output_name, std::string target_node_name, std::string target_input_name);

        //uint32_t connect_varaible_get(std::string target_node_name);    // only for get_global_variable nodes

        //uint32_t connect_varaible_set(std::string target_node_name);    // only for set_global_variable nodes

        uint32_t disconnect_node(std::string target_node_name, std::string target_input_name);

        uint32_t rebuild_execution_order();

        uint32_t run(const uint32_t* cycle_count);

        uint32_t force_stop();

        ~node_network();
};

#endif