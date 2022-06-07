Scaner:
	g++ -m32 -o Scaner source/HelpFunc.cpp source/TCPConnectScan.cpp source/TCPSYNScan.cpp source/TCPFINScan.cpp source/UDPScan.cpp source/Scaner.cpp -lpthread
clean:
	rm Scaner