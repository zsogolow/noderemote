var net = require('net');
var fs = require('fs');
var socketPath = '/tmp/hidden';

const client = net.connect(socketPath, () => {
    //'connect' listener
    console.log('connected to server!');
    client.write('11');
});

client.on('data', (data) => {
    console.log(data.toString());

    // end after we get a response
    client.end();
});

client.on('end', () => {
    console.log('disconnected from server');
});