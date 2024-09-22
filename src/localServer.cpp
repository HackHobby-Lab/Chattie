#include "localServer.h"


// Define the global variables
Preferences preferences;
WebServer server(80);
HTTPClient http;

const char *apSSID = "Chattie";
const char *apPassword = "12345678";
const int resetThreshold = 5;
WiFiClient espClient;

// Define the static IP addresses
IPAddress local_IP(192, 168, 1, 148); // Static IP address in STA mode
IPAddress gateway(192, 168, 1, 1);    // Gateway IP address
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress ap_IP(192, 168, 1, 160);    // Static IP address in AP mode

const int ledPin = 2; // LED pin, change this according to your setup
bool ledState = false;



struct User {
    String username;
    String password; // In a real app, hash this for security
};

std::vector<User> users;

// Load users from SPIFFS
void loadUsers() {
    Serial.println("Loading Users");
    File file = SPIFFS.open("/users.txt", "r");
    if (!file){Serial.println("File  not found"); return;}

    while (file.available()) {
        String line = file.readStringUntil('\n');
        int separatorIndex = line.indexOf(':');
        Serial.println("Reading....");
        if (separatorIndex > 0) {
            String username = line.substring(0, separatorIndex);
            String password = line.substring(separatorIndex + 1);
            Serial.print("Username: ");
            Serial.println(username);
            Serial.print("Password: ");
            Serial.println(password);
            users.push_back({username, password});
            
        }
    }
    file.close();
}

// Save a new user to SPIFFS
void saveUser(const String& username, const String& password) {
    Serial.println("In Save user");
    File file = SPIFFS.open("/users.txt", "a");
    if (file) {
        Serial.println("Saving");
        file.println(username + ":" + password);
        Serial.print("Username: ");
        Serial.println(username);
        Serial.print("Password: ");
        Serial.println(password);
        file.close();
    }
}


// Handle login request
void handleLogin() {
    Serial.println("In Login");
    String username = server.arg("username");
    String password = server.arg("password");

    for (const auto& user : users) {
        if (user.username == username && user.password == password) {
            server.send(200, "application/json", "{\"message\":\"Login successful!\"}");
            return;
        }
    }
    server.send(401, "application/json", "{\"message\":\"Wrong username or password!\"}");
}

// Handle signup request
void handleSignup() {
    Serial.println("In Signup");
    String username = "Hamza";
    String password = "123456";

    for (const auto& user : users) {
        if (user.username == username) {
            server.send(409, "application/json", "{\"message\":\"Username already exists!\"}");
            return;
        }
    }

    saveUser(username, password);
    users.push_back({username, password});
    server.send(200, "application/json", "{\"message\":\"Signup successful!\"}");
}


void startWebServer()
{
  server.on("/", HTTP_GET, []()
            {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      Serial.println("Failed to open file");
      server.send(500, "text/plain", "Internal Server Error");
      return;
    }

    String htmlContent;
    while (file.available()) {
      htmlContent += file.readString();
    }
    file.close();

    server.send(200, "text/html", htmlContent); });

  // Define web routes
//   server.on("/toggleLED", toggleLED);
    server.on("/login", HTTP_POST, handleLogin);
    server.on("/signup", HTTP_POST, handleSignup);

  server.begin();
  Serial.println("HTTP server started");
  loadUsers();
}

void startAPMode()
{
  WiFi.softAPConfig(ap_IP, ap_IP, subnet); // Set static IP for AP mode
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started with IP: ");
  Serial.println(WiFi.softAPIP());

  startWebServer();
}