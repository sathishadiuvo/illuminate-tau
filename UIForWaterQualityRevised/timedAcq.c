#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

// /************************************************************
// * TI TDC720x REGISTER SET ADDRESSES
// ************************************************************/

#define TI_TDC720x_CONFIG1_REG                         (0x00)                  //  
#define TI_TDC720x_CONFIG2_REG                         (0x01)                  //  
#define TI_TDC720x_INTRPT_STATUS_REG                   (0x02)                  //  
#define TI_TDC720x_INTRPT_MASK_REG                     (0x03)                  //  
#define TI_TDC720x_COARSE_COUNTER_OVH_REG              (0x04)                  //  
#define TI_TDC720x_COARSE_COUNTER_OVL_REG              (0x05)                  //  
#define TI_TDC720x_CLOCK_COUNTER_OVH_REG               (0x06)                  //  
#define TI_TDC720x_CLOCK_COUNTER_OVL_REG               (0x07)                  //  
#define TI_TDC720x_CLOCK_COUNTER_STOP_MASKH_REG        (0x08)                  //  
#define TI_TDC720x_CLOCK_COUNTER_STOP_MASKL_REG        (0x09)                  // 

#define TI_TDC720x_TIME1_REG                           (0x10)                  //  
#define TI_TDC720x_CLOCK_COUNT1_REG                    (0x11)                  //
#define TI_TDC720x_TIME2_REG                           (0x12)                  //  
#define TI_TDC720x_CLOCK_COUNT2_REG                    (0x13)                  // 
#define TI_TDC720x_TIME3_REG                           (0x14)                  //  
#define TI_TDC720x_CLOCK_COUNT3_REG                    (0x15)                  // 
#define TI_TDC720x_TIME4_REG                           (0x16)                  //  
#define TI_TDC720x_CLOCK_COUNT4_REG                    (0x17)                  //
#define TI_TDC720x_TIME5_REG                           (0x18)                  //  
#define TI_TDC720x_CLOCK_COUNT5_REG                    (0x19)                  // 
#define TI_TDC720x_TIME6_REG                           (0x1A)                  //
#define TI_TDC720x_CALIBRATION1_REG                    (0x1B)                  //
#define TI_TDC720x_CALIBRATION2_REG                    (0x1C)                  //

#define tdc_enable_pin 1 ///GPIO1 ->pin 12
#define osc_enable_pin 4 ///GPIO4 ->pin 16
#define tdc1_interu_pin 25///GPIO25 ->pin 37
#define tdc2_interu_pin 26///GPIO26 ->pin 32

// ///////GPIO program define start ////////////////
#define BCM2708_PERI_BASE       0x3f000000
#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)	// GPIO controller 

#define BLOCK_SIZE 		(4*1024)

#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
 
#define GPIO_SET  *(gpio.addr + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR  *(gpio.addr + 10) // clears bits which are 1 ignores bits which are 0
 
#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))
// ////////GPIO program define end/////////////////////

#define clockfrequency 8000000.0
///TDC clock frequency

// IO Access
struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};
struct bcm2835_peripheral gpio = {GPIO_BASE};

long getMicrotime();
int spi_open();
int spi_close();
int spi_configure();
int spi_txfr(uint8_t* data,int data_length);
void tdc_send(uint8_t addr, uint8_t value);
uint8_t tdc_recv(uint8_t addr);
uint32_t tdc_long_recv(uint8_t addr);
void tdc_init();
double tdc_measure();
void tdc_store(double value);
void path(char letter, int clear);

int spi_fd;
int spi_mode = 0x00;
uint8_t spi_bpw = 8;//8=cs0 7=cs0
int spi_speed = 25000000;
uint16_t spi_delay = 0;
int spi_toggle_cs = 0;

double calib2periods = 10.0;
double clockperiod = 1.0/clockfrequency;
char sampleData[80];
char export[]={"/export.csv"};
char file_path[100];

void main(int sec)
{
	int ExpDone = 0;
	int SetTime = sec;
	int i;
	double tdcval;

	printf("%s\n",file_path);
	printf("i am in C main function !!!!!!\n\n");\
	printf("Sec = %d\n",SetTime);

	uint32_t counts = 0;
	
	tdc_init();
	
	// FILE *fptr = fopen(path,"r");
	// if(fptr == NULL)
	// 	printf("File open failed: %s",strerror(errno));
	// fgets(sampleData, 100, fptr);
	// fclose(fptr);
	// strcat(sampleData,export);
	// printf("I am in c program main funciton - Raw data path = %s\n",sampleData);

	FILE *fptr= fopen(file_path,"w");
	if(fptr == NULL)
		printf("File open failed: %s",strerror(errno));
	fprintf(fptr,"tof\n");
	fclose(fptr);

	time_t time_start = time(NULL);
	
	while(1)
	{	
		if(time(NULL)-time_start > SetTime)
		{
     		break;
		}
		tdcval = tdc_measure();
		if(tdcval<=100.00)
		{
			tdc_store(tdcval);
		}
		i=0;
	}

}

