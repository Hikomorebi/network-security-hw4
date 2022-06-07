Scaner:
	g++ -m32 -o Scaner source/Scaner.cpp source/TCPConnectScan.cpp source/TCPSYNScan.cpp source/TCPFINScan.cpp source/UDPScan.cpp -lpthread
clean:
	rm Scaner
	