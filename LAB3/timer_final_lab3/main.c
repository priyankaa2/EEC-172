/*
 *
 * LAB 3: EEC 172
 * Priyanka Shah
 * Feng Cai
 *
 */




// Standard include
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "interrupt.h"
#include "hw_ints.h"
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "timer.h"
#include "utils.h"
#include "spi.h"
#include "uart.h"
// Common interface includes
#include "timer_if.h"
#include "gpio_if.h"
#include "gpio.h"
#include "pinmux.h"
#include "Adafruit_GFX.h"
//#include "test.h"
#include "i2c_if.h"
#include "uart_if.h"

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION        "1.1.1"
#define FOREVER                    1
#define CONSOLE              UARTA0_BASE
#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define MAX_STRING_LENGTH    80


extern int cursor_x;
extern int cursor_y;


// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

//*****************************************************************************
//                      Global Variables for Vector Table
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif



//*****************************************************************************
//
// Globals used by the timer interrupt handler.
//
//*****************************************************************************
#define PIN_READ 0x10
#define REMOTE_BASE GPIOA1_BASE
#define OLEDCS_BASE GPIOA0_BASE
#define OLEDCS_CONTROL 0x80
#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

/*
 * The values of the buttons read by our remote configured using
 * the remote ID: 1287
 *
 */
#define ONE 0x717ee81
#define TWO 2065047198
#define THREE -1156243794
#define FOUR 991370894
#define FIVE -619405642
#define SIX 1528209046
#define SEVEN -1693081946
#define EIGHT 454532742
#define NINE -350986566
#define ZERO 1796628122
#define MUTE 857161356
#define LAST -1886009693

extern void (* const g_pfnVectors[])(void);

volatile int temp2, bufferLength;
volatile int direction;

static unsigned long timer0Base = TIMERA0_BASE;
static unsigned long timer1Base = TIMERA1_BASE;
static unsigned long timer2Base = TIMERA2_BASE;

volatile short temp1, new, count = 1;

volatile signed long value;
volatile int timer_Count;
volatile int test;

volatile int bRxDone;

volatile int buttom_x, buttom_y;

char buffer[8] = "\0";
char newChar, oldChar = '\0';



void printChar(char ch, short direction)
{
    //Function to display a character on the OLED display without it overlapping, and until it fills the whole screen
    int textsize = 1;

    if (direction < 1) {
        fillRect(cursor_x, cursor_y, 6, 8, BLACK);
        if (direction < 0) {
            buffer[strlen(buffer)-1] = '\0';
        }
    }
    cursor_x += 6 * direction;
    if (cursor_x < 0) {
        if (cursor_y == 0) {
            cursor_x = 0;
        } else {
            cursor_x = 121;
            cursor_y -= 8;
        }
    }
    drawChar(cursor_x, cursor_y, ch, 0xFFFF, 0xFFFF, textsize);
    if (cursor_x > 124) {
        cursor_x = 0;
        cursor_y += 8;
    }
}

void printCharButtom(char ch, short direction)
{
    //function to print character on the OLED display after the button input is taken
    int textsize = 1;

    if (direction < 1) {
        fillRect(buttom_x, buttom_y, 6, 8, BLACK);
        if (direction < 0) {
            buffer[strlen(buffer)-1] = '\0';
        }
    }
    buttom_x += 6 * direction;
    if (buttom_x < 0) {
        if (buttom_y == 0) {
            buttom_x = 0;
        } else {
            buttom_x = 121;
            buttom_y -= 8;
        }
    }
    drawChar(buttom_x, buttom_y, ch, 0xFFFF, 0xFFFF, textsize);
    if (buttom_x > 124) {
        buttom_x = 0;
        buttom_y += 8;
    }
}


void timer0BaseIntHandler(void)
{
    //First base timer0 timer handler
    Timer_IF_InterruptClear(timer0Base);
    timer_Count++;
    temp2++;
}

void TimerRefIntHandler(void)
{
    //Second base timer1 timer handler
    Timer_IF_InterruptClear(timer1Base);
    MAP_GPIOIntEnable(REMOTE_BASE, PIN_READ);
   // MAP_TimerEnable(timer2Base, TIMER_A);
}

static void GPIOA0IntHandler(void) { // GPIO falling handler
    unsigned long ulStatus;
    int tmp;
    MAP_TimerDisable(timer0Base, TIMER_A);
    MAP_TimerDisable(timer1Base, TIMER_A);
    //MAP_TimerDisable(timer2Base, TIMER_A);
    Timer_IF_InterruptClear(timer0Base);
    ulStatus = MAP_GPIOIntStatus (REMOTE_BASE, true);
    MAP_GPIOIntClear(REMOTE_BASE, ulStatus);        // clear interrupts on GPIOA1
    tmp = timer_Count;
    timer_Count = 0;


   //test++;
   // printf("TEMP: %d\n",tmp);
   //printf("Timer_COUNTER: %d\n",tmp);


    value <<=1;
    if (tmp > 31){
        value++;
    }
    MAP_TimerEnable(timer0Base, TIMER_A);
}