void path(char letter, int clear)
{
	static int i;
	if(clear==0)
	{
		file_path[i]=letter;
		i++;
	}
	else
	{
		file_path[i]=letter;
		i=0;
	}
}

// void blink()
// {
// 	if(map_peripheral(&gpio) == -1) 
// 	{
// 	   printf("\rFailed to map the physical GPIO registers into the virtual memory space.\n");
// 	   // return 0;
// 	}
	  
// 	// Define pin 7 as output
// 	INP_GPIO(5);
// 	OUT_GPIO(5);
// 	while(1)
// 	{
// 	    printf("\rled on\n");
// 	    GPIO_SET = 1 << 5;
// 	    sleep(1);///1 sec on
	  
// 	    printf("\rled off\n");
// 	    GPIO_CLR = 1 << 5;
// 	    sleep(1);///1 sec off
// 	} 
// }
// //struct bcm2835_peripheral gpio = {GPIO_BASE};

int map_peripheral(struct bcm2835_peripheral *p)
{
   if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("\rFailed to open /dev/mem, try checking permissions.\n");
      return -1;
   }
 
   p->map = mmap(
      NULL,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      p->mem_fd,      // File descriptor to physical memory virtual file '/dev/mem'
      p->addr_p       // Address in physical map that we want this memory block to expose
   );
 
   if (p->map == MAP_FAILED) {
        perror("mmap");
        return -1;
   }
 
   p->addr = (volatile unsigned int *)p->map;
 
   return 0;
}

// void unmap_peripheral(struct bcm2835_peripheral *p) {
//     munmap(p->map, BLOCK_SIZE);
//     close(p->mem_fd);
// }

void tdc_send(uint8_t addr, uint8_t value) {
    uint8_t inst = 0x40 | (addr & 0x7F);
    uint8_t data[2];
    data[0] = inst;
    data[1] = value;
    spi_txfr(data,2);
}

uint8_t tdc_recv(uint8_t addr) {
	uint8_t inst = 0xBF & (addr & 0x7F);
	uint8_t data[2];
	data[0] = inst;
	data[1] = 0;
	spi_txfr(data,2);
	return data[1];
}

uint32_t tdc_long_recv(uint8_t addr)
{
	uint8_t inst = 0xBF & (addr & 0x7F);
    uint8_t data[3];
    data[0] = inst;
    data[1] = 0;
	data[2] = 0;
	data[3] = 0;
    spi_txfr(data,4);
    return data[1]<<16 | data[2]<<8 | data[3];
}

void tdc_init()
{
	if(map_peripheral(&gpio) == -1) 
	{
	   printf("\rFailed to map the physical GPIO registers into the virtual memory space.\n");
	   // return 0;
	}
	// if(wiringPiSetup() == -1)
	// {
	// 	printf("setup wiringPi failed !\n");
	// }


	// INP_GPIO(5);
	// OUT_GPIO(5);
// 	while(1)
// 	{
	    // printf("\rled on\n");
	    // GPIO_SET = 1 << 5;
	    // sleep(1);///1 sec on
	  
	    // printf("\rled off\n");
	    // GPIO_CLR = 1 << 5;
	    // sleep(1);///1 sec off

	// // pinMode(tdc_enable_pin, OUTPUT);
	OUT_GPIO(tdc_enable_pin);
	// // pinMode(osc_enable_pin, OUTPUT);
	OUT_GPIO(osc_enable_pin);
	// // pinMode(tdc2_interu_pin, INPUT);//Added to enable TDC2
	// INP_GPIO(tdc1_interu_pin);
	// // pinMode(tdc1_interu_pin, INPUT);
	// INP_GPIO(tdc2_interu_pin);
	GPIO_SET=1<<tdc_enable_pin;
	GPIO_SET=1<<osc_enable_pin;
	// digitalWrite(osc_enable_pin, HIGH);
	// printf("i am in tdc_init function");
	spi_open();
	spi_configure();
	printf("TI_TDC720x_CONFIG1_REG: %x\n",tdc_recv(TI_TDC720x_CONFIG1_REG));
	printf("TI_TDC720x_CONFIG2_REG: %x\n",tdc_recv(TI_TDC720x_CONFIG2_REG));
	tdc_send(TI_TDC720x_COARSE_COUNTER_OVH_REG,0x01);
	tdc_send(TI_TDC720x_COARSE_COUNTER_OVL_REG,0x20);
	printf("TI_TDC720x_COARSE_COUNTER_OVH_REG: %x\n",tdc_recv(TI_TDC720x_COARSE_COUNTER_OVH_REG));
	printf("TI_TDC720x_COARSE_COUNTER_OVL_REG: %x\n",tdc_recv(TI_TDC720x_COARSE_COUNTER_OVL_REG));
}
// void tdc_deinit()
// {
// 	spi_close();
// }

