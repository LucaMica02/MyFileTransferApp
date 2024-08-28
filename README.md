# MyFileTransferApp

**MyFileTransferApp** is a lightweight and efficient file transfer application that allows multiple clients to read from and write files to a central server. Designed for simplicity and ease of use, MyFileTransferApp facilitates seamless file management across remote locations.

## Features
- **Multiple Clients**: Supports simultaneous connections from multiple clients.
- **File Transfer**: Easily upload and download files between clients and the server.
- **Remote File Management**: Remotely view the contents of directories on the server.

## Getting Started

### Running the Server

To start the server, use the following command:
```bash
./myFTserver -a [address] -p [port] -f [root_directory]
```
**- a [address]:** Specify the IP address where the server will be hosted.
**-p [port]:** Specify the port on which the server will listen.
**-f [root_directory]:** Set the root directory for file storage. The directory will be created if it does not exist.

Example: 
```bash
./myFTserver -a 192.168.1.100 -p 8080 -f /server/myRoot
```
This will start the server on IP 192.168.1.100, port 8080, and use /server/myRoot as the root directory.

### Running the Client

**Uploading a File to the Server**
To upload a file from the client to the server, use:
```bash
./myFTclient -w -a [address] -p [port] -f [input_path] -o [output_path]
```
**-w:** Write mode (upload file).
**-a [address]:** Specify the server's IP address.
**-p [port]:** Specify the server's port.
**-f [input_path]:** Path to the file on the client machine.
**-o [output_path]:** Destination path on the server. If not provided, it defaults to input_path.

Example:
```bash
./myFTclient -w -a 192.168.1.100 -p 8080 -f /home/user/file.txt -o /server/docs/file.txt
```
This uploads file.txt from the client’s /home/user/ directory to /server/docs/ on the server.

**Downloading a File from the Server**
To download a file from the server to the client, use:
```bash
./myFTclient -r -a [address] -p [port] -f [input_path] -o [output_path]
```
**-r:** Read mode (download file).
**-a [address]:** Specify the server's IP address.
**-p [port]:** Specify the server's port.
**-f [input_path]:** Path to the file on the server.
**-o [output_path]:** Destination path on the client. If not provided, it defaults to input_path.

Example:
```bash
./myFTclient -r -a 192.168.1.100 -p 8080 -f /server/docs/file.txt -o /home/user/file.txt
```
This downloads file.txt from the server’s /server/docs/ directory to the client’s /home/user/ directory.

**Listing Directory Contents on the Server**
To list the contents of a directory on the server, use:
```bash
./myFTclient -l -a [address] -p [port] -f [/path]
```

**-l:** List mode (view directory contents).
**-a [address]:** Specify the server's IP address.
**-p [port]:** Specify the server's port.
**-f [/path]:** The directory path on the server to be listed.

Example:
```bash
./myFTclient -l -a 192.168.1.100 -p 8080 -f /server/docs
```

## Additional Information
Error Handling: The application provides error messages for invalid commands, missing files, or network issues to help diagnose problems quickly.

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing
Contributions are welcome! Please submit a pull request or open an issue for any bug fixes, features, or enhancements.
