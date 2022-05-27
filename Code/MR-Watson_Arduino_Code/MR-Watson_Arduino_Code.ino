//By: ACO-626

//BIBLIOTECAS
#include <NXTIoT_dev.h>

//RELEVADOR
#define pR 2      //Pin digital a relé
#define eR 3      //Pin digital para introducir estado relé con switch corredera
#define V5 5      //Auxiliar para definir estado lógico HIGH en switch rele
bool eRactual=0;    //Booleano para guardar el estado actual del relevador
bool eRanterior=0;  //Booleano guarda en anterior lectura del relevador 
void estadoRele(bool state);   //Subfunción para definir el estado de relevador
void checkER();                //Revisa si hay cambios en estado de relavador 

//TENSIÓN
#define pV A0    //Pin analógico a sensor tensión ZMPT101B
int aV = 512;    //Valor analógico de sensorVoltaje
int aSV = 512;   //Guarda pico superior de voltaje
int aIV = 512;   //Guarda pico inferior de voltaje
int aZV = 519;   //Guarda el Zero de voltaje
float aRealV;
float sensV = 182.4335/141;
    //promedioTensión
int iV=0;
int grupoV=250;
long sumatoriaV=0;


//CORRIENTE
#define pC A1   //Pian analógico a sensor corriente ACS712
int aC = 512;    //Valor analógico de sensor corriente
int aSC = 512;   //Valor superior análogo
int aIC = 512;   //Valor inferior análogo
int aZC = 520;   //Guarda el Zero de corriente
float aRealC;
float sensC = (1.0961/17.0);
   //promedioCorriente
int iC=0;
int grupoC=63;
long sumatoriaC=0;


//POTENCIA
float aActiveP;
float aParenteP;
   //Promediar potencia activa
int grupoPactiva = 200;
int iPactiva = 0;
long sumatoriaPactiva;
int activeP;
   //Potencia aparente
int parentP;
   //Potencia reactiva
int reactiveP;
   //Promediar rmsC
int grupoC2=grupoPactiva;
int iC2=0;
double sumatoriaC2;
float rmsC;
   //Promediar rmsV
int grupoV2=grupoPactiva;
int iV2=0;
double sumatoriaV2;
float rmsV;
int diferenciadorVC = 30;    //Valor que diferencia si algo es corriente o voltaje, no se espera Irms > 30;
  //Factor de potencia
float fp;

//PROTOTIPOS
void promediarInt(int &contador, int grupo, int sumando, long &sumatoria, int &promedio);

//MONITOREO SERIAL PC
void imprimirOscilos();
short cero = 0;

//SIGFOX PAYLOAD
NXTIoT_dev watsonSigfoxBrain;
unsigned int time2Send = 15; //Tiempo en minutos
unsigned int cicloCounter = 0;  

void setup() {
  //INICIALIZAR ESTADOS
  Serial.begin(9600);     //Inicialización comunicación serial
  time2Send = time2Send * 3;
  
  //DEFINICIONES
  pinMode(pR, OUTPUT);    //Salir para pin Relevador
  pinMode(V5,OUTPUT);     //Iniciamos salida en V5
  digitalWrite(V5,HIGH);     //Definimos estado en V5
  }

void loop() {
  checkER();
  aV = analogRead(pV);
  aC = analogRead(pC); 
  promediarInt(iV,grupoV,aV,sumatoriaV,aZV); //(contador,grupo,sumando,sumatoria,promedio)
  promediarInt(iC,grupoC,aC,sumatoriaC,aZC); //(contador,grupo,sumando,sumatoria,promedio)
  aRealV = (aV-aZV) * (sensV);
  aRealC = (aC-aZC) * (sensC);
  //Potencia activa
  aActiveP = aRealV * aRealC;
  promediarInt(iPactiva,grupoPactiva,aActiveP,sumatoriaPactiva,activeP); //(contador,grupo,sumando,sumatoria,promedio)
  //Potencia aparente
  promediarRMS(iC2,grupoC2,pow(aRealC,2),sumatoriaC2,rmsC);
  promediarRMS(iV2,grupoV2,pow(aRealV,2),sumatoriaV2,rmsV);
  
  delay(100);
  }

