const express = require('express');
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');
const selfsigned = require('selfsigned');
const controllerAPI = require('./build/Release/controller_API');

console.log('controller_API:', controllerAPI.api_call("test"));

// Import the methods from openrpc-methods.js
const methodHandlers = require('./openrpc-methods');

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

// Call the function to load users initially
loadUsers();

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
                    id,
                    result: { success: true }
                }));
            } else {
                // Respond with login failure
                ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    id,
                    result: { success: false, error: 'Invalid credentials' }
                }));
            }
            return;
        }

        // Check if the WebSocket connection is authenticated
        const userSession = userSessions.get(ws);
        if (!userSession) {
            // If not authenticated, reject the request
            ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id,
                error: { code: 401, message: 'Unauthorized' }
            }));
            return;
        }

        if (method === 'Logout') {
            // Handle user logout
            userSessions.delete(ws);
            ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id,
                result: { success: true, message: 'Logged out successfully' }
            }));
            ws.close();
        }

        // Check if the method exists and the user has permission to execute it
        if (methodHandlers[method] && userSession.permissions.includes(method)) {
            try {
                const result = methodHandlers[method](params, userSession);
                ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    id,
                    result
                }));
            } catch (error) {
                ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    id,
                    error: { code: 500, message: `Error executing method: ${error.message}` }
                }));
            }
        } else {
            ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id,
                error: { code: 32601, message: 'Method not found or insufficient permissions' }
            }));
        }
    });

    ws.on('close', () => {
        // Remove user session when WebSocket is closed
        const userSession = userSessions.get(ws);
        if (userSession) {
            console.log(`User ${userSession.username} disconnected`);
            userSessions.delete(ws); // Remove the user session from the map
        }
        console.log('Client disconnected');
    });
});

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
