const ws = new WebSocket('ws://localhost:8080');

ws.onmessage = function(event) {
    const dataDiv = document.getElementById('output');
    const newMessage = document.createElement('div');
    newMessage.textContent = event.data;
    dataDiv.appendChild(newMessage);
};

socket.onopen = () => {
    console.log("WebSocket is open, sending 'ready' message.");
    socket.send("ready");
};


ws.onclose = function() {
    console.log('WebSocket connection closed.');
};
