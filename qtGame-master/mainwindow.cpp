#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    myTimer = new QTimer(this);
    timeGame= new QTimer(this);
    mySerial = new QSerialPort(this);
    mySettings = new SettingsDialog();
    myPaintBox1 = new QPaintBox(0,0,ui->widget);
    estado = new QLabel;
    estado->setText("Desconectado............");
    ui->statusbar->addWidget(estado);
    ui->actionDesconectar->setEnabled(false);
    estadoProtocolo=START;
   ui->pushButton_2->setEnabled(false);
    estadoComandos=ALIVE;
    for(uint8_t i=0;i<4;i++){ //4 correspode a numero de leds
        ourButton[i].estado=1;
        ourButton[i].timeDown=0;
        ourButton[i].timeUP=0;
        ourButton[i].acierto=0;
        ourButton[i].timeReaction=3000;
        myLeds[i].ledState=0;
        myLeds[i].timeInside=0;
        myLeds[i].timeOutside=0;
        myLeds[i].timeSet=0;
        myLeds[i].inicializar=1;

   }
    QDateTime now;
    srand(now.currentDateTime().time().msec());

    numLed=0;
    timeDiff=0;
    tiempoReal=0;
    PUNTAJE=0;
    ERRORES=0;
    ACIERTOS=0;
    FALLOS=0;



    ///Conexión de eventos
    connect(mySerial,&QSerialPort::readyRead,this, &MainWindow::dataRecived );
    connect(myTimer, &QTimer::timeout,this, &MainWindow::myTimerOnTime);
    connect(timeGame,&QTimer::timeout,this,&MainWindow::juego);
    connect(ui->actionEscaneo_de_Puertos, &QAction::triggered, mySettings, &SettingsDialog::show);
    connect(ui->actionConectar,&QAction::triggered,this, &MainWindow::openSerialPort);
    connect(ui->actionConectar,&QAction::triggered,this, &MainWindow::myPaint);
    connect(ui->actionDesconectar, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionSalir,&QAction::triggered,this,&MainWindow::close );


    myTimer->start(10);
    timeGame->start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = mySettings->settings();
    mySerial->setPortName(p.name);
    mySerial->setBaudRate(p.baudRate);
    mySerial->setDataBits(p.dataBits);
    mySerial->setParity(p.parity);
    mySerial->setStopBits(p.stopBits);
    mySerial->setFlowControl(p.flowControl);
    mySerial->open(QSerialPort::ReadWrite);
    if(mySerial->isOpen()){
        ui->actionConectar->setEnabled(false);
        ui->actionDesconectar->setEnabled(true);
        ui->pushButton_2->setEnabled(true);
        ui->pushButton->setEnabled(false);
        estado->setText(tr("Conectado a  %1 ")
                                         .arg(p.name));
        estadoComandos=ALIVE;
        enviar();
        estadoComandos=BUTTON_STATE;
        enviar();
        estadoComandos=GET_LEDS;
        enviar();
        myPaint();
        estadoJuego=ESPERANDOJUGADOR;
        juego();
    }
    else{
        QMessageBox::warning(this,"Menu Conectar","No se pudo abrir el puerto Serie!!!!");
    }


}



void MainWindow::closeSerialPort()
{
    if(mySerial->isOpen()){
        mySerial->close();
        ui->actionDesconectar->setEnabled(false);
        ui->actionConectar->setEnabled(true);
        estado->setText("Desconectado................");
        ui->pushButton->setEnabled(true);
        ui->pushButton_2->setEnabled(false);
    }
    else{
         estado->setText("Desconectado................");
        ui->pushButton->setEnabled(true);
        ui->pushButton_2->setEnabled(false);
    }

}

void MainWindow::myTimerOnTime()
{

    if(rxData.timeOut!=0){
        rxData.timeOut--;
    }else{
        estadoProtocolo=START;
    }
}

