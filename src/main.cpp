

/*
 * (c)20016-17 Pawel A. Hernik
 * 
  ESP-01 pinout from top:

  GND    GP2 GP0 RX/GP3
  TX/GP1 CH  RST VCC

  GPIO 2 - DataIn
  GPIO 1 - LOAD/CS
  GPIO 0 - CLK

  ------------------------
  NodeMCU 1.0 pinout:

  D8 - DataIn
  D7 - LOAD/CS
  D6 - CLK

*/

//#include <Arduino.h>
#include <ESP8266WiFi.h>
WiFiClient client;
WiFiClientSecure sclient;

#define NUM_MAX 6
#define ROTATE  0

// for ESP-01 module
//#define DIN_PIN 2 // D4
//#define CS_PIN  3 // D9/RX
//#define CLK_PIN 0 // D3

// for NodeMCU 1.0/D1 mini
#define DIN_PIN 15  // D8
#define CS_PIN  13  // D7
#define CLK_PIN 14  // D6

#include "max7219.h"
#include "fonts.h"

// =======================================================================
// Your config below!
// =======================================================================
const char* ssid     = "ssid";      // SSID of local network
const char* password = "passwd";    // Password on network

String ytApiV3Key = "yourkey";  // Google 
String channelId1 = "UCFo2uTuDWU-klP58hE34iLg";
String channelId2 = "chan2id";

String weatherKey  = "yourkey";   // openweathermap.org key
String cityID      = "756135";    // Warsaw
String weatherLang = "&lang=en";  // pl, en or other language

String aqiToken    = "yourtoken"; // aqicn.org, api.waqi.info
String aqiStation1 = "poland/mazowieckie/warszawa/komunikacyjna";  // Warsaw
String aqiStation2 = "poland/mazowieckie/piastow/pulaskiego";  // Piastow

String smogStation1 = "cities/warszawa/stations/niepodleglosci/measurements";
String smogStation2 = "cities/piastow/stations/pulaskiego/measurements";

long utcOffset = 1; // for Warsaw,Poland
// =======================================================================

int dx = 0, dy = 0;
int h, m, s;
int day, month, year, dayOfWeek;
int summerTime = 0;
long localEpoc = 0;
long localMillisAtUpdate = 0;
String date;

float elapsed;
int elapsedH, elapsedM, elapsedS;
int totalH, totalM, totalS;
int state;
int kodiOK=0;

long viewCount[2],viewCount24h[2]={-1,-1},viewsGain24h[2];
char viewCountTime[2][10];
long subscriberCount[2], subscriberCount1h[2]={-1,-1}, subscriberCount24h[2]={-1},subsGain1h[2]={0}, subsGain24h[2]={0};
char subscriberCountTime[2][10];
//long videoCount;
unsigned long time1h[2],time24h[2];

float temp,temp_min,temp_max;
String humidity,clouds;
int pressure;
String wind_speed;
String weather_info;
int aqi=0,aqiH,aqiTz;

String currencyBuy,currencySell,currencyDir;

String buf="";



int cnt = 0;
#define LAST_DISP 14




char txt[30],txt2[20];
int scrollDel = 20;
int del = 3000;

//char* monthNames[] = {"STY","LUT","MAR","KWI","MAJ","CZE","LIP","SIE","WRZ","PAZ","LIS","GRU"};
char* monthNames[] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};

void showTime()
{
  sprintf(txt,"%d:%02d",h,m);
//  sprintf(txt2,"%d.%02d",day,month);
  sprintf(txt2,"%d%s%s",day,(day>19)?"":"&",monthNames[month-1]);
  int wd1 = stringWidth(txt,dig5x8rn,' ');
  int wd2 = stringWidth(txt2,small3x7,' ');
  int wd = NUM_MAX*8-wd1-wd2-1;
  printStringWithShift("  ",scrollDel,font,' ');
  printStringWithShift(txt, scrollDel, dig5x8rn, ' '); // real time
  while(wd-->0) printCharWithShift('_', scrollDel, font, ' ');
  printStringWithShift(txt2, scrollDel, small3x7, ' '); // date
}

void getTimeAndSubs(int i)
{
  String ch[2] = { channelId1, channelId2 };
  if(getYTData(ch[i],i)==0) {
    if(subscriberCount1h[i]<0) {
      time1h[i] = time24h[i] = millis();
      subscriberCount1h[i] = subscriberCount24h[i] = subscriberCount[i];
      viewCount24h[i] = viewCount[i];
      viewsGain24h[i] = 0;
      sprintf(subscriberCountTime[i],"%d:%02d",h,m);
      sprintf(viewCountTime[i],"%d:%02d",h,m);
    }
    if(millis()-time24h[i]>1000*60*60*24) {
      time24h[i] = millis();
      subscriberCount24h[i] = subscriberCount[i];
    }
    if(viewCount[i]-viewCount24h[i]) {
      viewsGain24h[i] = viewCount[i]-viewCount24h[i];
      viewCount24h[i] = viewCount[i];
      sprintf(viewCountTime[i],"%d:%02d",h,m);
    }
    subsGain24h[i] = subscriberCount[i]-subscriberCount24h[i];
  }
}

