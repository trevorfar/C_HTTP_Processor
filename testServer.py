import websocket

def on_message(ws, message):
    print("Message from server:", message)

def on_error(ws, error):
    print("WebSocket error:", error)

def on_close(ws, close_status_code, close_msg):
    print("Connection closed")

def on_open(ws):
    print("Connected to WebSocket server")
    ws.send("Hello Server!")

ws = websocket.WebSocketApp(
    "ws://localhost:8080",
    on_message=on_message,
    on_error=on_error,
    on_close=on_close,
)
ws.on_open = on_open
ws.run_forever()