static void UARTIntHandler()
{
    //
    // initiates the UART int handler
    //
    if(!bRxDone) {
        MAP_UARTDMADisable(UARTA0_BASE,UART_DMA_RX);
        bRxDone = true;
    }
    else {
        MAP_UARTDMADisable(UARTA0_BASE,UART_DMA_TX);
    }
    MAP_UARTIntClear(UARTA0_BASE,UART_INT_DMATX|UART_INT_DMARX);
    char tmp = UARTCharGet(UARTA1_BASE);
    printf("Im in the uart int handler\n");
    printCharButtom(tmp, 1);
}


void TimerPulseIntHandler(void)
{
    //Main timer handler
    Timer_IF_InterruptClear(timer2Base);
    MAP_TimerDisable(timer2Base, TIMER_A);
    strcat(buffer, &oldChar);
    if (oldChar != '\0') {
        bufferLength++;
    }
    oldChar = '\0';
    direction = 1;
    temp1= -1;
    count = 1;
}

/* Resets the array to the start value*/
void resetArray(short array[], short length)
{
    int i;
    for (i = 0; i < length; i++) {
        array[i] = 0;
    }
}

/*Finda max value*/
short maxIndex(short array[], short length)
{
    short i, index = 0, tmp = 0;
    for (i = 0; i < length; i++) {
        if (array[i] > tmp) {
            tmp = array[i];
            index = i;
        }
    }
    return index;
}
//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


/*This function decodes the signals recieved by the IR remote
 *  and then saves them in an output array by matching them to
 *   the decoded value found using the logic analyser to study the waveforms
 */

void decodeValue(unsigned long value, short output[12])
{
    switch (value){
    case ZERO:
        output[0]++;
        puts("ZERO");

        break;

    case ONE:
        output[1]++;
        puts("ONE");
        break;

    case TWO:
        output[2]++;
        puts("TWO");
        break;

    case THREE:
        output[3]++;
        puts("THREE");
        break;

    case FOUR:
        output[4]++;
        puts("FOUR");
        break;

    case FIVE:
        output[5]++;
        puts("FIVE");
        break;

    case SIX:
        output[6]++;
        puts("SIX");
        break;

    case SEVEN:
        output[7]++;
        puts("SEVEN");
        break;

    case EIGHT:
        output[8]++;
        puts("EIGHT");
        break;

    case NINE:
        output[9]++;
        puts("NINE");
        break;

    case MUTE:
        output[10]++;
        puts("MUTE");
        break;

    case LAST:
        output[11]++;
        puts("LAST");
        break;

    default:
        //puts("RANDOM");
        break;
    }
}



/*Uses the decoded button number pressed to convert the signal into a character
 * The variable count keeps track of how many times it was pressed and assigns
 * correct letter to the corresponding button presses
 */
char buttonToChar(int button, int count)
{
    short tmp;
    char res;
    switch(button) {
    case 2:
        tmp = count % 3; //mod 3 due to there being 3 characters for each button
        if (tmp) {
            res = 'a' + tmp - 1;
        } else {
            res = 'a' + 2;
        }
        break;
    case 3:
        tmp = count % 3;
        if (tmp) {
            res = 'd'+tmp- 1;
        } else {
            res = 'd'+2;
        }
        break;
    case 4:
        tmp = count % 3;
        if (tmp) {
            res = 'g'+tmp-1;
        } else {
            res = 'g'+2;
        }
        break;
    case 5:
        tmp = count % 3;
        if (tmp) {
            res = 'j'+tmp-1;
        } else {
            res = 'j'+2;
        }
        break;
    case 6:
        tmp = count % 3;
        if (tmp) {
            res = 'm' + tmp - 1;
        } else {
            res = 'm' + 2;
        }
        break;
    case 7:
        tmp = count % 4;
        if (tmp) {
            res = 'p'+ tmp - 1;
        } else {
            res = 'p'+3;
        }
        break;
    case 8:
        tmp = count % 3;
        if (tmp) {
            res = 't'+ tmp - 1;
        } else {
            res = 't'+2;
        }
        break;
    case 9:
        tmp = count % 4;
        if (tmp) {
            res = 'w'+ tmp - 1;
        } else {
            res = 'w'+3;
        }
        break;
    case 0:
        res = ' ';
        break;

    default: res = '\0'; break;
    }
    return res;
}


