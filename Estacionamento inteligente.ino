#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>

// Definir as credenciais Wi-Fi
#define WIFI_SSID "nome_wifi"
#define WIFI_PASSWORD "senha_wifi"
String phoneNumber = "numero_telefone: 55+ddd+numero";
String apiKey = "chave_api";
// Pinos
#define PIR_PIN 13
#define SERVO_PIN 22        
#define BUTTON_PIN 14         
#define LED1_PIN 18
#define LED2_PIN 19
#define LED3_PIN 21
#define LDR_PIN 34
float luz_baixa = 125.5;  //parâmetro escolhido depois de testes, para simular uma possivel falta de energia, ou troca de turno da tarde para noite
bool light = false;


bool vagaOcupada = false;
unsigned long tempoEntrada = 0;  // Tempo de entrada do carro (em milissegundos)
unsigned long tempoEstacionado = 0;  // Tempo de permanência na vaga (em minutos)
unsigned long tempoAtual = 0;  // Tempo atual (em milissegundos)
float taxaEstacionamento = 1.5;  // Taxa de R$ 1.50 por minuto

//função para envio de mensagem pelo whatsapp via api
void sendMessages(String message){
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);
    HTTPClient http;
    http.begin(url);

      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200) {
    Serial.print("Mensagem enviada com sucesso");
  } else {
    Serial.println("Erro no envio da mensagem");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi Conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);      // Sensor PIR
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Botão com resistor pull-up
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);       

  conectarWiFi();

  Serial.println("Vaga Livre");
  digitalWrite(SERVO_PIN, HIGH);        //Led liga para analogar com o portão aberto
}

void loop() {

  verifica_vaga();  // Verifica o estado da vaga ´[]
  verifica_luminosidade();  // Verifica a luminosidade
}

void verifica_vaga() {
  int estadoPIR = digitalRead(PIR_PIN);  // Lê o sensor PIR
  int estadoBotao = digitalRead(BUTTON_PIN);  // Lê o estado do botão de confirmação

  if (estadoPIR == HIGH && !vagaOcupada) {
    // Carro entrou na vaga
    vagaOcupada = true;
    tempoEntrada = random(1, 10) * 60000;  // Simulando o tempo de entrada
    digitalWrite(LED1_PIN, HIGH);          // Led vermelho Liga, sinalizando que a vaga está ocupada
    digitalWrite(LED2_PIN, LOW);
    Serial.println("Vaga Ocupada");
    sendMessages("Vaga Ocupada");

    digitalWrite(SERVO_PIN, LOW);      //Led Desliga para analogar com o portão fechado


  } else if (estadoPIR == LOW && vagaOcupada) {
    // Carro saiu da vaga
    //valores simulados apenas para simular o tempo do veículo no estacionamento
    vagaOcupada = false;
    tempoAtual = random(1, 15) * 60000;
    tempoEstacionado = (tempoAtual - tempoEntrada) / 1000 / 60;  // Calcula o tempo estacionado em minutos

    // Calcula o valor do estacionamento
    float valorEstacionamento = tempoEstacionado * taxaEstacionamento;

    // Fecha o portão (servo)
    digitalWrite(SERVO_PIN, LOW); //Portão continua fechado até efetuar o pagamento
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, LOW);
    sendMessages ("Pagamento pendente, por favor efetuar o pagamento, Valor: "+ String(valorEstacionamento, 2) + " " + "Tempo total" + String(tempoEstacionado,2));

    // Aguardar a confirmação do pagamento (botão)
    while (digitalRead(BUTTON_PIN) == HIGH) {
      delay(100);  // Aguarda até que o botão seja pressionado
    }

    // Se o botão for pressionado, o portão pode ser aberto novamente
    digitalWrite(SERVO_PIN, HIGH);  // Abre o portão
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, HIGH); // Led verde Liga, sinalizando que a vaga está livre   
    Serial.println("Pagamento confirmado, portão aberto novamente.");
    sendMessages("A vaga está livre, pagamento confirmado: " + String(valorEstacionamento, 2) + ", portão aberto novamente.");     // Envia a mensagem de Vaga Livre e efetuação do pagamento
    
  }
}

void verifica_luminosidade() {
  //costantes usadas para o calculo do nivel de luminosidade, elas foram escolhidas após testes com o sensor
  const float GAMMA = 0.7;  
  const float RL10 = 50;

int analogValue = analogRead(LDR_PIN);
float voltage = analogValue / 1024. * 5;
float resistance = 500 * voltage / (1 - voltage / 5);
float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));

  if((lux < luz_baixa) && (light == false)){
    light = true;
    digitalWrite(LED3_PIN, HIGH);
    Serial.println("O ambiente está escuro, iluminação automática ativada");
  }
  else if((lux >= luz_baixa) && (light == true)){
    light = false;
    digitalWrite(LED3_PIN, LOW);
    Serial.println("O ambiente está claro, iluminação automática desativada");
  }
}