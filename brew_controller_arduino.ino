/*
Project for Brew control (temperature, ramp time)

v1.1.0 - Keypad LCD shield and temperature sensor

Autor: Denis Magalhães de Almeida Eiras
e-mail: denis.eiras@gmail.com

History
- 1.0.0 - First version with buttons, LCD and temperature sensor
- 1.1.0 - this version

References:
https://www.youtube.com/watch?v=QsGPJMFbA50
https://www.youtube.com/watch?v=YUITi1nGLyc

TODO 

- create recipes 
- alert for putting the ingredients
- exclude ramp
- save status to eprom in case of energy fail


*/



//================= Libraries ========================

#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>


//================= Port / Wire Config ========================

// A0 - output - All buttons of keypad shield
// D4-09 - used by the Keypad Shield

// temperature sensor DS18B20 - yellow wire
#define TEMP_SENSOR 13

// RESISTANCE_PORT - orange wire
#define RESISTANCE_PORT 2

// PUMP_PORT - blue wire - NOT USED YET
//#define PUMP_PORT 1


// ================== System variables ==================

#define LCD_OUT 0
#define SERIAL_OUT 1
#define SYSTEM_OUT 0

#define pinButtons A0

#define buttCancel 8       //Botão para cancelar
#define buttOk 9           //Botão para confirmar
#define buttSub 10         //Botão para subtrair valor
#define buttAdd 11         //Botão para somar valor

#define pinRs 8
#define pinEn 9
#define pinD4 4
#define pinD5 5
#define pinD6 6
#define pinD7 7
#define pinBackLight 10

// system parameters
float tempOffsetOn = -0.5;
float tempOffsetOff = -0.5;
float tempOffsetSensor = 2.0;     // Temperatura Lida - Real
float resistance_power_max = 1.0;  // porcentagem da potencia maxima da resistencia via relé de estado sólido
float resistance_inteval_seconds = 60;
bool backHigh = true;

// variables definition
String lcd_serial[2];
String programPhase; // fase: configurar, brassagem .. etc
int rampLast = 0;    // ultima rampa incluida + 1
int rampTemp[4];
int rampMinutes[4];
char tempChar[4];
char tempChar2[4];
char tempChar3[4];
char tempChar4;

float tempNow;       // temp atual
int rampNow;         // rampa atual
int secondsRamp;
int secondsInitRamp;
int secondsPrint, minutesPrint, hoursPrint;
float resistance_power = 0.8;
float resistance_power_now = LOW;
float resistance_on_seconds;
float resistance_init_seconds;


// =================== Devices initialization =====================

OneWire oneWire(TEMP_SENSOR);
DallasTemperature sensors(&oneWire);
DeviceAddress sensor1;
LiquidCrystal lcd(pinRs, pinEn, pinD4, pinD5, pinD6, pinD7);


// =================== API ======================

// printing
void cls();
void cls_line(byte lin);
void print_out(String messg, byte lin);
void print_del(String messg, byte lin);
void print_out(String messg, byte lin, byte col);
void print_del(String messg, byte lin, byte col);
void print_lcd(String messg, byte lin, byte col);
void print_ser(String messg);

// button comands
bool pressed(int butt);
//int getValue(String strVarName, String unity, int minValue, int maxValue );
float getValue(String strVarName, String unity, float value_, float incr_, int minValue, int maxValue);
int getButtValue(int butt, int value, int incr);
bool getOk();
String menuSelect(String menu[], int menuSize);
void setPanelaOnOff(float tempNowCheck, int rampNowCheck);
int revOrFwd(int rampNow_old);
void read_res_power();

// sys functions
float readTemp();


// ===================== Setup ========================

void setup() { 
  Serial.begin(9600);
  programPhase = '*';
  lcd_serial[0] = "";
  lcd_serial[1] = "";

//  pinMode(PUMP_PORT, OUTPUT);
//  digitalWrite(PUMP_PORT, HIGH); //Desliga bomba
  
  pinMode(RESISTANCE_PORT, OUTPUT);
  digitalWrite(RESISTANCE_PORT, LOW); //Desliga rele

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  pinMode(pinBackLight, OUTPUT);
  digitalWrite(pinBackLight, HIGH);
  
  sensors.begin();
  if (!sensors.getAddress(sensor1, 0)) 
     Serial.println("Sensores nao encontrados !"); 
  
  print_del("Bem vindo!", 0);
  cls();
}


