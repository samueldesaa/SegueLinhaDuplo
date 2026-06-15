//Robo Samuel

const int IN1 = 13;
const int IN2 = 12;
const int IN3 = 14;
const int IN4 = 27;

const int ENB_DIREITA = 32;
const int ENA_ESQUERDA = 33;

const int SENSOR_IR1 = 35;
const int SENSOR_IR2 = 34;
const int botao = 25;
const int buzzer = 26;

int velocidade = 120;
int velocidadeCurva = 150;

// //Robo Franciele
// const int IN1 = 12;
// const int IN2 = 13;
// const int IN3 = 27;
// const int IN4 = 14;

// const int ENB_DIREITA = 32;
// const int ENA_ESQUERDA = 33;

// const int LDR = 35;
// const int SENSOR_IR = 34;
// const int botao = 25;


// int limiteLDR = 1700;


// int velocidade = 100;
// int velocidadeCurva = 100;

const int IR_LINHA = HIGH;

String rotaTexto = "frente, frente, frente, esquerda, retorno, esquerda, esquerda, parar";

const int MAX_COMANDOS = 20;
String comandos[MAX_COMANDOS];

int totalComandos = 0;
int comandoAtual = 0;

int tempoPassarIntersecao = 200;

int tempoFrenteAntesCurvaEsquerda = 300;
int tempoCurvaEsquerda = 400;

int tempoFrenteAntesCurvaDireita = 300;
int tempoCurvaDireita = 400;

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(botao, INPUT_PULLUP);

  pinMode(ENB_DIREITA, OUTPUT);
  pinMode(ENA_ESQUERDA, OUTPUT);

  pinMode(SENSOR_IR1, INPUT);
  pinMode(SENSOR_IR2, INPUT);

  pinMode(buzzer, OUTPUT);

  parar();

  carregarRota(rotaTexto);

  Serial.println("Robo seguidor de linha iniciado");
  Serial.println("Rota carregada:");

  for (int j = 0; j < totalComandos; j++) {
    Serial.print(j + 1);
    Serial.print(" - ");
    Serial.println(comandos[j]);
  }

  Serial.println("Aguardando botao para iniciar...");

  while (digitalRead(botao) == HIGH) {
    parar();
  }


  Serial.println("Comecando a andar...");
}

void loop() {
  int leituraIR1 = digitalRead(SENSOR_IR1);
  int leituraIR2 = digitalRead(SENSOR_IR2);

  bool irNaLinha1 = leituraIR1 == IR_LINHA;
  bool irNaLinha2 = leituraIR2 == IR_LINHA;

  Serial.print(" | IR1: ");
  Serial.print(leituraIR1);

  Serial.print(" | IR1: ");
  Serial.print(irNaLinha1 ? "SIM" : "NAO");

  Serial.print(" | IR2: ");
  Serial.print(leituraIR2);

  Serial.print(" | IR2: ");
  Serial.print(irNaLinha2 ? "SIM" : "NAO");

  Serial.print(" | Comando atual: ");

  if (comandoAtual < totalComandos) {
    Serial.print(comandos[comandoAtual]);
  } else {
    Serial.print("nenhum");
  }

  Serial.print(" | Acao: ");

  if (irNaLinha1 && irNaLinha2) {
    parar();
    delay(80);

    apitarInicio();
    delay(100);
    apitarInicio();

    if (comandoAtual < totalComandos) {
      executarComando(comandos[comandoAtual]);
      comandoAtual++;
    } else {
      Serial.println("Fim da rota. Parando.");
      parar();
      while (true) {
        parar();
      }
    }

    delay(100);
  }

  else if (irNaLinha1 && !irNaLinha2) {
    // re();
    girarEsquerda(100);
    Serial.println("Corrigindo esquerda");
  }

  else if (!irNaLinha1 && irNaLinha2) {
    // re();
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

void carregarRota(String texto) {
  texto.toLowerCase();
  texto.trim();

  totalComandos = 0;

  while (texto.length() > 0 && totalComandos < MAX_COMANDOS) {
    int posVirgula = texto.indexOf(',');

    String comando;

    if (posVirgula == -1) {
      comando = texto;
      texto = "";
    } else {
      comando = texto.substring(0, posVirgula);
      texto = texto.substring(posVirgula + 1);
    }

    comando.trim();

    if (comando.length() > 0) {
      comandos[totalComandos] = comando;
      totalComandos++;
    }
  }
}

void executarComando(String comando) {
  comando.toLowerCase();
  comando.trim();

  Serial.print("Executando comando: ");
  Serial.println(comando);

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
  else if(comando == "retorno"){
    parar();
    delay(200);
    virarEsquerda(0, 2*tempoCurvaEsquerda);
  }

  else if (comando == "parar") {
    Serial.println("Comando PARAR recebido. Robo parado completamente.");
    parar();

    while (true) {
      parar();
    }
  }

  else {
    Serial.println("Comando desconhecido. Passando direto por seguranca.");
    passarDireto();
  }
}

void passarDireto() {
  Serial.println("Passando direto pela intersecao");

  frente();
  delay(tempoPassarIntersecao);
}

void setVelocidade(int velDireita, int velEsquerda) {
  velDireita = constrain(velDireita, 0, 255);
  velEsquerda = constrain(velEsquerda, 0, 255);

  analogWrite(ENB_DIREITA, velDireita);
  analogWrite(ENA_ESQUERDA, velEsquerda);
}

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

  frente();
  delay(150);
}

void virarDireita(int frenteDel, int del) {
  Serial.println("Virando para direita");

  frente();
  delay(frenteDel);

  girarDireita(velocidadeCurva);
  delay(del);

  parar();
  delay(80);

  frente();
  delay(150);
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

void apitarInicio() {
  analogWrite(buzzer, 180);
  delay(100);
  analogWrite(buzzer, 0);
  delay(100);
}