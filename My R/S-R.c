
#include <mega32.h>
#include <nRF24L01+.h>
#include <stdio.h>
#include <delay.h>

char data1;
void main(void)
{

DDRA=0xff;
nRF_Config(1);
while (1)
      {
      if(State == 1)
        {
        data1 = payload[1];
        PORTA=data1;
        State = 0;
        }
      }
}
