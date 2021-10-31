#include "mbed.h"
#include <stdbool.h>

#define NUMBUTT             4

#define MAXLED              4

#define NUMBEAT             4

#define HEARBEATINTERVAL    100

#define TRUE        1
#define FALSE       0

#define INTERVAL    40

#define MS          200







/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,    //0
    BUTTON_UP,      //1
    BUTTON_FALLING, //2
    BUTTON_RISING   //3
}_eButtonState;

/**
 * @brief Enumeración de eventos del botón
 * Se usa para saber si el botón está presionado o nó
 */
typedef enum{
    EV_PRESSED,
    EV_NOT_PRESSED,
    EV_NONE
}_eButtonEvent;

/**
 * @brief Esturctura de Teclas
 * 
 */
typedef struct{
    _eButtonState estado;
    _eButtonEvent event;
    int32_t timeDown;
    int32_t timeUP;
}_sTeclas;

_sTeclas ourButton[NUMBUTT];
//                0001 ,  0010,  0100,  1000

    typedef struct{
        uint8_t ledState;
    }_sLeds;
    
    _sLeds myLeds[MAXLED];


uint16_t mask[]={0x0001,0x0002,0x0004,0x0008};

/**
 * @brief Enumeración de la MEF para decodificar el protocolo
 * 
 */
typedef enum{
    START,
    HEADER_1,
    HEADER_2,
    HEADER_3,
    NBYTES,
    TOKEN,
    PAYLOAD
}_eProtocolo;

_eProtocolo estadoProtocolo;

/**
 * @brief Enumeración de la lista de comandos
 * 
 */
 typedef enum{
        ACK=0x0D,
        ALIVE=0xF0,
        BUTTON_EVENT=0XFA,
        GET_LEDS=0xFB,
        SET_LEDS=0xFC,
        BUTTON_STATE=0xFD,
        OTHERS
    }_eID;

/**
 * @brief Estructura de datos para el puerto serie
 * 
 */
typedef struct{
    uint8_t timeOut;         //!< TiemOut para reiniciar la máquina si se interrumpe la comunicación
    uint8_t cheksumRx;       //!< Cheksumm RX
    uint8_t cheksumtx;       //!< Cheksumm Tx
    uint8_t indexWriteRx;    //!< Indice de escritura del buffer circular de recepción
    uint8_t indexReadRx;     //!< Indice de lectura del buffer circular de recepción
    uint8_t indexWriteTx;    //!< Indice de escritura del buffer circular de transmisión
    uint8_t indexReadTx;     //!< Indice de lectura del buffer circular de transmisión
    uint8_t bufferRx[256];   //!< Buffer circular de recepción
    uint8_t bufferTx[256];   //!< Buffer circular de transmisión
    uint8_t payload[32];     //!< Buffer para el Payload de datos recibidos
}_sDato ;

volatile _sDato datosComProtocol;



/**
 * @brief Mapa de bits para implementar banderas
 * Como uso una sola los 7 bits restantes los dejo reservados para futuras banderas
 */
typedef union{
    struct{
        uint8_t checkButtons :1;
        uint8_t Reserved :7;
    }individualFlags;
    uint8_t allFlags;
}_bGeneralFlags;

volatile _bGeneralFlags myFlags;

uint8_t hearBeatEvent;

/**
 * @brief Unión para descomponer/componer datos mayores a 1 byte
 * 
 */
typedef union {
    int32_t i32;
    uint32_t ui32;
    uint16_t ui16[2];
    uint8_t ui8[4];
}_udat;

_udat myWord;
_udat timerRead;




/*************************************************************************************************/
/* Prototipo de Funciones */

/**
 * @brief Inicializa la MEF
 * Le dá un estado inicial a myButton
 */
void startMef();

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 * @param buttonState Este parámetro le pasa a la MEF el estado del botón.
 */
void actuallizaMef();

/**
 * @brief Enciende o apaga los leds
 * Se envian las posiciones de los leds que se quieren encender 
 */
