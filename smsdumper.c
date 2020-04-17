/*
 * \file main.c
 * \brief Libusb software for communicate with STM32
 * \author X-death for Ultimate-Consoles forum (http://www.ultimate-consoles.fr)
 * \date 2019/11
 *
 * just a simple sample for testing libusb and new code of Sega Dumper
 *
 * --------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <libusb.h>
#include <assert.h>


#if defined(_WIN32)
	#include <conio.h>
#else
	#include <termios.h> //unix
	char getch(void){
	    char buf = 0;
	    struct termios old = {0};
	    fflush(stdout);
	    if(tcgetattr(0, &old) < 0)
	        perror("tcsetattr()");
	    old.c_lflag &= ~ICANON;
	    old.c_lflag &= ~ECHO;
	    old.c_cc[VMIN] = 1;
	    old.c_cc[VTIME] = 0;
	    if(tcsetattr(0, TCSANOW, &old) < 0)
	        perror("tcsetattr ICANON");
	    if(read(0, &buf, 1) < 0)
	        perror("read()");
	    old.c_lflag |= ICANON;
	    old.c_lflag |= ECHO;
	    if(tcsetattr(0, TCSADRAIN, &old) < 0)
	        perror("tcsetattr ~ICANON");
	    //printf("%c\n", buf);
	    return buf;
	}
#endif



// USB commands
#define WAKEUP  		0x10
#define READ 		    0x11
#define READ_SAVE 	 	0x12
#define WRITE_SAVE 		0x13
#define WRITE_FLASH 	0x14
#define ERASE_FLASH 	0x15
#define INFOS_ID 		0x18
#define DEBUG

#define PACKET_SIZE     0x4000 //16ko chunks


unsigned char buffer_in[64]  = {0};   /* 64 byte transfer buffer IN */
unsigned char buffer_out[64] = {0};  /* 64 byte transfer buffer OUT */
int idx_search = 0; //chipinfos
int idx_sizesearch = 0; //mappersize


const long idtable[] = {
0,			//Not readable
0x897E2401, //MT28EW01G
0x897E2301, //MT28EW512
0x897E2201, //MT28EW256
0x897E2101, //MT28EW128
0xC2A70000, //MX29LV320ET
0xC2A80000, //MX29LV320EB
0x017E1D00, //S29GL032N01/2
0x017E1A00, //S29GL032N04B
0x017E1A01, //S29GL032N03T
0x20570000, //M29W320EB
0xBF5E0000, //SST39VF3201C
0xBF5F0000, //SST39VF3201C
0xC2F90000, //MX29L3211
0x01C40000, //AM29LV160DT
0x01490000, //AM29LV160DB
0xC2F10000, //MX29F1610
0xC2FA0000, //MX29F1610A
0xC2D60000, //MX29F800T
0xC2580000, //MX29F800B
0x01230000, //M29F400FT
0x01AB0000, //M29F400FB
0xAD580000, //HY29F800B
0xADD60000, //HY29F800T
0xBFC80000, //SST39VF1681
0xBFC90000, //SST39VF1682
0xBFB70000, //SST39SF040
0xBFB60000, //SST39SF020
0xBFB50000, //SST39SF010
};

