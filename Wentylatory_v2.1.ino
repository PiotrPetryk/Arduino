#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_AM2320.h>
#include <PString.h>
#include <Streaming.h>

/*
##############################################################################################################
#      Program sterujacy wyjsciem cyfrowym (w zalozeniu wentylatorami) w zaleznosci od temperatury.          #
#              Program umozliwia okreslenie temperatury docelowej oraz histerezy temeratury,                 #
#                     wplywajacej na moment wlaczenia i wylaczenia wentylatorow.                             #
#     Dzieki mozliwosci ustawienia godziny mozliwe jest rowniez okreslenie jednego okresu w ciagu doby       #
#                          kiedy wentylatory nie beda dzialaly (cisza nocna).                                #
#                                                                                             - Piotr Petryk #
#Wersja 2.0 z dn. 01.03.2020. Pierwsza wersja funkcjonalna.                                                  #
#Wersja 2.1 z dn. 14.03.2020 Dodane wyłączanie poswietlenia LCD. Zmiana histerezy na +/-0.25 C               #
#                            Cisza nocnea w zakresie do 24 i od 00                                           #
#                                                                                                            #
##############################################################################################################

*/

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
Adafruit_AM2320 klimat = Adafruit_AM2320();

#define przycisk 2      //Aduino Uno obsluguje przerwania wylacznie na PIN 2 i 3
#define kreci 3
#define strona 4
#define wentylatory 12
byte minuty, godziny, tryb = 0;
double histereza = 1.0;
byte tempDocelowa = 30;
byte ciszaNocnaStart = 23;
byte ciszaNocnaKoniec = 6;
unsigned long czasstary, czas, czasmig, czasaktywnosci = 0;
bool widacZnak, wcisnietoPrzycisk, LCDLED = true;     //widacZnak - informacja dla migania znaków/cyfr itp
bool prawo, lewo, wyswietl, praca, ciszaN = false;    //wyswietl - informacja dla LCD ze ma wyswietlic
unsigned int okresmigania = 999;              //okres migania znaków - 999 milisek.
unsigned int czaspodswietlenia = 5000;        // czas podwietlenia LCD przy bezczynnosci

char pracujeb[6];
char tempb[5];
char histb[4];
char linia0Wb[17];                             //linia 0 kiedy widac znak bufor
char linia1Wb[17];                             //linia 1 kiedy widac znak bufor
char linia0Nb[17];                             //linia 0 kiedy nie widac znaku bufor
char linia1Nb[17];                             //linia 1 kiedy nie widac znaku bufor

PString linia0W(linia0Wb,17);                             //linia 0 kiedy widac znak
PString linia1W(linia1Wb,17);                             //linia 1 kiedy widac znak
PString linia0N(linia0Nb,17);                             //linia 0 kiedy nie widac znaku
PString linia1N(linia1Nb,17);                             //linia 1 kiedy nie widac znaku
PString pracuje(pracujeb,6);
PString Stemperatura(tempb,5);
PString Shistereza(histb,4);

double temperatura=0;
 
void obrot(){                     //odczyt obrotów enkodera
  if (LCDLED) {                     // oczcyt ma nastapic tylko jesli ekran podswietlony
    if (digitalRead(strona)){
      lewo=true;
    }else{
      prawo=true;
    }
    wyswietl = true;             //pomysł na wyswietlanie zmienianej wartosci
    widacZnak = true;
  }  
  czasaktywnosci = millis();
}

void wcisnieto(){                 //odczyt przycisku ekodera
  if (LCDLED) {                     // oczcyt ma nastapic tylko jesli ekran podswietlony
    wcisnietoPrzycisk = true;
    wyswietl = true;
    widacZnak = true;
  }
  czasaktywnosci = millis();
}


void zmianagodziny(){
  minuty ++;
  if (minuty == 60) {
    godziny ++;
    minuty = 0;
  }
  if (godziny == 24) {
    godziny = 0;
  }
  wyswietl = true;
  widacZnak = true;
}