void MainWindow::dataRecived()
{
    unsigned char *incomingBuffer;
    int count;

    count = mySerial->bytesAvailable();

    if(count<=0)
        return;

    incomingBuffer = new unsigned char[count];

    mySerial->read((char *)incomingBuffer,count);

    rxData.timeOut=5;
    for(int i=0;i<count; i++){
        switch (estadoProtocolo) {
            case START:
                if (incomingBuffer[i]=='U'){
                    estadoProtocolo=HEADER_1;
                    rxData.cheksum=0;
                }
                break;
            case HEADER_1:
                if (incomingBuffer[i]=='N')
                   estadoProtocolo=HEADER_2;
                else{
                    i--;
                    estadoProtocolo=START;
                }
                break;
            case HEADER_2:
                if (incomingBuffer[i]=='E')
                    estadoProtocolo=HEADER_3;
                else{
                    i--;
                   estadoProtocolo=START;
                }
                break;
        case HEADER_3:
            if (incomingBuffer[i]=='R')
                estadoProtocolo=NBYTES;
            else{
                i--;
               estadoProtocolo=START;
            }
            break;
            case NBYTES:
                rxData.nBytes=incomingBuffer[i];
               estadoProtocolo=TOKEN;
                break;
            case TOKEN:
                if (incomingBuffer[i]==':'){
                   estadoProtocolo=PAYLOAD;
                    rxData.cheksum='U'^'N'^'E'^'R'^ rxData.nBytes^':';
                    rxData.payLoad[0]=rxData.nBytes;
                    rxData.index=1;
                }
                else{
                    i--;
                    estadoProtocolo=START;
                }
                break;
            case PAYLOAD:
                if (rxData.nBytes>1){
                    rxData.payLoad[rxData.index++]=incomingBuffer[i];
                    rxData.cheksum^=incomingBuffer[i];
                }
                rxData.nBytes--;
                if(rxData.nBytes==0){
                    estadoProtocolo=START;
                    if(rxData.cheksum==incomingBuffer[i]){
                        decodeData();
                    }
                }
                break;
            default:
                estadoProtocolo=START;
                break;
        }
    }
    delete [] incomingBuffer;
}

void MainWindow::decodeData()
{
    QString str="";
    for(int a=1; a < rxData.index; a++){
        switch (rxData.payLoad[1]) {
            case ALIVE:
                str = "MBED-->PC *ID Válido* (I´M ALIVE)";
                break;
            case GET_LEDS:
                myWord.ui8[0]=rxData.payLoad[2];
                myWord.ui8[1]=rxData.payLoad[3];
                leds=myWord.i16[0];
                myPaint();

                break;
            case SET_LEDS:
                myPaint();
                break;
            case BUTTON_EVENT:
                 indiceButtons=rxData.payLoad[2];
                 ourButton[indiceButtons].estado=rxData.payLoad[3];

                 myWord.ui8[0]=rxData.payLoad[4];
                 myWord.ui8[1]=rxData.payLoad[5];
                 myWord.ui8[2]=rxData.payLoad[6];
                 myWord.ui8[3]=rxData.payLoad[7];
                 if(ourButton[indiceButtons].estado){
                     ourButton[indiceButtons].timeUP=myWord.ui32;
                 }else{
                     ourButton[indiceButtons].timeDown=myWord.ui32;
                 }

                 myPaint();

                 if(ourButton[indiceButtons].timeUP>ourButton[indiceButtons].timeDown){
                    timeDiff=(ourButton[indiceButtons].timeUP)-(ourButton[indiceButtons].timeDown);
                    if(estadoJuego==ESPERANDOJUGADOR && timeDiff>=1000){
                            estadoJuego=JUGANDO;
                            ERRORES=0;
                            ACIERTOS=0;
                            FALLOS=0;
                            PUNTAJE=0;
                            for(uint8_t i=0;i<4;i++){   //4 corresponde a nro de leds = botones
                                ourButton[i].timeReaction=3000;//incializo en 3000 para que no
                                myLeds[i].inicializar=1;
                                myLeds[i].out=0;
                                myLeds[i].timeInside=0;
                                myLeds[i].timeOutside=0;
                            }
                    }
                 }



            break;
            case BUTTON_STATE:;
                myWord.ui8[0]=rxData.payLoad[2];
                myWord.ui8[1]=rxData.payLoad[3];
                buttons=myWord.i16[0];
                myPaint();
            break;
            default:
                break;
        }
    }


}


