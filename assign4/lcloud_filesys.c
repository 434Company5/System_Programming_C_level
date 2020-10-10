		////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : XUELUN HOU
//   Last Modified : 5 / 1 / 2020
//

// Include files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project include files
#include <lcloud_filesys.h>
#include <lcloud_controller.h>
#include <lcloud_cache.h>
#include <cmpsc311_util.h>
//#include <lcloud_client.c>
// File system interface implementation

//Declare some Global Variables
uint64_t b0, b1, c0, c1, c2, d0, d1;
int32_t indexx = 0;//index for filehandle(fh)
LCloudRegisterFrame opcode;//created opcode
LCloudRegisterFrame call_io;//return value of iobus
int powered = 0;//open/close indicator
int device_id;//devide ID
//int *usedblock;
uint64_t dvmap;
int dn = 0;
int LC_DEVICE_NUMBER_BLOCKS;
int LC_DEVICE_NUMBER_SECTORS;
int curdiv;
int bbb = 0;
//Create the device struct
struct devicesystem{
	int devicenum;
	int scnum;
	int blnum;
};
struct devicesystem *devices;
//Create the File Struct
struct filesystem{
	char name[128];
	size_t position;
	size_t size;
	int openclose;// 1 for open, 0 for close
	int numused;
	struct idsy *ids;
};
struct filesystem file[1024];
//Create the location system
struct idsy{
	int dvid;
	int scid;
	int blid;
};
//Create the struct for block info
struct blinfo{
	int nextdv;
	int nextsc;
	int nextbl;
};
struct blinfo bls;
//Function for Creating the OPCODE
LCloudRegisterFrame create_lcloud_registers(uint64_t b00, uint64_t b11, uint64_t c00, uint64_t c11, uint64_t c22, uint64_t d00, uint64_t d11){
	uint64_t completeop = 0;
	//Using Bits Operation to construct this 64 bits opcode
	completeop = (b00 << 60) & 0xf000000000000000;
	completeop = completeop | ((b11 << 56) & 0x0f00000000000000);
	completeop = completeop | ((c00 << 48) & 0x00ff000000000000);
	completeop = completeop | ((c11 << 40) & 0x0000ff0000000000);
	completeop = completeop | ((c22 << 32) & 0x000000ff00000000);
	completeop = completeop | ((d00 << 16) & 0x00000000ffff0000);
	completeop = completeop | ( d11        & 0x000000000000ffff);
	return completeop;
}

