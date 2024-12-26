document.addEventListener('DOMContentLoaded', () => {
    // Select the output container
    const output = document.getElementById('output');

    // Connect to the WebSocket server
    const ws = new WebSocket('ws://localhost:8080');

    // Event: Connection opened
    ws.onopen = () => {
        console.log('WebSocket connection established');
        output.innerHTML = 'Connected to WebSocket server<br>';
    };

    // Event: Message received
    ws.onmessage = (event) => {
        console.log('Message from server:', event.data);

        // Append the received data to the output container
        const message = document.createElement('div');
        message.textContent = event.data;
        output.appendChild(message);
    };

    // Event: Connection closed
    ws.onclose = () => {
        console.log('WebSocket connection closed');
        const message = document.createElement('div');
        message.textContent = 'Connection closed';
        output.appendChild(message);
    };

    // Event: Error occurred
    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        const message = document.createElement('div');
        message.textContent = 'Error occurred';
        output.appendChild(message);
    };
});