// ===================== Loop ========================
void loop() {
  
  if (programPhase == "*") {
    String menu[] = {"Config. Rampas", "Brassagem", "Config. Params", "Temperatura", "Luz de Fundo" };
    programPhase = menuSelect(menu, 5);
  
  } else if ( programPhase == "Config. Rampas") {
    String menu[] = { "Mash", "Fervura", "Ver rampas", "Excluir rampa" };
    programPhase = menuSelect(menu, 4);

    if (programPhase == "Mash") {
      int diasAux = 0;
      // int diasAux = getValue("Tempo", "dias", 0, 180);
      
      // test
      //int tempC = getValue("Temperatura", "C", 27.0, 1.0, -20, +100);
      //int horasAux = getValue("Tempo", "horas", 0.0, 1.0, 0, 24);
      //int minAux = getValue("Tempo", "min", 2.0, 1.0, 0, 60 );

      // real
      int tempC = getValue("Temperatura", "C", 65.0, 1.0, -20, +100);
      int horasAux = getValue("Tempo", "horas", 1.0, 1.0, 0, 24);
      int minAux = getValue("Tempo", "min", 0.0, 1.0, 0, 60 );
      
      if ( ! getOk()) {
        programPhase = '*';
      } else {
        rampTemp[rampLast] = tempC;
        rampMinutes[rampLast] = diasAux*24*60 + horasAux*60 + minAux;        
        print_out("Rampa: ", 0);
        print_out(String(rampLast), 0, 8);
        print_del("incluida !", 1);
        programPhase = "Config. Rampas";
        rampLast++;
      }
    } else if (programPhase == "Fervura") {
      int tempC = 100;
      int horasAux = getValue("Tempo", "horas", 1.0, 1.0, 0, 24);
      int minAux = getValue("Tempo", "min", 0.0, 1.0, 0, 60 );
      if ( ! getOk()) {
        programPhase = '*';
      } else {
        rampTemp[rampLast] = tempC;
        rampMinutes[rampLast] = horasAux*60 + minAux;        
        print_out("Rampa: ", 0);
        print_out(String(rampLast), 0, 8);
        print_del("incluida !", 1);
        programPhase = "Config. Rampas";
        rampLast++;
      }
    
    } else if (programPhase == "Ver rampas") {
      for (byte i = 0; i < rampLast; i++) {
        cls();
        print_out("Rampa: ", 0);
        print_out(String(i), 0, 8);
        print_del(String(rampTemp[i]) + " C " + String(rampMinutes[i]) + " minutos", 1, 0);
      }
      programPhase = "Config. Rampas";
    }


  } else if (programPhase == "Brassagem") {
    
    cls();
    print_out("Iniciar Brass.?", 0);
    print_out("Canc) Ok)", 1);
    if (getOk()) {
      rampNow = 0;
      while (rampNow < rampLast) {
     
        print_out("Ok) Rampa " + String(rampNow), 0);
        print_out("Canc) ", 1);
        if (!getOk()) {
          // desisitiu no comeco
          print_out("Ok) pula rampa", 0);   
          print_out("Canc) rampa ant", 1);
          if (getOk()) {
            rampNow ++;
          } else {
            if (rampNow > 0) {
              rampNow --;
            }
          }
          continue;
        }

        //se mash
        bool rampChanged = false;
        if(rampNow < rampLast-1) {
          // loop até atingir a temp da rampa
          resistance_init_seconds = 0;
          do {
            tempNow = readTemp();
            setPanelaOnOff(tempNow, rampNow);
            dtostrf(tempNow, 2, 1, tempChar);
            dtostrf(rampTemp[rampNow], 2, 0, tempChar2);
            dtostrf(resistance_power * 100, 2, 0, tempChar3);
            tempChar4 = resistance_power_now == LOW ? ' ' : '*';
           
//            if (resistance_power_now==LOW) {
//              tempChar4 = '*';
//            } else {
//              tempChar4 = ' ';
//            }
  
            print_out("Alcanc Ramp " + String(rampNow), 0); 
            
            print_del("P" + String(tempChar4) + String(tempChar3) + " " + String(tempChar2) + "/" + String(tempChar) + " C", 1);
            if(pressed(buttCancel)) {
              rampNow = revOrFwd(rampNow);
              rampChanged = true;
              break;
            } 
            read_res_power();
              
          } while(tempNow < rampTemp[rampNow]);
          
          if (rampChanged) {
            rampChanged = false;
            continue;
          }
          
          print_out("Rampa " + String(rampNow) + " Temp: " + String(rampTemp[rampNow]), 0);
          print_del("Temp. atingida!", 1);
        } else {
          resistance_init_seconds = 0;
          while(true) {
            tempNow = readTemp();
            setPanelaOnOff(tempNow, rampNow);
            dtostrf(tempNow, 2, 1, tempChar);
            dtostrf(rampTemp[rampNow], 2, 0, tempChar2);
            dtostrf(resistance_power * 100, 2, 0, tempChar3);
            tempChar4 = resistance_power_now == LOW ? ' ' : '*';
 
            print_out("Ok fervura?" + String(rampNow), 0); 

            print_del("P" + String(tempChar4) + String(tempChar3) + " " + String(tempChar2) + "/" + String(tempChar) + " C", 1);
            if(pressed(buttCancel)) {
              rampNow = revOrFwd(rampNow);
              rampChanged = true;
              break;
            }
            read_res_power();
            if(pressed(buttOk)) {
              break;
            }
          }
        }
        
        // loop rampa   
        secondsInitRamp = (int) ((float) millis() / 1000.0);
        hoursPrint = (int) ((float)rampMinutes[rampNow] / 60.0);
        minutesPrint = (int) (rampMinutes[rampNow] % 60);
          
        print_out("R: " + String(rampNow) + " " + String(rampTemp[rampNow]) + "C " + String(hoursPrint) + ":" + String(minutesPrint) + ":00" , 0);
        resistance_init_seconds = 0;
        do {
          tempNow = readTemp();
          setPanelaOnOff(tempNow, rampNow);
          dtostrf(tempNow, 2, 1, tempChar);
          dtostrf(resistance_power * 100, 2, 0, tempChar3);
          tempChar4 = resistance_power_now == LOW ? ' ' : '*';

          secondsRamp = (int) ((float) millis() / 1000.0) - secondsInitRamp;
          hoursPrint = (int) ((float) secondsRamp / 3600.0);
          minutesPrint = (int) (((float) (secondsRamp % 3600) / 60.0));
          secondsPrint = (int) (secondsRamp % 3600) % 60;
          print_out( String(tempChar4) + String(tempChar3) + " " +String(tempChar) + "C " + String(hoursPrint) + ":" + String(minutesPrint) + ":" + String(secondsPrint), 1);
          delay(990);
          
          if(pressed(buttCancel)) {
            rampNow = revOrFwd(rampNow);
            rampChanged = true;
            break;
          }
          read_res_power();

//          Serial.println("SecondsRamp = " + String(secondsRamp));
//          Serial.println("RampNow = " + String(rampNow));
//          Serial.println("Ramp last = " + String(rampLast));
//          Serial.println("rampMinutes[rampNow] " + String(rampMinutes[rampNow]));
//          Serial.println("rampMinutes[rampNow] * 60 " + String(rampMinutes[rampNow] * 60));
     
        } while (secondsRamp < rampMinutes[rampNow] * 60 );

        if (rampChanged) {
          rampChanged = false;
          continue;
        }
        
        print_out("Rampa " + String(rampNow), 0);
        print_del("Tempo atingido!", 1);
        rampNow++;
      }
      digitalWrite(RESISTANCE_PORT, LOW);  //DesLiga rele
    }
    programPhase = "*";


  } else if (programPhase == "Temperatura") {
    print_out("Iniciar Resfr.?", 0);
    print_out("Canc / Ok", 1);
    if (getOk()) {
      print_out("Coloque o sensor", 0);
      print_out("no fermentador", 1);
      if (getOk()) {
        do {
          tempNow = readTemp();
          dtostrf(tempNow, 2, 1, tempChar);
          print_out("Temp: " + String(tempChar), 0);
          print_out("Canc) Fin. Resfr", 1);
        } while(!pressed(buttCancel));
      }
    }
    programPhase = "*";


  } else if (programPhase == "Luz de Fundo") {
    do {
      delay(200);
    } while(!pressed(buttOk));
    
    backHigh = !backHigh;
    // Todo - not working
    //if(backHigh) {
    //  lcd.setBacklight(HIGH);
    //} else {
    //lcd.setBacklight(LOW);
    //}

    programPhase = "*";
  
  } else if ( programPhase == "Config. Params") {
    resistance_inteval_seconds = getValue("Intrv Resist seg", "C", resistance_inteval_seconds, 1.0, +10, +600);
    tempOffsetSensor = getValue("Temp Diff Sensor", "C", tempOffsetSensor, 0.1, -10, +10);
    tempOffsetOn = getValue("Temp Offset on", "C", tempOffsetOn, 0.1, -10, +10);
    tempOffsetOff = getValue("Temp Offset off", "C", tempOffsetOff, 0.1, -10, +10);
    programPhase = "*";
  }
  
}