void showSubs(int i)
{
  printStringWithShift("    YT subs: ",scrollDel,font,' '); // pol
  printValueWithShift(subscriberCount[i],scrollDel,0);
  if(subsGain24h[i]==0) cnt++;
}

void showSubsGain(int i)
{
    printStringWithShift("  Gain since "+String(subscriberCountTime[i])+" ",scrollDel,font,' '); // eng
    //printStringWithShift("  Przyrost od "+String(subscriberCountTime[i])+" ",scrollDel,font,' '); // pol
    printValueWithShift(subsGain24h[i],scrollDel,1);
}

void showViews(int i)
{
  printStringWithShift("  Views: ",scrollDel,font,' '); // eng
  //printStringWithShift("  Wyświetlenia: ",scrollDel,font,' '); // pol
  printValueWithShift(viewCount[i],scrollDel,0);
  if(viewsGain24h[i]==0) cnt++;
}

void showViewsGain(int i)
{
  printStringWithShift("  Gain at "+String(viewCountTime[i])+" ",scrollDel,font,' '); // eng
  //printStringWithShift("  Przyrost o "+String(viewCountTime[i])+" ",scrollDel,font,' '); // pol
  printValueWithShift(viewsGain24h[i],scrollDel,1);
}

void showWeather(int i)
{
  int wd;
  if(i==0) { // temp
    printStringWithShift("  Temp:  ", scrollDel, font, ' ');
    dtostrf(temp,2,1,txt);
    strcpy(txt2,"`C");
    int wd1 = stringWidth(txt,digits5x7,' ')+1;
    int wd2 = stringWidth(txt2,font,' ');
    int wd = wd1+wd2+1;
    wd1 = (NUM_MAX*8 - wd)/2;
    wd2 = NUM_MAX*8 - wd1 - wd;
    while(wd1-->0) printCharWithShift('_', scrollDel, font, ' ');
    printStringWithShift(txt, scrollDel, digits5x7, ' ');
    printStringWithShift(txt2, scrollDel, font, ' ');
    while(wd2-->0) printCharWithShift('_', scrollDel, font, ' ');
    if(temp==temp_min && temp==temp_max) cnt++;
  } else
  if(i==1) { // temp min/max
    printStringWithShift("   Min/Max:  ", scrollDel, font, ' ');
    dtostrf(temp_min,2,(temp_min<=-10) ? 0 : 1,txt);
    String str = String(txt);
    dtostrf(temp_max,2,(temp_max<=-10) ? 0 : 1,txt);
    str+=((temp_max>=20)?"`.. ":"`... ")+String(txt)+"`";
    printStringCenter(str, scrollDel, font, ' ');
  } else
  if(i==2) { // description
    printStringWithShift("  ", scrollDel, font, ' ');
    printStringCenter(weather_info, scrollDel, font, ' ');
  } else
  if(i==3) { // humidity/clouds
    printStringWithShift("   Humidity/Clouds:  ", scrollDel, font, ' ');
    //printStringWithShift("   Wilgotność/Zachmurzenie:  ", scrollDel, font, ' ');
    if(humidity=="100") humidity="99"; // nasty patch
    if(clouds=="100") clouds="99";
    sprintf(txt,"H:%s%%",humidity.c_str());
    sprintf(txt2,"C:%s%%",clouds.c_str());
    int wd = NUM_MAX*8-stringWidth(txt,font,' ')-stringWidth(txt2,font,' ')-1;
    printStringWithShift(txt, scrollDel, font, ' ');
    while(wd-->0) printCharWithShift('_', scrollDel, font, ' ');
    printStringWithShift(txt2, scrollDel, font, ' ');
  } else
  if(i==4) {
    printStringWithShift("   Wind:  ", scrollDel, font, ' ');
    //printStringWithShift("   Wiatr:  ", scrollDel, font, ' ');
    strcpy(txt2," m/s");
    int wd1 = stringWidth(wind_speed,digits5x7,' ')+1;
    int wd2 = stringWidth(txt2,font,' ');
    int wd = wd1+wd2+1;
    wd1 = (NUM_MAX*8 - wd)/2;
    wd2 = NUM_MAX*8 - wd1 - wd;
    while(wd1-->0) printCharWithShift('_', scrollDel, font, ' ');
    printStringWithShift(wind_speed, scrollDel, digits5x7, ' ');
    printStringWithShift(txt2, scrollDel, font, ' ');
    while(wd2-->0) printCharWithShift('_', scrollDel, font, ' ');
  }
}