void MainWindow:: myPaint(){
    uint8_t ancho=60, alto=60, ladoSquare=80;
    int16_t posx,posy, divide, auxButton=0,auxLeds=0;
    QPen PenW, Pen;
    QPainter paint(myPaintBox1->getCanvas());
    myPaintBox1->getCanvas()->fill(QColor(46, 59, 60));
    PenW.setColor(Qt::white);
    PenW.setWidth(4);
    Pen.setColor(Qt::black);
    Pen.setWidth(4);
    divide=myPaintBox1->width()/4;
    posx=(divide/2)-(ancho/2);
    posy=(myPaintBox1->height()/4);

    paint.setRenderHint(QPainter::HighQualityAntialiasing);
    paint.setFont(QFont("Arial",13,QFont::Bold));
    paint.setPen(PenW);
    paint.drawText(posx,posy-10,"Led 1");
    paint.setBrush((Qt::gray));
    if((myLeds[auxLeds].ledState)==1){
        paint.setBrush((Qt::red));
    }
    paint.setPen(Pen);
    paint.drawEllipse(posx,posy,ancho,alto);
    paint.setBrush((Qt::gray));
    posx+=divide;
    auxLeds++;
    paint.setPen(PenW);
    paint.drawText(posx,posy-10,"Led 2");
    if((myLeds[auxLeds].ledState)==1){
        paint.setBrush((Qt::yellow));
    }
    paint.setPen(Pen);
    paint.drawEllipse(posx,posy,ancho,alto);
    paint.setBrush((Qt::gray));
    posx+=divide;
    auxLeds++;
    if((myLeds[auxLeds].ledState)==1){
        paint.setBrush((Qt::green));
    }
    paint.setPen(PenW);
    paint.drawText(posx,posy-10,"Led 3");
    paint.setPen(Pen);
    paint.drawEllipse(posx,posy,ancho,alto);
    paint.setBrush((Qt::gray));
    posx+=divide;
    auxLeds++;
    if((myLeds[auxLeds].ledState)==1){
        paint.setBrush((Qt::blue));
    }
    paint.setPen(PenW);
    paint.drawText(posx,posy-10,"Led 4");
    paint.setPen(Pen);
    paint.drawEllipse(posx,posy,ancho,alto);
    posx=(divide/2)-(ancho/2);
    posy=posy+120;
    auxButton=0;
    paint.setBrush((Qt::gray));
    if((ourButton[auxButton].estado)==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
    paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);
    posx+=divide;

    auxButton++;
    paint.setBrush((Qt::gray));
    if((ourButton[auxButton].estado)==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
     paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);
    posx+=divide;

    auxButton++;
    paint.setBrush((Qt::gray));
    if(ourButton[auxButton].estado==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
    paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);
    posx+=divide;

    auxButton++;
    paint.setBrush((Qt::gray));
    if((ourButton[auxButton].estado)==0){
        paint.setBrush((Qt::darkGray));
    }
    paint.drawRect(posx-10,posy,ladoSquare,ladoSquare);
    paint.setBrush((Qt::darkGray));
    paint.drawEllipse(posx,posy+10,ancho,alto);

    myPaintBox1->update();
}


void MainWindow::on_pushButton_clicked()
{
    openSerialPort();

}

void MainWindow::on_pushButton_2_clicked()
{
    closeSerialPort();
}


void MainWindow::enviar()
{
    txData.index=0;
    txData.payLoad[txData.index++]='U';
    txData.payLoad[txData.index++]='N';
    txData.payLoad[txData.index++]='E';
    txData.payLoad[txData.index++]='R';
    txData.payLoad[txData.index++]=0;
    txData.payLoad[txData.index++]=':';

    switch (estadoComandos) {
    case ALIVE:
        txData.payLoad[txData.index++]=ALIVE;
        txData.payLoad[NBYTES]=0x02;
        break;
    case GET_LEDS:
        txData.payLoad[txData.index++]=GET_LEDS;
        txData.payLoad[NBYTES]=0x02;
        break;
    case SET_LEDS:
        txData.payLoad[txData.index++]=SET_LEDS;
        txData.payLoad[txData.index++] = numLed;
        txData.payLoad[txData.index++] = myLeds[numLed].ledState;
        txData.payLoad[NBYTES]=0x04;
        break;

    case BUTTON_STATE :
        txData.payLoad[txData.index++]=BUTTON_STATE;
        txData.payLoad[NBYTES]=0x02;
    case OTHERS:
        break;
    default:
        break;
    }
   txData.cheksum=0;
   for(int a=0 ;a<txData.index;a++)
       txData.cheksum^=txData.payLoad[a];
    txData.payLoad[txData.index]=txData.cheksum;
    if(mySerial->isWritable()){
        mySerial->write((char *)txData.payLoad,txData.payLoad[NBYTES]+6);
    }


}


