#include <WiFi.h>
#include <PubSubClient.h>

// ================= CONFIGURAÇÃO DO ROBÔ =================
#define ROBO_SAMUEL
// #define ROBO_FRANCIELE

// ================= ROBO SAMUEL =================
#ifdef ROBO_SAMUEL

const int IN1 = 13;
const int IN2 = 12;
const int IN3 = 14;
const int IN4 = 27;

const int ENB_DIREITA = 32;
const int ENA_ESQUERDA = 33;

const int SENSOR_IR1 = 35;
const int SENSOR_IR2 = 34;
const int buzzer = 26;

int velocidade = 130;
int velocidadeCurva = 180;

int tempoPassarIntersecao = 200;

int tempoFrenteAntesCurvaEsquerda = 300;
int tempoCurvaEsquerda = 400;

int tempoFrenteAntesCurvaDireita = 300;
int tempoCurvaDireita = 500;

int tempoCurvaVolta = 800;

#endif

// ================= ROBO FRANCIELE =================
#ifdef ROBO_FRANCIELE

const int IN1 = 12;
const int IN2 = 13;
const int IN3 = 27;
const int IN4 = 14;

const int ENB_DIREITA = 32;
const int ENA_ESQUERDA = 33;

const int SENSOR_IR1 = 35;
const int SENSOR_IR2 = 34;
const int buzzer = 26;

int velocidade = 100;
int velocidadeCurva = 100;

int tempoPassarIntersecao = 200;

int tempoFrenteAntesCurvaEsquerda = 300;
int tempoCurvaEsquerda = 400;

int tempoFrenteAntesCurvaDireita = 300;
int tempoCurvaDireita = 400;

int tempoCurvaVolta = 1000;

#endif

// ================= WIFI =================
const char* WIFI_SSID = "S20FE";
const char* WIFI_PASSWORD = "samuelsa";

// ================= MQTT =================
const char* MQTT_BROKER = "broker.emqx.io";
const int MQTT_PORT = 1883;

const char* MQTT_CLIENT_ID = "robo1_esp32_samuel";

const char* TOPICO_COMANDOS = "robos/robo1/comandos";
const char* TOPICO_STATUS = "robos/robo1/status";
const char* MENSAGEM_AGUARDANDO = "Samuel conectado e aguardando rota";

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ================= SENSORES =================
const int IR_LINHA = HIGH;

// ================= ROTA POR STRING =================
const int MAX_COMANDOS = 20;
String comandos[MAX_COMANDOS];

int totalComandos = 0;
int comandoAtual = 0;

String comandoAtualRota = "parar";

// ================= CONTROLE =================
bool rotaRecebida = false;
bool roboAtivo = false;
bool comandoPararRecebido = true;
bool jaExecutouNestaIntersecao = false;

unsigned long ultimoStatusAguardando = 0;
const unsigned long intervaloStatusAguardando = 3000;

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(ENB_DIREITA, OUTPUT);
  pinMode(ENA_ESQUERDA, OUTPUT);

  pinMode(SENSOR_IR1, INPUT);
  pinMode(SENSOR_IR2, INPUT);

  pinMode(buzzer, OUTPUT);

  parar();

  Serial.println("Robo seguidor de linha com MQTT iniciado");
  Serial.println("Conectando...");

  conectarWiFi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(callbackMQTT);

  conectarMQTT();

  pararTotal("Robo conectado. Aguardando rota MQTT.");
}

void loop() {
  if (!mqtt.connected()) {
    conectarMQTT();
  }

  mqtt.loop();

  if (comandoPararRecebido) {
    parar();
    publicarAguardandoRota();
    delay(20);
    return;
  }

  if (!rotaRecebida || !roboAtivo) {
    parar();
    publicarAguardandoRota();
    delay(20);
    return;
  }

  String comando = comandoAtualRota;
  comando.trim();
  comando.toLowerCase();

  if (comando == "parar") {
    pararTotal("Comando PARAR encontrado na rota. Robo parado.");
    return;
  }

  int leituraIR1 = digitalRead(SENSOR_IR1);
  int leituraIR2 = digitalRead(SENSOR_IR2);

  bool irNaLinha1 = leituraIR1 == IR_LINHA;
  bool irNaLinha2 = leituraIR2 == IR_LINHA;

  Serial.print("IR1: ");
  Serial.print(leituraIR1);

  Serial.print(" | IR1 linha: ");
  Serial.print(irNaLinha1 ? "SIM" : "NAO");

  Serial.print(" | IR2: ");
  Serial.print(leituraIR2);

  Serial.print(" | IR2 linha: ");
  Serial.print(irNaLinha2 ? "SIM" : "NAO");

  Serial.print(" | Comando atual: ");
  Serial.print(comando);

  Serial.print(" | Acao: ");

  if (irNaLinha1 && irNaLinha2) {
    if (!jaExecutouNestaIntersecao) {
      jaExecutouNestaIntersecao = true;

      parar();
      delay(80);

      publicarStatus("Intersecao detectada");

      apitarInicio();
      delay(100);
      apitarInicio();

      executarComando(comando);
      avancarParaProximoComando();

      delay(100);
    }
  }

  else {
    jaExecutouNestaIntersecao = false;

    if (irNaLinha1 && !irNaLinha2) {
      girarEsquerda(100);
      Serial.println("Corrigindo esquerda");
    }

    else if (!irNaLinha1 && irNaLinha2) {
      girarDireita(100);
      Serial.println("Corrigindo direita");
    }

    else {
      frente();
      delay(70);
      parar();
      delay(50);
      Serial.println("Linha perdida / seguindo em frente");
    }
  }
}

