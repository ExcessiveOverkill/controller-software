const express = require('express');
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');
const selfsigned = require('selfsigned');
const controllerAPI = require('./build/Release/controller_API');


// Import the methods from openrpc-methods.js
const methodHandlers = require('./openrpc-methods');
const { exit } = require('process');

// Generate self-signed certificate for WebSocket
const attrs = [{ name: 'commonName', value: 'localhost' }];
const pems = selfsigned.generate(attrs, { days: 3650 }); // Cert valid for 10 years

// SSL credentials using the self-signed certificate for WebSocket
const credentials = {
    key: pems.private,
    cert: pems.cert,
};

const app = express();

// Serve the HTML file using HTTPS
app.use(express.static(path.join(__dirname, 'public')));

app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Create an HTTPS server for both website and WebSocket (wss://)
const httpsServer = https.createServer(credentials, app);

// Create a WebSocket server (wss://)
const wss = new WebSocket.Server({ server: httpsServer });

// In-memory storage of WebSocket connections and their associated users
const userSessions = new Map(); // Map: ws => { username, permissions }

// Load user credentials and permissions from file
const usersFilePath = path.join(__dirname, 'users.json');
let usersData = {};

// Load users from the JSON file
function loadUsers() {
    try {
        const data = fs.readFileSync(usersFilePath, 'utf8');
        usersData = JSON.parse(data);
        console.log('Users loaded from file:', usersData);
    } catch (error) {
        console.error('Error reading users file:', error);
    }
}

// load users initially
loadUsers();

// maps to track pending calls
const pendingCalls = new Map();
const wsPendingCalls = new Map();

// handle API calls
wss.on('connection', (ws) => {
    console.log('Client connected');

    ws.on('message', (message) => {
        console.log(`Received message: ${message}`);
        
        // Parse the JSON message
        let parsedMessage;
        try {
            parsedMessage = JSON.parse(message);
        } catch (e) {
            console.error('Invalid JSON:', message);
            ws.send(JSON.stringify({ error: 'Invalid JSON' }));
            return;
        }

        const { method, params, id } = parsedMessage;
        const user_call_id = id;

        // Handle login
        if (method === 'Login') {
            const [username, password] = params;
            
            // Check if the user exists in the usersData
            const user = usersData[username];
            if (user && user.password === password) {
                // Store the user session for this WebSocket
                userSessions.set(ws, { username, permissions: user.allowedFunctions });
                
                // Respond with success
                ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    id: user_call_id,
                    result: { success: true }
                }));
            } else {
                // Respond with login failure
                ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    id: user_call_id,
                    result: { success: false, error: 'Invalid credentials' }
                }));
            }
            return;
        }

        // Check if the connection is authenticated
        const userSession = userSessions.get(ws);
        if (!userSession) {
            // not authenticated, reject the request
            ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id: user_call_id,
                error: { code: 401, message: 'Unauthorized' }
            }));
            return;
        }

        // Handle logout
        if (method === 'Logout') {
            // Handle user logout
            userSessions.delete(ws);
            ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id: user_call_id,
                result: { success: true, message: 'Logged out successfully' }
            }));
            ws.close();
        }

        // Check if the method exists and the user has permission to execute it
        if (methodHandlers[method] && userSession.permissions.includes(method)) {
            try {
                const result = methodHandlers[method](params, userSession);
                if (result.immediate == true) {  // Check if the method returned an immediate result
                    // immediate response
                    ws.send(JSON.stringify({
                        jsonrpc: '2.0',
                        id: user_call_id,
                        result
                    }));
                }
                else {  // Method requires asynchronous processing
                    // add call to controller (actually written to controller later)
                    const controller_call_id = controllerAPI.api_call(result.call_data);

                    // Store the pending call
                    pendingCalls.set(controller_call_id, { ws, userSession, user_call_id });
                    // Also store in wsPendingCalls for cleanup on disconnection
                    let wsCalls = wsPendingCalls.get(ws);
                    if (!wsCalls) {
                        wsCalls = new Set();
                        wsPendingCalls.set(ws, wsCalls);
                    }
                    wsCalls.add(controller_call_id);
                }
            } catch (error) {
                ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    id: user_call_id,
                    error: { code: 500, message: `Error executing method: ${error.message}` }
                }));
            }
        } else {
            ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id: user_call_id,
                error: { code: 32601, message: 'Method not found or insufficient permissions' }
            }));
        }
    });

    // handle websocket close
    ws.on('close', () => {
        // Remove user session when WebSocket is closed
        const userSession = userSessions.get(ws);
        if (userSession) {
            console.log(`User ${userSession.username} disconnected`);
            userSessions.delete(ws); // Remove the user session from the map
        }

        // Remove any pending calls associated with this WebSocket (calls will still be processed but no response will be sent)
        const wsCalls = wsPendingCalls.get(ws);
        if (wsCalls) {
            for (const id of wsCalls) {
                pendingCalls.delete(id);
            }
            wsPendingCalls.delete(ws);
        }
        console.log('Client disconnected');
    });
});

// Handle pending calls
function processCompletedCalls() {
    // send new calls
    controllerAPI.send_data();

    // Get completed calls
    const completedCalls = controllerAPI.get_responses();
    if (completedCalls == null){    // No completed calls
        return;
    }
    console.log('Completed calls:', completedCalls);
    for (const controller_call_id in completedCalls) {
        const result = completedCalls[controller_call_id];
        const pendingCall = pendingCalls.get(controller_call_id);
        if (pendingCall) {
            const { ws, user_call_id } = pendingCall;
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    id: user_call_id,
                    result
                }));
            }
            // Clean up
            pendingCalls.delete(controller_call_id);
            const wsCalls = wsPendingCalls.get(ws);
            if (wsCalls) {
                wsCalls.delete(controller_call_id);
                if (wsCalls.size === 0) {
                    wsPendingCalls.delete(ws);
                }
            }
        }
    }
}
setInterval(processCompletedCalls, 100);    // Interval to check for completed calls and send responses



// Define the HTTPS port
const HTTPS_PORT = process.env.HTTPS_PORT || 443;

// Start the HTTPS server (serving website and WebSocket)
httpsServer.listen(HTTPS_PORT, () => {
    console.log(`Server is listening on https://localhost:${HTTPS_PORT}`);
});

// Create an HTTP server to redirect to HTTPS
const httpApp = express();

httpApp.use((req, res) => {
    res.redirect(`https://${req.headers.host}${req.url}`);
});

// Define the HTTP port
const HTTP_PORT = process.env.HTTP_PORT || 80;

// Start the HTTP server
http.createServer(httpApp).listen(HTTP_PORT, () => {
    console.log(`HTTP server is listening on http://localhost:${HTTP_PORT} and redirecting to HTTPS`);
});
