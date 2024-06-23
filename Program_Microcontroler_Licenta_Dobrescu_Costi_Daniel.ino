#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <time.h>
#include <Firebase_ESP_Client.h> 
#include "addons/TokenHelper.h" 
#include "addons/RTDBHelper.h" 

#define RST_PIN 0 // Pinul de resetare pentru toate cititoarele
//16  Pinul Slave Select pentru primul cititor
//21   Pinul Slave Select pentru al doilea cititor
//5  Pinul Slave Select pentru al treilea cititor
byte ssPins[] = {16, 21, 5};

MFRC522 rfid[3];  // Trei instanțe MFRC522 pentru cele trei cititoare RFID

#define PIN_BUTON 35
//14 Pinul pentru releul incuietorii 1 (incaperea 1)
//13 Pinul pentru releul incuietorii 2 (incaperea 2)
//27 Pinul pentru releul incuietorii 3 (incaperea 3)
byte lockPins[] = {14,13,27};

String uidString = "";
String path = "";

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//Variabile Globale/constante Time
char timeString[30];
String fbdoDate = "";

void setup() {
  Serial.begin(9600); //Initializare comunicatie seriala
  SPI.begin(); //Initializare bus SPI
  init_rfids(); //Initializare cititoare RFID
  //declarare pini control ca si pini de output
  pinMode(lockPins[0], OUTPUT); 
  pinMode(lockPins[1], OUTPUT); 
  pinMode(lockPins[2], OUTPUT); 
  //setare pini control incuietori pe valoarea 1(incuietori inchise)
  digitalWrite(lockPins[0], HIGH);
  digitalWrite(lockPins[1], HIGH); 
  digitalWrite(lockPins[2], HIGH);
  pinMode(PIN_BUTON, INPUT_PULLUP);
  //conectare la retea WiFi
  WifiConnection();
  //conectare la baza de date Firebase
  FirebaseSignIn();
  //Configuram timpul utilizand functia configTime()
  //gmtOffset_sec = 7200; // GMT+2
  //daylightOffset_sec = 0; // Fara compensatie pentru ora de vara
  //ntpServer = "ro.pool.ntp.org";
  configTime(10800, 0, "ro.pool.ntp.org");
  //Se asteapta pana cand configTime() finalizeaza actualizarea timpului
  while (!time(nullptr)) {
    delay(2000);
    Serial.println("Asteptare sincronizare NTP...");
  } 
}

void loop() {
  //Se verifica daca conexiunea cu Firebase este pregatita
  if (Firebase.ready()) { 
    //Se verifica daca un card este apropiat de fiecare cititor in parte
    for (uint8_t i = 0; i < sizeof(rfid)/sizeof(rfid[0]); i++) {
      if (rfid[i].PICC_IsNewCardPresent() && rfid[i].PICC_ReadCardSerial()) {
        uidString = "";
        path = "";
        //Se obtine UID-ul cardului in momentul citirii acestuia
        String uidString = getUIDString(i, rfid[i].uid.uidByte, rfid[i].uid.size);
        //Se formeaza path-ul unde este salvat UID-ul in baza de date
        path = "UIDs_Saved/" + uidString;
        //Se obtine informatia existenta in path-ul respectiv
        Firebase.RTDB.getString(&fbdo, path);
        //Se verifica daca informatia obtinuta nu este nula
        if (fbdo.dataType() == "json" && fbdo.dataType() != NULL) {
          //Se formeaza path-ul unde este salvata data de expirare a cardului
          path = path + "/DataExpirare";
          //Se obtine data de expirare a cardului
          Firebase.RTDB.getString(&fbdo, path);
          //Se obtine timpul real
          updateTimeString();
          //Se verifica daca cardul nu este expirat
          fbdoDate = "";
          fbdoDate = fbdo.stringData().substring(0, 8);
          String timeStringStr(timeString);
          if (fbdoDate.substring(6, 8) > timeStringStr.substring(6, 8) ||
            (fbdoDate.substring(6, 8) == timeStringStr.substring(6, 8) && fbdoDate.substring(0, 2) > timeStringStr.substring(0, 2)) ||
            (fbdoDate.substring(6, 8) == timeStringStr.substring(6, 8) && fbdoDate.substring(0, 2) == timeStringStr.substring(0, 2) && fbdoDate.substring(3, 5) > timeStringStr.substring(3, 5)))
          {
            path = "";
            //Se obtin drepturile de acces ale angajatului
            path = "UIDs_Saved/" + uidString + "/TipAngajat";
            Firebase.RTDB.getString(&fbdo, path);
            if (fbdo.dataType() == "string") {
              //Se proceseaza daca angajatul are drepturile necesare deschiderii usii
              processAccess(i, uidString, digitalRead(PIN_BUTON), fbdo.stringData());
            }else{
              Serial.println("Datele nu sunt de tip string!");
            }
          }else{
            Serial.println("Cardul angajatului " + uidString + " este expirat!");
          }
        }else {
           Serial.println("Cardul citit " + uidString + " nu exista in baza de date!");
        }
        rfid[i].PICC_HaltA(); //Opreste comunicarea cu cardul RFID curent
        rfid[i].PCD_StopCrypto1(); //Dezactiveaza criptarea pe cititorul RFID dupa terminarea comunicarii
        delay(3000); 
      }
    }
  }
}