void showAQI(String name)
{
  if(aqi<=0) return;
  printStringWithShift("  AQI PM2.5 "+name+" "+String(aqiH)+":00  ", scrollDel, font, ' ');
  printStringCenter(String(aqi)+" - "+100*aqi/25+"%", scrollDel, font, ' ');
}

void showCurrency()
{
  printStringWithShift("  USD/PLN:  ", scrollDel, font, ' ');
  int wd1 = stringWidth(currencyBuy,digits3x7,' ');
  int wd2 = stringWidth(currencySell,digits3x7,' ');
  int wd = NUM_MAX*8-wd1-wd2-5;
  //Serial.println(wd); Serial.println(wd1); Serial.println(wd2);
  //Serial.println(currencyBuy); Serial.println(currencySell); Serial.println(currencyDir);
  wd1 = wd/2;
  wd2 = wd-wd1;
  printStringWithShift(currencyBuy, scrollDel, digits3x7, ' '); // buy
  while(wd1-->0) printCharWithShift(' ', scrollDel, digits3x7, ' ');
  printStringWithShift(currencyDir, scrollDel, digits3x7, ' '); // arrow
  while(wd2-->0) printCharWithShift(' ', scrollDel, digits3x7, ' ');
  printStringWithShift(currencySell, scrollDel, digits3x7, ' '); // sell
}

// =======================================================================

// =======================================================================

void display()
{
  switch(cnt) {
    case 0: showTime(); break;
    case 1: getTimeAndSubs(0); getTimeAndSubs(1); break;
    case 2: showSubs(0); break;
    case 3: showSubsGain(0); break;
    case 4: showViews(0); break;
    case 5: showViewsGain(0); break;
    case 6: getWeatherData(); break;
    case 7: showWeather(0); break;
    case 8: showWeather(1); break;
    case 9: showWeather(2); break;
    case 10: showWeather(3); break;
    case 11: showWeather(4); break;
    //case 12: getSmog(smogStation1); showAQI("Warszawa"); break;
    case 12: getAQI(aqiStation1); showAQI("Warszawa"); break;
    case 13: getCurrencyRates(); break;
    case 14: showCurrency(); break;
  }
}

// =======================================================================

int printVal(int val, int i)
{
  int r0 = val / 100;
  int r1 = val / 10 - (val / 100) * 10;
  int r2 = val % 10;
  if (r0) {
    showDigit(r0,  i, dig3x5);
    i += (r0 == 1) ? 3 : 4;
  }
  if (r1 || r0) {
    showDigit(r1,  i, dig3x5);
    i += (r1 == 1) ? 3 : 4;
  }
  showDigit(r2, i, dig3x5);
  i += (r2 == 1) ? 3 : 4;
  return i;
}

// =======================================================================

void drawProgress()
{
  for (int i = 0; i < NUM_MAX * 8; i++) {
    /*
    // old style
    byte val = i & 1 ? 0x80 : 0;
    if (i < elapsed * NUM_MAX * 8 / 100) val = 0xc0;
    scr[i] = (scr[i] >> 3) | val;
    */
    byte val = 0;
    if(i==5 || i==17 || i==29 || i==41) val = 0x80;
    if(i==0 || i==11 || i==23 || i==35 || i==47) val = 0xc0;
    if (i < elapsed * NUM_MAX * 8 / 100) val |= 0x80;
    scr[i] = (scr[i] >> 3) | val;
  }
}

// =======================================================================

void showElapsed()
{
  unsigned int total = totalH * 3600 + totalM * 60 + totalS;
  unsigned int elaps = elapsedH * 3600 + elapsedM * 60 + elapsedS;
  unsigned int remain = (total - elaps + 30) / 60;
  total = (total + 30) / 60;
  elaps = elaps / 60;
  int x;
  
  dx = dy = 0;
  clr();
  if (total == 0) {    // stop
    scr[0] = B01110000;
    scr[1] = B01110000;
    scr[2] = B01110000;
  } else if (state == 1) {    // play
    scr[0] = B11111000;
    scr[1] = B01110000;
    scr[2] = B00100000;
  } else if (state > 1) {    // ff
    scr[0] = B10001000;
    scr[1] = B01010000;
    scr[2] = B00100000;
  } else if (state < 0) {    // rew
    scr[0] = B00100000;
    scr[1] = B01010000;
    scr[2] = B10001000;
  } else if (state == 0) {    // pause
    scr[0] = B01110000;
    scr[1] = B00000000;
    scr[2] = B01110000;
  }
  if (total>0)  {
    x = remain>=200 ? 4 : 5;
    //x=printVal(remain,x);
    x=printVal(elaps,x);
    scr[x++] = B00100000;
    scr[x++] = B00100000;
    x++;
    //x=printVal(total,x);
    x=printVal(remain,x);
  }
  x = 31; // clock
  if (h / 10)
    showDigit(h / 10, h/10==1 ? x+1 : x, dig3x5);
  showDigit(h % 10, x+4, dig3x5);
  showDigit(m / 10, x+10, dig3x5);
  showDigit(m % 10, x+14, dig3x5);
  setCol(x+8, B01010000);

  drawProgress();
  refreshAll();
}

