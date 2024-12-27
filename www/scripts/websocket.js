let ws;

function initializeWebSocket() {
    if (ws) {
        console.log('WebSocket already initialized');
        return;
    }

    ws = new WebSocket('ws://localhost:8080');

    ws.onopen = () => {
        console.log('WebSocket connection established');
        
        ws.send('ready');
    };

    ws.onmessage = (event) => {
        const outputDiv = document.getElementById('output');
        const waitingMessage = document.querySelector('h1');
        if(waitingMessage){
            outputDiv.removeChild(waitingMessage);
        }
        if (outputDiv) {
            const newMessage = document.createElement('p');
            newMessage.textContent = event.data;
            outputDiv.appendChild(newMessage);
        }
    };

    ws.onclose = () => {
        console.log('WebSocket connection closed. Attempting to reconnect...');
        setTimeout(() => {
            ws = null; 
            initializeWebSocket(); 
        }, 5000);
        if (!waitingMessage) {
            const newWaitingMessage = document.createElement('h1');
            newWaitingMessage.textContent = "Waiting for data...";
            outputDiv.appendChild(newWaitingMessage);
        }
    };
}

initializeWebSocket();
