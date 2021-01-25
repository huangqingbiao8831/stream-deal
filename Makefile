objects = main.o ring_buffer.o unix_socket.o unix_client.o
main : $(objects)
	cc -o main $(objects) -lpthread
clean:  
	- rm -f $(objects) *.o 
