#include <Arduino.h>
#include "config.h"
#include "big_clock_html.h"

/*//Interrupt timer
void onTimerISR(){
    x++;
    if(x=1000){
      IncrementaCronometro(); 
      x=0;
      timer1_write(600000);//1ms
    }
}*/

////////////////////////////////////////////////////////////////////////////
//Setup
///////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.setBrightness(brilho);
  fill_solid(leds, NUM_LEDS, ColorFromPalette(RGB_colors, index_color));
  FastLED.show();
  
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(SSID_1, PASS_1);
  wifiMulti.addAP(SSID_2, PASS_2);
  wifiMulti.addAP(SSID_3, PASS_3);

  Serial.println("Connecting Wifi...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(200);
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  timeClient.update();

  server.on("/", htmlRelogio);
  server.on("/cron", htmlCronometro);
  server.on("/cron/play", htmlIniciaCronometro); 
  server.on("/cron/reset", htmlZeraCronometro);
  server.on("/timer", htmlTimer);
  server.begin();                  //Start server
}

////////////////////////////////////////////////////////////////////////////
//Main
///////////////////////////////////////////////////////////////////////////
void loop() {
  server.handleClient();
  switch(func){
    case 0:
      relogio();
      break;
    case 1:
      cronometro();
      break;
    case 2:
      timer();
  }
}

////////////////////////////////////////////////////////////////////////////
//Funções
///////////////////////////////////////////////////////////////////////////
void htmlRelogio(){
  func=0;  
  server.send(200, "text/html", RELOGIO_page); //Send web page
}

void htmlCronometro(){ 
  func=1;  
  server.send(200, "text/html", CRONOMETRO_page); //Send web page
}

void htmlTimer(){ 
  htmlTimer2();
  //server.send(200, "text/html", TIMER_page); //Send web page
}

void htmlIniciaCronometro() {
  if(play_cronometro==2||play_cronometro==0)play_cronometro=1;
  else play_cronometro=2;
  server.send(200, "text/html", CRONOMETRO_page); //Send web page
  //server.send(200, "text/plain", "INICIOU OU PAUSOUUUU"); //Send ADC value only to client ajax request
}
void htmlZeraCronometro() {
  play_cronometro=0;
  server.send(200, "text/html", CRONOMETRO_page); //Send web page
  //server.send(200, "text/plain", "ZEROUUUUU"); //Send ADC value only to client ajax request
}

void relogio(void) {
  currentMillis = millis();//Tempo atual em ms
  //Lógica de verificação do tempo
  if (currentMillis - previousMillis > 500) {
    previousMillis = currentMillis;    // Salva o tempo atual
    ponto=!ponto;
    MostrarPonto(ponto);
    if(ponto){
      horas = timeClient.getHours();
      minutos = timeClient.getMinutes();
      segundos = timeClient.getSeconds();
      MostraHoras();
    }
  }
}

void cronometro(void){
  server.handleClient();
  
  //while(play_cronometro==-1){server.handleClient();}//espera apertar play ou zerar////////acrescentar depois
  if(play_cronometro==0){
    //segundos_cron=0;
    minutos_cron=-1; // pra começar com 0
    ZeraDisplays();
    while(play_cronometro==0){server.handleClient();if(func!=1)return;}//espera apertar play
    segundos_aux = timeClient.getSeconds();
  }
  currentMillis = millis();//Tempo atual em ms
  if (currentMillis - previousMillis > 500){
    previousMillis = currentMillis;
    ponto=!ponto;
    MostrarPonto(ponto);
    if(ponto){
      segundos_cron = timeClient.getSeconds()-segundos_aux;
      Serial.println(segundos_cron);
      if(segundos_cron<0) segundos_cron=segundos_cron+60;
      if(segundos_cron==0)minutos_cron++;
      MostraCronometro();
    }
  }
  if(play_cronometro==2){
    MostrarPonto(1);
    MostraCronometro(); //para o caso de voltar da func relógio
    while(play_cronometro==2){server.handleClient();if(func!=1)return;} //se pausar, espera retomar ou zerar
    if(segundos_cron<0)segundos_cron=segundos_cron-60;
    segundos_aux=timeClient.getSeconds()-segundos_cron; //necessário para retomar onde parou
    //if(segundos_aux2<0) segundos_aux2=segundos_aux2+60;
  }
}

void timer(){
  minutos_timer=timer_min; // minutos_cron=0; //?
  segundos_timer=timer_seg+1;
  while (!(minutos_timer==0 & segundos_timer==0)){
    server.handleClient();
    currentMillis = millis();//Tempo atual em ms
    if (currentMillis - previousMillis > 500){
      previousMillis = currentMillis;
      ponto=!ponto;
      MostrarPonto(ponto);
      if(ponto){
        if(segundos_timer==0)minutos_timer--;
        segundos_timer--;
        if(segundos_timer<0) segundos_timer=59;
        MostraTimer();
      }
    }
  }
  segundos_timer=0;
  minutos_timer=0;
  ZeraDisplays();
  while(func==2){server.handleClient();}
}

void MostraHoras(){
  MostrarAlgarismo(minutos%10,3); //mostra unidade de minutos
  MostrarAlgarismo(minutos/10,2); //mostra dezena de minutos
  MostrarAlgarismo(horas%10,1);  //mostra unidade de horas
  MostrarAlgarismo(horas/10,0);  //mostra dezenas de horas
}

