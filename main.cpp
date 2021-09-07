#include "mbed.h"
#include <stdlib.h>
#include <stdio.h>

typedef union{
    struct{
        unsigned char b0: 1;
        unsigned char b1: 1;
        unsigned char b2: 1;
        unsigned char b3: 1;
        unsigned char b4: 1;
        unsigned char b5: 1;
        unsigned char b6: 1;
        unsigned char b7: 1;
    }bit;
    unsigned char byte;
}_flag;

_flag flag1;

#define ON          1
#define OFF         0

#define TRUE        1
#define FALSE       0

#define INTERVAL    40

#define MS          200
#define TPRESS      1000    
#define BASETIME    500
#define TIMEMAX     1501
#define MAXLED      4
#define NUMBOTONES  4

#define ESPERAR         0
#define JUEGO           1
#define JUEGOTERMINADO  2
#define TECLAS          3

#define RIGHTPRESS      flag1.bit.b1
#define WON             flag1.bit.b2



typedef enum{
    BUTTON_DOWN,//0
    BUTTON_UP,//1
    BUTTON_FALLING,//2
    BUTTON_RISING//3
}_eButtonState;

typedef struct{
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;



_sTeclas ourButton[NUMBOTONES];

uint16_t mask[] = {0x0001,0x0002,0x0004,0x0008};

uint8_t estadoJuego=ESPERAR;

_eButtonState myButton;

void actuallizaMef(uint8_t indice); 

void startMef(uint8_t indice);

void togleLed(uint8_t indice);

BusIn botones(PB_6,PB_7,PB_8,PB_9);
BusOut leds(PB_12,PB_13,PB_14,PB_15);

DigitalOut HearBeat(PC_13);

Timer miTimer;



//int tiempoSB = 0;

int tiempoMs = 0;

RIGHTPRESS=0;
WON=0;



int main(){
    miTimer.start();
    
    int HearBeat=0;

    int tiempoBeat=0;

    tiempoBeat = miTimer.read_ms();
    
    uint16_t ledAuxRandom=0;
    
    uint16_t ledAuxRandomTime=0;
    
    uint8_t indiceAux=0;

    uint8_t indicePress;

    int tiempoRandom=0;
    
    int ledAuxJuegoStart=0;
    uint8_t cont=0;
    
    for(uint8_t indice=0; indice<NUMBOTONES;indice++){
      startMef(indice);  
    }

    while(TRUE){

        switch(estadoJuego){
            case ESPERAR:
                    if((miTimer.read_ms()-tiempoMs) > INTERVAL){
                        tiempoMs = miTimer.read_ms(); 
                        for(uint8_t indice = 0; indice <NUMBOTONES;indice++){
                            actuallizaMef(indice);
                            if(ourButton[indice].timeDiff >= TPRESS){
                                srand(miTimer.read_us());
                                estadoJuego = TECLAS;
                     
                            } 
                        }
                    }

            
            break;
            case TECLAS:
                        for(indiceAux=0; indiceAux<NUMBOTONES; indiceAux++){
                            actuallizaMef(indiceAux);
                            if(ourButton[indiceAux].estado!=BUTTON_UP)
                            break;
                        }
                        if(indiceAux==NUMBOTONES){
                            estadoJuego = JUEGO;
                            leds=15;
                            ledAuxJuegoStart = miTimer.read_ms();
                        }
                        
                        
            break;
            case JUEGO:
                if(leds==0){
                    ledAuxRandom = rand() % (MAXLED);
                    togleLed(ledAuxRandom);
                    ledAuxRandomTime = (rand() % (TIMEMAX))+BASETIME;
                    tiempoRandom=miTimer.read_ms();
                    leds=mask[ledAuxRandom];
                }else{
                    if((miTimer.read_ms()-ledAuxJuegoStart)>TPRESS){
                        if(leds==15){
                            ledAuxJuegoStart=miTimer.read_ms();
                            leds=0;
                        }
                    }
                }
                
                if ((miTimer.read_ms()-tiempoRandom)<=ledAuxRandomTime){
                    for(indicePress = 0; indicePress<NUMBOTONES;indicePress++){
                            actuallizaMef(indicePress);
                            if(ourButton[indicePress].estado == BUTTON_DOWN){
                                cont++;
                                if(indicePress == ledAuxRandom){
                                    RIGHTPRESS=1;
                                }
                            }
                    }
                    
                }
                if ((miTimer.read_ms()-tiempoRandom)>ledAuxRandomTime){
                    leds=0;
                    if(cont==1 && RIGHTPRESS==1){
                        WON = 1;
                    }else{
                        WON=0;
                    }
                    estadoJuego = JUEGOTERMINADO;
                }
                
            break;
            case JUEGOTERMINADO:
                if(WON){

                }else{

                }
            break;
            default :
                    estadoJuego = ESPERAR;
        }
        
       if((miTimer.read_ms()-tiempoBeat) > MS ){
           
           tiempoBeat = miTimer.read_ms();
           HearBeat = !HearBeat;

       }
       if((miTimer.read_ms()-tiempoRandom) > ledAuxRandomTime){
           leds=0;
       }

      
       


    return 0;
    }
}

void startMef(uint8_t indice){  
    ourButton[indice].estado = BUTTON_UP;
}

void actuallizaMef(uint8_t indice){

    switch ((ourButton[indice].estado)){
        case BUTTON_DOWN:
            
            if(botones.read() & mask[indice]){
                ourButton[indice].estado = BUTTON_RISING;
                
            }
        break;
        case BUTTON_UP:
            if(!botones.read() & mask[indice])
                ourButton[indice].estado = BUTTON_FALLING;
        
        break;
        case BUTTON_FALLING:
            if(!botones.read() & mask[indice]){
                ourButton[indice].estado = BUTTON_DOWN;
                //Flanco de bajada
                ourButton[indice].timeDown = miTimer.read_ms();
            }
            else
                ourButton[indice].estado = BUTTON_UP;    
        break;
        case BUTTON_RISING:
            if(botones.read() & mask[indice]){
                ourButton[indice].estado  = BUTTON_UP;
                //Flanco de Subida
                ourButton[indice].timeDiff = miTimer.read_ms()-ourButton[indice].timeDown;
               
                
            }
            else
                ourButton[indice].estado = BUTTON_DOWN;
        break;
        
        default:
            startMef(indice);
            break;
    }
}

void togleLed(uint8_t indice){
    leds = mask[indice];
}