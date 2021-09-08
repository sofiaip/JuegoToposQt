#include "mbed.h"
#include <stdlib.h>

/**
 * @brief Union de estructura que contiene un mapa de bit
 * se usa para banderas
 * 
 */
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

/**
 * @brief Declaro mi flag
 * 
 */
_flag flag1;

/**
 * @brief Defino valor encendido-apagado
 * 
 */
#define ON          1
#define OFF         0

/**
 * @brief Defino valor para cuando una sentencia es verdadera o falsa
 * 
 */
#define TRUE        1
#define FALSE       0

/**
 * @brief Defino el intervalo entre lecturas para "filtrar" el ruido del Botón
 * 
 */
#define INTERVAL    40

/**
 * @brief Defino el intervalo entre latidos del hearbeat
 * 
 */
#define MS          200
/**
 * @brief Defino el intervalo de espera al presionar una tecla
 * 
 */
#define TPRESS      1000 
/**
 * @brief Defino minimo valor del tiempo aleatorio
 * 
 */   
#define BASETIME    500
/**
 * @brief Defino maximo valor del tiempo aleatorio
 * 
 */
#define TIMEMAX     1501
/**
 * @brief Defino el numero de leds
 * 
 */
#define MAXLED      4
/**
 * @brief Defino el numero de botones
 * 
 */
#define NUMBOTONES  4
/**
 * @brief Defino el intervalo entre latidos del hearbeat
 * 
 */

/**
 * @brief defino los valores de la maquina de estado
 * 
 */
#define ESPERAR         0
#define JUEGO           1
#define JUEGOTERMINADO  2
#define TECLAS          3

/**
 * @brief Defino el intervalo de destello al finalizar el juego
 * 
 */
#define FINISHED        500
/**
 * @brief Defino cantidad de destellos +1
 * Comienza con led en 0 
 * Enciende 3 veces
 * Apaga 3 veces
 */
#define DESTELLOS       7
/**
 * @brief Defino valor de todos los leds
 * 
 */
#define TOTALLED        15

/**
 * @brief Bandera para identificar si fue presionado el boton correcto
 * 1 = boton correcto
 * 0 = boton incorrecto
 */
#define RIGHTPRESS      flag1.bit.b0
/**
 * @brief Bandera para identificar si se gano el juego
 * 1 = verdadero
 * 0 = falso
 */
#define WON             flag1.bit.b1
/**
 * @brief Bandera para identificar si se empezo el juego
 * 1 = verdadero
 * 0 = falso
 */
#define GAMEON          flag1.bit.b2 
/**
* @brief Bandera para identificar el estado del led
 * 1 = encendido
 * 0 = apagado
 */
#define STATE           flag1.bit.b3    
   
/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,//0
    BUTTON_UP,//1
    BUTTON_FALLING,//2
    BUTTON_RISING//3
}_eButtonState;

/**
 * @brief Estructura para reconocer el estado de un boton
 * para conocer su tiempo presionado
 * y la diferencia de tiempo
 */
