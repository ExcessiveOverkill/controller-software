import asyncio
import websockets
import json

# Global variables
request_id = 1
login_request_id = None

def create_rpc_request(method, params=[]):
    """Utility function to create JSON-RPC 2.0 requests."""
    global request_id
    req = {
        'jsonrpc': '2.0',
        'id': request_id,
        'method': method,
        'params': params
    }
    request_id += 1
    return req

def handle_response(response):
    """Handle responses from the WebSocket server."""
    print("Received response:", response)
    if 'error' in response:
        print(f"Error {response['error']['code']}: {response['error']['message']}")
    else:
        # Handle login response
        if response['id'] == login_request_id:
            if response['result'] and response['result']['success']:
                print("Login successful.")
            else:
                print("Login failed.")
        else:
            # Handle other responses
            print("Received result:", response['result'])

def get_credentials():
    """Prompt the user for username and password."""
    return 'admin', 'password'
    username = input('Enter username: ')
    password = input('Enter password: ')
    return username, password

async def handle_login(ws):
    """Handle user login."""
    global login_request_id
    username, password = get_credentials()
    login_request = create_rpc_request('Login', [username, password])
    login_request_id = login_request['id']
    await ws.send(json.dumps(login_request))
    print(f'Sent login request with id {login_request_id}')
    # Wait for response
    response = await ws.recv()
    response = json.loads(response)
    handle_response(response)

async def send_print_uint32(ws, value: int = 0):
    """Send a print_uint32 request."""
    print_request = create_rpc_request('print_uint32', [value])
    await ws.send(json.dumps(print_request))
    print(f'Sent print_uint32 request with id {print_request["id"]}')
    # Wait for response
    response = await ws.recv()
    response = json.loads(response)
    handle_response(response)

async def handle_logout(ws):
    """Handle user logout."""
    logout_request = create_rpc_request('Logout')
    await ws.send(json.dumps(logout_request))
    print(f'Sent logout request with id {logout_request["id"]}')
    # Wait for response
    response = await ws.recv()
    response = json.loads(response)
    handle_response(response)

async def main():
    """Main function to connect to the WebSocket and perform actions."""
    uri = 'wss://localhost:443'
    try:
        async with websockets.connect(uri) as ws:
            print('WebSocket connection opened')
            await handle_login(ws)
            await send_print_uint32(ws, 1234)
            await handle_logout(ws)
        print('WebSocket connection closed')
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    asyncio.run(main())