const char * nametable[] = {
"No information",
"MT28EW01G 1Gb(8/16) 3.3v",
"MT28EW512 512Mb(8/16) 3.3v",
"MT28EW256 256Mb(8/16) 3.3v",
"MT28EW128 128Mb(8/16) 3.3v",
"MX29LV320ET 32Mb(8/16) 3.3v",
"MX29LV320EB 32Mb(8/16) 3.3v",
"S29GL032N01/2 32Mb(8/16) 3.3v",
"S29GL032N04B 32Mb(8/16) 3.3v",
"S29GL032N03T 32Mb(8/16) 3.3v",
"M29W320EB 32Mb(8/16) 3.3v",
"SST39VF3201C 32Mb(16) 3.3v",
"SST39VF3201C 32Mb(16) 3.3v",
"MX29L3211 32Mb(8/16) 3.3v",
"AM29LV160DT 16Mb(8/16) 3.3v",
"AM29LV160DB 16Mb(8/16) 3.3v",
"MX29F1610 16Mb(8/16) 5v",
"MX29F1610A 16Mb(8/16) 5v",
"MX29F800T 8Mb(8/16) 5v",
"MX29F800B 8Mb(8/16) 5v",
"M29F400FT 4Mb(8/16) 5v",
"M29F400FB 4Mb(8/16) 5v",
"HY29F800B 8Mb(8/16) 5v",
"HY29F800T 8Mb(8/16) 5v",
"SST39VF1681 16Mb(8) 3.3v",
"SST39VF1682 16Mb(8) 3.3v",
"SST39SF040 4Mb(8) 5v",
"SST39SF020 4Mb(8) 5v",
"SST39SF010 4Mb(8) 5v",
};

const int sizetable[] = {
0,		//Not readable
134217728, //MT28EW01G
67108864, //MT28EW512
33554432, //MT28EW256
16777216, //MT28EW128
4194304, //MX29LV320ET
4194304, //MX29LV320EB
4194304, //S29GL032N01/2
4194304, //S29GL032N04B
4194304, //S29GL032N03T
4194304, //M29W320EB
4194304, //SST39VF3201C
4194304, //SST39VF3201C
4194304, //MX29L3211
2097152, //AM29LV160
2097152, //AM29LV160
2097152, //MX29F1610
2097152, //MX29F1610A
1048576, //MX29F800T
1048576, //MX29F800B
524288, //M29F400FT
524288, //M29F400FB
1048576, //HY29F800B
1048576, //HY29F800T
2097152, //SST39VF1681
2097152, //SST39VF1682
524288, //SST39SF040
262144, //SST39SF020
131072, //SST39SF010
};


typedef struct{
  //command, special, adr in, length, slotsize (2 bytes), slobnbn (1 bytes) slotreg (8bytes)
  unsigned char mode; //0
  unsigned char special; //1
  unsigned long adr_start; //2-3-4
  unsigned long size; //5-6-7
  unsigned int chipid; //8-9
  unsigned char flash_command; //10
} STM_CMD;
STM_CMD stm_cmd;


unsigned int crc32(unsigned int seed, const void* data, int data_size){
    unsigned int crc = ~seed;
    static unsigned int crc32_lut[256] = { 0 };
    if(!crc32_lut[1]){
        const unsigned int polynomial = 0xEDB88320;
        unsigned int i, j;
        for (i = 0; i < 256; i++){
            unsigned int crc = i;
            for (j = 0; j < 8; j++)
                crc = (crc >> 1) ^ ((unsigned int)(-(int)(crc & 1)) & polynomial);
            crc32_lut[i] = crc;
        }
    }

    {
    	const unsigned char* data8 = (const unsigned char*)data;
    	int n;
    	for (n = 0; n < data_size; n++)
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ data8[n]];
    }
    return ~crc;
}

void usb_command(){
  //common
  //mode
	buffer_out[0] = stm_cmd.mode; //SEE #defines
	//special
	buffer_out[1] = stm_cmd.special;
	//WRITE
	if(stm_cmd.mode==ERASE_FLASH){
		//chipid
    	buffer_out[2] = stm_cmd.chipid & 0xFF;
    	buffer_out[3] = (stm_cmd.chipid & 0xFF00)>>8;
	}
	//is old algo
	buffer_out[4] = stm_cmd.flash_command;
	//adr start
	buffer_out[6] = stm_cmd.adr_start & 0xFF;
	buffer_out[7] = (stm_cmd.adr_start & 0xFF00)>>8;
	buffer_out[8] = (stm_cmd.adr_start & 0xFF0000)>>16;
	buffer_out[9] = (stm_cmd.adr_start & 0xFF000000)>>24;
	//end
	buffer_out[10] = (stm_cmd.adr_start + stm_cmd.size) & 0xFF;
	buffer_out[11] = ((stm_cmd.adr_start + stm_cmd.size) & 0xFF00)>>8;
	buffer_out[12] = ((stm_cmd.adr_start + stm_cmd.size) & 0xFF0000)>>16;
	buffer_out[13] = ((stm_cmd.adr_start + stm_cmd.size) & 0xFF000000)>>24;
}