double tdc_measure()
{
	// if(wiringPiSetup() == -1)
	// {
	// 	printf("setup wiringPi failed !\n");
	// }
	tdc_send(TI_TDC720x_CONFIG1_REG,0x01);//Start Measurement
	//time_t tMeasStart=time(NULL);
	//struct timeval tval_before, tval_after, tval_result;
    //gettimeofday(&tval_before, NULL);
	//printf("Waiting for measurement\n");
	// while(digitalRead(tdc2_interu_pin))//Waiting for TDC 2 interrupt to deassert
	while(GPIO_READ(tdc2_interu_pin))
	{
	   // gettimeofday(&tval_after, NULL); 	
       // timersub(&tval_after, &tval_before, &tval_result);
       // if (tval_result.tv_usec>200)//Detects if the TDC is idle for more than 200 microseconds (arrived by experimental testing) and breaks out
	   // {
		   // printf("TDC is idle fot too long!\n");
		   // NoActivityOnTDC=true;
		   // break;
	   // }
	};//Wait for start and stop
	// printf("Measurement done\n");
	double calib1 = (double)tdc_long_recv(TI_TDC720x_CALIBRATION1_REG);
	double calib2 = (double)tdc_long_recv(TI_TDC720x_CALIBRATION2_REG);
	//The following three lines of code are being moved out as global steps to reduce redundant delay.
	//double calib2periods = 10.0;
	//double clockfrequency = 8000000.0;
	//double clockperiod = 1.0 / clockfrequency;
	double tdctime = (double)(tdc_long_recv(TI_TDC720x_TIME1_REG) & 0x7FFFFF);//Get time counter value
	double calcount = (calib2 - calib1) / (calib2periods - 1.0);
	double normLSB = (clockperiod / calcount)*1000000000;
	double TOF = tdctime * normLSB;
	return TOF;
}

void tdc_store(double value)
{
	// strcat(sampleData,export);
	// printf("I am in c program main funciton - Raw data path = %s\n",sampleData);

	FILE *fptr = fopen(file_path,"a");
	if(fptr == NULL)
		printf("File open failed: %s",strerror(errno));
	fprintf(fptr,"%lu,%f\n",getMicrotime(),value);
	fclose(fptr);
}

int spi_open()
{
    spi_fd = open("/dev/spidev0.1",O_RDWR);
    if (spi_fd == -1)
    {
        printf("spi_open failed: %s\n",strerror(errno));
        return -1;
    }
    return 0;
}
int spi_close()
{
    close(spi_fd);
    return 0;
}
int spi_configure()
{
    int ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode);
    if (ret == -1)
    {
        printf("SPI_IOC_WR_MODE failed: %s\n",strerror(errno));
        spi_close();
        return -1;
    }
    ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &spi_mode);
    if (ret == -1)
    {
        printf("SPI_IOC_RD_MODE failed: %s\n",strerror(errno));
        spi_close();
        return -1;
    }
    ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bpw);
    if (ret == -1)
    {
        printf("SPI_IOC_WR_BITS_PER_WORD failed: %s\n",strerror(errno));
        spi_close();
        return -1;
    }
    ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bpw);
    if (ret == -1)
    {
        printf("SPI_IOC_RD_BITS_PER_WORD failed: %s\n",strerror(errno));
        spi_close();
        return -1;
    }
    ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
    if (ret == -1)
    {
        printf("SPI_IOC_WR_MAX_SPEED_HZ failed: %s\n",strerror(errno));
        spi_close();
        return -1;
    }
    ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
    if (ret == -1)
    {
        printf("SPI_IOC_RD_MAX_SPEED_HZ failed: %s\n",strerror(errno));
        spi_close();
        return -1;
    }
    return 0;
}

long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

int spi_txfr(uint8_t* data,int data_length)
{
    struct spi_ioc_transfer spi_xfr = {0};
    char txbuf[data_length];
    char rxbuf[data_length];
    memcpy(txbuf, data, data_length);
    memset(rxbuf, 0xff, data_length);
    spi_xfr.tx_buf        = (unsigned long)txbuf;
    spi_xfr.rx_buf        = (unsigned long)rxbuf;
    spi_xfr.len           = data_length;
    spi_xfr.speed_hz      = spi_speed;
    spi_xfr.delay_usecs   = spi_delay;
    spi_xfr.bits_per_word = spi_bpw;
    spi_xfr.cs_change     = spi_toggle_cs;
    spi_xfr.pad           = 0;
    int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_xfr);
    if (ret < 0)
    {
        printf("SPI_IOC_MESSAGE failed: %s\n",strerror(errno));
        spi_close();
        return -1;
    }
	for(int i = 0; i<data_length;i++)
		data[i] = rxbuf[i];
    return 0;
}

// #include <stdio.h> 
// int fun(int a,int b);
// int call(int a,int e);

// int call(int a,int e) 
// { 
//     // fun(a,e); 
//     return(fun(a,e));
// } 

// int fun(int a,int b) 
// { 
//     return a*b; 
// } 