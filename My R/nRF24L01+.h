#ifndef __nRF24L01+_H
#define __nRF24L01+_H

#include <mega32.h>
#include <nRF24L01+.h>
#include <stdio.h>
#include <delay.h>
// SPI functions
#include <spi.h>


#define CE  PORTB.3
#define CSN PORTB.4

/* Instruction Mnemonics */
#define R_REGISTER            0x00
#define W_REGISTER            0x20
#define REGISTER_MASK         0x1F
#define ACTIVATE              0x50
#define R_RX_PL_WID           0x60
#define R_RX_PAYLOAD          0x61
#define W_TX_PAYLOAD          0xA0
#define W_ACK_PAYLOAD         0xA8
#define FLUSH_TX              0xE1
#define FLUSH_RX              0xE2
#define REUSE_TX_PL           0xE3
#define NOP                   0xFF


/* Memory Map */
#define CONFIG                0x00
#define EN_AA                 0x01
#define EN_RXADDR             0x02
#define SETUP_AW              0x03
#define SETUP_RETR            0x04
#define RF_CH                 0x05
#define RF_SETUP              0x06
#define STATUS                0x07
#define OBSERVE_TX            0x08
#define CD                    0x09
#define RX_ADDR_P0            0x0A
#define RX_ADDR_P1            0x0B
#define RX_ADDR_P2            0x0C
#define RX_ADDR_P3            0x0D
#define RX_ADDR_P4            0x0E
#define RX_ADDR_P5            0x0F
#define TX_ADDR               0x10
#define RX_PW_P0              0x11
#define RX_PW_P1              0x12
#define RX_PW_P2              0x13
#define RX_PW_P3              0x14
#define RX_PW_P4              0x15
#define RX_PW_P5              0x16
#define FIFO_STATUS           0x17
#define DYNPD                 0x1C
#define FEATURE               0x1D




#pragma used+
/* library function prototypes */

flash unsigned char Base_Addrs[5]={0x00,0x01,0x03,0x07,0x00};
unsigned char Temp_Addrs[5]={0x00,0x01,0x03,0x07,0x00};
unsigned char payload[33];
unsigned char Command_Reg = 0,Status_Reg = 0,State = 0;
char Opr_Mode;
bit send_actived = 0;


void Set_Reg(unsigned char ins)
{
    int i;                              
    CSN = 0;    
    Status_Reg=spi(ins);       
    switch(ins & 0xE0)
    {
        case 0x00:
        {                         
            if((ins & 0x1F)==0x0A || (ins & 0x1F)==0x0B || (ins & 0x1F)==0x10) 
            {
                for(i=4;i>=0;i--)
                {
                    Temp_Addrs[i]=spi(NOP);
                }
            }
            else
            {
                Command_Reg=spi(NOP);   
            }
            break;
        }
        case W_REGISTER:  
        {                         
            if((ins & 0x1F)==0x0A || (ins & 0x1F)==0x0B || (ins & 0x1F)==0x10)
            {
                for(i=4;i>=0;i--)
                {
                    spi(Base_Addrs[i]);
                } 
            }
            else
            {  
                spi(Command_Reg);
            }
            break;
        }         
        case R_RX_PL_WID:
        {
            if((ins & 0x01)==1)
            {
                i=payload[0];
                while(i!=0)
                {
                    payload[i]=spi(NOP); 
                    i--;
                }    
            }
            else 
            {
                Command_Reg=spi(NOP);    
            }
            break;
        }
        case W_TX_PAYLOAD:
        {    
            i=payload[0];
            while(i!=0)
            {
                spi(payload[i]); 
                i--;
            }
            break;
        }
        
    }   
    CSN=1;  
    delay_us(10);
}




