let ws;
let reconnectTimeout;
let isReconnect = false;

function initializeWebSocket() {
    if (ws && ws.readyState !== WebSocket.CLOSED) {
        console.log('WebSocket already initialized');
        return;
    }

    ws = new WebSocket('ws://localhost:8080');

    ws.onopen = () => {
        console.log('WebSocket connection established');
        ws.send('WebSocket Ready');
        if (isReconnect) {
            ws.send('Reconnect Signal'); 
        } else {
            ws.send('WebSocket Ready'); 
        }
        isReconnect = true;
    };

    ws.onmessage = (event) => {
        const outputDiv = document.getElementById('output');
        if (!outputDiv) return;

        const waitingMessage = document.querySelector('h1');
        if (waitingMessage) {
            outputDiv.removeChild(waitingMessage);
        }

        const newMessage = document.createElement('p');
        newMessage.textContent = event.data;
        outputDiv.appendChild(newMessage);
    };

    ws.onclose = () => {
        console.log('WebSocket connection closed. Attempting to reconnect...');
        clearTimeout(reconnectTimeout);
        reconnectTimeout = setTimeout(() => {
            ws = null;
            initializeWebSocket();
        }, 5000);

        const outputDiv = document.getElementById('output');
        if (!outputDiv) return;

        const waitingMessage = document.querySelector('h1');
        if (!waitingMessage) {
            const newWaitingMessage = document.createElement('h1');
            newWaitingMessage.textContent = "Waiting for data...";
            outputDiv.appendChild(newWaitingMessage);
        }
    };

    ws.onerror = (error) => {
        console.error('WebSocket encountered an error:', error);
        ws.close();
    };
}

initializeWebSocket();
