const ws = new WebSocket('ws://localhost:8080');

ws.onmessage = function(event) {
    const dataDiv = document.getElementById('output');
    const newMessage = document.createElement('p');
    newMessage.textContent = event.data;
    dataDiv.appendChild(newMessage);
    dataDiv.scrollTop = dataDiv.scrollHeight;
};

ws.onopen = () => {
    console.log("WebSocket is open, sending 'ready' message.");
    const waitingMessage = document.querySelector('h1');
    if(waitingMessage){
        dataDiv.removeChild(waitingMessage);
    }
    ws.send("ready");
};

ws.onclose = function() {
    console.log('WebSocket connection closed.');
    if (!waitingMessage) {
        const newWaitingMessage = document.createElement('h1');
        newWaitingMessage.textContent = "Waiting for data...";
        dataDiv.appendChild(newWaitingMessage);
    }
};