// ===================== API ==========================

void read_res_power() {

  if(pressed(buttAdd)) {
    resistance_power += 0.1;
    resistance_power = min(resistance_power, 1.0);
  } else if(pressed(buttSub)) {
    resistance_power -= 0.1;
    resistance_power = max(resistance_power, 0.0);
  }

}

void setPanelaOnOff(float tempNowCheck, int rampNowCheck) {

//  if(tempNowCheck > rampTemp[rampNowCheck] && digitalRead(RESISTANCE_PORT) == LOW) {

  if(tempNowCheck >= rampTemp[rampNowCheck] + tempOffsetOff) {    
    resistance_power_now = LOW;
    Serial.println("resistance_power_now = " + String(resistance_power_now));
    digitalWrite(RESISTANCE_PORT, resistance_power_now);
    
  } else if(tempNowCheck < rampTemp[rampNowCheck] + tempOffsetOn) {

    float resistance_on_seconds =  resistance_power * resistance_inteval_seconds;
    float resistance_off_seconds = resistance_inteval_seconds - resistance_on_seconds;
    Serial.println(" Res on secs =  " + String(resistance_on_seconds));
    Serial.println(" Res off secs = " + String(resistance_off_seconds));
    Serial.println(" Res init     = " + String(resistance_init_seconds));


    if (resistance_power_now == HIGH && resistance_off_seconds > 0 && ((float) millis() / 1000.0) - resistance_init_seconds  > resistance_on_seconds) {
      resistance_power_now = LOW;
      Serial.println("resistance_power_now 1 = " + String(resistance_power_now));
      digitalWrite(RESISTANCE_PORT, resistance_power_now);
      resistance_init_seconds = ((float) millis() / 1000.0);
      Serial.println(" Res init     = " + String(resistance_init_seconds));
    } else if (resistance_power_now == LOW && resistance_on_seconds > 0 && ((float) millis() / 1000.0) - resistance_init_seconds > resistance_off_seconds) {
      Serial.println("resistance_power_now 2 = " + String(resistance_power_now));
      resistance_power_now = HIGH;
      digitalWrite(RESISTANCE_PORT, resistance_power_now);
      resistance_init_seconds = ((float) millis() / 1000.0);
      Serial.println(" Res init     = " + String(resistance_init_seconds));
    }

    
//    resistance_power = resistance_power_max;
//    digitalWrite(RESISTANCE_PORT, resistance_power * 255);
    
    // utilizando potenciometro
    //    resistance_power = analogRead(potentiometer_port);
    //    resistance_power = resistance_power * 255 / 1024;
  }
  
}


