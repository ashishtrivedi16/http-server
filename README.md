# HTTP Web-Server
This is a basic http web server which implements the http protocol fully coded in C.
> It responds to request made by client by creating a seperate thread using the pthread library in C.

> Uses socket programming to connect two nodes in a network so that they can communicated with each other.

To start the server just compile it on a linux terminal using the following command:
```c
gcc -o server server.c -lpthread
./server
```

Start the client in a similar way but don't forget to pass the port address:
```c
gcc -o client client.c
./client localhost 9001
```