//FUNCIÓN PROMEDIADORA ANÁLISA EL CERO EN LECTURA
void promediarInt(int &contador, int grupo, int sumando, long &sumatoria, int &promedio){
  if(contador >= grupo){
    promedio = (sumatoria/contador);
    sumatoria =0;
    contador = 0;
  }else{
    sumatoria = sumatoria + sumando;
    contador++;    
    }
  }
void promediarRMS(int &contador, int grupo, float sumando, double &sumatoria, float &promedio){
  if(contador >= grupo){
    promedio = sqrt(sumatoria/contador);
    
    if(sumatoria == sumatoriaV2 ){ //Si ya se actualiza Vrms calcula potencia
      parentP = rmsV * rmsC;
      reactiveP = parentP - activeP;
      fp = (float)activeP/(float)parentP;
      imprimirVCP();
      cicloCounter++;
      
      //Envio de mensaje a back en tiempo determinado
      if(cicloCounter >= time2Send){
        watsonSigfoxBrain.initpayload();
        watsonSigfoxBrain.addfloat(rmsV);
        watsonSigfoxBrain.addfloat(rmsC);
        watsonSigfoxBrain.addint(activeP);
        watsonSigfoxBrain.sendmessage();
        cicloCounter=0;
      }
    }
    sumatoria =0;
    contador = 0;
  }else{
    sumatoria = sumatoria + sumando;
    contador++;    
    }
  }
//FUNCIÓN DE REVISA SI HUBO CAMBIOS PARA ACTUALIZAR RELÉ
void checkER(){
  eRactual = digitalRead(eR);
  if(eRanterior != eRactual)
  {
    estadoRele(eRactual);
    eRanterior = eRactual;   
   }
  }

//SUBFUNCIÓN DE ESTADO RELEVADOR
void estadoRele(bool state){
  digitalWrite(pR,state);
  }
//ANEXO
void imprimirOscilos(bool aVC, bool centro, bool bandas, bool pot, bool realCV){ //OSCILOSCOPIO
  if(aVC){
    //La impresión de comillas es para el formato de serialplot
    Serial.print("aVolt:");
    Serial.print(aV);   //Impresión de voltaje analógico
    Serial.print(",");
    Serial.print("aCorri:");  
    Serial.print(aC);  //Impresión de corriente analógico
    Serial.print(",");
    }
  if(centro){
    Serial.print("CeroV:");
    Serial.print(aZV);
    Serial.print(",");
    Serial.print("CeroC:");
    Serial.print(aZC);
    Serial.print(",");
    }
  if(bandas){
    int aZIV = aZV-6;  //Guarda el limite inferior de banda cero
    int aZSV = aZV+6;  //Guarda el limite superior de banda cero  
    int aZIC = aZC-3;  //Guarda el limite inferior de banda cero
    int aZSC = aZC+3;  //Guarda el limite superior de banda cero
     
    //La impresión de comillas es para el formato de serialplot
    Serial.print("BandaV0:");
    Serial.print(aZIV);//Impresión de banda inf Voltaje
    Serial.print(","); 
    Serial.print("BandaV1:");
    Serial.print(aZSV);//Impresión de banda superior Voltaje
    Serial.print(",");
    Serial.print("BandaC0:");
    Serial.print(aZIC); //Impresión de banda inferior Corriente
    Serial.print(",");
    Serial.print("BandaC1:");
    Serial.print(aZSC); //Impresión de banda superior Corriente
    Serial.print(",");
    }
  if(pot){
    Serial.print("PotenciaActiva:");
    Serial.print(aActiveP);        //Impresión de la lectura de potencia instantanea
    Serial.print(",");
    }
  if(realCV){
    Serial.print("Voltaje:");
    Serial.print(aRealV);
    Serial.print(",");
    Serial.print("Corriente:");
    Serial.print(aRealC);
    Serial.print(",");
    }  
  Serial.print("cero:");
  Serial.println(cero);
  }
void imprimirVCP(){  //Imprimir valores en tabla
   Serial.print("Voltaje:");
   Serial.print(rmsV);
   Serial.print(",");
   Serial.print("Corriente:");
   Serial.print(rmsC);
   Serial.print(",");
   Serial.print("P-activa[W]:");
   Serial.print(activeP);
   Serial.print(",");
   Serial.print("P-reactiva[VAR]:");
   Serial.print(reactiveP);
   Serial.print(",");
   Serial.print("P-aparente[W]:");
   Serial.print(parentP);
   Serial.print(",");
   Serial.print("FP:");
   Serial.println(fp);  
  }