#include <Servo.h>
#include <LiquidCrystal.h>

#define TRIG_SAI A0
#define ECHO_SAI A1

#define TRIG_ENT A2
#define ECHO_ENT A3

#define LED_AUTORIZADO 9
#define LED_NAO_AUTORIZADO 8

#define LED_R 10
#define LED_G 6

#define TMP_SENSOR A5
#define BTN_TEMP 7
#define BUZZER A4

Servo catraca;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int outputs[] = {TRIG_ENT, TRIG_SAI, LED_AUTORIZADO, LED_NAO_AUTORIZADO,
                       LED_R, LED_G, BUZZER};
const int inputs[] = {ECHO_ENT, ECHO_SAI, TMP_SENSOR, BTN_TEMP};

const int velocidadeSomMS = 343,
          quantMaxPessoas = 10,
          intervaloCatracaMS = 2000;

const float tempMinima = 32, tempMaxima = 38,
			distanciaCatraca = 0.3;

int statusDoSistema = 0,
    quantPessoas = 0;

float temperatura = 0;
  
long tempoEntradaAntiga = 0;

bool existeAlguemEntrada = 0, existeAlguemSaida = 0,
	 estadoBotaoAnterior = 0;

enum status {
  padrao,
  autorizado,
  naoAutorizado,
  lotado
};

/*  Monta simbolo do graus Celsius
*/
byte grau[8] = {
  B00100,
  B01010,
  B01010,
  B00100,
  B00000,
  B00000,
  B00000,
  B00000,
};

void setup() {
  lcd.createChar(0, grau);
  lcd.begin(2, 16);

  catraca.attach(13);

  for (byte i = 0; i < (sizeof(outputs) / sizeof(outputs[0])); i++) {
    pinMode(outputs[i], OUTPUT);
  }

  for (byte i = 0; i < (sizeof(inputs) / sizeof(inputs[0])); i++) {
    pinMode(inputs[i], INPUT);
  }
  
  estadoPadrao();
}

void loop() {
  realizaContagemPessoas();
  
  realizaTemperatura();
  
  int status = pegaStatus();

  switch (status) {
    case 0:
      estadoPadrao();
      break;
    case 1:
      estadoAutorizado();
      break;
    case 2:
      estadoNaoAutorizado();
      break;
    case 3:
      estadoLotado();
      break;
    default:
      estadoPadrao();
      break;
  }
  
  if (existeAlguemEntrada && status == autorizado) {
    atualizaTempoEntrada();
  } else if (millis() - tempoEntradaAntiga >= intervaloCatracaMS) {
    atualizaTempoEntrada();
    colocaStatusComoPadrao();
  }
  
  delay(150);
}

/*
   Temperatura
*/
void realizaTemperatura() {
  const bool estadoBotao = digitalRead(BTN_TEMP);
  
  if (estadoBotao != estadoBotaoAnterior && estadoBotao) {
	verificaTemperaturaGraus();
    const bool podeEntrar = estaAutorizadaEntrada();
    
    if(podeEntrar)
      colocaStatusComoAutorizado();
    else if (!estaLotado())
      colocaStatusComoNaoAutorizado();
  }
  
  estadoBotaoAnterior = estadoBotao;
}

void verificaTemperaturaGraus() {
  const float tmpValor = analogRead(TMP_SENSOR);
  temperatura = -1 * (((358 - tmpValor) * 165 / 338) - 125);
}


bool estaAutorizadaEntrada() {
  return (possuiTempCorreta() && !estaLotado());
}

bool possuiTempCorreta() {
  return (temperatura >= tempMinima && temperatura <= tempMaxima);
}

void atualizaTempoEntrada() {
	tempoEntradaAntiga = millis();
}

/*
   Contagem de Pessoas
*/

void realizaContagemPessoas() {
  fazGradiente();

  const float distanciaMetrosEntrada = pegaDistancia(ECHO_ENT, TRIG_ENT);
  const float distanciaMetrosSaida = pegaDistancia(ECHO_SAI, TRIG_SAI);

  contaQuantPessoas(distanciaMetrosEntrada, existeAlguemEntrada, 1);
  contaQuantPessoas(distanciaMetrosSaida, existeAlguemSaida, -1);

  if (estaLotado())
    colocaStatusComoLotado();
  else if (pegaStatus() != autorizado && 
           pegaStatus() != naoAutorizado)
    colocaStatusComoPadrao();
}

