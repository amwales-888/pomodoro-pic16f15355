/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F15355
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"

//#define BLINKTEST 1


static void timerInterrupt(void);
static void processButton(void);
static void nullFunc(void);
static void processDisplay(void);
static void processWorkExpired(void);
static void processRelaxExpired(void);
static void processIntegrator(void);
static void buttonReleased(void);
static void buttonHeld(void);
static void buttonPressed(void);
static void buzzStart(void);
static void buzzStop(void);

#define MSPERTICK      10
#define MSTOTICKS(x)   ((x)/MSPERTICK)
#define TICKSTOSECS(x) ((x)/(1000/MSPERTICK))

static volatile uint32_t sysTick;
static bool sleep;

static uint8_t buttonIntegrator;
static uint16_t buttonCount;
static bool buttonDstate;

#define DEBOUNCE_TIME               0.05    /* Seconds */
#define DEBOUNCE_SAMPLE_FREQUENCY	100     /* Hertz */

#define DEBOUNCE_MAXIMUM			(DEBOUNCE_TIME * DEBOUNCE_SAMPLE_FREQUENCY)
#define DEBOUNCE_INTEGRATOR_DELAYMS ((1.0/DEBOUNCE_SAMPLE_FREQUENCY)*1000)
#define DEBOUNCE_DELAYMS            (DEBOUNCE_TIME * 1000)

static enum buttonState_e {
    
    BUTTONIDLE=0,
    BUTTONPRESSED,
    BUTTONHELD,
    BUTTONRELEASED,   
            
} buttonState;

#ifdef BLINKTEST
static void testFunc(void) {
    
    IO_LEDG0_Toggle();    
    IO_LEDG1_Toggle();    
}
#endif

static struct process_s {
    
    uint32_t periodTick;
    uint32_t lastTick;
    
    void (*func)(void);

} processList[] = {

#ifdef BLINKTEST    
    { (uint32_t)MSTOTICKS(1000),                          0, testFunc          },
#else    
    { (uint32_t)0,                                        0, nullFunc          },
    { (uint32_t)MSTOTICKS(DEBOUNCE_DELAYMS),              0, processButton     },
    { (uint32_t)MSTOTICKS(1000),                          0, processDisplay    },
    { (uint32_t)MSTOTICKS(DEBOUNCE_INTEGRATOR_DELAYMS),   0, processIntegrator },    
#endif

};

static uint32_t workTimeTicks;
static uint32_t relaxTimeTicks;

static enum ledState_e {
    
    STATEWORK=0,
    STATEWORKWAITINGON,
    STATEWORKWAITINGOFF,
    STATERELAX,
    STATERELAXWAITINGON,        
    STATERELAXWAITINGOFF,        
            
} ledState;

static void timerInterrupt(void) {
 
    /* We are called every system tick */
    
    sysTick++;
}

void setupTimer(void) {
   
    /* Read Work Time */

    if (IO_WORK0_GetValue() == 0) {
        workTimeTicks = MSTOTICKS((uint32_t)15*60*1000);
    }
    else if (IO_WORK1_GetValue() == 0) {
        workTimeTicks = MSTOTICKS((uint32_t)30*60*1000);        
    }
    else if (IO_WORK2_GetValue() == 0) {
        workTimeTicks = MSTOTICKS((uint32_t)45*60*1000);
    }
    else if (IO_WORK3_GetValue() == 0) {
        workTimeTicks = MSTOTICKS((uint32_t)60*60*1000);
    }
        
    /* Read Relax Time*/

    if (IO_RELAX0_GetValue() == 0) {   
        relaxTimeTicks = MSTOTICKS((uint32_t)0*60*1000);
    }
    else if (IO_RELAX1_GetValue() == 0) {
        relaxTimeTicks = MSTOTICKS((uint32_t)5*60*1000);
    }
    else if (IO_RELAX2_GetValue() == 0) {
        relaxTimeTicks = MSTOTICKS((uint32_t)10*60*1000);
    }
    else if (IO_RELAX3_GetValue() == 0) {
        relaxTimeTicks = MSTOTICKS((uint32_t)15*60*1000);
    }
}

