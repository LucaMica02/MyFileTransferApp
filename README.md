# MyFileTransferApp
MyFileTransferApp allow a one or more clients to write and read file from a Server.

# Run The Server
./myFTserver -a [address] -p [port] -f [root_directory]
This command run the server at the socket address:port and set it into listening at the specified root, if not exists is created.

# Run The Client
./myFTclient -w -a [address] -p [port] -f [input_path] -o [output_path]
This command connect the client at the socket address:port and write the input file from the client specified with -f at the output path specified with -o on the server.
* If -o is not provided output_path will be equal to input_path

./myFTclient -r -a [address] -p [port] -f [input_path] -o [output_path]
This command connect the client at the socket address:port and write the input file from the server specified with -f at the output path specified with -o on the client.
* If -o is not provided output_path will be equal to input_path

./myFTclient -l -a [address] -p [port] -f [/path] 
This command connect the client at the socket address:port runs the ls -la command remotely and allows the client to view the output.
