#include <Arduino.h>
#include "Sodaq_RN2XX3.h"

#define debugSerial SerialUSB
#define loraSerial Serial2

#define NIBBLE_TO_HEX_CHAR(i) ((i <= 9) ? ('0' + i) : ('A' - 10 + i))
#define HIGH_NIBBLE(i) ((i >> 4) & 0x0F)
#define LOW_NIBBLE(i) (i & 0x0F)

//Default é 1000
#define RXDELAY "1000"
//Port utillizada para esse tipo de dado
#define LORAWAN_PORT 2  
//Delay entre loops
#define LOOP_DELAY 10000

uint8_t myBuffer[DEFAULT_RECEIVED_PAYLOAD_BUFFER_SIZE];

void setupLoRa();
void setupLoRaOTAA();
String getTemperature();
void setupLoRaABP();

//Use OTAA, set to false to use ABP
bool OTAA = true;

// ABP
// USE YOUR OWN KEYS!
const uint8_t devAddr[4] =
{
    0x00, 0x00, 0x00, 0x00
};

// USE YOUR OWN KEYS!
const uint8_t appSKey[16] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// USE YOUR OWN KEYS!
const uint8_t nwkSKey[16] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// OTAA
// With using the GetHWEUI() function the HWEUI will be used
// DevEUI:00 C7 2D 74 36 8B 54 5E
//00 AE 8A 73 FB 45 6A 0C
//00AE8A73FB456A0C
static uint8_t DevEUI[8]
{ 0x00, 0xAE, 0x8A, 0x73, 0xFB, 0x45, 0x6A, 0x0C };

//BOX: { 0x00, 0xAE, 0x8A, 0x73, 0xFB, 0x45, 0x6A, 0x0C };

//AppEUI: 70 B3 D5 7E D0 01 52 CF
//70 B3 D5 7E D0 01 52 CF
//70B3D57ED00152CF
const uint8_t AppEUI[8] =
{ 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x01, 0x52, 0xCF };
//BOX: { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x01, 0x52, 0xCF };


//JoinEUI: E5 AB 20 E7 21 5A 4A B6 17 F5 EA 3B E2 51 2A EE
//EA 4E E9 76 86 2F C1 B7 E1 DF 3E 04 E4 85 AF 3E
const uint8_t AppKey[16] =
{ 0xEA, 0x4E, 0xE9, 0x76, 0x86, 0x2F, 0xC1, 0xB7, 0xE1, 0xDF, 0x3E, 0x04, 0xE4, 0x85, 0xAF, 0x3E };
//BOX { 0xEA, 0x4E, 0xE9, 0x76, 0x86, 0x2F, 0xC1, 0xB7, 0xE1, 0xDF, 0x3E, 0x04, 0xE4, 0x85, 0xAF, 0x3E };
void RED() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

void GREEN() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);
}

void BLUE() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, LOW);
}

//typedef void(*ReceiveCallback)(const uint8_t* buffer,uint8_t port, uint16_t size);
void receiveCallback(const uint8_t* buffer,uint8_t port, uint16_t size)
{
      debugSerial.print("Downlink message (callback): ");  
      debugSerial.print(port);  
      debugSerial.print(" ");  
      for (uint16_t i=0; i<size; i++)
      {
        debugSerial.print((char) buffer[i]);      
      }
      debugSerial.println("");
}


void setup()
{
  //Desliga bluetooth
  pinMode(BLUETOOTH_WAKE,OUTPUT);
  digitalWrite(BLUETOOTH_WAKE, HIGH);
/*
  I disable the USB of the uP with USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;
I set the uP to deep sleep mode with SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
And I set the processor to sleep with __WFI();
*/
   // initialize digital pin LED_BUILTIN as an output.
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  //Liga led azul - Sinaliza que esta funcionado
  digitalWrite(LED_BUILTIN, HIGH);

  delay(1000);
  
  while ((!debugSerial) && (millis() < 10000)){
    // Wait 10 seconds for debugSerial to open
  }

  debugSerial.println("Start");

  // Start streams
  debugSerial.begin(57600);
  loraSerial.begin(LoRaWAN.getDefaultBaudRate());

  LoRaWAN.setDiag(debugSerial); // to use debug remove //DEBUG inside library
  LoRaWAN.init(loraSerial, LORA_RESET);

  //Use the Hardware EUI
  //getHWEUI();

  // Print the Hardware EUI
  debugSerial.print("LoRa HWEUI: ");
  for (uint8_t i = 0; i < sizeof(DevEUI); i++) {
    debugSerial.print((char)NIBBLE_TO_HEX_CHAR(HIGH_NIBBLE(DevEUI[i])));
    debugSerial.print((char)NIBBLE_TO_HEX_CHAR(LOW_NIBBLE(DevEUI[i])));
  }
  debugSerial.println();  
  
  setupLoRa();

  //Configura rxdelay
  //LoRaWAN.setMacParam("rxdelay1 ", RXDELAY);

  //Passa carga da bateria
  // 0: Fonte externar
  // 1 - 254: Carga bateria
  // 255: Não foi possivel medir a bateria
  //LoRaWAN.setMacParam("bat ", "255");

  //Configura callback que recebe menssagens
  LoRaWAN.setReceiveCallback(&receiveCallback);

}


