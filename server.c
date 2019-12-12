#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT 32000
#define TIMEOUT 3

//Primitives
#define ENDPACKETID 0XFFFF

//Packet Types
#define ACKPACKET 0XFFF2
#define REJECTPACKET 0XFFF3

//Reject sub codes
#define OUTOFSEQUENCECODE 0XFFF4
#define LENGTHMISMATCHCODE 0XFFF5
#define ENDOFPACKETMISSINGCODE 0XFFF6
#define DUPLICATECODE 0XFFF7

//Data Packet Format
struct dataPacket {
	
	uint16_t packetID;
	uint8_t clientID;
	uint16_t type;
	uint8_t segment_No;
	uint8_t length;
	char payload[255];
	uint16_t endpacketID;
};

//ACK(Acknowledge) Packet Format
struct ackPacket {
	
	uint16_t packetID;
	uint8_t clientID;
	uint16_t type;
	uint8_t segment_No;
	uint16_t endpacketID;
};

//REJECT Packet Format
struct rejectPacket {
	
	uint16_t packetID;
	uint8_t clientID;
	uint16_t type;
	uint16_t subCode;
	uint8_t segment_No;
	uint16_t endpacketID;
};

//print all the packet details
void printPacketDetails(struct dataPacket data) {
	
	printf("\n INFO: Received packet:\n");
	printf("  packetID: %hx\n",data.packetID);
	printf("  Client id : %hhx\n",data.clientID);
	printf("  Data: %x\n",data.type);
	printf("  Segment no : %d\n",data.segment_No);
	printf("  Length %d\n",data.length);
	printf("  Payload: %s\n",data.payload);
	printf("  End of packet id : %x\n",data.endpacketID);
}

//create REJECT packet
struct rejectPacket createRejectPacket(struct dataPacket data) {
	
	struct rejectPacket reject;
	
	reject.packetID = data.packetID;
	reject.clientID = data.clientID;
	reject.segment_No = data.segment_No;
	reject.type = REJECTPACKET;
	reject.endpacketID = data.endpacketID;
	
	return reject;
}

//create ACK packet
struct ackPacket createACKPacket(struct dataPacket data) {
	
	struct ackPacket ack;
	
	ack.packetID = data.packetID;
	ack.clientID = data.clientID;
	ack.segment_No = data.segment_No;
	ack.type = ACKPACKET ;
	ack.endpacketID = data.endpacketID;
	
	return ack;
}


int main(int argc, char**argv)
{
	int sockfd,n;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;
	struct dataPacket data;
	struct ackPacket  ack;
	struct rejectPacket reject;

	//store all packets in buffer
	int buffer[20];
	for(int j = 0; j < 20;j++) {
		buffer[j] = 0;
	}
	
	//start the server
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	int expectedPacket = 1;
	bzero(&serverAddr,sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	serverAddr.sin_port=htons(PORT);
	bind(sockfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
	addr_size = sizeof serverAddr;
	printf("\n Server started successfully \n");
	
	while(1) {
		
		//wait and recieve client packet
		n = recvfrom(sockfd,&data,sizeof(struct dataPacket),0,(struct sockaddr *)&serverStorage, &addr_size);
		
		//print the recieved packet details
		printPacketDetails(data);
		
		buffer[data.segment_No]++;
		if(data.segment_No == 11 || data.segment_No == 12) {
			buffer[data.segment_No] = 1;
		}
		int length = strlen(data.payload);
		
		//Case-4 : REJECT Duplicate packet
		if(buffer[data.segment_No] != 1) {
			
			reject = createRejectPacket(data);
			reject.subCode = DUPLICATECODE;
			sendto(sockfd,&reject,sizeof(struct rejectPacket),0,(struct sockaddr *)&serverStorage,addr_size);
			
			printf("\n ERROR: duplicate packet \n");
		}
		
		//Case-2 : REJECT length mismatch
		else if(length != data.length) {
			
			reject = createRejectPacket(data);
			reject.subCode = LENGTHMISMATCHCODE ;
			sendto(sockfd,&reject,sizeof(struct rejectPacket),0,(struct sockaddr *)&serverStorage,addr_size);
			
			printf("\n ERROR: packet length mismatch \n");
		}
		
		//Case-3 : REJECT End of packet missing
		else if(data.endpacketID != ENDPACKETID ) {
			
			reject = createRejectPacket(data);
			reject.subCode = ENDOFPACKETMISSINGCODE ;
			sendto(sockfd,&reject,sizeof(struct rejectPacket),0,(struct sockaddr *)&serverStorage,addr_size);
			
			printf("\n ERROR: End of packet missing \n");
		}
		
		//Case-1 : REJECT out of sequence
		else if(data.segment_No != expectedPacket && data.segment_No != 11 && data.segment_No != 12) {
			
			reject = createRejectPacket(data);
			reject.subCode = OUTOFSEQUENCECODE;
			sendto(sockfd,&reject,sizeof(struct rejectPacket),0,(struct sockaddr *)&serverStorage,addr_size);
			
			printf("\n ERROR: out of sequence packet \n");
		}
		
		//Send ACK
		else {
			if(data.segment_No == 11) {
				sleep(7);
			}
			ack = createACKPacket(data);
			sendto(sockfd,&ack,sizeof(struct ackPacket),0,(struct sockaddr *)&serverStorage,addr_size);
		}
		
		expectedPacket++;
		printf("\n ---------------------------------------------------------------------- \n");
	}
}