void MostraCronometro(){
  MostrarAlgarismo(segundos_cron%10,3); //mostra unidade de segundos
  MostrarAlgarismo(segundos_cron/10,2); //mostra dezena de segundos
  MostrarAlgarismo(minutos_cron%10,1);  //mostra unidade de minutos
  MostrarAlgarismo(minutos_cron/10,0);  //mostra dezenas de minutos
}

void MostraTimer(){
  MostrarAlgarismo(segundos_timer%10,3); //mostra unidade de segundos
  MostrarAlgarismo(segundos_timer/10,2); //mostra dezena de segundos
  MostrarAlgarismo(minutos_timer%10,1);  //mostra unidade de minutos
  MostrarAlgarismo(minutos_timer/10,0);  //mostra dezenas de minutos
}

// Mostra um determinado numero no display (algarismo, posição do display)
void MostrarAlgarismo(char alg,char disp){
  for(int i=0;i<14;i++){
    if(a[alg][i]!=nulo){
      leds[dis[disp]+i]= ColorFromPalette(RGB_colors, index_color);
    }
    else{
      leds[dis[disp]+i]= CRGB(0,0,0);
    }
  }
  FastLED.show();
}

//Mostra ou apaga pontos que dividem no meio
void MostrarPonto(bool on){
  if (on==1){
    leds[28] = ColorFromPalette(RGB_colors, index_color);
    leds[29] = ColorFromPalette(RGB_colors, index_color);
  }
  else{
    leds[28] = CRGB(0,0,0);
    leds[29] = CRGB(0,0,0);
  }
  FastLED.show();
}

void MostraData()
{
  String data;
  int dia1,dia2,mes1,mes2;
  
  data = timeClient.getFormattedDate();

  dia2=atoi(data.substring(9, 10).c_str());
  dia1=atoi(data.substring(8, 9).c_str());
  mes2=atoi(data.substring(6, 7).c_str());
  mes1=atoi(data.substring(5, 6).c_str());
  
  MostrarAlgarismo(mes2,3); //algarismo 2 do mes
  MostrarAlgarismo(mes1,2); //algarismo 1 do mes
  MostrarAlgarismo(dia2,1);  //algarismo 2 do dia
  MostrarAlgarismo(dia1,0);  //algarismo 1 do dia

  //Acende ponto de baixo (para diferenciar das horas)
  leds[28] = CRGB(0,0,0);
  leds[29] = ColorFromPalette(RGB_colors, index_color);
}

void ZeraDisplays(void){
  //zerar displays
  MostrarAlgarismo(0,3);
  MostrarAlgarismo(0,2);
  MostrarAlgarismo(0,1);
  MostrarAlgarismo(0,0);
  MostrarPonto(1);
}

void forceUpdate(void) {
  timeClient.forceUpdate();
}

// Desliga apenas 1 display
void desl_display(char disp){
  for(int i=dis[disp];i<(NUM_SEG+dis[disp]);i++){
    leds[i]= CRGB(0,0, 0);
  }
}

// Liga todos os leds
void desl_displays(void){
  for(int i=0;i<=NUM_LEDS;i++){
      leds[i]= ColorFromPalette(RGB_colors, index_color);
      FastLED.show();
      delay(100);
  }
}

#define REQ_BUF_SZ   60 // size of buffer used to capture HTTP requests
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string

/*boolean GetTime(byte *hr, byte *mn)
{
  boolean valid_time = false;  // hour and minute received flag
  char *str_part;              // pointer to part of the HTTP_req string
  char str_time[3] = {0};      // used to extract the times from HTTP_req string
  
  // get pointer to the beginning of hour data in string
  str_part = strstr(HTTP_req, "&h=");
  if (str_part != NULL) {
    // get the hour from the string
    str_time[0] = str_part[3];
    str_time[1] = str_part[4];
    *hr = atoi(str_time);
    // get pointer to the beginning of minute data in string
    str_part = strstr(HTTP_req, "&m=");
    if (str_part != NULL) {
      // get the minute from the string
      str_time[0] = str_part[3];
      str_time[1] = str_part[4];
      *mn = atoi(str_time);
      // got the hour and the minute
      valid_time = true;
    }
  }
}*/

void htmlTimer2(){
  bool flag=0;
  String Stimer_min= String(timer_min);
  //Stimer_min += (char)timer_min;
  server.sendContent("<!DOCTYPE html><html><style>.font {font-size: 100px;}</style><form class='font' action='/timer'><label class='font' for='minutos'>Minutos:</label><input class='font' type='text' name='minutos' value='"+Stimer_min+"'><br><br><input class='font' type='submit' value='Update'></form>");
  for (int i = 0; i < server.args(); i++) { // Só entra se tiver argumentos
    flag=1; //Indica que apertou o botão Update.
    String server_argName = server.argName(i);
    if ((server_argName=="minutos") & (server.arg("minutos")!="")){
      Stimer_min=server.arg("minutos");
      timer_min=Stimer_min.toInt();
    }
    /*else if ((server_argName=="ip") & (server.arg("ip")!="")){
      wifi_ip=server.arg("ip");
      wifi_ip.toCharArray(WIFI_IP,20); // Necessário, pois a entrada da função EEPROM.put deve ter tamanho da saída do get.
      EEPROM.put(20,WIFI_IP); EEPROM.commit(); //Armazena dados no flash
    }*/
  }
  if(flag==1){
    func=2; 
    server.sendContent("<meta http-equiv='refresh' content='0; URL=/timer' />"); // volta a pagina de monitoração em 0 s
    //html_monitor(); 
  }
}