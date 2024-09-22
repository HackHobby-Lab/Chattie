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
    String password; 
    bool isAdmin;
};

std::vector<User> users;
User currentUser;
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

String cleanString(const String& str) {
    String cleaned = str;
    cleaned.replace("\n", ""); // Remove newline characters
    cleaned.trim();             // Trim leading and trailing whitespace
    return cleaned;
}

bool isAdminUser() {
    // Check if the logged-in user is admin
    // You could store the current username in a global variable upon login
    return currentUser.username == "admin"; // Adjust as per your implementation
}

void handleViewUsers() {
    // Only allow access if the logged-in user is admin
    if (!isAdminUser()) {
        server.send(403, "application/json", "{\"message\":\"Forbidden! Admins only.\"}");
        return;
    }

    String userList = "[";
    for (const auto& user : users) {
        userList += "{\"username\":\"" + user.username + "\",\"isAdmin\":" + String(user.isAdmin) + "},";
    }
    userList.remove(userList.length() - 1); // Remove last comma
    userList += "]";

    server.send(200, "application/json", userList);
}
void handleAddUser() {
    if (!isAdminUser()) {
        server.send(403, "application/json", "{\"message\":\"Forbidden! Admins only.\"}");
        return;
    }

    DynamicJsonDocument doc(1024);
    String jsonBody = server.arg("plain");
    Serial.print("Received JSON body: "); Serial.println(jsonBody);

    DeserializationError error = deserializeJson(doc, jsonBody);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        server.send(400, "application/json", "{\"message\":\"Invalid JSON!\"}");
        return;
    }

    String username = doc["username"];
    String password = doc["password"];
    // Check if the user already exists...
    
    users.push_back({username, password, false}); // New users are regular by default
    server.send(200, "application/json", "{\"message\":\"User added successfully!\"}");
}



void handleRemoveUser() {
    if (!isAdminUser()) {
        server.send(403, "application/json", "{\"message\":\"Forbidden! Admins only.\"}");
        return;
    }

    DynamicJsonDocument doc(1024);
    String jsonBody = server.arg("plain");
    Serial.print("Received JSON body: "); Serial.println(jsonBody);

    DeserializationError error = deserializeJson(doc, jsonBody);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        server.send(400, "application/json", "{\"message\":\"Invalid JSON!\"}");
        return;
    }

    String username = doc["username"];
    
    // Find and remove the user...
    
    server.send(200, "application/json", "{\"message\":\"User removed successfully!\"}");
}



void handleLogin() {
    Serial.println("In Login");
    
    DynamicJsonDocument doc(1024);
    String jsonBody = server.arg("plain");
    Serial.print("Received JSON body: "); Serial.println(jsonBody);

    DeserializationError error = deserializeJson(doc, jsonBody);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        server.send(400, "application/json", "{\"message\":\"Invalid JSON!\"}");
        return;
    }

    String username = doc["username"];
    String password = doc["password"];
    Serial.print("Extracted Username: "); Serial.println(username);
    Serial.print("Extracted Password: "); Serial.println(password);

    for (const auto& user : users) {
        Serial.print("Checking against stored user: "); Serial.println(user.username);
        Serial.print("Stored Password: "); Serial.println(user.password);
         String storedPassword = cleanString(user.password);
        
        if (user.username == username && storedPassword == password) {
             if (user.username == "admin" ) {
            currentUser = user;
            server.send(200, "application/json", "{\"message\":\"Admin Login successful!\", \"isAdmin\": " + String(user.isAdmin) + "}");

            // server.send(200, "application/json", "{\"message\":\"Admin login successful!\"}");
            return;
    }
            
            Serial.println("Login successful!");
            server.send(200, "application/json", "{\"message\":\"Login successful!\"}");
            return;
        }

       
    }
    
    Serial.println("Login failed: Wrong username or password!");
    server.send(401, "application/json", "{\"message\":\"Wrong username or password!\"}");
}



void handleSignup() {
    Serial.println("In Signup");
    
    // Create a buffer for the JSON data
    DynamicJsonDocument doc(1024);
    String jsonBody = server.arg("plain");

    DeserializationError error = deserializeJson(doc, jsonBody);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        server.send(400, "application/json", "{\"message\":\"Invalid JSON!\"}");
        return;
    }

    String username = doc["username"];
    String password = doc["password"];
    Serial.print("Signup Username: "); Serial.println(username);
    Serial.print("Signup Password: "); Serial.println(password);

    // Check for existing username
    for (const auto& user : users) {
        if (user.username == username) {
            server.send(409, "application/json", "{\"message\":\"Username already exists!\"}");
            return;
        }
    }

    saveUser(username, password);
    users.push_back({username, password});  // Store user
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
     server.on("/view_users", HTTP_GET, handleViewUsers);
    server.on("/add_user", HTTP_POST, handleAddUser);
    server.on("/remove_user", HTTP_POST, handleRemoveUser);

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