// ================= MQTT =================
void conectarWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.println("Conectando ao MQTT...");

    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println("MQTT conectado!");

      mqtt.subscribe(TOPICO_COMANDOS);

      publicarStatus(MENSAGEM_AGUARDANDO);

      publicarStatus("MQTT conectado no ESP32. Robo parado.");

      Serial.print("Inscrito no topico: ");
      Serial.println(TOPICO_COMANDOS);
    }

    else {
      Serial.print("Falha MQTT. Estado: ");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String mensagem = "";

  for (unsigned int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  mensagem.trim();
  mensagem.toLowerCase();

  Serial.print("Mensagem MQTT recebida: ");
  Serial.println(mensagem);

  if (mensagem == "parar" || mensagem.length() == 0) {
    pararTotal("Comando PARAR recebido via MQTT. Robo completamente parado.");
    return;
  }

  carregarRota(mensagem);
}

void publicarStatus(const char* mensagem) {
  if (mqtt.connected()) {
    mqtt.publish(TOPICO_STATUS, mensagem);
  }

  Serial.print("Status publicado: ");
  Serial.println(mensagem);
}

void publicarAguardandoRota() {
  if (!mqtt.connected()) {
    return;
  }

  unsigned long agora = millis();

  if (agora - ultimoStatusAguardando >= intervaloStatusAguardando) {
    mqtt.publish(TOPICO_STATUS, MENSAGEM_AGUARDANDO);
    Serial.print("Status aguardando publicado: ");
    Serial.println(MENSAGEM_AGUARDANDO);
    ultimoStatusAguardando = agora;
  }
}

// ================= ROTA =================
void carregarRota(String texto) {
  parar();

  texto.toLowerCase();
  texto.trim();

  totalComandos = 0;
  comandoAtual = 0;
  comandoAtualRota = "parar";

  rotaRecebida = false;
  roboAtivo = false;
  comandoPararRecebido = false;
  jaExecutouNestaIntersecao = false;

  while (texto.length() > 0 && totalComandos < MAX_COMANDOS) {
    int posVirgula = texto.indexOf(',');

    String comando;

    if (posVirgula == -1) {
      comando = texto;
      texto = "";
    }

    else {
      comando = texto.substring(0, posVirgula);
      texto = texto.substring(posVirgula + 1);
    }

    comando.trim();
    comando.toLowerCase();

    if (comando.length() > 0) {
      comandos[totalComandos] = comando;
      totalComandos++;
    }
  }

  rotaRecebida = totalComandos > 0;

  if (!rotaRecebida) {
    pararTotal("Nenhuma rota valida recebida. Robo parado.");
    return;
  }

  comandoAtualRota = comandos[0];

  Serial.println("Rota carregada via MQTT:");

  for (int j = 0; j < totalComandos; j++) {
    Serial.print(j + 1);
    Serial.print(" - ");
    Serial.println(comandos[j]);
  }

  String status = "Rota recebida via MQTT. Comandos: " + String(totalComandos);
  publicarStatus(status.c_str());

  apitarInicio();
  iniciarRotaAutomaticamente();
}

void iniciarRotaAutomaticamente() {
  if (!rotaRecebida) {
    parar();
    return;
  }

  roboAtivo = true;
  comandoPararRecebido = false;
  jaExecutouNestaIntersecao = false;

  publicarStatus("Iniciando rota automaticamente.");

  Serial.println("Comecando a andar automaticamente...");

  frente();
}

void avancarParaProximoComando() {
  if (comandoPararRecebido) {
    return;
  }

  comandoAtual++;

  if (comandoAtual >= totalComandos) {
    finalizarRota();
    return;
  }

  comandoAtualRota = comandos[comandoAtual];
  comandoAtualRota.trim();
  comandoAtualRota.toLowerCase();

  String status = "Proximo comando: " + comandoAtualRota;
  publicarStatus(status.c_str());
}

void finalizarRota() {
  parar();

  rotaRecebida = false;
  roboAtivo = false;
  comandoPararRecebido = true;
  jaExecutouNestaIntersecao = false;

  totalComandos = 0;
  comandoAtual = 0;
  comandoAtualRota = "parar";

  publicarStatus("Fim da rota. Robo parado.");

  apitarFimRota();
}

// ================= EXECUÇÃO DOS COMANDOS =================
void executarComando(String comando) {
  comando.toLowerCase();
  comando.trim();

  Serial.print("Executando comando: ");
  Serial.println(comando);

  String status = "Executando comando: " + comando;
  publicarStatus(status.c_str());

  if (comando == "frente") {
    parar();
    delay(200);
    passarDireto();
  }

  else if (comando == "direita") {
    parar();
    delay(200);
    virarDireita(tempoFrenteAntesCurvaDireita, tempoCurvaDireita);
  }

  else if (comando == "esquerda") {
    parar();
    delay(200);
    virarEsquerda(tempoFrenteAntesCurvaEsquerda, tempoCurvaEsquerda);
  }

  else if (comando == "retorno") {
    parar();
    delay(200);
    virarEsquerda(0, tempoCurvaVolta);
    re();
    delay(700);
    parar();
    apitarInicio();
  }

  else if (comando == "parar") {
    pararTotal("Comando PARAR executado. Robo parado completamente.");
  }

  else {
    Serial.println("Comando desconhecido. Passando direto por seguranca.");
    publicarStatus("Comando desconhecido. Passando direto por seguranca.");
    passarDireto();
  }
}

void passarDireto() {
  Serial.println("Passando direto pela intersecao");

  frente();
  delay(tempoPassarIntersecao);
}

void pararTotal(const char* motivo) {
  parar();

  rotaRecebida = false;
  roboAtivo = false;
  comandoPararRecebido = true;
  jaExecutouNestaIntersecao = false;

  totalComandos = 0;
  comandoAtual = 0;
  comandoAtualRota = "parar";

  publicarStatus(motivo);

  Serial.println(motivo);
}

// ================= VELOCIDADE =================
void setVelocidade(int velDireita, int velEsquerda) {
  velDireita = constrain(velDireita, 0, 255);
  velEsquerda = constrain(velEsquerda, 0, 255);

  analogWrite(ENB_DIREITA, velDireita);
  analogWrite(ENA_ESQUERDA, velEsquerda);
}

// ================= MOVIMENTOS =================
void frente() {
  setVelocidade(velocidade, velocidade);

  digitalWrite(IN2, HIGH);
  digitalWrite(IN1, LOW);

  digitalWrite(IN4, HIGH);
  digitalWrite(IN3, LOW);
}

void esquerda() {
  setVelocidade(velocidadeCurva, velocidadeCurva);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN4, HIGH);
  digitalWrite(IN3, LOW);

  setVelocidade(velocidade, velocidade);
}

