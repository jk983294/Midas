/**
 * usage: node server.js 1234
 */

var http = require("http"),
    url = require("url"),
    path = require("path"),
    fs = require("fs");

port = process.argv[2] || 8888;

var mimeTypes = {
    "htm": "text/html",
    "html": "text/html",
    "jpeg": "image/jpeg",
    "jpg": "image/jpeg",
    "png": "image/png",
    "gif": "image/gif",
    "js": "text/javascript",
    "css": "text/css",
    "json": "application/json"
};

/**
 * directory not under current directory
 */
var virtualDirectories = {
    //"tmp": "/tmp/"
};

http.createServer(function(request, response) {
    var uri = url.parse(request.url).pathname;
    var filename = path.join(process.cwd(), uri);
    var root = uri.split("/")[1];

    var virtualDirectory = virtualDirectories[root];

    if(virtualDirectory){
        uri = uri.slice(root.length + 1, uri.length);
        filename = path.join(virtualDirectory ,uri);
    }

    fs.exists(filename, function(exists) {
        if(!exists) {
            response.writeHead(404, {"Content-Type": "text/plain"});
            response.write("404 Not Found\n");
            response.end();
            return;
        }

        if (fs.statSync(filename).isDirectory()) filename += '/index.html';

        fs.readFile(filename, "binary", function(err, file) {
            if(err) {
                response.writeHead(500, {"Content-Type": "text/plain"});
                response.write(err + "\n");
                response.end();
                return;
            }

            var mimeType = mimeTypes[path.extname(filename).split(".")[1]];
            response.writeHead(200, {"Content-Type": mimeType});
            response.write(file, "binary");
            response.end();
        });
    });
}).listen(parseInt(port, 10));

console.log("Static file server running at\n  => http://localhost:" + port + "/");
