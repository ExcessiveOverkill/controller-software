#include "node_factory.h"


class bool_constant: public base_node {
    private:
        bool output_value = false;

    public:

        bool_constant(){
            execution_number = -1;

            outputs.emplace("output", output(io_type::BOOL, &output_value, &execution_number));
        }

        unsigned int run() override {
            return 0;
        }

};

static Node_Registrar<bool_constant> node_registrar_bool_constant("bool_constant");