void direita() {
  setVelocidade(velocidadeCurva, velocidadeCurva);

  digitalWrite(IN2, HIGH);
  digitalWrite(IN1, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  setVelocidade(velocidade, velocidade);
}

void re() {
  setVelocidade(velocidade, velocidade);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN4, LOW);
  digitalWrite(IN3, HIGH);
}

void virarEsquerda(int frenteDel, int del) {
  Serial.println("Virando para esquerda");

  frente();
  delay(frenteDel);

  girarEsquerda(velocidadeCurva);
  delay(del);

  parar();
  delay(80);
}

void virarDireita(int frenteDel, int del) {
  Serial.println("Virando para direita");

  frente();
  delay(frenteDel);

  girarDireita(velocidadeCurva);
  delay(del);

  parar();
  delay(80);
}

void girarEsquerda(int vel) {
  setVelocidade(vel, vel);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  setVelocidade(velocidade, velocidade);
}

void girarDireita(int vel) {
  setVelocidade(vel, vel);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  setVelocidade(velocidade, velocidade);
}

void parar() {
  analogWrite(ENB_DIREITA, 0);
  analogWrite(ENA_ESQUERDA, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ================= BUZZER =================
void apitarInicio() {
  analogWrite(buzzer, 180);
  delay(100);
  analogWrite(buzzer, 0);
  delay(100);
}

void apitarFimRota() {
  analogWrite(buzzer, 180);
  delay(100);
  analogWrite(buzzer, 0);
  delay(80);

  analogWrite(buzzer, 180);
  delay(100);
  analogWrite(buzzer, 0);
  delay(80);

  analogWrite(buzzer, 180);
  delay(160);
  analogWrite(buzzer, 0);
}