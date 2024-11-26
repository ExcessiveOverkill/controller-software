#include "generic_nodes/logic_not.h"
#include "generic_nodes/bool_constant.h"
#include "generic_nodes/logic_and.h"
#include "generic_nodes/bool_print_cout.h"
#include "generic_nodes/nothing_delay.h"
#include <pthread.h>
#include <sched.h>
#include <ctime>
#include <cerrno>


class node_network {
    public:
        enum update_type{
            SYNC,
            ASYNC
        };
    private:
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


        static void* sync_run(void* args_){
            thread_args* args = static_cast<thread_args*>(args_);

            // TODO: add threading support (if nodes have same execution number they may be run in parallel)
            for (auto& node : *(args->execution_order_)) {
                if(node->run() !=0){
                    std::cerr << "Error running node" << std::endl;
                    
                    // TODO: figure out if we want to just ride through or stop on error
                    *args->return_code = 1;
                    return nullptr;
                }
            }

            *args->return_code = 0;
            return nullptr;
        }

        // for now this is the same as sync_run(), but may later be modified separately
        static void* async_run(void* args_){
            thread_args* args = static_cast<thread_args*>(args_);

            // TODO: add threading support (if nodes have same execution number they may be run in parallel)
            for (auto& node : *(args->execution_order_)) {
                if(node->run() !=0){
                    std::cerr << "Error running node" << std::endl;
                    
                    // TODO: figure out if we want to just ride through or stop on error
                    *args->return_code = 1;
                    return nullptr;
                }
            }

            *args->return_code = 0;
            return nullptr;
        }

        void set_thread_priority(pthread_t* thread, int priority, bool ignore_no_such_process = false){
            struct sched_param param;
            param.sched_priority = priority;

            // Set the scheduling policy and priority of the thread
            int ret = pthread_setschedparam(*thread, SCHED_RR, &param);
            if (ret == ESRCH && ignore_no_such_process) {
                return;
            }
            if (ret != 0) {
                std::cerr << "Failed to set thread priority: " << strerror(ret) << std::endl;
            }
        }
        
    public:
        std::map<std::string, std::shared_ptr<base_node>> nodes;
        
        node_network(){
            thread_args_.return_code = &thread_return_code;
            thread_args_.execution_order_ = &execution_order;
        }

        uint32_t get_node_count(){
            return nodes.size();
        }

        uint32_t set_type(update_type type_){
            if(thread_running){
                std::cerr << "Error: cannot connect nodes while network is running" << std::endl;
                return 4;   // cannot connect nodes while network is running
            }
            config.type = type_;
            return 0;
        }

        uint32_t set_update_cycle_trigger_count(uint32_t count){
            config.update_cycle_trigger_count = count;
            return 0;
        }

        uint32_t set_timeout_sec(uint32_t seconds){
            config.timeout_s = seconds;
            config.timeout_ns = 0;
            return 0;
        }

        uint32_t set_timeout_msec(uint32_t milliseconds){
            config.timeout_s = milliseconds / 1000;
            config.timeout_ns = milliseconds % 1000 * 1000000;
            assert(config.timeout_ns < 1000000000);    // nanoseconds should be less than 1 second
            return 0;
        }

        uint32_t set_timeout_usec(uint32_t microseconds){
            config.timeout_s = microseconds / 1000000;
            config.timeout_ns = microseconds % 1000000 * 1000;
            assert(config.timeout_ns < 1000000000);    // nanoseconds should be less than 1 second
            return 0;
        }

        uint32_t set_sync_thread_priority(int priority){
            sync_thread_priority = priority;
            return 0;
        }

        uint32_t set_async_thread_priority(int priority){
            async_thread_priority = priority;
            return 0;
        }

        uint32_t set_async_allowed_late_cycles(uint32_t cycles){
            config.async_allowed_late_cycles = cycles;
            return 0;
        }

        uint32_t set_enable(bool enable){
            config.enable = enable;
            return 0;
        }

        uint32_t set_execution_order(uint32_t order){
            config.execution_order = order;
            return 0;
        }

