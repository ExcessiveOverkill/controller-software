// openrpc-methods.js

module.exports = {
    // 'Machine_on': (params, userSession) => {
    //     // Logic for turning the machine on
    //     console.log(`Machine turned on by ${userSession.username}`);
    //     return { success: true, message: 'Machine is now on.' };
    // },
    // 'Machine_off': (params, userSession) => {
    //     // Logic for turning the machine off
    //     console.log(`Machine turned off by ${userSession.username}`);
    //     return { success: true, message: 'Machine is now off.' };
    // },
    'print_uint32': (params, userSession) => {
        // Logic for printing a uint32 value
        console.log(`Printing uint32 value ${params[0]} by ${userSession.username}`);
        return { 
            immediate: false,
            call_data: {call_name: "print_uint32", value: params[0]},
        };
    }
};