void contaQuantPessoas(float distanciaMetros, bool &existeAlguem, int incremento) {
  if (distanciaMetros <= distanciaCatraca && !existeAlguem) {
    incrementaQuantPessoas(incremento);
    existeAlguem = true;
  } else if (distanciaMetros > distanciaCatraca) {
    existeAlguem = false;
  }
}

float pegaDistancia(int echo, int trig) {
  digitalWrite(trig, 1);
  delayMicroseconds(10);
  digitalWrite(trig, 0);
  const float tempoSeg = pulseIn(echo, 1) * 0.000001 * 0.5;

  return tempoSeg * velocidadeSomMS;
}

bool estaLotado() {
  return (quantPessoas >= quantMaxPessoas);
}

void incrementaQuantPessoas(int incremento) {
  quantPessoas += incremento;
}

/*
    LED RGB
*/

void fazGradiente() {
  int intervaloVermelho = map(quantPessoas, 0, quantMaxPessoas, 0, 255);
  int intervaloVerde = map(quantPessoas, 0, quantMaxPessoas, 255, 0);

  int valorVermelho = constrain(intervaloVermelho, 0, 255);
  int valorVerde = constrain(intervaloVerde, 0, 255);

  analogWrite(LED_R, valorVermelho);
  analogWrite(LED_G, valorVerde);
}

/*
   Status
*/

int pegaStatus() {
  return statusDoSistema;
}

void colocaStatusComoPadrao() {
  statusDoSistema = padrao;
}

void colocaStatusComoAutorizado() {
  statusDoSistema = autorizado;
}

void colocaStatusComoNaoAutorizado() {
  statusDoSistema = naoAutorizado;
}

void colocaStatusComoLotado() {
  statusDoSistema = lotado;
}

/*
   Estado
*/

void estadoPadrao() {
  desligaLed(LED_AUTORIZADO);
  desligaLed(LED_NAO_AUTORIZADO);
  
  desligaBuzzer();
  
  fechaCatraca();
  
  escreveLCDTotalPessoas();
}

void estadoAutorizado() {
  ligaLed(LED_AUTORIZADO);
  desligaLed(LED_NAO_AUTORIZADO);
  
  desligaBuzzer();
  
  abreCatraca();
  
  escreveLCDBemVindo();
  escreveLCDTemperatura(temperatura);
}

void estadoNaoAutorizado() {
  desligaLed(LED_AUTORIZADO);
  ligaLed(LED_NAO_AUTORIZADO);
  
  ligaBuzzer();
  
  fechaCatraca();
  
  escreveLCDEntradaNaoAutorizada();
}

void estadoLotado() {
  desligaLed(LED_AUTORIZADO);
  desligaLed(LED_NAO_AUTORIZADO);
  
  desligaBuzzer();
  
  fechaCatraca();
  
  escreveLCDTotalPessoas();
  escreveLCDLotado();
}

/*
   LEDs
*/

void ligaLed(int led) {
  digitalWrite(led, 1);
}

void desligaLed(int led) {
  digitalWrite(led, 0);
}

/*
    Catraca
*/

void abreCatraca() {
  catraca.write(90);
}

void fechaCatraca() {
  catraca.write(0);
}

/*
    Buzzer
*/

void ligaBuzzer() {
  tone(BUZZER, 528);
}

void desligaBuzzer() {
  noTone(BUZZER);
}

/*
    Display LCD
*/

void escreveLCDTotalPessoas() {
  String totalPessoas = String(quantPessoas) + "/" + String(quantMaxPessoas);
  lcd.setCursor(0, 0);
  lcd.print(totalPessoas);

  for (int i = totalPessoas.length(); i < 16; i ++) {
    lcd.setCursor(i, 0);
    lcd.print("=");
  }
  
  for (int i = 0; i < 16; i ++) {
    lcd.setCursor(i, 1);
    lcd.print("=");
  }
}

void escreveLCDLotado() {
  lcd.setCursor(0, 1);
  lcd.print("==== Lotado ====");
}

void escreveLCDTemperatura(float temperatura) {
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(" ");
  lcd.print(temperatura);
  lcd.setCursor(11, 1);
  lcd.write(byte(0));
  lcd.print("C");
}

void escreveLCDBemVindo() {
  limpaLinhaLCD(1);
  lcd.setCursor(0, 0);
  lcd.print("Seja bem-vindo!!");
}

void escreveLCDEntradaNaoAutorizada() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Entrada");
  lcd.setCursor(1, 1);
  lcd.print("nao-autorizada");
}

void limpaLinhaLCD(int linha){
  for(int i = 0; i < 16; i++){
  	lcd.setCursor(i, linha);
  	lcd.print(" ");
  }
}