        uint32_t get_execution_order(){
            return config.execution_order;
        }
        
        bool config_allowed(){
            return !thread_running && !config.enable;
        }

        uint32_t add_node(std::string type, std::string name){ // add a node to the network
            if(thread_running){
                std::cerr << "Error: cannot add node while network is running" << std::endl;
                return 4;
            }

            if(nodes.find(name) != nodes.end()){
                std::cerr << "Error: node name already exists" << std::endl;
                return 1;   // node name already exists
            }
            std::shared_ptr<base_node> node;

            try{
                node = Node_Factory::create_shared(type);
            }
            catch(const std::runtime_error& e){
                std::cerr << "Error: failed to create node with factory" << std::endl;
                return 2;
            }
            
            nodes.emplace(name, node);
            
            return 0;
        }

        uint32_t remove_node(std::string name){ // remove a node from the network
            if(thread_running){
                std::cerr << "Error: cannot remove node while network is running" << std::endl;
                return 4;
            }

            if(nodes.find(name) == nodes.end()){
                std::cerr << "Error: node name not found" << std::endl;
                return 1;   // node name not found
            }

            // mark selected node for deletion
            nodes[name]->mark_for_deletion();

            // update all nodes to no longer reference nodes marked for deletion
            for(auto& pair : nodes){
                pair.second->reconnect_inputs();
            }

            // erase node object
            nodes.erase(name);

            execution_order_update_required = true;
            return 0;
        }

        uint32_t connect_nodes(std::string source_node_name, std::string source_output_name, std::string target_node_name, std::string target_input_name){ // connect an output to an input
            if(thread_running){
                std::cerr << "Error: cannot connect nodes while network is running" << std::endl;
                return 4;   // cannot connect nodes while network is running
            }
            if(nodes.find(source_node_name) == nodes.end()){
                std::cerr << "Error: source node name not found" << std::endl;
                return 1;   // source node name not found
            }

            if(nodes.find(target_node_name) == nodes.end()){
                std::cerr << "Error: target node name not found" << std::endl;
                return 2;   // target node name not found
            }

            if(nodes[target_node_name]->connect_input(target_input_name, nodes[source_node_name], source_output_name) != 0){
                std::cerr << "Error: connecting input" << std::endl;
                return 3;   // error connecting input
            }

            execution_order_update_required = true;

            return 0;
        }

        uint32_t disconnect_node(std::string target_node_name, std::string target_input_name){ // disconnect an input
            if(thread_running){
                std::cerr << "Error: cannot disconnect node while network is running" << std::endl;
                return 4;
            }

            if(nodes.find(target_node_name) == nodes.end()){
                std::cerr << "Error: target node name not found" << std::endl;
                return 1;   // target node name not found
            }

            if(nodes[target_node_name]->disconnect_input(target_input_name) != 0){
                std::cerr << "Error: disconnecting input" << std::endl;
                return 2;   // error disconnecting input
            }

            execution_order_update_required = true;

            return 0;
        }

        uint32_t rebuild_execution_order(){

            if(thread_running){
                std::cerr << "Error: cannot rebuild node execution order while network is running" << std::endl;
                return 4;
            }

            execution_order.clear();

            // reset all execution numbers except output-only nodes
            bool output_only_exists = false;
            for (auto& pair : nodes) {
                if(pair.second->execution_number > -1){
                    pair.second->execution_number = 0;
                }
                else{
                    output_only_exists = true;
                }
            }

            if(!output_only_exists){
                std::cerr << "Error: no output-only nodes found" << std::endl;
                return 1;   // no output-only nodes, at least one node must exist
            }

            bool execution_number_modified = false;  // if any node changes its execution number, this will be set to true
            int max_execution_number = -256;

            for(int i=0; i<nodes.size()+1; i++){
                execution_number_modified = false;

                for (auto& pair : nodes) {
                    if(pair.second->execution_number == -1){   // output-only nodes do not need updated
                        continue;
                    }

                    pair.second->configure_execution_number(&execution_number_modified);

                    if(pair.second->execution_number > max_execution_number){
                        max_execution_number = pair.second->execution_number;
                    }
                }

                if(!execution_number_modified){
                    break;  // updating complete
                }
                
                if(i == nodes.size()){
                    std::cerr << "Error: circular dependency detected during execution order calculation" << std::endl;
                    return 2;   // execution number update looped too many times, this means there is a circular dependency
                }
            }

            // build execution order    (any numbers below -1 will not get run)
            for(int i=-1; i<max_execution_number+1; i++){
                for (auto& pair : nodes) {
                    if(pair.second->execution_number == i){
                        execution_order.push_back(pair.second);
                    }
                }
            }

            execution_order_update_required = false;

            return 0;
        }

