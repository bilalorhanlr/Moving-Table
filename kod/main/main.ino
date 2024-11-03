#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi bilgileri
const char* ssid = "Bilal"; // Wi-Fi ağının SSID'si
const char* password = "bilal...."; // Wi-Fi ağının parolası

// Web sunucusu
WebServer server(80); // 80 numaralı portta çalışan bir web sunucusu oluşturuyoruz

// Motor sürücü pinleri
const int RPWM_PIN = 22; // Motorun ileri hareketi için PWM pini
const int LPWM_PIN = 23; // Motorun geri hareketi için PWM pini

// HC-SR04 pinleri
const int TRIG_PIN = 5; // HC-SR04 ultrasonik sensörün TRIG pini
const int ECHO_PIN = 18; // HC-SR04 ultrasonik sensörün ECHO pini

// Buton pinleri
const int BUTTON_FORWARD_PIN = 19; // İleri hareket için buton pini
const int BUTTON_BACKWARD_PIN = 21; // Geri hareket için buton pini

long duration; // Ses dalgasının geri dönüş süresi
int distance; // Hesaplanan mesafe
bool buttonPressed = false; // Butona basılıp basılmadığını takip eden bayrak değişkeni
bool isButtonControl = false; // Buton kontrolünün etkin olup olmadığını takip eden bayrak değişkeni

void setup() {
  Serial.begin(115200); // Seri haberleşmeyi başlat

  // WiFi bağlantısı
  WiFi.begin(ssid, password); // WiFi ağına bağlan
  while (WiFi.status() != WL_CONNECTED) { // WiFi bağlanana kadar bekle
    delay(1000); // 1 saniye bekle
    Serial.print("."); // Her beklemede bir nokta yazdır
  }
  
  Serial.println(""); // Yeni satıra geç
  Serial.println("WiFi bağlantısı başarılı!"); // WiFi bağlantısının başarılı olduğunu yazdır
  Serial.print("IP Adresi: "); // IP adresi etiketini yazdır
  Serial.println(WiFi.localIP()); // ESP32'nin aldığı IP adresini yazdır

  // Motor pinlerini çıkış olarak ayarla
  pinMode(RPWM_PIN, OUTPUT); // RPWM pini çıkış olarak ayarla
  pinMode(LPWM_PIN, OUTPUT); // LPWM pini çıkış olarak ayarla

  // HC-SR04 pinlerini ayarla
  pinMode(TRIG_PIN, OUTPUT); // TRIG pini çıkış olarak ayarla
  pinMode(ECHO_PIN, INPUT); // ECHO pini giriş olarak ayarla

  // Buton pinlerini giriş olarak ayarla
  pinMode(BUTTON_FORWARD_PIN, INPUT_PULLUP); // İleri buton pini giriş olarak ayarla ve dahili pull-up direnci etkinleştir
  pinMode(BUTTON_BACKWARD_PIN, INPUT_PULLUP); // Geri buton pini giriş olarak ayarla ve dahili pull-up direnci etkinleştir
  
  // Web sunucusu iletici
  server.on("/", handleRoot); // Ana sayfa için handleRoot fonksiyonunu kullan
  server.on("/forward", handleForward); // /forward isteği için handleForward fonksiyonunu kullan
  server.on("/backward", handleBackward); // /backward isteği için handleBackward fonksiyonunu kullan
  server.on("/stop", handleStop); // /stop isteği için handleStop fonksiyonunu kullan
  server.on("/distance", handleDistance); // /distance isteği için handleDistance fonksiyonunu kullan
  
  server.begin(); // Web sunucusunu başlat
  Serial.println("Web sunucusu başladı!"); // Web sunucusunun başladığını yazdır
}

void loop() {
  server.handleClient(); // Web sunucusu istemcilerini kontrol et ve işleme al
  checkButtons(); // Butonları kontrol et
}

