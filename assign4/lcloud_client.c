////////////////////////////////////////////////////////////////////////////////
//
//  File          : lcloud_client.c
//  Description   : This is the client side of the Lion Clound network
//                  communication protocol.
//
//  Author        : Patrick McDaniel
//  Last Modified : Sat 28 Mar 2020 09:43:05 AM EDT
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <cmpsc311_util.h>
#include <lcloud_network.h>
#include <cmpsc311_log.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_lcloud_bus_request
// Description  : This the client regstateeration that sends a request to the 
//                lion client server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed
//

//set socket handle
int socket_hd = -1;

LCloudRegisterFrame client_lcloud_bus_request( LCloudRegisterFrame reg, void *buf ) {
    //char terminal = '\0';
	LCloudRegisterFrame returnv;
	//check if socket is -1, if so, connect
    if(socket_hd==-1){
        struct sockaddr_in addr;
        socket_hd = socket(AF_INET,SOCK_STREAM,0);
        memset(&addr,0,sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(LCLOUD_DEFAULT_PORT);
        inet_pton(AF_INET,LCLOUD_DEFAULT_IP,&addr.sin_addr);
        connect(socket_hd,(struct sockaddr *)&addr,sizeof(addr));
    }
	//extract the opcode using a helper function from filesys.c
    int frame_len = sizeof(LCloudRegisterFrame);
    uint64_t B0, B1, C0, C1, C2, D0, D1;
    extract_lcloud_registers(reg, &B0, &B1, &C0, &C1, &C2, &D0, &D1);
	//read operation
	if(C0==LC_BLOCK_XFER&&C2==LC_XFER_READ){        
		char* buf_to_read = (char *) buf;
		uint64_t *net_opcode = malloc(frame_len);
		*net_opcode = htonll64(reg);
		write(socket_hd,net_opcode,sizeof(net_opcode));
		LCloudRegisterFrame *op = malloc(frame_len);
		read(socket_hd,op,sizeof(LCloudRegisterFrame));
		read(socket_hd,(char *)(buf),256);
		returnv = ntohll64(*op);
		return returnv;
	//write operation
	}else if(C0==LC_BLOCK_XFER&&C2==LC_XFER_WRITE){
		char* buf_to_write = (char *) buf;
		uint64_t *net_opcode = malloc(frame_len);
		*net_opcode = htonll64(reg);
		write(socket_hd,net_opcode,sizeof(LCloudRegisterFrame));
		write(socket_hd,buf_to_write,256);
		LCloudRegisterFrame *op = malloc(frame_len);
		read(socket_hd,op,sizeof(LCloudRegisterFrame));
		returnv = ntohll64(*op);
		return returnv;
	}else if(C0==LC_POWER_OFF&&C2==0){
	//close operation
		uint64_t *net_opcode = malloc(frame_len);
		*net_opcode = htonll64(reg);
		write(socket_hd,net_opcode,sizeof(LCloudRegisterFrame));
		LCloudRegisterFrame *op = malloc(frame_len);
		read(socket_hd,op,sizeof(LCloudRegisterFrame));
		close(socket_hd);
		socket_hd = -1;
		returnv = ntohll64(*op);
		return returnv;
	}else{
	//other operations
		uint64_t *net_opcode = malloc(frame_len);
		*net_opcode = htonll64(reg);
		write(socket_hd,net_opcode,sizeof(net_opcode));
		LCloudRegisterFrame *op = malloc(frame_len);
		read(socket_hd,op,sizeof(LCloudRegisterFrame));
		returnv = ntohll64(*op);
		return returnv;
	}  
}
