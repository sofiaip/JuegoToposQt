#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QLabel>
#include "settingsdialog.h"
#include "qpaintbox.h"
#include <QDateTime>



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void openSerialPort();

    void closeSerialPort();

    void myTimerOnTime();

    void dataRecived();

    void decodeData();

    void myPaint();

    void on_pushButton_clicked();//conectar

    void on_pushButton_2_clicked();//desconectar

    void enviar();

    void juego();

    void checkPoints();


private:
    Ui::MainWindow  *ui;
    QSerialPort *mySerial;
    QTimer *myTimer,*timeGame;
    QPaintBox *myPaintBox1;
    SettingsDialog *mySettings;
    QLabel *estado;

    #define SECOND 1000;
    #define TICKERINT 30;
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

    typedef enum{
        ALIVE=0xF0,
        BUTTON_EVENT=0XFA,
        GET_LEDS=0xFB,
        SET_LEDS=0xFC,
        BUTTON_STATE=0xFD,
        OTHERS
    }_eID;

    _eID estadoComandos;

    typedef enum{
        DOWN,
        BUTTON_UP,      //1
        BUTTON_FALLING, //2
        BUTTON_RISING,
    }_eButtonState;
    _eButtonState estadoButtons;

    typedef enum{
            ESPERANDOJUGADOR,
            JUGANDO,
    }_eSTATE;
    _eSTATE estadoJuego;
    typedef enum{
        TMINWAIT=1000,
        TMAXWAIT=6000,
        TMINOUT=500,
        TMAXOUT=2000,
    }_eTimes;
    _eTimes tiempoTopo;

    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos rxData, txData;


    typedef struct{
        uint8_t estado;
        uint8_t acierto;
        int32_t timeDown;
        int32_t timeUP;
        uint32_t timeReaction;
    }_sTeclas;

    _sTeclas ourButton[4];

    typedef struct{
        uint8_t ledState;
        uint8_t inicializar;
        uint8_t out;
        uint32_t timeInside;
        uint32_t timeOutside;
        uint32_t timeSet;
    }_sLeds;

    _sLeds myLeds[4];

    typedef union {
        float f32;
        int i32;
        unsigned int ui32;
        unsigned short ui16[2];
        short i16[2];
        uint8_t ui8[4];
        char chr[4];
        unsigned char uchr[4];
    }_udat;

    _udat myWord;

    uint8_t numLed;//indice de los leds
    uint8_t indiceButtons,ERRORES,ACIERTOS,FALLOS;
    uint16_t leds, buttons;
    uint32_t timeJuego,tiempoReal,timeDiff;
    int32_t PUNTAJE;


};
#endif // MAINWINDOW_H
