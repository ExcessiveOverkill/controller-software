#include "api_drivers/controller_api.h"
#include "nodes/node_core.h"
#include <iostream>


int main() {

    // startup internal API
    // controller_api api;

    node_network* net = new node_network();

    net->set_type(node_network::update_type::ASYNC);
    net->set_timeout_usec(100);
    //net->set_timeout_sec(2);

    unsigned int cycle = 0;

    net->add_node(new bool_constant(), "constant");
    net->add_node(new logic_not(), "not_0");
    net->add_node(new logic_not(), "not_1");
    net->add_node(new logic_not(), "not_2");
    net->add_node(new logic_not(), "not_3");
    net->add_node(new logic_and(), "and_0");
    net->add_node(new bool_print_cout(), "bool_print");
    net->add_node(new nothing_delay(), "delay_0");

    output* out = nullptr;

    net->connect_nodes("constant", "output", "delay_0", "input");
    net->connect_nodes("delay_0", "output", "not_0", "input");
    net->connect_nodes("not_0", "output", "not_1", "input");
    net->connect_nodes("not_1", "output", "not_2", "input");
    net->connect_nodes("not_2", "output", "bool_print", "input");

    //net->nodes["not_2"]->get_output_object("output", out);

    //net->nodes[1]->connect_input("input", net->nodes[4], "output"); // connect first not to fourth not (creates circular dependency)

    //std::cout << "output: " << *reinterpret_cast<bool*>(out->data_pointer) << std::endl;

    net->rebuild_execution_order();    
    net->run(&cycle);
    cycle++;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    net->run(&cycle);
    cycle++;
    
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // net->connect_nodes("not_2", "output", "and_0", "input_A");
    // net->connect_nodes("not_0", "output", "and_0", "input_B");
    // net->connect_nodes("and_0", "output", "bool_print", "input");

    // net->rebuild_execution_order();
    net->run(&cycle);



}