static void nullFunc(void){
    
}

static void buzzStart(void) {
    
    /* PWM3POL active_hi; PWM3EN enabled; */
    PWM3CON = 0x80;   
}

static void buzzStop(void) {
    
    /* PWM3POL active_hi; PWM3EN disabled; */
    PWM3CON = 0x00;   
}

static void processIntegrator(void) {

    /* This function MUST be called every 1/DEBOUNCE_SAMPLE_FREQUENCY seconds
     */
    if (IO_BUTTON_GetValue() != 0) {                
        if (buttonIntegrator > 0) {                
            buttonIntegrator--;
        }                               
    } else if (buttonIntegrator < DEBOUNCE_MAXIMUM) {
        buttonIntegrator++;
    }
}

static void buttonReleased(void) {

    if (sleep) {

        /* Stop our system timer */
        
        TMR0_StopTimer();

        /* Wait for any button bouncing to settle */

        __delay_ms(1000);

        /* Go to sleep, the only interrupt active should be on our button */
        
        SLEEP();

        /* An interrupt has woken us up from sleep */
        
        NOP();
        NOP();
        NOP();

        /* restart */
        
        RESET();
    }
}

static void buttonHeld(void) {
    
    /* Test to see if the button has been held down for 3 seconds.
     * If it has then we shutdown 
     */    
        
    if (buttonCount > ((3 * 1000) / DEBOUNCE_DELAYMS)) {
        
        /* Turn off ALL LEDs */
        
        LATA &= 0x80;
        LATC &= 0x80;        
        
        buzzStop();
        
        /* Signal that we want to enter sleep */
        
        sleep = 1;        
    }    
}

static void buttonPressed(void) {

    /* The button was pressed, this is only relevant when the 
     * we are blinking in the work/relax state, if we are then
     * we can reset the time and switch state.
     */    
    switch (ledState) {
        case STATEWORK:
            break;
        case STATEWORKWAITINGON:
        case STATEWORKWAITINGOFF:

            buzzStop();
                        
            processList[0].periodTick = relaxTimeTicks;
            processList[0].lastTick   = sysTick;
            processList[0].func       = processRelaxExpired;

            ledState = STATERELAX;
            break;

        case STATERELAX:
            break;
        case STATERELAXWAITINGON:
        case STATERELAXWAITINGOFF:

            buzzStop();

            processList[0].periodTick = workTimeTicks;
            processList[0].lastTick   = sysTick;
            processList[0].func       = processWorkExpired;

            ledState = STATEWORK;
            break;
    }
}

static void processButton(void) {

    bool report = 0;
    
    /* This function MUST be called every DEBOUNCE_TIME seconds
     */                        
    if  (buttonIntegrator == DEBOUNCE_MAXIMUM) {
        if (buttonDstate == 0) {

            buttonState = BUTTONPRESSED;  
            report = 1;

        } else {

            buttonState = BUTTONHELD;  
            report = 1;
        }

        buttonCount++;
        buttonDstate = 1;

    } else if (buttonIntegrator == 0) {

        if (buttonDstate) {

            /* Released */
            buttonState = BUTTONRELEASED;  
            report = 1;

        } else {

            buttonState = BUTTONIDLE;                      
        }

        buttonCount  = 0;
        buttonDstate = 0;
    } 
    
    if (report) {        
        switch (buttonState) {
            case BUTTONIDLE:
                break;
            case BUTTONRELEASED:
                buttonReleased();
                break;
            case BUTTONHELD:
                buttonHeld();
                break;
            case BUTTONPRESSED:
                buttonPressed();
                break;
        }
    }
}