String menuSelect(String menu[], int menuSize) {
  int itemLine = 0;
  int itemLineAux = -1;

  delay(500); //avoid entering button ok pressed again
  do {
    itemLine = getButtIncr(buttAdd, itemLine, -1);
    itemLine = getButtIncr(buttSub, itemLine, +1);

    if (itemLine >= 0 && itemLine < menuSize) {
      if (itemLine != itemLineAux) {
        itemLineAux = itemLine;
        if (itemLine % 2 == 0) {
          print_out("* " + menu[itemLine], 0);
          print_out("  " + menu[itemLine + 1], 1);
        } else {
          print_out("  " + menu[itemLine - 1], 0);
          print_out("* " + menu[itemLine], 1);
        }
      }
    } else {
      itemLine = itemLineAux;
    }
  } while (! (pressed(buttCancel) || pressed(buttOk) ) );

  if (pressed(buttCancel)) return "*";
  return menu[itemLine];
}


bool pressed(int butt) {

  int valButtons = analogRead(pinButtons);

  if ((valButtons < 800) && (valButtons >= 600)) { // SELECT
     return butt == buttOk;
     
  } else if ((valButtons < 600) && (valButtons >= 400)) { // LEFT
     return butt == buttCancel;
     
  } else if ((valButtons < 400) && (valButtons >= 200)) { // UP
     return butt == buttSub;
     
  } else if ((valButtons < 200) && (valButtons >= 60)) { // DOWN
     return butt == buttAdd;
     
//  } else if  (valButtons < 60) { // RIGHT
//     estadoBotao(btRIGHT);
     
  } else {
     return false;
  }
}


