// openrpc-methods.js

module.exports = {
    'Machine_on': (params, userSession) => {
        // Logic for turning the machine on
        console.log(`Machine turned on by ${userSession.username}`);
        return { success: true, message: 'Machine is now on.' };
    },
    'Machine_off': (params, userSession) => {
        // Logic for turning the machine off
        console.log(`Machine turned off by ${userSession.username}`);
        return { success: true, message: 'Machine is now off.' };
    },
    // Add more methods as needed...
};