static void processDisplay(void) {
     
    if (sleep) return;
    
    /* Depending on our current work/relax state reflect the values
     * on the 7 LEDs     
     */    
    switch (ledState) {
        case STATEWORK: {
            
            /* Port C is relax display */
            LATC &= 0x80;
            
            int percentage = (int)((sysTick - processList[0].lastTick) * 100 / processList[0].periodTick);
            int count = (percentage - 1) / (100/7);
            int i;
                        
            /* Port A is working display */

            for (i=0; i<count; i++) {                
                LATA &= ~(0x1 << i);                
            }            
            
            LATA ^= 0x01 << count;
            
            for (i=count+1; i<7; i++) {
                LATA |= 0x1 << i;                
            }
            
            break;
        }
            
        case STATEWORKWAITINGON:
            
            buzzStart();

            /* Port C is relax display */
            LATC &= 0x80;
            
            /* Port A is working display */
            LATA |= 0x7F; 
            
            ledState = STATEWORKWAITINGOFF;
            break;
            
        case STATEWORKWAITINGOFF:
            
            buzzStop();

            /* Port C is relax display */
            LATC &= 0x80;

            /* Port A is working display */
            LATA &= 0x80;

            ledState = STATEWORKWAITINGON;
            break;

        case STATERELAX: {
 
            /* Port A is working display */
            LATA &= 0x80;
            
            int percentage = (int)((sysTick - processList[0].lastTick) * 100 / processList[0].periodTick);
            int count = (percentage - 1) / (100/7);
            int i;

            /* Port C is relax display */
            
            for (i=0; i<count; i++) {                
                LATC &= ~(0x1 << i);                
            }            

            LATC ^= 0x01 << count;
            
            for (i=count+1; i<7; i++) {
                LATC |= 0x1 << i;                
            }

            break;
        }

        case STATERELAXWAITINGON:
            
            buzzStart();

            /* Port C is relax display */
            LATC |= 0x7F;
            
            /* Port A is working display */
            LATA &= 0x80;            
            
            ledState = STATERELAXWAITINGOFF;
            break;

        case STATERELAXWAITINGOFF:

            buzzStop();
            
            /* Port C is relax display */
            LATC &= 0x80;            

            /* Port A is working display */
            LATA &= 0x80;            
            
            ledState = STATERELAXWAITINGON;
            break;
    }          
}

static void processWorkExpired(void) {
     
    if (sleep) return;

    /* Our work time has expired move to work blink state
     */    
    if (ledState == STATEWORK) {        
        ledState = STATEWORKWAITINGON;
    }
}

static void processRelaxExpired(void) {

     if (sleep) return;

     /* Our relax time has expired move to relax blink state
     */    
    if (ledState == STATERELAX) {
        ledState = STATERELAXWAITINGON;
    }
}

/*
                         Main application
 */
void main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    
    /* Wait 1 second for any button bouncing to settle after reset, we may have
     * just been woken with a button press and need to filter it out
     */
    while (IO_BUTTON_GetValue() == 0) {
        __delay_ms(100);        
    }
        
    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();
    
    /* Setup our task timer */
    TMR0_SetInterruptHandler(timerInterrupt);
 
#ifndef BLINKTEST
    /* Read the pin to determine work time and relax time */
    setupTimer();
    
    /* Setup our initial work/relax state, waiting in relaxed state for button 
     * press 
     */   
    processList[0].periodTick = relaxTimeTicks;
    processList[0].lastTick   = sysTick;
    processList[0].func       = processRelaxExpired;
    
    ledState = STATERELAXWAITINGON;
#endif
    
    /* Main loop scheduling processes */
    while (1) {
                
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))        
                
        /* Step through each process and test if period has expired and
         * if has, dispatch the function and adjust it last run time
         */        
        for (int i=0; i<ARRAYSIZE(processList); i++) {
            
            if ((uint32_t)(sysTick - processList[i].lastTick) >= processList[i].periodTick) {
                
                (*processList[i].func)();
                processList[i].lastTick = sysTick;
            }
        }        
    }    
}
/**
 End of File
*/