int getButtIncr(int butt, int value, int incr) {
  int valButt = value;
  if (pressed(butt)) {
    valButt += incr;
    delay(400);
    while (pressed(butt)) {
      valButt += incr;
      delay(100);
    }
  }
  return valButt;
}


float getButtValue(int butt, float value, float incr, bool printValue, String unity, int minValue, int maxValue) {
  float valButt = value;
  if (pressed(butt)) {
    if(valButt + incr >= minValue && valButt + incr <= maxValue)
      Serial.println(" valButt + incr = " + String(valButt + incr));
      valButt += incr;
      Serial.println(" valButt" + String(valButt));
      if (printValue) {
        print_out(String(valButt) + " " + unity, 1);
      }
      delay(400);
      while (pressed(butt)) {
        if(valButt + incr >= minValue && valButt + incr <= maxValue) {
          valButt += incr;
          if (printValue) {
            print_out(String(valButt) + " " + unity, 1);
          }
          delay(100);
        }
      }
    }

  return valButt;

}


float getValue(String strVarName, String unity, float value_, float incr_, int minValue, int maxValue) {

  float value = value_;
  float incr = incr_;
  bool cancelled = false;
  bool ok = false;

  cls();
  print_out(strVarName + "?", 0);
  print_out(String(value) + " " + unity, 1);
  delay(300);

  do {

    value = getButtValue(buttAdd, value, incr, true, unity, minValue, maxValue);
    value = getButtValue(buttSub, value, -incr, true, unity, minValue, maxValue);
    cancelled = pressed(buttCancel);
    ok = pressed(buttOk);

  } while (! (cancelled || ok) );

  if (cancelled) {
    return -9999;
  } else {
    Serial.println("get = " + String(value));
    return value;
  }

}

bool getOk() {

  bool ok = false;
  bool cancelled = false;

  delay(500); //avoid entering button ok pressed again
  do {
    cancelled = pressed(buttCancel);
    ok = pressed(buttOk);
  } while (! (cancelled || ok) );

  return ok;

}

void cls() {
  cls_line(0);
  cls_line(1);
}


void cls_line(byte lin) {
  if(SYSTEM_OUT == LCD_OUT) {
    print_lcd("                ", lin, 0);
  } else {
    print_ser("                ");
  }
}


void print_out(String messg, byte lin) {
  cls_line(lin);
  print_out(messg, lin, 0);
}


void print_del(String messg, byte lin) {
  cls_line(lin);
  print_del(messg, lin, 0);
}


void print_out(String messg, byte lin, byte col) {
  if(SYSTEM_OUT == LCD_OUT) {
    print_lcd(messg, lin, col);
  } else {
    print_ser("\n****************");
    lcd_serial[lin] = messg;
    print_ser(lcd_serial[0]);
    print_ser(lcd_serial[1]);
    print_ser("****************");
  }
}


void print_del(String messg, byte lin, byte col) {
  print_out(messg, lin, col);
  delay(1000);
}


void print_lcd(String messg, byte lin, byte col ) {
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(col, lin);
  lcd.print(messg);
}


void print_ser(String messg) {
  Serial.println(messg); 
}


float readTemp() {
  char tempChar[4];
  //String tempStr; // , tempStrMax, tempStrMin;

  // real
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(sensor1);

  // corrigida
  return tempC + tempOffsetSensor;
}


int revOrFwd(int rampNow_old) {
  rampNow = rampNow_old;
  print_out("Ok) pula rampa", 0);   
  print_out("Canc) rampa ant", 1);
  if (getOk()) {
    rampNow ++;
  } else {
    rampNow --;
  }
  return rampNow;
}