//Function for Extracting the OPCODE
void extract_lcloud_registers(LCloudRegisterFrame resp, uint64_t *b0, uint64_t *b1, uint64_t *c0, uint64_t *c1, uint64_t *c2, uint64_t *d0, uint64_t *d1) {
	//using Bits Operation to extract the returned opcode to 7 parts
	*b0 = (resp >> 60);
	*b1 = (resp >> 56) & 0x000f;
	*c0 = (resp >> 48) & 0x00ff;
	*c1 = (resp >> 40) & 0x00ff;
	*c2 = (resp >> 32) & 0x00ff;
	*d0 = (resp >> 16) & 0xffff;
	*d1 = (resp      ) & 0xffff;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

LcFHandle lcopen( const char *path ) {
	//poweron part
	int exist;
	int checker;
	int32_t returnvalue = 0;
	int r;
	//if no device turned on yet
	if (powered == 0){
		//construct the opcode and call it for Power On
		opcode = create_lcloud_registers(0,0,LC_POWER_ON,0,0,0,0);
		call_io = client_lcloud_bus_request(opcode, NULL);
		extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
		//check if successful
		if ((b0 == 1) & (b1 != 1) & (c0 == LC_POWER_ON) & (c1 == 0) & (c2 == 0) & (d0 == 0) & (d1 == 0)) {
			returnvalue = -1;
		}else{
			powered = 1;
		}
		//opcode and call for Probe
		opcode = create_lcloud_registers(0,0,LC_DEVPROBE,0,0,0,0);
		call_io = client_lcloud_bus_request(opcode, NULL);
		extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
		//check if probe succeed
		if ((b0 == 1) & (b1 != 1) & (c0 == LC_DEVPROBE) & (c1 == 0) & (c2 == 0) & (d0 == 0) & (d1 == 0)) {
			returnvalue = -1;
		}else{
		}
		//find the device that is avaible
		dvmap = d0;
		devices = NULL;
		for (checker = 1; checker < 17; checker++){
			if (dvmap % 2 == 0){
				dvmap = dvmap >> 1;
			}else{
				device_id = checker - 1;
				opcode = create_lcloud_registers(0,0,LC_DEVINIT,device_id,0,0,0);
				call_io = client_lcloud_bus_request(opcode, NULL);
				extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
				devices = (struct devicesystem *)realloc(devices,(dn+1)*sizeof(struct devicesystem));
				devices[dn].devicenum = device_id;
				devices[dn].scnum = d0;
				devices[dn].blnum = d1;
				dn = dn + 1;
				dvmap = dvmap >> 1;
			}
		}
		lcloud_initcache(LC_CACHE_MAXBLOCKS);
		//free(devices);
		curdiv = 0;
		LC_DEVICE_NUMBER_SECTORS = devices[curdiv].scnum;
		LC_DEVICE_NUMBER_BLOCKS = devices[curdiv].blnum;
		bls.nextdv = devices[curdiv].devicenum;
		bls.nextsc = 0;
		bls.nextbl = 0;
		//file = NULL;
	}
	//open part
	if (returnvalue != -1){
		//a loop to check if file already exists
		if (indexx == 0){
			exist = 0;
		}else{
			for (int i = 0; i < 1024; i++){
				r = strcmp(file[i].name, path);
				if (!r){
					//set to 0 position and break loop
					file[i].position = 0;
					file[i].openclose = 1;//open this file
					exist = 1;//indicate there's this file
					returnvalue = i;
					break;
				}else{
					exist = 0;
				}
			}
		}
		//if not exist, create a new one
		if (exist == 0){
			//initialize everything and status to open
			strcpy(file[indexx].name, path);
			file[indexx].position = 0;
			file[indexx].size = 0;
			file[indexx].openclose = 1;
			file[indexx].numused = 0;
			file[indexx].ids = NULL;
			returnvalue = indexx;//return value for open is its filehandle, which is the indexx
			indexx = indexx + 1;//global variable to label filehandle, plus one each time create a new file
		}
	}
    return(returnvalue); // return the file index(filehandle)
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcread
// Description  : Read data from the file 
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure
int lcread( LcFHandle fh, char *buf, size_t len ) {
	//a temporary buffer to do read
	char tempbuf[LC_DEVICE_BLOCK_SIZE + 2];
	//some local variable to use
	int bits_left;
	int returnvalue;
	int scid;
	int blid;
	int dvv;
	int onblock;
	int fullct = 0;
	int partct = 0;
	int jumpbit;
	int bits_to_read;
	int has_read = 0;
	//check if fildhandle is valid and if file is opened
	if (fh < 0 || fh > indexx || file[fh].openclose == 0){
		return (-1);
	//if all good, then read
	}else{

		//get the return value depending on the situation
		if (file[fh].position + len > file[fh].size){
			bits_left = file[fh].size - file[fh].position;
			returnvalue = bits_left;
		}else{
			bits_left = len;
			returnvalue = bits_left;
		}
		//start read
		while (bits_left > 0){
			//if a read block starts from the first bit of the block
			if (file[fh].position % LC_DEVICE_BLOCK_SIZE == 0){
				//if a read is through the whole block
				while (bits_left >= LC_DEVICE_BLOCK_SIZE){
					//get the block the position is on
					onblock = file[fh].position / LC_DEVICE_BLOCK_SIZE;
					//sector id and block id
					scid = file[fh].ids[onblock].scid;
					blid = file[fh].ids[onblock].blid;
					dvv = file[fh].ids[onblock].dvid;			
					//construct the opcode and call it
					opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_READ, scid, blid);
					call_io = client_lcloud_bus_request(opcode, tempbuf);
					tempbuf[LC_DEVICE_BLOCK_SIZE] = '\0';
					extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
					//check if succeed
					//read the data and copy it to the return buf
					memcpy(buf + has_read, tempbuf, LC_DEVICE_BLOCK_SIZE);
					//update the position value and the bits we have left to read
					file[fh].position = file[fh].position + LC_DEVICE_BLOCK_SIZE;
					has_read = has_read + LC_DEVICE_BLOCK_SIZE;
					bits_left = bits_left - LC_DEVICE_BLOCK_SIZE;
					//a counter to keep on track how many bits we already copied to return buf
					fullct = fullct + 1;
				}
				//if there's some bits less than the whole block on the last block to read
				onblock = file[fh].position / LC_DEVICE_BLOCK_SIZE;
				scid = file[fh].ids[onblock].scid;
				blid = file[fh].ids[onblock].blid;
				dvv = file[fh].ids[onblock].dvid;
				//construct the opcode
				opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_READ, scid, blid);
				call_io = client_lcloud_bus_request(opcode, tempbuf);
				tempbuf[bits_left] = '\0';
				extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
				//similarly update information and copy data to return buf
				memcpy(buf + has_read, tempbuf, bits_left);
				file[fh].position = file[fh].position + bits_left;
				bits_left = 0;
			}else{
				//if the read did not start from the first bits of the block, meaning it is the first part to read
				onblock = file[fh].position / LC_DEVICE_BLOCK_SIZE;
				scid = file[fh].ids[onblock].scid;
				blid = file[fh].ids[onblock].blid;
				dvv = file[fh].ids[onblock].dvid;
				//get how many bits we have to read on the first block
				jumpbit = file[fh].position % LC_DEVICE_BLOCK_SIZE;
				bits_to_read = LC_DEVICE_BLOCK_SIZE - jumpbit;
				if (bits_to_read > bits_left){
					bits_to_read = bits_left;
				}
				//construct the opcode and call it
				opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_READ, scid, blid);
				call_io = client_lcloud_bus_request(opcode, tempbuf);
				tempbuf[jumpbit + bits_to_read] = '\0';
				extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
				//copy this partial block to the return buf
				memcpy(buf + has_read, tempbuf + jumpbit, bits_to_read);
				//update the position 
				file[fh].position = file[fh].position + bits_to_read;
				has_read = has_read + bits_to_read;
				//a counter to track if there is partial ones read
				partct = 1;
				bits_left = bits_left - bits_to_read;
			}

		}
		//print out if successful with those imformations

	}
	return (returnvalue);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure

int lcwrite( LcFHandle fh, char *buf, size_t len ) {
	char tempbuf[LC_DEVICE_BLOCK_SIZE + 1];//a tempbuf needed for write
	//some local variables
	int bits_left = len;
	int onblock;
	int scid;
	int blid;
	int fullct = 0;
	int partct = 0;
	int usedbits = 0;
	int bkb;
	int bka;
	int numb;
	int dvv;
	int offset;
	//int kppt;
	//check if fildhandle is valid and if file is opened
	if (fh < 0 || fh > indexx || file[fh].openclose == 0){
		return (-1);
	}else{
		//if all good, then write
		//justify the size after this operation
		if (file[fh].size >= file[fh].position + len){
			//kppt = 256 - ((file[fh].position + len) % 256) ;
			file[fh].size = file[fh].size;
		}else{
			//decide how many additional blocks needed for write
			//those if...else... is for deciding if a new block is needed when it reaches or begins at both ends
			if (file[fh].size % LC_DEVICE_BLOCK_SIZE != 0){
				bkb = file[fh].size / LC_DEVICE_BLOCK_SIZE;
			}else{
				bkb = (file[fh].size / LC_DEVICE_BLOCK_SIZE) - 1;
			}
			file[fh].size = file[fh].position + len;
			if (file[fh].size % LC_DEVICE_BLOCK_SIZE != 0){
				bka = file[fh].size / LC_DEVICE_BLOCK_SIZE;
			}else{
				bka = (file[fh].size / LC_DEVICE_BLOCK_SIZE) - 1;
			}
			numb = bka - bkb;
			bbb = bka + bbb;
			//assign blockids and usedblocks that will be used for write
			while (numb > 0){
				file[fh].ids = (struct idsy*)realloc(file[fh].ids,2*(bbb + 1)*sizeof(struct idsy));
				if (bls.nextbl < LC_DEVICE_NUMBER_BLOCKS){
					file[fh].ids[file[fh].numused].dvid = bls.nextdv;
					file[fh].ids[file[fh].numused].scid = bls.nextsc;
					file[fh].ids[file[fh].numused].blid = bls.nextbl;
				}else if (bls.nextsc + 1 < LC_DEVICE_NUMBER_SECTORS){
					file[fh].ids[file[fh].numused].dvid = bls.nextdv;
					bls.nextbl = 0;
					bls.nextsc = bls.nextsc + 1;
					file[fh].ids[file[fh].numused].scid = bls.nextsc;
					file[fh].ids[file[fh].numused].blid = bls.nextbl;
				}else{
					curdiv = curdiv + 1;
					LC_DEVICE_NUMBER_BLOCKS = devices[curdiv].blnum;
					LC_DEVICE_NUMBER_SECTORS = devices[curdiv].scnum;
					bls.nextbl = 0;
					bls.nextsc = 0;
					bls.nextdv = devices[curdiv].devicenum;
					file[fh].ids[file[fh].numused].dvid = bls.nextdv;
					file[fh].ids[file[fh].numused].scid = bls.nextsc;
					file[fh].ids[file[fh].numused].blid = bls.nextbl;
				}
				bls.nextbl = bls.nextbl + 1;
				file[fh].numused = file[fh].numused + 1;
				numb = numb - 1;
			}
		}
		//similarly like read
		while (bits_left > 0){
			if (file[fh].position % LC_DEVICE_BLOCK_SIZE == 0){
				onblock = file[fh].position / LC_DEVICE_BLOCK_SIZE;
				scid = file[fh].ids[onblock].scid;
				blid = file[fh].ids[onblock].blid;
				dvv = file[fh].ids[onblock].dvid;
				while (bits_left >= LC_DEVICE_BLOCK_SIZE){
					memcpy(tempbuf, buf + len - bits_left, LC_DEVICE_BLOCK_SIZE);
					//construct the opcode
					opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_WRITE, scid, blid);
					call_io = client_lcloud_bus_request(opcode, tempbuf);
					tempbuf[LC_DEVICE_BLOCK_SIZE] = '\0';
					extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
					//update and keep on track with everything
					file[fh].position = file[fh].position + LC_DEVICE_BLOCK_SIZE;
					onblock = file[fh].position / LC_DEVICE_BLOCK_SIZE;
					dvv = file[fh].ids[onblock].dvid;
					scid = file[fh].ids[onblock].scid;
					blid = file[fh].ids[onblock].blid;
					fullct = fullct + 1;
					bits_left = bits_left - LC_DEVICE_BLOCK_SIZE;
				}
				//construct the opcode
				if (bits_left != 0){
					onblock = file[fh].position / LC_DEVICE_BLOCK_SIZE;
					dvv = file[fh].ids[onblock].dvid;
					scid = file[fh].ids[onblock].scid;
					blid = file[fh].ids[onblock].blid;
					offset = file[fh].position % LC_DEVICE_BLOCK_SIZE;
					opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_READ, scid, blid);
					call_io = client_lcloud_bus_request(opcode, tempbuf);
					extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
					memcpy(tempbuf + offset, buf + len - bits_left, bits_left);
					opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_WRITE, scid, blid);
					call_io = client_lcloud_bus_request(opcode, tempbuf);
					extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
					file[fh].position = file[fh].position + bits_left;
					bits_left = 0;		
				}
			}else{
				onblock = file[fh].position / LC_DEVICE_BLOCK_SIZE;
				scid = file[fh].ids[onblock].scid;
				blid = file[fh].ids[onblock].blid;
				dvv = file[fh].ids[onblock].dvid;
				usedbits = file[fh].position % LC_DEVICE_BLOCK_SIZE;
				//construct the opcode, read partial and copy and read another part and memcpy it
				opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_READ, scid, blid);
				call_io = client_lcloud_bus_request(opcode, tempbuf);
				extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
				if (usedbits + bits_left < 256){
					memcpy(tempbuf + usedbits, buf, bits_left);
					opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_WRITE, scid, blid);
					call_io = client_lcloud_bus_request(opcode, tempbuf);
					file[fh].position = file[fh].position + bits_left;
					bits_left = 0;
				}else{
					memcpy(tempbuf + usedbits, buf, LC_DEVICE_BLOCK_SIZE - usedbits);
					opcode = create_lcloud_registers(0, 0, LC_BLOCK_XFER, dvv, LC_XFER_WRITE, scid, blid);
					call_io = client_lcloud_bus_request(opcode, tempbuf);
					file[fh].position = file[fh].position + LC_DEVICE_BLOCK_SIZE - usedbits;
					bits_left = bits_left - (LC_DEVICE_BLOCK_SIZE - usedbits);
				}
			}
		}
		return (len);//return value
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : 0 if successful test, -1 if failure

