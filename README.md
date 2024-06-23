# ProiectLicenta

Sistem de control al accesului cu carduri RFID

Pentru funcționarea sistemului este nevoie atât de partea practică (sistemul fizic de control al accesului format din componente HW) cât și de partea software ce cuprinde programul de pe microcontroler, configurarea unui proiect Firebase dar și un browser web pentru a deschide site-ul pentru administrare.
Sistemul este dependent de o conexiune stabilă la internet.
Conexiunile HW ale sistemului sunt bine documentate în lucrarea de licență.
Voi explica în continuare cum se configurează partea SW a proiectului.

Programul C++ pentru microcontroler
-> Fișierul Program_Microcontroler cu extensia .ino conține codul sursă pentru microcontroler.
-> Pentru a putea fi compilat și încărcat pe microcontroler trebuie folosită ultima versiune de Arduino IDE, trebuie descărcate paketul următor pentru a putea folosi ESP32 în Arduino IDE (link https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json), trebuie instalate bibliotecile #include <WiFi.h>, <SPI.h>, <MFRC522.h>, <time.h>, <Firebase_ESP_Client.h> din LIBRARY MANAGER.
-> În codul funcției WifiConnection() trebuie introdus numele retelei WiFi si parola (linia 191).
-> În codul funcției FirebaseSignIn() trebuie introduse cheia API a proiectului firebase și url-ul bazei de date (liniile 207 și 209), emailul și parola unui cont autorizat ce are acces la baza de date (liniile 217 și 218).

Baza de date Firebase Realtime Database
-> Pentru a putea configura baza de date trebuie să importăm fișierul StructuraBazeiDeDate.json într-un proiect Firebase Realtime Database.

Aplicația pentru Administrarea Informațiilor
-> Se găsește în arhiva .RAR, fișierul trebuie dezarhivat și deschis doar fișierul login_form.html cu un browser existent pe calculator.
