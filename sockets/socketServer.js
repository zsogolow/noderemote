var net = require('net');
var fs = require('fs');
var socketPath = '/tmp/hidden';

fs.stat(socketPath, function (err) {

    if (!err) {
        fs.unlinkSync(socketPath);
    } else {
        console.log(err);
    }

    var unixServer = net.createServer(function (localSerialConnection) {
        localSerialConnection.on('data', function (data) {
            // data is a buffer from the socket
            console.log(data);

            // send ack
           // localSerialConnection.write('ack!');
        });
        // write to socket with localSerialConnection.write()
    });

    unixServer.listen(socketPath);
});