void setupLoRa(){
  if(!OTAA){
    // ABP
    setupLoRaABP();
  } else {
    //OTAA
    setupLoRaOTAA();
  }
  // Uncomment this line to for the RN2903 with the Actility Network
  // For OTAA update the DEFAULT_FSB in the library
  // 2 para canais de 8 a 15
  //LoRaWAN.setFsbChannels(2);

  //LoRaWAN.setSpreadingFactor(9);
}

void setupLoRaABP(){  
  if (LoRaWAN.initABP(loraSerial, devAddr, appSKey, nwkSKey, true))
  {
    debugSerial.println("Communication to LoRaWAN successful.");
  }
  else
  {
    debugSerial.println("Communication to LoRaWAN failed!");
  }
}

void setupLoRaOTAA(){
  
  if (LoRaWAN.initOTA(loraSerial, DevEUI, AppEUI, AppKey, false))
  {
    debugSerial.println("Network connection successful.");
  }
  else
  {
    debugSerial.println("Network connection failed!");
  }
}

void loop()
{
    String reading = getTemperature();
    debugSerial.print("Uplink message: ");
    debugSerial.println(reading);

    //Liga led azul
    digitalWrite(LED_BUILTIN, HIGH);

    //(LoRaWAN.send(1, (uint8_t*)reading.c_str(), reading.length()))
    int error = LoRaWAN.sendReqAck(LORAWAN_PORT, (uint8_t*)reading.c_str(), reading.length(),1);
 
    switch (error)
    {
    case NoError:
      debugSerial.println("Successful transmission.");
      break;
    case NoResponse:
      debugSerial.println("There was no response from the device.");
      break;
    case Timeout:
      debugSerial.println("Connection timed-out. Check your serial connection to the device! Sleeping for 20sec.");
      delay(20000);
      break;
    case PayloadSizeError:
      debugSerial.println("The size of the payload is greater than allowed. Transmission failed!");
      break;
    case InternalError:
      debugSerial.println("Oh No! This shouldn't happen. Something is really wrong! The program will reset the RN module.");
      setupLoRa();
      break;
    case Busy:
      debugSerial.println("The device is busy. Sleeping for 10 extra seconds.");
      delay(10000);
      break;
    case NetworkFatalError:
      debugSerial.println("There is a non-recoverable error with the network connection. The program will reset the RN module.");
      setupLoRa();
      break;
    case NotConnected:
      debugSerial.println("The device is not connected to the network. The program will reset the RN module.");
      setupLoRa();
      break;
    case NoAcknowledgment:
      debugSerial.println("There was no acknowledgment sent back!");
      break;
    default:
      break;
    }

    //Retreive Downlink message is available
    //Retrieve the message that may be arrived after the uplink.
    //This is different of using the callback.
    uint8_t port;  
    uint8_t buffer[64];
    uint8_t size = LoRaWAN.receive(buffer,sizeof(buffer),&port,0);
    if(0 != size)
    {
      debugSerial.print("Downlink message (receive): ");  
      debugSerial.print(port);  
      debugSerial.print(" ");  
      for (uint16_t i=0; i<size; i++)
      {
        debugSerial.print((char) buffer[i]);      
      }
      debugSerial.println("");
    }

    // Recebeu error
    if (error != NoError)
    {
      digitalWrite(LED_RED, LOW);
      delay(1000);
      digitalWrite(LED_RED, HIGH);
    }

    //Sem erro e com downlink
    if (error == NoError && size != 0)
    {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);
      delay(1000);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, HIGH);
    }

    //Sem erro e sem downlink    
    if (error == NoError  && size == 0)
    {
      digitalWrite(LED_GREEN, LOW);
      delay(1000);
      digitalWrite(LED_GREEN, HIGH);
    }


    //Delay entre loops
    digitalWrite(LED_BUILTIN, LOW);   // Desliga LED azul
    LoRaWAN.sleep();
    delay(LOOP_DELAY); 
    LoRaWAN.wakeUp();
}

String getTemperature()
{
  //10mV per C, 0C is 500mV
  float mVolts = (float)analogRead(TEMP_SENSOR) * 3300.0 / 1023.0;
  float temp = (mVolts - 500.0) / 10.0;
  
  return String(temp);
}

/**
* Gets and stores the LoRa module's HWEUI/
*/
static void getHWEUI()
{
  uint8_t len = LoRaWAN.getHWEUI(DevEUI, sizeof(DevEUI));
}