void handleRoot() {
  // Web arayüzü HTML içeriği
  String html = "<html>"
                "<head>"
                "<title>Ahmet KURU</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }"
                "h1 { color: #333; }"
                ".button { display: inline-block; padding: 15px 25px; font-size: 24px; cursor: pointer; text-align: center; text-decoration: none; outline: none; color: #fff; background-color: #4CAF50; border: none; border-radius: 15px; box-shadow: 10 9px #000; margin: 10px; }"
                ".button:hover { background-color: #3e8e41; }"
                ".button:active { background-color: #3e8e41; box-shadow: 0 5px #666; transform: translateY(4px); }"
                "</style>"
                "<script>"
                "function sendCommand(cmd) {"
                "  var xhttp = new XMLHttpRequest();"
                "  xhttp.open('GET', cmd, true);"
                "  xhttp.send();"
                "}"
                "function getDistance() {"
                "  var xhttp = new XMLHttpRequest();"
                "  xhttp.onreadystatechange = function() {"
                "    if (this.readyState == 4 && this.status == 200) {"
                "      document.getElementById('distance').innerHTML = this.responseText + ' cm';"
                "    }"
                "  };"
                "  xhttp.open('GET', '/distance', true);"
                "  xhttp.send();"
                "}"
                "setInterval(getDistance, 1000);" // Mesafeyi her saniye güncelle
                "</script>"
                "</head>"
                "<body>"
                "<h1>USKUDAR UNIVERSITESI</h1>"
                "<h2>Motor Kontrol Paneli</h2>"
                "<button class=\"button\" onclick=\"sendCommand('/forward')\">Ileri</button>" // İleri komutu gönder
                "<button class=\"button\" onclick=\"sendCommand('/backward')\">Geri</button>" // Geri komutu gönder
                "<button class=\"button\" onclick=\"sendCommand('/stop')\">Durdur</button>" // Durdurma komutu gönder
                "<h2>Mesafe: <span id=\"distance\">-</span></h2>" // Mesafe göstergesi
                "</body>"
                "</html>";
  server.send(200, "text/html", html); // HTML içeriğini gönder
}

void handleForward() {
  if (!isButtonControl) { // Buton kontrolü etkin değilse
    digitalWrite(RPWM_PIN, HIGH); // Motoru ileri hareket ettir
    digitalWrite(LPWM_PIN, LOW); 
    server.send(200, "text/plain", "Motor İleri"); // Yanıt olarak "Motor İleri" gönder
  }
}

void handleBackward() {
  if (!isButtonControl) { // Buton kontrolü etkin değilse
    digitalWrite(RPWM_PIN, LOW); // Motoru geri hareket ettir
    digitalWrite(LPWM_PIN, HIGH);
    server.send(200, "text/plain", "Motor Geri"); // Yanıt olarak "Motor Geri" gönder
  }
}

void handleStop() {
  if (!isButtonControl) { // Buton kontrolü etkin değilse
    digitalWrite(RPWM_PIN, LOW); // Motoru durdur
    digitalWrite(LPWM_PIN, LOW);
    server.send(200, "text/plain", "Motor Durduruldu"); // Yanıt olarak "Motor Durduruldu" gönder
  }
}

void handleDistance() {
  // Mesafe ölçümü
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH); // ECHO pininde sinyal süresini ölç
  distance = duration * 0.034 / 2; // Süreyi mesafeye çevir
  
  server.send(200, "text/plain", String(distance)); // Mesafeyi yanıt olarak gönder
}

void checkButtons() {
  // Butonları kontrol et
  if (digitalRead(BUTTON_FORWARD_PIN) == LOW) { // İleri butonuna basılmışsa
    isButtonControl = true; // Buton kontrolünü etkinleştir
    digitalWrite(RPWM_PIN, HIGH); // Motoru ileri hareket ettir
    digitalWrite(LPWM_PIN, LOW);
  } else if (digitalRead(BUTTON_BACKWARD_PIN) == LOW) { // Geri butonuna basılmışsa
    isButtonControl = true; // Buton kontrolünü etkinleştir
    digitalWrite(RPWM_PIN, LOW); // Motoru geri hareket ettir
    digitalWrite(LPWM_PIN, HIGH);
  } else {
    if (isButtonControl) { // Eğer buton kontrolü etkinse ve butona basılmıyorsa
      digitalWrite(RPWM_PIN, LOW); // Motoru durdur
      digitalWrite(LPWM_PIN, LOW);
      isButtonControl = false; // Buton kontrolünü devre dışı bırak
    }
  }
}
