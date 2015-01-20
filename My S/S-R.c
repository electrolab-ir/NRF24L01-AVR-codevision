#include <mega32.h>
#include <stdio.h>
#include <delay.h>
#include <nRF24L01+.h>

char data1;
void main(void)
{
 nRF_Config(0);
 DDRA=0xff;
while (1)
      {
      data1++;
      Send_Data(1 , &data1);
      PORTA=data1;
      delay_ms(500);
      if(data1==255){
      data1=0;
      }
}}