void manejadorLed();

/**
 * @brief Función que se llama en la interrupción de recepción de datos
 * Cuando se llama la fucnión se leen todos los datos que llagaron.
 */
void onDataRx(void);

/**
 * @brief Decodifica las tramas que se reciben 
 * La función decodifica el protocolo para saber si lo que llegó es válido.
 * Utiliza una máquina de estado para decodificar el paquete
 */
void decodeProtocol(void);

/**
 * @brief Procesa el comando (ID) que se recibió
 * Si el protocolo es correcto, se llama a esta función para procesar el comando
 */
void decodeData(void);

/**
 * @brief Envía los datos a la PC
 * La función consulta si el puerto serie está libra para escribir, si es así envía 1 byte y retorna
 */
void sendData(void);

/**
 * @brief Función que se llama con la interrupción del TICKER
 * maneja el tiempo de debounce de los botones, cambia un falg cada vez que se deben revisar los botones
 */
void OnTimeOut(void);

/**
 * @brief Función Hearbeat
 * Ejecuta las tareas del hearbeat 
 */
void hearbeatTask(void);

/*****************************************************************************************************/
/* Configuración del Microcontrolador */

BusIn buttonArray(PB_6,PB_7,PB_8,PB_9);

BusOut leds(PB_12,PB_13,PB_14,PB_15);

DigitalOut HEARBEAT(PC_13); //!< Defino la salida del led

Serial pcCom(PA_9,PA_10,115200); //!< Configuración del puerto serie, la velocidad (115200) tiene que ser la misma en QT

Timer miTimer; //!< Timer general

Ticker timerGeneral;

uint8_t numLed=0;
uint8_t indice=0;
uint8_t flanco= ourButton[indice].estado;
/*****************************************************************************************************/
/************  Función Principal ***********************/


int main()
{
    int hearbeatTime=0;

    miTimer.start();

    myFlags.individualFlags.checkButtons=false;

    hearBeatEvent=0;
   
    pcCom.attach(&onDataRx,Serial::RxIrq);

    timerGeneral.attach_us(&OnTimeOut, 50000);

    for(indice=0; indice<NUMBUTT;indice++){
        startMef();
    }

    while(TRUE)
    {

        if(myFlags.individualFlags.checkButtons){
            myFlags.individualFlags.checkButtons=false;
            for(indice=0; indice < NUMBUTT; indice++){
                if (buttonArray & mask[indice]){
                    ourButton[indice].event =EV_NOT_PRESSED;
                }else{
                    ourButton[indice].event =EV_PRESSED; 
                }
                actuallizaMef();
            }
        }
        if ((miTimer.read_ms()-hearbeatTime)>=HEARBEATINTERVAL){
            hearbeatTime=miTimer.read_ms();
            hearbeatTask();
            
        }
        if(datosComProtocol.indexReadRx!=datosComProtocol.indexWriteRx) 
            decodeProtocol();

        if(datosComProtocol.indexReadTx!=datosComProtocol.indexWriteTx) 
            sendData();

    }




    return 0;
}


/*****************************************************************************************************/
/************  MEF para DEBOUNCE de botones ***********************/

void startMef(){
   ourButton[indice].estado=BUTTON_UP;
}


void actuallizaMef(){

    switch (ourButton[indice].estado)
    {
    case BUTTON_DOWN:
        if(ourButton[indice].event )
           ourButton[indice].estado=BUTTON_RISING;

    break;
    case BUTTON_UP:
        if(!(ourButton[indice].event))
            ourButton[indice].estado=BUTTON_FALLING;
    break;
    case BUTTON_FALLING:
        if(!(ourButton[indice].event))
        {
            ourButton[indice].timeDown=miTimer.read_ms();
            ourButton[indice].estado=BUTTON_DOWN;
            datosComProtocol.payload[1] = BUTTON_EVENT;
            flanco = BUTTON_DOWN;
            
            decodeData();
            
            //Flanco de bajada
        }
        else
            ourButton[indice].estado=BUTTON_UP;  

    break;
    case BUTTON_RISING:
        if(ourButton[indice].event){
            ourButton[indice].estado=BUTTON_UP;
            //Flanco de Subida
            flanco = BUTTON_UP;
            ourButton[indice].timeUP=miTimer.read_ms();
            datosComProtocol.payload[1] = BUTTON_EVENT;
            decodeData();

        }

        else
            ourButton[indice].estado=BUTTON_DOWN;
    
    break;
    
    default:
    startMef();
        break;
    }
}