void MainWindow::juego(){

    QString str;
    tiempoReal=timeJuego*TICKERINT;
    switch(estadoJuego){
        case ESPERANDOJUGADOR:
            timeJuego=0;
            str=("ESPERANDO JUGADOR");
            for(uint i=0;i<4;i++){ //4 corresponde a nro de leds
                numLed=i;
                myLeds[numLed].ledState=0;
                estadoComandos=SET_LEDS;
                enviar();
            }

        break;
        case JUGANDO:
            timeJuego++;
            if(timeJuego>=1000){ // si fue llamado 1000 veces se cumple 30 segundos
                estadoJuego=ESPERANDOJUGADOR;
            }
            str=("          JUGANDO");
            for(uint8_t i=0;i<4;i++){ //4 corresponde a nro de leds
              if(myLeds[i].inicializar){
                  myLeds[i].inicializar=0; //flag para inicializar numeros
                  myLeds[i].timeInside = rand() % (TMAXWAIT-TMINWAIT)+TMINWAIT;
                  myLeds[i].timeOutside = rand() % (TMAXOUT-TMINOUT)+ TMINOUT;
                  myLeds[i].timeSet=tiempoReal;
                  myLeds[i].out=0;//flag que controla si el topo salio de la madriguera
              }
            }



            numLed++;
            if(numLed>=4){//4 corresponde a nro de leds
                numLed=0;
            }

            if(myLeds[numLed].out==0){
                if((tiempoReal-myLeds[numLed].timeSet)<myLeds[numLed].timeInside){
                    if((ourButton[numLed].estado==0)&&myLeds[numLed].ledState==0){
                        PUNTAJE=PUNTAJE-20;
                        ERRORES=ERRORES+1;
                    }

                }
                if((tiempoReal-myLeds[numLed].timeSet)>=myLeds[numLed].timeInside){
                     myLeds[numLed].timeSet=tiempoReal;
                     myLeds[numLed].ledState=1;
                     estadoComandos=SET_LEDS;
                     myLeds[numLed].out=1;
                     enviar();



                }
            }
             if(myLeds[numLed].out){
                 if(ourButton[numLed].estado==0){
                     ourButton[numLed].timeReaction=(tiempoReal-myLeds[numLed].timeSet);
                     if(myLeds[numLed].timeOutside>=ourButton[numLed].timeReaction){
                         ourButton[numLed].acierto=1;
                     }else{
                         ourButton[numLed].acierto=0;
                     }
                 }
                 if(((tiempoReal-myLeds[numLed].timeSet)>=myLeds[numLed].timeOutside)||ourButton[numLed].acierto){
                     myLeds[numLed].ledState=0;
                     estadoComandos=SET_LEDS;
                     enviar();
                     myLeds[numLed].inicializar=1;
                     myLeds[numLed].timeSet=tiempoReal;
                     myLeds[numLed].out=0;
                     checkPoints();


                 }
             }



        break;

    }
    ui->label_6->setText(str);
    ui->lcdNumber->display(PUNTAJE);
    ui->lcdNumber_3->display(ERRORES);
    ui->lcdNumber_2->display(ACIERTOS);
    ui->lcdNumber_4->display(FALLOS);


}

void MainWindow::checkPoints(){
    if(ourButton[numLed].acierto){
        ACIERTOS++;
        PUNTAJE=PUNTAJE+((TMAXOUT*TMAXOUT)/(myLeds[numLed].timeOutside*ourButton[numLed].timeReaction));
        ourButton[numLed].acierto=0;

    }else{
        FALLOS++;
        PUNTAJE=PUNTAJE-10;
    }
    ourButton[numLed].timeReaction=3000;
}