int array_search_chipid(unsigned long find){
	int i;
	for(i=0; i<(sizeof(idtable)/8); i++){
		if(idtable[i] == (find & 0xFFFFFFFF)){
			return i;
		}
	}
	return 0; //nothing found
}


#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"


int main(int argc, char *argv[]){ 

    int res                      = 0;        /* return codes from libusb functions */
    libusb_device_handle* handle = 0;        /* handle for USB device */
    int kernelDriverDetached     = 0;        /* Set to 1 if kernel driver detached */
    int numBytes                 = 0;        /* Actual bytes transferred. */
    unsigned long len            = 0;        /* Number of bytes transferred. */

    unsigned long i=0;
    unsigned long j=0;
    unsigned long address=0;
    unsigned char *buffer_read = NULL;
    unsigned int crc32_rom;
    unsigned int crc32_file;
    int packets;
    int pourcent;
    int chipid = 0;
    unsigned char mapper;
    FILE *myfile;
    int game_size=0;
    int max_size=0;

	if(argc < 3){
	    printf("%s---------------------------\n", KBLU);
   	 	printf(" SMSDumper / Fast CLI v1.0\n");
   		printf("---------------------------%s\n", KNRM);
		printf ("Syntax should be smsdumper <read|write> <mapper> <file if write|size if read (kB)> <old pcb>\n");
		printf ("<mapper> can be\n");
		printf ("1: Sega Master System compatible\n");
		printf ("2: Codemasters\n");
		printf ("3: 4 Pak All Action\n");
		printf ("4: Korean 1MB\n");
		printf ("5: Korean X-in-1\n");
		printf ("6: Korean MSX 8Kb\n");
		printf ("7: Korean Jagun\n");
		printf ("8: SG1000 Taiwan MSX\n");
		printf ("9: Sega Master System EXT\n\n");
		return 0;
	}

_rewrite:

    res = libusb_init(0);
    if(res != 0){
        fprintf(stderr, "Error initialising LibUSB.\n");
        return 0;
    }


   	handle = libusb_open_device_with_vid_pid(0, 0x0483, 0x5740);
	if(!handle){
	    fprintf(stderr, "Waiting for SMS Dumper\nPress CTRL+C to abort...\n");
	    while(!handle){  	
	    	handle = libusb_open_device_with_vid_pid(0, 0x0483, 0x5740);
	    	usleep(100000);
	    }
	}

    /* Claim interface #0. */
    res = libusb_claim_interface(handle, 1); // 1?
    if(res != 0){
        printf("Error claiming interface.\n");
      	libusb_release_interface(handle, 1);
        libusb_close(handle);
		libusb_exit(NULL);
        return 0;
    }
    
    //READY    
    buffer_out[0] = WAKEUP;
    libusb_bulk_transfer(handle, 0x01, buffer_out, sizeof(buffer_out), &numBytes, 500);
    libusb_bulk_transfer(handle, 0x82, buffer_in, sizeof(buffer_in), &numBytes, 500);
	printf("~SMS: %.*s connected\n", 9, buffer_in);
	
	mapper = atoi(argv[2]);
	
	// READ - CRC32 at end
	if(!(strcmp(argv[1],"read"))){
		stm_cmd.mode = READ; //commun
	  	game_size = (atol(argv[3]) * 1024);
	  	packets = (game_size/PACKET_SIZE); //nbpackets
	  	if(!packets) packets = 1; //min
	  	stm_cmd.size = PACKET_SIZE; //bytes
	  	stm_cmd.special = mapper; //mapper
	  	buffer_read = (unsigned char *)malloc(game_size);
		printf("~SMS: Read 0%%");
		for(i=0;i<packets;i++){
			stm_cmd.adr_start = (i*PACKET_SIZE); //byte
		    usb_command();
		    libusb_bulk_transfer(handle, 0x01, buffer_out, 64, &numBytes, 500);
		    res = libusb_bulk_transfer(handle, 0x82, buffer_read+(i*PACKET_SIZE), PACKET_SIZE, &numBytes, 500);
		    if(res!=0){
		        printf("~SMS: Error while reading (err:%u)\n", res);
		        free(buffer_read);
		        libusb_release_interface(handle, 1);
		        libusb_close(handle);
				libusb_exit(NULL);	
		        return 0;
		     }
		     pourcent = (i*100/packets);
		     printf("\r~SMS: Read %d%%",pourcent);
		     fflush(stdout);
		  }
		printf("\r~SMS: Read 100%%\n"); 
		
		crc32_rom = crc32(0, buffer_read, game_size);
		printf("~SMS: CRC32 %s0x%X%s\n", KBLU, crc32_rom, KNRM); 
        
	    char tmpname[256] = {0};
		sprintf(tmpname, "dump_%X.bin", crc32_rom);
        myfile = fopen(tmpname,"wb");
        fwrite(buffer_read, 1, game_size, myfile);
        fclose(myfile);
	  	
	  	free(buffer_read);
        libusb_release_interface(handle, 1);
		libusb_close(handle);
		libusb_exit(NULL);	  	
	}

	if(!(strcmp(argv[1],"write"))){
		//flash id
		stm_cmd.mode = INFOS_ID;
		stm_cmd.special = 0;
		if(argv[4] != NULL){ stm_cmd.special = 1; }
		stm_cmd.adr_start = 0;
		stm_cmd.size = 0;
		usb_command();
		libusb_bulk_transfer(handle, 0x01, buffer_out, 64, &numBytes, 500);
		libusb_bulk_transfer(handle, 0x82, buffer_in, 64, &numBytes, 500);
		chipid = (buffer_in[0]<<24)|(buffer_in[1]<<16)|(buffer_in[2]<<8)|buffer_in[3];
		idx_search = array_search_chipid(chipid); //2=idtable
		max_size = sizetable[idx_search];
		printf("~SMS: ChipID: %08X\n~SMS: %s\n~SMS: Size: %u bytes\n", chipid, nametable[idx_search], max_size);
   
        myfile = fopen(argv[3],"rb");
		fseek(myfile, 0, SEEK_END);
		game_size = ftell(myfile);
		rewind(myfile);
		
		if(max_size < game_size){
			printf("~SMS: Error - Filesize bigger than Flash\n"); 
			printf("~SMS: File %u bytes <>  Flash %u bytes\n", game_size, max_size);
			libusb_release_interface(handle, 1); 
			libusb_close(handle);
			libusb_exit(NULL);	
			return 0; 
		}
		
	  	buffer_read = (unsigned char *)malloc(game_size);
		fread(buffer_read, game_size, 1, myfile);
		fclose(myfile);
		crc32_file = crc32(0, buffer_read, game_size);
		
		printf("~SMS: File %s\n~SMS: %d bytes\n", argv[3], game_size);
		printf("~SMS: CRC32 %s0x%X%s\n", KBLU, crc32_file, KNRM); 

		if((chipid & 0xFFFF0000)==0x897E0000){ mapper = 9; }  //is micron ?

		//erase
		stm_cmd.mode = ERASE_FLASH;
		stm_cmd.adr_start = 0;
		stm_cmd.size = game_size;
		stm_cmd.chipid = (chipid>>16); //keep only msb values
		packets = (game_size/64);
		buffer_in[0] = 0;
		pourcent = 0;
		usb_command();
		libusb_bulk_transfer(handle, 0x01, buffer_out, 64, &numBytes, 2000);
		printf("~SMS: Erase 0%%");
		while(buffer_in[0]!=0xFF){
			res = libusb_bulk_transfer(handle, 0x82, buffer_in, 64, &numBytes, 2000);
			if(res!=0){
				printf("~SMS: Error while erasing (err:%u)\n", res);
				free(buffer_read);
				libusb_release_interface(handle, 1);
				libusb_close(handle);
				libusb_exit(NULL);	
				return 0;
			}
			printf("\r~SMS: Erase %d%%",(pourcent++));
			fflush(stdout);
		}
		printf("\r~SMS: Erase 100%%\n");

		//write
		stm_cmd.mode = WRITE_FLASH;
		stm_cmd.adr_start = 0;
		stm_cmd.size = game_size;
		usb_command();
		libusb_bulk_transfer(handle, 0x01, buffer_out, 64, &numBytes, 1000);
		packets = (game_size/PACKET_SIZE);
		printf("~SMS: Write 0%%");
		if(!packets) packets = 1; //min
		for(i=0;i<packets;i++){
			res = libusb_bulk_transfer(handle, 0x02, buffer_read+(i*PACKET_SIZE), PACKET_SIZE, &numBytes, 2000); //specific endpoint write
			if(res!=0){
				printf("~SMS: Error while reading (err:%u)\n", res);
		        free(buffer_read);
		        libusb_release_interface(handle, 1);
	        	libusb_close(handle);
				libusb_exit(NULL);	
		        return 0;
			}
			pourcent = (i*100/packets);
			printf("\r~SMS: Write %d%%",pourcent);
			fflush(stdout);
		}
 		printf("\r~SMS: Write 100%%\n");
      	free(buffer_read);
		
		//read
		stm_cmd.mode = READ; //commun
	  	packets = (game_size/PACKET_SIZE); //nbpackets
	  	stm_cmd.size = PACKET_SIZE; //bytes
	  	stm_cmd.special = mapper; //mapper
	  	buffer_read = (unsigned char *)malloc(game_size);
		printf("~SMS: Read 0%%");
		for(i=0;i<packets;i++){
			stm_cmd.adr_start = (i*PACKET_SIZE); //byte
		    usb_command();
		    libusb_bulk_transfer(handle, 0x01, buffer_out, 64, &numBytes, 500);
		    res = libusb_bulk_transfer(handle, 0x82, buffer_read+(i*PACKET_SIZE), PACKET_SIZE, &numBytes, 500);
		    if(res!=0){
		        printf("~SMS: Error while reading (err:%u)\n", res);
		        free(buffer_read);
		        libusb_release_interface(handle, 1);
	        	libusb_close(handle);
				libusb_exit(NULL);	
		        return 0;
		     }
		     pourcent = (i*100/packets);
		     printf("\r~SMS: Read %d%%",pourcent);
		     fflush(stdout);
		  }
		printf("\r~SMS: Read 100%%\n"); 
		
		crc32_rom = crc32(0, buffer_read, game_size);
		printf("~SMS: CRC32 %s0x%X%s\n", KBLU, crc32_rom, KNRM); 

		
		if(crc32_file != crc32_rom){
			printf("~SMS: CRC32 not matching - %sBAD!%s\n", KRED, KNRM);
		}else{
			printf("~SMS: CRC32 match - %sGOOD!%s\n", KGRN, KNRM);
		}
	  	free(buffer_read);
	  	libusb_release_interface(handle, 1);
		libusb_close(handle);
		libusb_exit(NULL);	

		printf("Write same file again? Y or N \n");  
		while(1){
			char c = getch();
			usleep(100000);
			//printf("getch: %c strcmp y:%u strcmp n:%u\n", c, (strcmp(&c,"y")), (strcmp(&c,"n")));
			if(!(strcmp(&c,"y")) || (strcmp(&c,"y")&0xFF)==64){ goto _rewrite; }
			if(!(strcmp(&c,"n")) || (strcmp(&c,"n")&0xFF)==64){ break; }
		}	
	} 
    return 0;

}





