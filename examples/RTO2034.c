#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#undef gpib		// undef GPIB=VXI11

#ifdef gpib
#include "gpib/ib.h"
#else
#include "vxi11_user.h"
#endif

#define mydebug

#ifdef gpib
void relit(int dev,uint8_t *buffer,int buffer_length)
#else
void relit(CLINK *clink,char *buffer,int buffer_length)
#endif
{
#ifdef gpib
	ibrd(dev, buffer,buffer_length );
	buffer[ThreadIbcntl()-1]=0;
#else
	int ret;
	ret=vxi11_receive(clink,buffer,buffer_length);
	buffer[ret-1]=0;
#endif
}

#ifdef gpib
void envoi(int dev, uint8_t *buffer)
#else
void envoi(CLINK *clink,char *buffer)
#endif
{
#ifdef gpib
 if (ibwrt(dev,buffer,strlen(buffer))&ERR) printf("error writing\n");
#else
 vxi11_send(clink, buffer);
#endif
}

int main( int argc, char *argv[] )
{
#ifdef gpib
	int dev,board_index=0,pad=21,sad=0,send_eoi=1,eos_mode=0;
#else
	char	device_ip[25];
	CLINK	*dev;
#endif
	int delai=1;
	char *buffer;
	static const unsigned long buffer_length = 10000000;
	time_t t1,t2;
	FILE *ff,*ft;
        char filenamef[255];
        char filenamet[255];

	buffer = (char *)malloc( buffer_length );
#ifdef gpib
	dev = ibdev( board_index, pad, sad, T1s, send_eoi, eos_mode );
	if (dev <0) {printf("error opening device\n");return(0);}
#else
	sprintf(device_ip,"169.254.84.124");   // ZNB8
	sprintf(device_ip,"192.168.1.13");     // ZNC3
	sprintf(device_ip,"192.168.1.201");     // ZNC3
	dev = new CLINK;                       // allocate some memory
        if (vxi11_open_device(device_ip,dev)!=0) printf("erreur ouverture\n");
           else printf("connect OK\n");
#endif

	sprintf(buffer,"*IDN?\n");
	envoi(dev,buffer);
	relit(dev,buffer,buffer_length);
	printf("%s\n",buffer);

    float _rate=5000000000.;
    float range=1.0;
    int _channels=3;
    float duration=1e-4;
    int offset;
    int _sample_size = (int)(duration * _rate);
    char *_data_buffer=NULL;
    char mystring[256];
    _data_buffer=(char*)malloc(2*_sample_size+100);
    sprintf(buffer,"ACQ:SRATE %f\n",_rate);envoi(dev,buffer); 
    sprintf(buffer,"ACQ:SRATE?\n");envoi(dev,buffer); relit(dev,buffer,256);
    printf("SRATE OK %s\n",buffer);
    for (int c=1;c<=_channels;c++)
       {sprintf(buffer,"CHAN%d:STAT ON\n",c);envoi(dev,buffer);
        sprintf(buffer,"CHAN%d:STAT?\n",c);envoi(dev,buffer);relit(dev,buffer,256);
        printf("%s\n",buffer);
        sprintf(buffer,"CHAN%d:SCAL %f\n",c,range);envoi(dev,buffer); // RANG is not working ?!
       }
    sprintf(buffer,"CHAN1:SCAL?\n");envoi(dev,buffer); relit(dev,buffer,256);
    printf("SCAL OK %s\n",buffer);
    sprintf(buffer,"TIM:RANGE %f\n",duration);envoi(dev,buffer);     // of the diagram:
    sprintf(buffer,"TIM:RANGE?\n",duration);envoi(dev,buffer); relit(dev,buffer,256);
    printf("TIM OK %s\n",buffer);
    sprintf(buffer,"*CLS\n");
    sprintf(buffer,"FORM:DATA INT,16\n");envoi(dev,buffer); // LSB first by default
    sprintf(buffer,"RUNSINGLE\n");envoi(dev,buffer);
    sprintf(buffer,"*OPC?"); envoi(dev,buffer); relit(dev,buffer,256);

    for (int chan_count=1;chan_count<=_channels;chan_count++)
       {sprintf(mystring,"CHAN%d:WAV:DATA?",chan_count);
        vxi11_send_and_receive(dev, mystring, _data_buffer, (2*_sample_size+100), 100*VXI11_READ_TIMEOUT); // extend timeout
        if (_data_buffer[0]!='#')
           printf("error in trace header\n"); //  printf("%c",buffer[0]);  // #
        else
           {offset=_data_buffer[1]-'0';       // ASCII -> dec
#ifdef mydebug
            printf("#ok => skipping %ld chars\n",offset);
#endif
           }
       }
return 0;
}