typedef struct{
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;


/**
 * @brief Declaro array de tipo _sTeclas del tamanio de la cantidad de botones 
 * 
 */
_sTeclas ourButton[NUMBOTONES];

/**
 * @brief Dato del tipo uint16_t para uso de mascara
 * 
 */
uint16_t mask[] = {0x0001,0x0002,0x0004,0x0008,0xF};

/**
 * @brief Dato del tipo uint8_t para controlar la maquina de estados
 * 
 */
uint8_t estadoJuego=ESPERAR;


/**
 * @brief Dato del tipo Enum para asignar los estados de la MEF
 * 
 */
_eButtonState myButton;

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 * @param buttonState Este parámetro le pasa a la MEF el estado del botón.
 */
void actuallizaMef(uint8_t indice); 

/**
 * @brief Inicializa la MEF
 * Le dá un estado inicial a myButton
 */
void startMef(uint8_t indice);

/**
 * @brief Función para cambiar el estado del LED cada vez que sea llamada.
 * 
 */
void togleLed(uint8_t indice,uint8_t state);

/**
 * @brief Identifico las entradas asignada a los botones
 * 
 */
BusIn botones(PB_6,PB_7,PB_8,PB_9);
/**
 * @brief Identifico las salidas asignada a los leds
 * 
 */
BusOut leds(PB_12,PB_13,PB_14,PB_15);

/**
 * @brief defino la salida para el Hearbeat(led)
 * 
 */
DigitalOut HearBeat(PC_13);

/**
 * @brief variable donde voy a almacenar el tiempo del timmer una vez cumplido
 * 
 */
Timer miTimer;


int tiempoMs = 0;

int main(){
    miTimer.start();

    WON=0;
    GAMEON=0;
    RIGHTPRESS=0;
    STATE=0;

    int tiempoBeat=0;
    int tiempoF=0;
    int cont=0;
    
    uint16_t ledAuxRandom=0;
    
    uint16_t ledAuxRandomTime=0;
    
    uint8_t indiceAux=0;

    uint8_t indicePress;

    int tiempoRandom=0;
    
    int ledAuxJuegoStart=0;
    
    
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
                                for(uint8_t indice=0; indice<NUMBOTONES;indice++ ){
                                    startMef(indice);
                                }
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
                            leds=TOTALLED;
                            ledAuxJuegoStart = miTimer.read_ms();
                            estadoJuego = JUEGO;
                            for(uint8_t indice=0; indice<NUMBOTONES;indice++ ){//
                                    startMef(indice);
                            }
                            
                        }
                        
                        
            break;
            case JUEGO:
                if(leds==0){
                    if((miTimer.read_ms()-ledAuxJuegoStart)>TPRESS){
                        GAMEON=1;
                    }
                    if(GAMEON){
                        ledAuxRandom = rand() % (MAXLED);
                        togleLed(ledAuxRandom,ON);
                        ledAuxRandomTime = (rand() % (TIMEMAX))+BASETIME;
                        tiempoRandom=miTimer.read_ms();
                        leds=mask[ledAuxRandom];
                    }
                }else{
                    if(leds==15){
                        if((miTimer.read_ms()-ledAuxJuegoStart)>TPRESS){
                            ledAuxJuegoStart=miTimer.read_ms();
                            leds=0;
                        }
                    }
                }
                
               
                for(indicePress = 0; indicePress<NUMBOTONES;indicePress++){
                                actuallizaMef(indicePress);
                                if(ourButton[indicePress].estado == BUTTON_DOWN){
                                    if(indicePress == ledAuxRandom){
                                        RIGHTPRESS=1;
                                    }
                                    if(indicePress != ledAuxRandom){
                                        RIGHTPRESS=0;
                                        break;
                                    }
                                }
                }
                  
                
                if((miTimer.read_ms()-tiempoRandom)>ledAuxRandomTime && GAMEON){
                    leds=0;
                    if(RIGHTPRESS==1){
                        WON = 1;
                    }else{
                        WON=0;
                    }
                    estadoJuego = JUEGOTERMINADO;
                }
                
            break;
            case JUEGOTERMINADO:
                if(WON){
                    if((miTimer.read_ms()-tiempoF)>FINISHED){
                        togleLed(MAXLED,STATE);
                        tiempoF=miTimer.read_ms();
                        STATE=!STATE;
                        cont++;
                    }
                    
                    
                }else{
                    if((miTimer.read_ms()-tiempoF)>FINISHED){
                            togleLed(ledAuxRandom,STATE);
                            tiempoF=miTimer.read_ms();
                            STATE=!STATE;
                            cont++;
                        }
                    }
                    if(cont==DESTELLOS){
                        for(uint8_t indice=0; indice<NUMBOTONES;indice++){
                            startMef(indice); 
                            ourButton[indice].timeDiff = 0;
                            ourButton[indice].timeDown=0;
                        }
                        tiempoF = miTimer.read_ms();
                        cont=0;
                        leds=0;
                        GAMEON=0;
                        estadoJuego=ESPERAR;
                    }
                
            break;
            default :
                    estadoJuego = ESPERAR;
        }
        
       if((miTimer.read_ms()-tiempoBeat) > MS ){
           
           tiempoBeat = miTimer.read_ms();
           HearBeat = !HearBeat;

       }

      
       


    
    }
    return 0;
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
            if(!(botones.read() & mask[indice]))
                ourButton[indice].estado = BUTTON_FALLING;
        
        break;
        case BUTTON_FALLING:
            if(!(botones.read() & mask[indice])){
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

void togleLed(uint8_t indice,uint8_t state){
    if(state){
        leds = mask[indice];
    }else{
        leds=0;
    }
}