/*****************************************************************************************************/
/************  Funciónes para manejar los LEDS ***********************/



void manejadorLed (){
    uint16_t ledsAux=leds, auxmask=0;
    auxmask |= 1<<numLed;
    if((auxmask & leds) && myLeds[numLed].ledState==0){
        ledsAux &= ~(1 << (numLed)) ; 
    }else{
        if((auxmask & leds)==0 && myLeds[numLed].ledState==1){
         ledsAux |= 1 << (numLed) ;
        }
    }
    leds = ledsAux ;
}




/*****************************************************************************************************/
/************  MEF para decodificar el protocolo serie ***********************/
void decodeProtocol(void)
{
    static int8_t nBytes=0, indice=0;
    while (datosComProtocol.indexReadRx!=datosComProtocol.indexWriteRx)
    {
        switch (estadoProtocolo) {
            case START:
                if (datosComProtocol.bufferRx[datosComProtocol.indexReadRx++]=='U'){
                    estadoProtocolo=HEADER_1;
                    datosComProtocol.cheksumRx=0;
                }
                break;
            case HEADER_1:
                if (datosComProtocol.bufferRx[datosComProtocol.indexReadRx++]=='N')
                   estadoProtocolo=HEADER_2;
                else{
                    datosComProtocol.indexReadRx--;
                    estadoProtocolo=START;
                }
                break;
            case HEADER_2:
                if (datosComProtocol.bufferRx[datosComProtocol.indexReadRx++]=='E')
                    estadoProtocolo=HEADER_3;
                else{
                    datosComProtocol.indexReadRx--;
                   estadoProtocolo=START;
                }
                break;
        case HEADER_3:
            if (datosComProtocol.bufferRx[datosComProtocol.indexReadRx++]=='R')
                estadoProtocolo=NBYTES;
            else{
                datosComProtocol.indexReadRx--;
               estadoProtocolo=START;
            }
            break;
            case NBYTES:
                nBytes=datosComProtocol.bufferRx[datosComProtocol.indexReadRx++];
               estadoProtocolo=TOKEN;
                break;
            case TOKEN:
                if (datosComProtocol.bufferRx[datosComProtocol.indexReadRx++]==':'){
                   estadoProtocolo=PAYLOAD;
                    datosComProtocol.cheksumRx ='U'^'N'^'E'^'R'^ nBytes^':';
                    datosComProtocol.payload[0]=nBytes;
                    indice=1;
                }
                else{
                    datosComProtocol.indexReadRx--;
                    estadoProtocolo=START;
                }
                break;
            case PAYLOAD:
                if (nBytes>1){
                    datosComProtocol.payload[indice++]=datosComProtocol.bufferRx[datosComProtocol.indexReadRx];
                    datosComProtocol.cheksumRx ^= datosComProtocol.bufferRx[datosComProtocol.indexReadRx++];
                }
                nBytes--;
                if(nBytes<=0){
                    estadoProtocolo=START;
                    if(datosComProtocol.cheksumRx == datosComProtocol.bufferRx[datosComProtocol.indexReadRx]){
                        decodeData();
                    }
                }
                break;
            default:
                estadoProtocolo=START;
                break;
        }
    }
    

}