        uint32_t run(const uint32_t* cycle_count){

            // check if it is time to run
            if((int)(*cycle_count - next_update_cycle) >= 0 && config.enable){
                next_update_cycle += config.update_cycle_trigger_count;
            }
            else{
                return 0;   // not time to run
            }

            if(execution_order_update_required){
                std::cerr << "Error: execution order not updated" << std::endl;
                return 1;   // execution order not updated
            }

            if(config.type == SYNC){

                // run nodes in separate thread
                pthread_create(&thread, nullptr, sync_run, &thread_args_);

                // set thread to high priority (still less than this thread)
                set_thread_priority(&thread, sync_thread_priority, true);

                thread_running = true;

                // wait for thread to finish with timeout
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += config.timeout_s;
                ts.tv_nsec += config.timeout_ns;

                int ret = pthread_timedjoin_np(thread, nullptr, &ts);
                if(ret == 0){
                    // thread finished normally
                    thread_running = false;
                    return 0;
                }

                // TODO: network state should be reset if any of the following errors occur
                else if(ret == ETIMEDOUT){
                    // thread timed out, force close thread
                    force_stop();

                    std::cerr << "Error: network did not finish running withing the timeout period, forcefully stopped" << std::endl;
                    thread_running = false;
                    return 2;   // thread timed out
                }
                else{
                    // thread error
                    std::cerr << "Error: unknown thread error" << std::endl;
                    thread_running = false;
                    return 3;   // thread error
                }
                
            }
            else if(config.type == ASYNC){
                // run nodes in a separate lower priority thread

                // attempt to join thread (should have finished from previous update cycle)
                if(thread_running){
                    int ret = pthread_tryjoin_np(thread, nullptr);
                    if(ret == 0){
                        // thread finished normally
                        thread_running = false;
                        late_cycles = 0;
                    }
                    else if(ret == EBUSY){
                        // thread still running
                        if(config.async_allowed_late_cycles > late_cycles){
                            // allow late update
                            late_cycles++;

                            // TODO: this should probably trigger some sort of warning
                            std::cerr << "Warning: network did not finish withing the set cycle" << std::endl;
                            return 0;
                        }
                        else{
                            // too many cyclec late, force close thread
                            force_stop();

                            std::cerr << "Error: thread did not finish running within allowed late cycles, forcefully stopped" << std::endl;
                            thread_running = false;
                            return 4;   // thread still running
                        }

                        return 4;   // thread still running
                    }
                    else{
                        // thread error
                        std::cerr << "Error: unknown thread error" << std::endl;
                        return 5;   // thread error
                    }
                }
                else{   // likely means this is the first run so we havent started the thread yet
                    // nothing special happens here, just start the new thread
                }

                // run nodes in separate thread
                pthread_create(&thread, nullptr, async_run, &thread_args_);

                // set thread to low priority (less than sync threads)
                set_thread_priority(&thread, async_thread_priority, true);

                thread_running = true;
                return 0;
            }
            else{
                std::cerr << "Error: unknown update type" << std::endl;
                return 6;   // unknown update type
            }

            return 0;
        }

        uint32_t force_stop(){
            // immediately stop thread
            pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
            pthread_cancel(thread);
            pthread_join(thread, nullptr);
            thread_running = false;
            return 0;
        }

        ~node_network(){

            // no need to do individual deletion marking since every node will be deleted
            // for (auto& pair : nodes) {
            //     delete pair.second;
            // }
        }
};