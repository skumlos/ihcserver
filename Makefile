all: 
	g++ -g utils/Uart.cpp utils/Subject.cpp utils/Thread.cpp utils/TCPSocket.cpp utils/TCPSocketServer.cpp Configuration.cpp IHCInterface.cpp IHCServerWorker.cpp IHCServer.cpp main.cpp -lpthread -lssl -o ihcserver
