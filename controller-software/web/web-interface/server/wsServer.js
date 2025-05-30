// server/wsServer.js
const { WebSocketServer } = require('ws');

function initWebSocketServer(server) {
  // Attach a WebSocketServer to an existing HTTP/S server
  const wss = new WebSocketServer({ server });

  wss.on('connection', (ws) => {
    console.log('[WS] Client connected.');

    ws.on('message', (data) => {
      try {
        const message = JSON.parse(data.toString());
        
        // Expecting JSON-RPC 2.0 format: { "jsonrpc": "2.0", "method": "...", "params": {...}, "id": 123 }
        if (message.jsonrpc !== '2.0' || !message.method) {
          return ws.send(JSON.stringify({
            jsonrpc: '2.0',
            id: message.id || null,
            error: { code: -32600, message: 'Invalid Request' }
          }));
        }

        console.log('[WS] Received message:', message);

        // Example: If method is "updateGamepadValue" or something
        if (message.method === 'updateGamepadValue') {
          // message.params might look like { "value": 0.5 }
          const { value } = message.params;

          // Log or process the numeric value
          console.log(`Gamepad value: ${value}`);

          // Respond with a JSON-RPC success
          ws.send(JSON.stringify({
            jsonrpc: '2.0',
            id: message.id,
            result: { status: 'OK', receivedValue: value }
          }));
        } else {
          // Method not recognized
          ws.send(JSON.stringify({
            jsonrpc: '2.0',
            id: message.id,
            error: { code: -32601, message: 'Method not found' }
          }));
        }
      } catch (err) {
        console.error('Error parsing WS message:', err);
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          id: null,
          error: { code: -32700, message: 'Parse error' }
        }));
      }
    });

    ws.on('close', () => {
      console.log('[WS] Client disconnected.');
    });
  });

  console.log('[WS] WebSocket server initialized.');
}

module.exports = { initWebSocketServer };