int lcseek( LcFHandle fh, size_t off ) {
	//check if filehandle is valid
	if (fh < 0 || fh > 1024 || file[fh].openclose == 0 || off > file[fh].size){
		return (-1);
	//if valid then seek
	}else{
		file[fh].position = off;
		return (off);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int lcclose( LcFHandle fh ) {
	//check if the filehandle is valid and if file is opened
	int scid;
	int blid;
	int dvv;
	if (fh < 0 || fh > indexx || file[fh].openclose == 0){
		return (-1);
	//if valid and opened then close
	}else{
		//free memory
		for (int i = 0; i < file[fh].numused; i++){
			scid = file[fh].ids[i].scid;
			blid = file[fh].ids[i].blid;
			dvv = file[fh].ids[i].dvid;
		}
		//initialize all values
		file[fh].openclose = 0;
		file[fh].size = 0;
		file[fh].position = 0;
		return (0);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int lcshutdown( void ) {
	//close all files
	for (LcFHandle i = 0; i < LC_DEVICE_NUMBER_BLOCKS * LC_DEVICE_NUMBER_SECTORS; i++){
		lcclose(i);
	}
	//free(file);
	//construct the opcode for shutdown
	opcode = create_lcloud_registers(0,0,LC_POWER_OFF,0,0,0,0);
	call_io = client_lcloud_bus_request(opcode, NULL);
	extract_lcloud_registers(call_io, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
	lcloud_closecache;
	//check if shutdown is successful
	if ((b0 == 1) & (b1 == 1) & (c0 == LC_POWER_OFF) & (c1 == 0) & (c2 == 0) & (d0 == 0) & (d1 == 0)) {
		return (0);
	}else{
		return (-1);
	}
}