void setup() {
  pinMode (przycisk, INPUT_PULLUP);
  pinMode (kreci, INPUT_PULLUP);
  pinMode (strona, INPUT_PULLUP);
  pinMode (wentylatory, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(kreci), obrot, FALLING);
  attachInterrupt(digitalPinToInterrupt(przycisk), wcisnieto, FALLING);
  
  //Serial.begin(9600);

  lcd.begin(16,2); 
  lcd.backlight();

  klimat.begin();
  Wire.begin();  
 
  temperatura=(double)klimat.readTemperature();

  pracuje << "OFF";
  godziny = 0;
  minuty = 0;
  
  czasstary=millis();
  czasaktywnosci = millis();
  digitalWrite(wentylatory, HIGH);

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  
  czas=millis();
  
  if ((unsigned long)(czas - czasstary) >= 59999){
    zmianagodziny();
    czasstary=czas; 
  }

  if ((unsigned long)(czas - czasmig) >= okresmigania) {
    widacZnak = !widacZnak;
    czasmig=czas;
    temperatura=(double)klimat.readTemperature();
    Stemperatura ="";
    Stemperatura << temperatura;
    wyswietl=true;
  }

  if (((unsigned long)(czas - czasaktywnosci) < czaspodswietlenia) and !LCDLED) {
    lcd.backlight();
    LCDLED=true;
  }
  if (((unsigned long)(czas - czasaktywnosci) >= czaspodswietlenia) and LCDLED) {
    lcd.noBacklight();
    LCDLED=false;
  }

  if (wcisnietoPrzycisk) {
    wcisnietoPrzycisk = false;
    tryb++;
    if (tryb == 6) {
      tryb =0;
    }
  }

  switch (tryb){
    case 1:                   //ustwienie czasu - godziny
      if (prawo){
        godziny++;
        if (godziny == 24){
          godziny=0;
        }
      }
      if (lewo){
        godziny--;
        if (godziny == 255){
          godziny=23;
        }
      }
      linia0W << "Ustaw czas:     ";
      linia1W << ((godziny<10)?"0":"") << godziny << ":" << ((minuty<10)?"0":"") << minuty << "           ";      
      linia0N << "Ustaw czas:     ";
      linia1N << "  :" << ((minuty<10)?"0":"") << minuty << "           ";
    break;
 
    case 2:                   //ustwienie czasu - minuty
      if (prawo){
        minuty++;
        if (minuty == 60){
          minuty=0;
        }
      }
      if (lewo){
        minuty--;
        if (minuty == 255){
          minuty=59;
        }
      }
      linia0W << "Ustaw czas:     ";
      linia1W << ((godziny<10)?"0":"") << godziny << ":" << ((minuty<10)?"0":"") << minuty << "           ";      
      linia0N << "Ustaw czas:     ";
      linia1N << ((godziny<10)?"0":"") << godziny << ":             ";
    break;

    case 3:                   //ustwienie histerezy
      if (prawo){
        histereza=histereza+0.2;
        if (histereza == 3.2){
          histereza=3;
        }
      }
      if (lewo){
        histereza=histereza-0.2;
        if (histereza == 0){
          histereza=0.2;
        }
      }
      Shistereza="";
      Shistereza << histereza;
      linia0W << "Ustaw histereze:";
      linia1W << Shistereza << char(223) << "C.          ";      
      linia0N << "Ustaw histereze:";
      linia1N << "   " << char(223) << "C.          ";
    break;

    case 4:                   //ustwienie poczatku ciszy nocnej
      if (prawo){
        ciszaNocnaStart++;
        if (ciszaNocnaStart == 25){
          ciszaNocnaStart=24;
        }
      }
      if (lewo){
        ciszaNocnaStart--;
        if (ciszaNocnaStart == 11){
          ciszaNocnaStart=12;
        }
      }
      linia0W << "Cisza nocna:    ";
      linia1W << ((ciszaNocnaStart<10)?"0":"") << ciszaNocnaStart << " - " << ((ciszaNocnaKoniec<10)?"0":"") << ciszaNocnaKoniec << "         ";      
      linia0N << "Cisza nocna:    ";
      linia1N << "   - " << ((ciszaNocnaKoniec<10)?"0":"") << ciszaNocnaKoniec << "         ";
    break;

    case 5:                   //ustwienie konca ciszy nocnej
      if (prawo){
        ciszaNocnaKoniec++;
        if (ciszaNocnaKoniec == 13){
          ciszaNocnaKoniec=12;
        }
      }
      if (lewo){
        if (ciszaNocnaKoniec > 0){
          ciszaNocnaKoniec--;
        } else {
          ciszaNocnaKoniec=0;
        }
      }
      linia0W << "Cisza nocna:    ";
      linia1W << ((ciszaNocnaStart<10)?"0":"") << ciszaNocnaStart << " - " << ((ciszaNocnaKoniec<10)?"0":"") << ciszaNocnaKoniec << "         ";      
      linia0N << "Cisza nocna:    ";
      linia1N << ((ciszaNocnaStart<10)?"0":"") << ciszaNocnaStart << " -            ";
    break;
 
    default:
      if (prawo){
        tempDocelowa++;
        if (tempDocelowa == 36){
          tempDocelowa=35;
        }
      }
      if (lewo){
        tempDocelowa--;
        if (tempDocelowa == 19){
          tempDocelowa=20;
        }
      }

      if (temperatura > (double(tempDocelowa+histereza))) praca = true;
      if (temperatura < (double(tempDocelowa-histereza))) praca = false;
      if (((godziny*60+minuty) <= ciszaNocnaKoniec*60) or ((godziny*60+minuty) >= ciszaNocnaStart*60)){
        ciszaN = true;          
      } 
      else {
        ciszaN = false;
      }

      if (praca and !ciszaN) {
        digitalWrite(wentylatory, LOW);
        pracuje << "W:ON ";
      }
      if (praca and ciszaN) {
        digitalWrite(wentylatory, HIGH);
        pracuje << "Cisza";
      }
      if (!praca){
        digitalWrite(wentylatory, HIGH);
        pracuje << "W:OFF";
      }
          

      linia0W << ((godziny<10)?"0":"") << godziny << ":" << ((minuty<10)?"0":"") << minuty << " Temp" << Stemperatura << char(223) <<"C";
      linia1W << "Docel:" << tempDocelowa << char(223) <<"C " << pracuje;      
      linia0N << ((godziny<10)?"0":"") << godziny << " " << ((minuty<10)?"0":"") << minuty << " Temp" << Stemperatura << char(223) <<"C";
      linia1W << "Docel:" << tempDocelowa << char(223) <<"C " << pracuje;
    break;
  }
  
  if (wyswietl){

    if (widacZnak){
      lcd.setCursor(0,0);
      lcd.print(linia0W);
      lcd.setCursor(0,1);
      lcd.print(linia1W);
    }
    else{
      lcd.setCursor(0,0);
      lcd.print(linia0N);
      lcd.setCursor(0,1);
      lcd.print(linia1N);
    }
    wyswietl = false;
  }
  linia0W ="";
  linia1W ="";
  linia0N ="";
  linia1N ="";
  pracuje ="";
  prawo = false;
  lewo = false;
  delay (50);
}
