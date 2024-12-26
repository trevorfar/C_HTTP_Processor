const ws = new WebSocket('ws://localhost:8080');

ws.onmessage = function(event) {
    const dataDiv = document.getElementById('output');
    const newMessage = document.createElement('div');
    newMessage.textContent = event.data;
    dataDiv.appendChild(newMessage);
};

ws.onopen = function() {
    console.log('WebSocket connection established.');
};

ws.onclose = function() {
    console.log('WebSocket connection closed.');
};