/*****************************************************************************************************/
/************  Función para procesar el comando recibido ***********************/
void decodeData(void)
{
    uint8_t auxBuffTx[50], indiceAux=0, cheksum;
    auxBuffTx[indiceAux++]='U';
    auxBuffTx[indiceAux++]='N';
    auxBuffTx[indiceAux++]='E';
    auxBuffTx[indiceAux++]='R';
    auxBuffTx[indiceAux++]=0;
    auxBuffTx[indiceAux++]=':';

    switch (datosComProtocol.payload[1]) {
        case ALIVE:
            auxBuffTx[indiceAux++]=ALIVE;
            auxBuffTx[indiceAux++]=ACK;
            auxBuffTx[NBYTES]=0x03;
            break;
        case GET_LEDS:
            auxBuffTx[indiceAux++]=GET_LEDS;
            myWord.ui16[0]=leds;
            auxBuffTx[indiceAux++]=myWord.ui8[0];
            auxBuffTx[indiceAux++]=myWord.ui8[1];
            auxBuffTx[NBYTES]=0x04;
            break;
        case SET_LEDS:
            auxBuffTx[indiceAux++]=SET_LEDS;
            numLed=datosComProtocol.payload[2];
            myLeds[numLed].ledState=datosComProtocol.payload[3];
            auxBuffTx[NBYTES]=0x02;
            manejadorLed();
            break;
        case BUTTON_EVENT:
            auxBuffTx[indiceAux++]=BUTTON_EVENT;
            auxBuffTx[indiceAux++]=indice;
            auxBuffTx[indiceAux++]=flanco;
            if(ourButton[indice].estado == BUTTON_DOWN)
                timerRead.ui32=ourButton[indice].timeDown;
            if(ourButton[indice].estado == BUTTON_UP)
                timerRead.ui32=ourButton[indice].timeUP;

            auxBuffTx[indiceAux++]=timerRead.ui8[0];
            auxBuffTx[indiceAux++]=timerRead.ui8[1];
            auxBuffTx[indiceAux++]=timerRead.ui8[2];
            auxBuffTx[indiceAux++]=timerRead.ui8[3];
            auxBuffTx[NBYTES]= 0x08;
            break;
        case BUTTON_STATE:
            auxBuffTx[indiceAux++]=BUTTON_STATE;
            myWord.ui16[0]=buttonArray;
            auxBuffTx[indiceAux++]=myWord.ui8[0];
            auxBuffTx[indiceAux++]=myWord.ui8[1];
            auxBuffTx[NBYTES]=0x04;
            break;

        default:
            auxBuffTx[indiceAux++]=0xDD;
            auxBuffTx[NBYTES]=0x02;
            break;
    }
   cheksum=0;
   for(uint8_t a=0 ;a < indiceAux ;a++)
   {
       cheksum ^= auxBuffTx[a];
       datosComProtocol.bufferTx[datosComProtocol.indexWriteTx++]=auxBuffTx[a];
   }
    datosComProtocol.bufferTx[datosComProtocol.indexWriteTx++]=cheksum;

}


/*****************************************************************************************************/
/************  Función para enviar los bytes hacia la pc ***********************/
void sendData(void)
{
    if(pcCom.writable())
        pcCom.putc(datosComProtocol.bufferTx[datosComProtocol.indexReadTx++]);

}


/*****************************************************************************************************/
/************  Función para hacer el hearbeats ***********************/
void hearbeatTask(void)
{
    if(hearBeatEvent < NUMBEAT){
        HEARBEAT=!HEARBEAT;
        hearBeatEvent++;
    }else{
        HEARBEAT=1;
        hearBeatEvent = (hearBeatEvent>=25) ? (0) : (hearBeatEvent+1);    
    }
}


/**********************************************************************************/
/* Servicio de Interrupciones*/

void onDataRx(void)
{
    while (pcCom.readable())
    {
        datosComProtocol.bufferRx[datosComProtocol.indexWriteRx++]=pcCom.getc();
    }
}

void OnTimeOut(void)
{
    if(!myFlags.individualFlags.checkButtons)
        myFlags.individualFlags.checkButtons=true;
} 
