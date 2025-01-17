/**************************************************************
 * ESP8266 Serial-to-ChatGPT Example
 * 
 * Requirements:
 *  - ArduinoJson library (v6 or higher)
 *  - ESP8266 board package in Arduino IDE
 *  
 * Functionality:
 *  1. Connects to Wi-Fi.
 *  2. Waits for user input via Serial Monitor.
 *  3. Sends the prompt to OpenAI Chat GPT.
 *  4. Prints the response to Serial Monitor.
 **************************************************************/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* WIFI_SSID      = "learn";
const char* WIFI_PASSWORD  = "98122360";

// Provide your OpenAI API Key
const String OPENAI_API_KEY = "sk-QtTjV51A9Q5BryyapBg6Y3i3Pji8ZibitdGnxukBSAZCR85h";
// If you have a custom base URL or use the default, set it here.
// Example: "https://api.openai.com"
const String OPENAI_BASE_URL = "api.aigc369.com";  
// The full endpoint for Chat Completion
// e.g., "/v1/chat/completions"
const String OPENAI_ENDPOINT = "/v1/chat/completions";

// Model name, e.g. "gpt-3.5-turbo" or any other ChatGPT model
const String MODEL_NAME = "gpt-3.5-turbo";

// The global secure client
WiFiClientSecure client;

String sendToChatGPT(const String& userPrompt);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  // Connect to Wi-Fi
  Serial.println();
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi connected with IP: " + WiFi.localIP().toString());
  
  // Configure secure client
  client.setInsecure();  // Not recommended in production. 
                         // Ideally, set the root CA certificate.
  
  Serial.println("Ready! Type your prompt and press Enter...");
}

void loop() {
  // Check if user typed something in the Serial Monitor
  if (Serial.available() > 0) {
    // Read the entire line
    String userPrompt = Serial.readStringUntil('\n');
    userPrompt.trim();

    if (userPrompt.length() > 0) {
      Serial.println("Sending to ChatGPT...");
      String response = sendToChatGPT(userPrompt);
      Serial.println("ChatGPT response:\n" + response);
      Serial.println("\n------------------------------------------\n");
    }
  }
}

/**
 * Sends the user prompt to the OpenAI Chat API and returns the assistant's response.
 */
String sendToChatGPT(const String& userPrompt) {
  // Construct JSON body for the Chat Completion endpoint
  // The messages array must follow ChatGPT's conversation format:
  //   [{ "role": "user", "content": "Your prompt here" }, ...]
  StaticJsonDocument<512> doc;
  
  doc["model"] = MODEL_NAME;
  
  // We create the "messages" array with a single user message
  JsonArray messages = doc.createNestedArray("messages");
  JsonObject messageObject = messages.createNestedObject();
  messageObject["role"] = "user";
  messageObject["content"] = userPrompt;

  // If you want to add temperature or other parameters:
  // doc["temperature"] = 0.7;
  
  // Serialize JSON into a string to send to OpenAI
  String requestBody;
  serializeJson(doc, requestBody);

  // Open the connection
  if (!client.connect(OPENAI_BASE_URL.c_str(), 443)) {
    Serial.println("Connection failed!");
    return "Error: Unable to connect";
  }

  // Construct the HTTP POST request:
  String request = 
    String("POST ") + OPENAI_ENDPOINT + " HTTP/1.1\r\n" +
    "Host: " + OPENAI_BASE_URL + "\r\n" +
    "Authorization: Bearer " + OPENAI_API_KEY + "\r\n" +
    "Content-Type: application/json\r\n" +
    "Content-Length: " + String(requestBody.length()) + "\r\n" +
    "Connection: close\r\n\r\n" +
    requestBody + "\r\n";

  // Send the HTTP request
  client.print(request);

  ///After client.print(request);, 
  //the OpenAI server (or your proxy) will respond with an HTTP response. It looks like:

  //HTTP/1.1 200 OK
  //Content-Type: application/json
  //Content-Length: 597
  //
  //{
  // "id": "...",
  //  "object": "chat.completion",
  //  "model": "...",
  // "choices": [
  //    {
  //      "message": {
  //        "role": "assistant",
  //        "content": "Hello! How can I help you?"
  //      }
  //    }
  //  ]
  //  ...
  //}

  ///
  // Read the response
  String response;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line); // Print the header line
    if (line == "\r") {
      // Headers are finished
      break;
    }
  }
  
  // Now read the response body
  String responseBody;
  while (client.available()) {
    responseBody += client.readString();
  }
  client.stop();

  // 1) Find the index of the first '{'
  //exclude the headers sent by the server to get the JSON response
  int jsonStart = responseBody.indexOf('{');

  // 2) If found, slice off everything before that
  if (jsonStart >= 0) {
    responseBody = responseBody.substring(jsonStart);
  }

  // Debug: show final string
  Serial.println("Final response body (after slicing):");
  Serial.println(responseBody);

  // Parse the JSON response to get the assistant's reply
  // The structure from OpenAI for chat completions:
  // {
  //   "choices": [
  //     {
  //       "message": {
  //         "role": "assistant",
  //         "content": "...this is the response..."
  //       }
  //     }
  //   ],
  //   ...
  // }
  StaticJsonDocument<2048> respDoc;
  DeserializationError error = deserializeJson(respDoc, responseBody);

  if (error) {
    Serial.println("JSON parse failed!");
    // Print the raw body to see the error
    Serial.println(responseBody);
    return "Error: JSON parse failed!";
  }
  
  // Extract the content from the first choice (assuming we only have one)
  String assistantText;
  if (respDoc["choices"][0]["message"]["content"] != nullptr) {
    assistantText = respDoc["choices"][0]["message"]["content"].as<String>();
  } else {
    assistantText = "No valid response.";
  }
  
  return assistantText;
}