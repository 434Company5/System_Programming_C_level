#ifndef LCLOUD_NETWORK_INCLUDED
#define LCLOUD_NETWORK_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File          : lcloud_network.h
//  Description   : This is the network definitions for the Lion Cloud virtual
//                  device simulator.
//
//  Author        : Patrick McDaniel
//  Last Modified : Sat 28 Mar 2020 08:19:52 AM EDT
//

// Include Files

// Project Include Files
#include <lcloud_controller.h>

// Defines
#define LCLOUD_MAX_BACKLOG 5
#define LCLOUD_NET_HEADER_SIZE sizeof(LCloudRegisterFrame)
#define LCLOUD_DEFAULT_IP "127.0.0.1"
#define LCLOUD_DEFAULT_PORT 24567

// Global data

//
// Functional Prototypes

LCloudRegisterFrame client_lcloud_bus_request(LCloudRegisterFrame reg, void *buf);
	// This is the implementation of the client operation, as implemented 
	//  by the 311 student code.


#endif