int
main(void)
  {
    unsigned long ulStatus;
    short mainArray[12];

    //resetArray(Array, 12);
    temp1= -1;

      //Initialize board, pinmux and terminal
    BoardInit();
    PinMuxConfig();
    InitTerm();
    ClearTerm();
   // bRxDone = false;
    puts("1");
    
      
      //
    // Reset the peripheral
    //

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
       MAP_PRCMPeripheralReset(PRCM_GSPI);
       //MAP_SPIReset(GSPI_BASE);
       MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                       SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_3,
                       (SPI_SW_CTRL_CS |
                       SPI_4PIN_MODE |
                       SPI_TURBO_OFF |
                       SPI_CS_ACTIVELOW |
                       SPI_WL_8));
       MAP_SPIEnable(GSPI_BASE);
       //MAP_SPICSEnable(GSPI_BASE);
       Adafruit_Init();
       fillScreen(BLUE);


       //Configure UART

    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                            UART_BAUD_RATE,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));
    MAP_UARTIntRegister(UARTA1_BASE,UARTIntHandler);
    MAP_UARTIntEnable(UARTA1_BASE,UART_INT_DMARX);
    MAP_UARTDMAEnable(UARTA1_BASE,UART_DMA_TX);
    printf(" uart enabledr\n");



/* Configure interrupts */

    Timer_IF_Init(PRCM_TIMERA0, timer0Base, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_Init(PRCM_TIMERA1, timer1Base, TIMER_CFG_PERIODIC, TIMER_A, 0);
      
      
      // Setup the interrupts for the timer timeouts.
       // puts("1");
       puts("2");
       Timer_IF_IntSetup(timer0Base, TIMER_A, timer0BaseIntHandler);
       Timer_IF_IntSetup(timer1Base, TIMER_A, TimerRefIntHandler);


       MAP_TimerLoadSet(timer0Base, TIMER_A, MILLISECONDS_TO_TICKS(0.1));
       MAP_TimerLoadSet(timer1Base, TIMER_A, MILLISECONDS_TO_TICKS(100));


      
       // Register the interrupt handlers
       puts("3");
       MAP_GPIOIntRegister(REMOTE_BASE, GPIOA0IntHandler);


    
       // Configure falling edge interrupts
       puts("4");
       MAP_GPIOIntTypeSet(REMOTE_BASE, PIN_READ, GPIO_FALLING_EDGE);
       ulStatus = MAP_GPIOIntStatus (REMOTE_BASE, false);
       MAP_GPIOIntClear(REMOTE_BASE, ulStatus);
      // clear interrupts on REMOTE


       // Enable interrupts
       puts("5");
       MAP_GPIOIntEnable(REMOTE_BASE, PIN_READ);
       MAP_TimerEnable(timer1Base, TIMER_A);

       //Initialize registers etc for OLED
       Adafruit_Init();

/* This while loop calls all other functions to print the corresponding characters on OLED once a button is pressed */
       while(1)
    {
        //fillScreen(RED);
         //printf("Printed red");

           if (timer_Count > 6000  )
        {

        MAP_TimerDisable(timer0Base, TIMER_A);
        decodeValue(value, mainArray);
        printf("No. of times pressed: %d\n", Array[1]);

        new = maxIndex(mainArray, 12);
                    MAP_TimerLoadSet(timer2Base, TIMER_A, MILLISECONDS_TO_TICKS(1000));
                    if (temp1> 0) {
                        if (new == temp1) {
                           
                            count++; //Gives the correct character according to how many times one button is pressed
                             direction = 0; //used for printing on OLED
                            
                        } else {
                           
                            count = 1;
                             direction = 1;
                        }
                    }
                    newChar = buttonToChar(new, count); //Convert button press to charachters, stored in newChar
                    printf("Alpha: %c <---\n", newChar);
                   //fillScreen(RED);
                   //printf("Printed red");

                   value=0;
        timer_Count = 0;

        if (new == 10) { // mute
                        direction = -1;
                    }
                    if (new == 11) { // last
                        int len = strlen(buffer);
                        int i;
                        for (i = 0; i < len; i++) {
                            printf("buffer data %s", buffer[i]);
                            UARTCharPut(UARTA1_BASE, buffer[i]);
                        }
                        fillRect(0, 0, 127, 64, BLACK);
                        setCursor(0, 64); //Offset on the OLED screen
                        for (i = 0; i < len; i++) {
                               printChar(buffer[i], direction); //Printing on the OLED
                        }
                        buffer[0] = '\0';

                    }

                    printChar(newChar, direction);
                    if ((direction > 0) && (temp1!= new)) {
                        strcat(buffer, &oldChar);
                        if (oldChar != '\0'){
                            bufferLength++;
                        }
                        oldChar = '\0';
                    }

            
                    oldChar = newChar;
                    temp1= new;

            /* Resetting all variables to initial conditions*/
            
                    resetArray(mainArray, 12);
            temp2= 0;
            value = 0;
            
                    timer_Count = 0;

            

            //Reenable timers
                    MAP_TimerEnable(timer1Base, TIMER_A);
                    MAP_TimerEnable(timer2Base, TIMER_A);

        }
    }
}