// External Interrupt 2 service routine
interrupt [EXT_INT2] void ext_int2_isr(void)
{
// Place your code here
    if(Opr_Mode==0)
    {
        Set_Reg(NOP); 
        if(Status_Reg & W_REGISTER)  
        {
            State = 2;
            Set_Reg(FIFO_STATUS);        
            if((Command_Reg & 0x01)==0)
            {
                Set_Reg(R_RX_PL_WID);  
                if(Command_Reg<=32)
                {
                    payload[0]=Command_Reg; 
                    Set_Reg(R_RX_PAYLOAD);    
                    State = 3;
                }
                else
                    Set_Reg(FLUSH_RX);
            }          
        }
        else 
        {
            State = 4;
        }
    }                                                     
    else
    {
        Set_Reg(R_RX_PL_WID); 
        if(Command_Reg>32)
        {
            Set_Reg(FLUSH_RX);
        }
        else
        {
            payload[0]=Command_Reg;
            Set_Reg(R_RX_PAYLOAD);
            State = 1;
        }
    }     
    Command_Reg = 0x7E; 
    Set_Reg(0x27);    
    Set_Reg(FLUSH_TX);        
}





void Send_Data(char num , char *data)
{ 
    int counter = 0; 
    
    payload[0] = num;
    
    for(counter=0;counter<num;counter++)
        payload[counter+1] = *(data+counter);
    
    if(send_actived)
    {
        send_actived = 0;  
        if((Temp_Addrs[4]==Base_Addrs[4]) && (Temp_Addrs[3]==Base_Addrs[3]) && (Temp_Addrs[2]==Base_Addrs[2]) && (Temp_Addrs[1]==Base_Addrs[1]) && (Temp_Addrs[0]==Base_Addrs[0]))
        {
            Set_Reg(FLUSH_TX); 
            Set_Reg(W_TX_PAYLOAD);           
            delay_ms(10);
            CE = 1;
            delay_us(20);
            CE = 0;
            delay_ms(10);
        }
        else
            State = 5; 
    } 
    
    if(State!=0)
        {
        send_actived=1; //faal sazi ersal mojadad
        State=0;
        }                                      
}



void nRF_Config(char mode)
{
    // Port B initialization
    // Func7=Out Func6=In Func5=Out Func4=Out Func3=In Func2=In Func1=In Func0=In 
    // State7=0 State6=T State5=0 State4=0 State3=T State2=T State1=T State0=T 
    PORTB=0x00;
    DDRB=0xB0;
    
    CSN = 1; 
    CE = 0;
    
    
    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    // INT2: On
    // INT2 Mode: Falling Edge
    GICR|=0x20;
    MCUCR=0x00;
    MCUCSR=0x00;
    GIFR=0x20;
    
    
    // SPI initialization
    // SPI Type: Master
    // SPI Clock Rate: 2000.000 kHz
    // SPI Clock Phase: Cycle Start
    // SPI Clock Polarity: Low
    // SPI Data Order: MSB First
    SPCR=0x50;
    SPSR=0x00;

    
    #asm("sei")
           
    delay_ms(110);
    
    Opr_Mode = mode;   
    
    Command_Reg = 0x01;
    Set_Reg(0x21);   
    
    Command_Reg = 0x01;
    Set_Reg(0x22);   
      
    Command_Reg = 0x03;
    Set_Reg(0x23);   
    
    Command_Reg = 0x2f;
    Set_Reg(0x24);   
                       
    Command_Reg = 0x01;   
    Set_Reg(0x25);      
    
    Command_Reg = 0x06;   
    Set_Reg(0x26);      
    
    Set_Reg(0x2A);      
    
    Set_Reg(0x30);      
    
    Command_Reg = 0x01;   
    Set_Reg(0x3C);      
        
    Command_Reg = 0x07;   
    Set_Reg(0x3D);          
    
    if(mode==0)
    {
        Command_Reg = 0x4E;
        Set_Reg(W_REGISTER);   
        delay_ms(100);
        send_actived = 1;
    }
    else
    {
        Command_Reg = 0x3F;
        Set_Reg(W_REGISTER);   
        delay_ms(5);
        CE = 1;
    } 
}


#pragma used-


//#pragma library nRF24L01+.lib


#endif