// =======================================================================

void showDigit(char ch, int col, const uint8_t *data)
{
  if (dy < -8 | dy > 8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if (col + i >= 0 && col + i < 8 * NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if (!dy) scr[col + i] = v; else scr[col + i] |= dy > 0 ? v >> dy : v << -dy;
    }
}

// =======================================================================

void setCol(int col, byte v)
{
  if (dy < -8 | dy > 8) return;
  col += dx;
  if (col >= 0 && col < 8 * NUM_MAX)
      if (!dy) scr[col] = v; else scr[col] |= dy > 0 ? v >> dy : v << -dy;
}

// =======================================================================
int dualChar = 0;

unsigned char convertPolish(unsigned char _c)
{
  unsigned char c = _c;
  if (c == 196 || c == 197 || c == 195) {
    dualChar = c;
    return 0;
  }
  if (dualChar) {
    switch (_c) {
      case 133: c = 1 + '~'; break; // 'ą'
      case 135: c = 2 + '~'; break; // 'ć'
      case 153: c = 3 + '~'; break; // 'ę'
      case 130: c = 4 + '~'; break; // 'ł'
      case 132: c = dualChar == 197 ? 5 + '~' : 10 + '~'; break; // 'ń' and 'Ą'
      case 179: c = 6 + '~'; break; // 'ó'
      case 155: c = 7 + '~'; break; // 'ś'
      case 186: c = 8 + '~'; break; // 'ź'
      case 188: c = 9 + '~'; break; // 'ż'
      //case 132: c = 10+'~'; break; // 'Ą'
      case 134: c = 11 + '~'; break; // 'Ć'
      case 152: c = 12 + '~'; break; // 'Ę'
      case 129: c = 13 + '~'; break; // 'Ł'
      case 131: c = 14 + '~'; break; // 'Ń'
      case 147: c = 15 + '~'; break; // 'Ó'
      case 154: c = 16 + '~'; break; // 'Ś'
      case 185: c = 17 + '~'; break; // 'Ź'
      case 187: c = 18 + '~'; break; // 'Ż'
      default:  break;
    }
    dualChar = 0;
    return c;
  }
  switch (_c) {
    case 185: c = 1 + '~'; break;
    case 230: c = 2 + '~'; break;
    case 234: c = 3 + '~'; break;
    case 179: c = 4 + '~'; break;
    case 241: c = 5 + '~'; break;
    case 243: c = 6 + '~'; break;
    case 156: c = 7 + '~'; break;
    case 159: c = 8 + '~'; break;
    case 191: c = 9 + '~'; break;
    case 165: c = 10 + '~'; break;
    case 198: c = 11 + '~'; break;
    case 202: c = 12 + '~'; break;
    case 163: c = 13 + '~'; break;
    case 209: c = 14 + '~'; break;
    case 211: c = 15 + '~'; break;
    case 140: c = 16 + '~'; break;
    case 143: c = 17 + '~'; break;
    case 175: c = 18 + '~'; break;
    default:  break;
  }
  return c;
}

// =======================================================================

int charWidth(const char ch, const uint8_t *data, int offs)
{
  char c = convertPolish(ch);
  if(c < offs || c > MAX_CHAR) return 0;
  c -= offs;
  int len = pgm_read_byte(data);
  return pgm_read_byte(data + 1 + c * len);
}

// =======================================================================

int stringWidth(const char *s, const uint8_t *data, int offs)
{
  int wd=0;
  while(*s) wd += 1+charWidth(*s++, data, offs);
  return wd-1;
}

int stringWidth(String str, const uint8_t *data, int offs)
{
  return stringWidth(str.c_str(), data, offs);
}

// =======================================================================
int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  scr[NUM_MAX*8] = 0;
  for (i = 0; i < w; i++)
    scr[NUM_MAX*8+i+1] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  return w;
}
// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay, const uint8_t *data, int offs) 
{
  c = convertPolish(c);
  if(c < offs || c > MAX_CHAR) return;
  c -= offs;
  int w = showChar(c, data);
  for (int i=0; i<w+1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================
//#define SPEED (strlen(s)+wdR<5 ? shiftDelay*3/2 : shiftDelay)
//#define SPEED2 (strlen(s)<5 ? shiftDelay*3/2 : shiftDelay)
#define SPEED shiftDelay
#define SPEED2 shiftDelay

void printStringWithShift(const char *s, int shiftDelay, const uint8_t *data, int offs)
{
  while(*s) printCharWithShift(*s++, shiftDelay, data, offs);
}

void printStringWithShift(String str, int shiftDelay, const uint8_t *data, int offs)
{
  printStringWithShift(str.c_str(), shiftDelay, data, offs);
}

// =======================================================================

void printStringCenter(const char *s, int shiftDelay, const uint8_t *data, int offs)
{
  int wd = stringWidth(s, data, offs);
  int wdL = (NUM_MAX*8 - wd)/2;
  int wdR = NUM_MAX*8 - wdL - wd;
  while(wdL>0) { printCharWithShift('_', shiftDelay, data, ' '); wdL--; }
  while(*s) printCharWithShift(*s++, SPEED, data, offs);
  while(wdR>0) { printCharWithShift('_', SPEED, data, ' '); wdR--; }
}

void printStringCenter(String str, int shiftDelay, const uint8_t *data, int offs)
{
  printStringCenter(str.c_str(), shiftDelay, data, offs);
}

// =======================================================================
// converts int to string
// centers string on the display
// chooses proper font for string/number length
void printValueWithShift(long val, int shiftDelay, int sign)
{
  // for 4*8=32
  //const uint8_t *digits = digits5x7;       // good for max 5 digits = 5*6=30 (99 999)
  //if(val>1999999) digits = digits3x7;      // good for max 8 digits = 8*4=32
  //else if(val>99999) digits = digits4x7;   // good for max 6-7 digits = 6*5+2=32 (1 999 999)

  // for 6*8=48 no gaps
  //const uint8_t *digits = digits5x7;         // good for max 8 digits = 8d*6p=48 (99 999 999)
  //if(val>1999999999) digits = digits3x7;     // good for max 12 digits = 12d*4p=48
  //else if(val>99999999) digits = digits4x7;  // good for max 9-10 digits = 9d*5p+2p=47 (1999 999 999)

  // for 6*8=48 with gaps
  const uint8_t *digits = digits5x7;         // good for max 7-8 digits = 18+1+18+1+6+3=47 (19 999 999)
  if(val>999999999) digits = digits3x7;      // good for max 11 digits = 12+1+12+1+12+1+8=47 (99 999 999 999>max32b!)
  else if(val>19999999) digits = digits4x7;  // good for max 9 digits = 15+1+15+1+15=47 (999 999 999)
  int sg = (val>0) ? 1 : -1;
  String str = String((sg<0) ? -val : val);
  int len = str.length();
  int gaps=1;
  if(len>4 && gaps) {
    int st = len-(len/3)*3;
    String copy = str;
    str="";
    for(int i=st;i<=len;i+=3) { str+=copy.substring((i-3<0)?0:i-3,i); if(i<len && i>0) str+="&"; }
  }
  if(sign) {
    str=((sg<0)?"-":"+")+str;
  }
  const char *s = str.c_str();
  int wd = 0;
  while(*s) wd += 1+charWidth(*s++, digits,' ');
  wd--;
  int wdL = (NUM_MAX*8 - wd)/2;
  int wdR = NUM_MAX*8 - wdL - wd;
  //Serial.println(wd); Serial.println(wdL); Serial.println(wdR);
  s = str.c_str();
  while(wdL>0) { printCharWithShift(' ', shiftDelay, digits, ' '); wdL--; }
  while(*s) printCharWithShift(*s++, SPEED, digits, ' ');
  while(wdR>0) { printCharWithShift(' ', SPEED, digits, ' '); wdR--; }
}

// =======================================================================
int strS=0,strE=0,i;
// adds " char to token end
// uses global buf to avoid frequent memory reallocation
String find(String token, const char endCh=',', int ofs=0)
{
  strS = buf.indexOf(token+"\"");
  if(strS<0) return "";
  if(buf.length()-strS<token.length()+2+ofs) return "";
  strS+=token.length()+2+ofs;
  for(i=strS;i<buf.length();i++)
    if(buf[i]==endCh || buf[i]=='}' || buf[i]==']') break;
  strE = i;
  return buf.substring(strS,strE);
}

// adds " char to token and subtoken end
String findSub(String token, String subToken)
{
  strS = buf.indexOf(token+"\"");
  if(strS<0) return "";
  if(buf.length()-strS<token.length()+2) return "";
  strS+=token.length()+2;
  int strS2 = buf.indexOf(subToken+"\"",strS);
  if(strS2<0) return "";
  if(buf.length()-strS2<subToken.length()+2) return "";
  strS2+=subToken.length()+2;
  for(i=strS2;i<buf.length();i++)
    if(buf[i]==',' || buf[i]=='}' || buf[i]==']') break;
  strE = i;
  return buf.substring(strS2,strE);
}
// =======================================================================
// =======================================================================

int getKodi(char *kodiIP)
{
  Serial.print("getting data from "); Serial.println(kodiIP);
  if(!client.connect(kodiIP, 8080)) {
    Serial.println("connection to kodi failed");
    return 0;
  }
  client.println(String("GET /jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetProperties%22,%22params%22:{%22playerid%22:1,%22properties%22:[%22time%22,%22totaltime%22,%22percentage%22,%22speed%22]},%22id%22:%221%22}%20}\r\n") +
                 "Host: " + kodiIP + "\r\nUser-Agent: ArduinoWiFi/1.1\r\nConnection: close\r\n\r\n");
  buf="";
  int num=0;
  unsigned long startTime = millis();
  while(client.connected()) {
    if(!client.available()) {
      Serial.println("Waiting for data ...");
      if((num>0 && millis()-startTime>500) || (num==0 && millis()-startTime>3000)) {
        Serial.print("Timeout. Received bytes: "); Serial.println(num);
        break;
      }
      delay(50);
    } else {
      startTime = millis();
      buf += (char)client.read();
      num++;
    }
  }
  client.stop();
  //Serial.println(buf);

  state    = findSub("result","speed").toInt();
  elapsed  = findSub("result","percentage").toFloat();
  elapsedH = findSub(String("\"")+"time","hours").toInt();
  elapsedM = findSub(String("\"")+"time","minutes").toInt();
  elapsedS = findSub(String("\"")+"time",String("\"")+"seconds").toInt();
  totalH   = findSub("totaltime","hours").toInt();
  totalM   = findSub("totaltime","minutes").toInt();
  totalS   = findSub("totaltime",String("\"")+"seconds").toInt();

  //Serial.println("Elapsed: "+String(elapsed)+" Time: "+String(elapsedH)+":"+String(elapsedM)+" TotalTime: "+String(totalH)+":"+String(totalM)+" speed:"+String(state));
  return 1;
}

// =======================================================================
// example: Thu, 19 Nov 2015
// decodes: day, month(1..12), dayOfWeek(1-Mon,7-Sun), year
void decodeDate(String date)
{
  switch(date.charAt(0)) {
    case 'M': dayOfWeek=1; break;
    case 'T': dayOfWeek=(date.charAt(1)=='U')?2:4; break;
    case 'W': dayOfWeek=3; break;
    case 'F': dayOfWeek=5; break;
    case 'S': dayOfWeek=(date.charAt(1)=='A')?6:7; break;
  }
  int midx = 6;
  if(isdigit(date.charAt(midx))) midx++;
  midx++;
  switch(date.charAt(midx)) {
    case 'F': month = 2; break;
    case 'M': month = (date.charAt(midx+2)=='R') ? 3 : 5; break;
    case 'A': month = (date.charAt(midx+1)=='P') ? 4 : 8; break;
    case 'J': month = (date.charAt(midx+1)=='A') ? 1 : ((date.charAt(midx+2)=='N') ? 6 : 7); break;
    case 'S': month = 9; break;
    case 'O': month = 10; break;
    case 'N': month = 11; break;
    case 'D': month = 12; break;
  }
  day = date.substring(5, midx-1).toInt();
  year = date.substring(midx+4, midx+9).toInt();
  return;
}

// =======================================================================

const char *ytHost = "www.googleapis.com";

int getYTData(String channelID, int i)
{
  Serial.print("connecting to "); Serial.println(ytHost);
  if (!sclient.connect(ytHost, 443)) {
    Serial.println("connection failed");
    return -1;
  }
  String cmd = String("GET /youtube/v3/channels?part=statistics&id=") + channelID + "&key=" + ytApiV3Key+ " HTTP/1.1\r\n" +
               "Host: " + ytHost + "\r\nUser-Agent: ESP8266/1.1\r\nConnection: close\r\n\r\n";
  //Serial.println(cmd);
  sclient.print(cmd);

  buf="";
  int num=0;
  String line="";
  int startJson=0, dateFound=0;
  unsigned long startTime = millis();
  while(sclient.connected()) {
    if(!sclient.available()) {
      Serial.println("Waiting for data ...");
      if((num>0 && millis()-startTime>500) || (num==0 && millis()-startTime>3000)) {
        Serial.print("Timeout. Received bytes: "); Serial.println(num);
        break;
      }
      delay(50);
    } else {
      startTime = millis();
      line = sclient.readStringUntil('\n');
      if(line[0]=='{') startJson=1;
      if(startJson) buf+=line+"\n";
      // Date: Thu, 19 Nov 2015 20:25:40 GMT
      if(!dateFound && line.startsWith("Date: ")) {
        localMillisAtUpdate = millis();
        dateFound = 1;
        date = line.substring(6, 22);
        date.toUpperCase();
        decodeDate(date);
        //Serial.println(line);
        h = line.substring(23, 25).toInt();
        m = line.substring(26, 28).toInt();
        s = line.substring(29, 31).toInt();
        summerTime = checkSummerTime();
        if(h+utcOffset+summerTime>23) {
          if(++day>31) { day=1; month++; };  // needs better patch
          if(++dayOfWeek>7) dayOfWeek=1; 
        }
        Serial.println(String(h) + ":" + String(m) + ":" + String(s)+"   Date: "+day+"."+month+"."+year+" ["+dayOfWeek+"] "+(utcOffset+summerTime)+"h");
        localEpoc = h * 60 * 60 + m * 60 + s;
      }
    }
  }
  sclient.stop();
  //Serial.println(buf);

  viewCount[i]       = find("viewCount",'\"',2).toInt();
  subscriberCount[i] = find("subscriberCount",'\"',2).toInt();
  //videoCount[i]      = find("videoCount",'\"',2).toInt();
  //Serial.println(String("viewCount=")+viewCount[i]);
  //Serial.println(String("subscriberCount=")+subscriberCount[i]);
  //Serial.println(String("videoCount=")+videoCount[i]);
  return 0;
}

// =======================================================================

int checkSummerTime()
{
  if(month>3 && month<10) return 1;
  if(month==3 && day>=31-(((5*year/4)+4)%7) ) return 1;
  if(month==10 && day<31-(((5*year/4)+1)%7) ) return 1;
  return 0;
}
// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * (utcOffset+summerTime) + 86400L) % 86400L;
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

// =======================================================================

const char *weatherHost = "api.openweathermap.org";

void getWeatherData()
{
  Serial.print("connecting to "); Serial.println(weatherHost);
  if(!client.connect(weatherHost, 80)) {
    Serial.println("connection failed");
    return;
  }
  client.println(String("GET /data/2.5/weather?id=") + cityID + "&units=metric&appid=" + weatherKey + weatherLang + "\r\n" +
                 "Host: " + weatherHost + "\r\nUser-Agent: ArduinoWiFi/1.1\r\nConnection: close\r\n\r\n");
  buf="";
  int num=0;
  unsigned long startTime = millis();
  while(client.connected()) {
    if(!client.available()) {
      Serial.println("Waiting for data ...");
      if((num>0 && millis()-startTime>500) || (num==0 && millis()-startTime>3000)) {
        Serial.print("Timeout. Received bytes: "); Serial.println(num);
        break;
      }
      delay(50);
    } else {
      startTime = millis();
      buf += (char)client.read();
      num++;
    }
  }
  client.stop();
  //Serial.println(buf);

  weather_info = find("description",'\"',1);
  temp       = findSub("main","temp").toFloat();
  temp_min   = findSub("main","temp_min").toFloat();
  temp_max   = findSub("main","temp_min").toFloat();
  pressure   = findSub("main","pressure").toInt();
  wind_speed = findSub("wind","speed");
  humidity   = findSub("main","humidity");
  clouds     = findSub("clouds","all");
  
  Serial.println(String("temp=")+temp);
  Serial.println(String("temp_min=")+temp_min);
  Serial.println(String("temp_max=")+temp_max);
  Serial.println(String("pressure=")+pressure);
  Serial.println(String("humidity=")+humidity);
  Serial.println(String("wind_speed=")+wind_speed);
  Serial.println(String("clouds=")+clouds);
  Serial.println(String("info=")+weather_info);
  Serial.println("---------------");
}

// =======================================================================

const char* currencyHost = "cinkciarz.pl";

void getCurrencyRates()
{
  currencySell = currencyBuy = "0.00";
  currencyDir = "-";
  Serial.print("connecting to "); Serial.println(currencyHost);
  if (!sclient.connect(currencyHost, 443)) {
    Serial.println("connection failed");
    return;
  }
  sclient.print(String("GET / HTTP/1.1\r\n") + "Host: " + currencyHost + "\r\nConnection: close\r\n\r\n");

  //Serial.print("request sent");
  int repeatCounter = 10;
  while (!sclient.available() && repeatCounter--) {
    delay(200); Serial.println("c.");
  }
  Serial.println("connected");
  buf="";
  while(sclient.connected() && sclient.available()) {
    buf = sclient.readStringUntil('\n');
    //Serial.println(buf.length()); Serial.println(buf);
    yield();
    int currIdx = buf.indexOf("/kantor/kursy-walut-cinkciarz-pl/usd");
    if (currIdx > 0) {
      //Serial.println(line);
      //String curr = buf.substring(currIdx + 33, currIdx + 33 + 3);
      //curr.toUpperCase();
      buf = sclient.readStringUntil('\n');
      //Serial.println(buf);
      int rateIdx = buf.indexOf("\">");
      if (rateIdx <= 0) {
        Serial.println("Found rate but wrong structure!");
        return;
      }
      if (buf[rateIdx - 1] == 'n') currencyDir = "$"; else currencyDir = "#"; // down/up
      currencyBuy = buf.substring(rateIdx + 2, rateIdx + 8);

      buf = sclient.readStringUntil('\n');
      //Serial.println(buf);
      rateIdx = buf.indexOf("\">");
      if (rateIdx <= 0) {
        Serial.println("Found rate but wrong structure!");
        return;
      }
      currencySell = buf.substring(rateIdx + 2, rateIdx + 8);
      currencyBuy.replace(',', '.');
      currencySell.replace(',', '.');
      break;
    }
  }
  sclient.stop();
  Serial.println(currencyBuy); Serial.println(currencySell);
}

// =======================================================================
// https://aqicn.org/json-api/doc/

String aqiHost = "api.waqi.info";

void getAQI(String station)
{
  aqi = 0;
  Serial.print("connecting to "); Serial.println(aqiHost);
  if (!client.connect(aqiHost.c_str(), 80)) {
    Serial.println("connection failed");
    return;
  }
  client.print(String("GET /feed/") + station + "/?token=" + aqiToken + " HTTP/1.1\r\n" + "Host: " + aqiHost + "\r\nConnection: close\r\n\r\n");

  buf="";
  int num=0;
  unsigned long startTime = millis();
  while(client.connected()) {
    if(!client.available()) {
      Serial.println("Waiting for data ...");
      if((num>0 && millis()-startTime>500) || (num==0 && millis()-startTime>4000)) {
        Serial.print("Timeout. Received bytes: "); Serial.println(num);
        break;
      }
      delay(50);
    } else {
      startTime = millis();
      buf += (char)client.read();
      num++;
    }
  }
  client.stop();
  //Serial.println(buf);

  aqi = find("pm25\":{\"v").toInt();
  aqiH = find("\"s",':',12).toInt();
  //aqiTz = find("tz",':',2).toInt();
  
  Serial.println(aqi); Serial.println(aqiH); //Serial.println(aqiTz);
}

// =======================================================================
/*
 * http://api.smog.info.pl/cities/warszawa/stations
 * http://api.smog.info.pl/cities/warszawa/stations/niepodleglosci/measurements
 * http://api.smog.info.pl/cities/piastow/stations/pulaskiego/measurements
 * 
 */
 
String smogHost = "api.smog.info.pl";

void getSmog(String station)
{
  aqi = -1;
  aqiH = -1;
  Serial.print("connecting to "); Serial.println(smogHost);
  if (!client.connect(smogHost.c_str(), 80)) {
    Serial.println("connection failed");
    return;
  }
  client.print(String("GET /") + station + " HTTP/1.1\r\n" + "Host: " + smogHost + "\r\nConnection: close\r\n\r\n");

  buf="";
  int num=0;
  unsigned long startTime = millis();
  while(client.connected()) {
    if(!client.available()) {
      Serial.println("Waiting for data ...");
      if((num>0 && millis()-startTime>500) || (num==0 && millis()-startTime>3000)) {
        Serial.print("Timeout. Received bytes: "); Serial.println(num);
        break;
      }
      delay(50);
    } else {
      buf = client.readStringUntil('\n');
      if(buf.indexOf("pm25")>=0)
        aqi = find("pm25",',',1).toInt();
      else
      if(buf.indexOf("measuredAt")>=0)
        aqiH = find("measuredAt",':',13).toInt();
      if(aqi>=0 && aqiH>=0) break;
      num++;
      startTime = millis();
    }
  }
  client.stop();
  //Serial.println(buf);
 
  Serial.println(aqi); Serial.println(aqiH);
}

// =======================================================================

void setup() {
  buf.reserve(500);
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 0);
  Serial.print("Connecting to WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift("Connecting", 15, font, ' ');
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("MyIP: "); Serial.println(WiFi.localIP());
  printStringWithShift((String("  MyIP: ")+WiFi.localIP().toString()).c_str(), 15, font, ' ');
  getTimeAndSubs(0);
}

// =======================================================================

void loop()
{
  unsigned long startTime = millis(), delTime;
  int ret1 = 0, ret2 = 0;
  //ret1 = getKodi("192.168.1.129"); // tv
  ret2=getKodi("192.168.1.99"); // projector
  //ret1=1;
  kodiOK = (!ret1 && !ret2) ? 0 : 1;
  updateTime();
  if(kodiOK) {
    sendCmdAll(CMD_INTENSITY, 0);
    showElapsed();
    delay(500);
  } else {
    sendCmdAll(CMD_INTENSITY, (h>=7&&h<=16)?1:0);
    display();
    cnt++;
    if(cnt>LAST_DISP) cnt=0;
    delTime = millis()-startTime;
    if(delTime<5000) delay(5000-delTime);
  }
}