void init_rfids() {
  uint8_t k = 1;
  pinMode(RST_PIN, INPUT);
  if (digitalRead(RST_PIN) == HIGH) { // Daca cipul MFRC522 nu este in modul de oprire a alimentarii
    pinMode(RST_PIN, OUTPUT); // Setam pinul de resetare ca iesire digitala
    digitalWrite(RST_PIN, LOW); // Ne asiguram că avem un LOW pe reset.
    delayMicroseconds(2); // Cerintele de timp pentru resetare 
    digitalWrite(RST_PIN, HIGH); // Iesim din modul de oprire a alimentarii. Acest lucru declanseaza hard reset pentru cititoare.
    delay(50); // Timpul de pornire al oscilatorului.
  }
  else {
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, HIGH); // Iesim din modul de oprire a alimentarii. Acest lucru declanseaza hard reset.
    delay(50); // Timp de pornire a oscilatorului.
  }
  
  for (uint8_t i = 0; i < sizeof(rfid)/sizeof(rfid[0]); i++) {
    rfid[i].PCD_Init(ssPins[i], -1); // Initializam modulele RFID cu SOFT RESET
    if (!rfid[i].PCD_PerformSelfTest()) {
      k=0;
    }
    rfid[i].PCD_Init(ssPins[i], -1);
  }
  Serial.println(String(k));
}

void processAccess(uint8_t i, String uidString, uint8_t input, String tipAngajat) {
  uint8_t j = i + 1;
  String path = "UIDs_History/" + uidString;
  Serial.println("UID: " + uidString);
  updateTimeString();
  if((i == 1 && (tipAngajat == "Student internship" || tipAngajat == "Inginer")) || (i == 2 && tipAngajat != "Director")){
    Serial.println("Angajatul " + uidString + " nu are acces in sala " + String(j));
    return;
  }  
  if(i == 0 && input == 1) 
  {
    Firebase.RTDB.pushString(&fbdo, path, "Acces iesire sala " + String(j) + ", Timestamp: " + timeString);
    Serial.println("Acces iesire sala " + String(j) + " " + tipAngajat + " UID: " + uidString);
  }else {
    Firebase.RTDB.pushString(&fbdo, path, "Acces intrare sala " + String(j) + ", Timestamp: " + timeString);
    Serial.println("Acces intrare sala " + String(j) + " " + tipAngajat + " UID: " + uidString);
  }
  digitalWrite(lockPins[i], LOW);
  delay(3000);
  digitalWrite(lockPins[i], HIGH);
}

String getUIDString(uint8_t reader, byte *buffer, byte bufferSize) {
  String uidString = "";
  //Construiesc sirul UID
  for (byte i = 0; i < bufferSize; i++) {
    //Adaug zero in fata valorilor mai mici de 0x10
    if (buffer[i] < 0x10) {
      uidString += "0";
    }
    //Convertesc byte-ul la reprezentarea hexazecimala si il adaugam la sir
    uidString += String(buffer[i], HEX);
  }
  uidString.toUpperCase();
  return uidString;
}

void updateTimeString() {
  //Obtinem timpul curent ca numarul de secunde incepand de la 1 ianuarie 1970 (Epoch)
  time_t now = time(nullptr);
  // Convertim timpul într-o structură tm
  struct tm* localTime = localtime(&now);
  //Formatăm timpul în șirul de caractere folosind strftime()
  strftime(timeString, sizeof(timeString), "%m/%d/%y %H:%M:%S", localTime);
}

void WifiConnection(){
  //WIFI connection
  WiFi.begin("NUME_RETEA", "PAROLA");
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void FirebaseSignIn()
{
    //Firebase connection
    /* Assign the api key (required) */
    config.api_key = "AIzaSyBVKVFogXlM5qfud8_jJGHxptUTCDvkGkM";
    /* Assign the RTDB URL (required) */
    config.database_url = "https://proiectlicenta-287bd-default-rtdb.europe-west1.firebasedatabase.app/";
    fbdo.setResponseSize(4096);
    // Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
    // Assign the maximum retry of token generation
    config.max_token_generation_retry = 5;
    Firebase.reconnectWiFi(true);
    /* Assign the user sign in credentials */
    auth.user.email = "daniel_dobrescu23@yahoo.com";
    auth.user.password = "adminsuperuser$2001#";
    /* Reset stored authen and config */
    Firebase.reset(&config);
    /* Initialize the library with the Firebase authen and config */
    Firebase.begin(&config, &auth);
}
