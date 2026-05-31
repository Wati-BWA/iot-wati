#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include "config.h"

// --- Inicialización del Sensor de Temperatura ---
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// --- Variables para Temporización Asíncrona (millis) ---
unsigned long lastReadTime = 0;
unsigned long lastTransmitTime = 0;

// --- Variables de Acumulación para Promedio ---
float tempSum = 0.0;
int tempCount = 0;

// --- ID de Dispositivo Dinámico (MAC Address) ---
String deviceID = "";

// Función para obtener la dirección MAC física y formatearla
String obtenerDeviceID() {
    uint8_t mac[6];
    char macStr[18];
    WiFi.macAddress(mac);
    // Formatea como ESP32_MAC_A1B2C3D4E5F6
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return "ESP32_MAC_" + String(macStr);
}

// Función para asegurar la reconexión WiFi si se pierde la señal
void verificarConexionWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Conexión perdida. Intentando reconectar...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        int intentos = 0;
        while (WiFi.status() != WL_CONNECTED && intentos < 20) {
            delay(500);
            Serial.print(".");
            intentos++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[WIFI] Reconectado con éxito!");
            Serial.print("[WIFI] Dirección IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\n[WIFI] Error al reconectar. Se reintentará en el próximo ciclo.");
        }
    }
}

void setup() {
    // Inicializar Monitor Serial para depuración
    Serial.begin(115200);
    delay(1000); // Pequeña pausa para estabilizar serial
    
    Serial.println("\n=============================================");
    Serial.println("  ESP32-S3 HACKATHON TEMPERATURE NODE START  ");
    Serial.println("=============================================");

    // Inicializar Sensor DS18B20
    sensors.begin();
    Serial.println("[HARDWARE] Sensor DS18B20 inicializado.");

    // Conectar a la Red WiFi Abierta
    Serial.printf("[WIFI] Conectando a la red abierta: '%s'...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Esperar conexión inicial (bloqueo controlado en Setup)
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 30) {
        delay(500);
        Serial.print(".");
        intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Conectado exitosamente!");
        Serial.print("[WIFI] Dirección IP obtenida: ");
        Serial.println(WiFi.localIP());
        
        // Obtener ID único basado en la MAC real
        deviceID = obtenerDeviceID();
        Serial.printf("[DEVICE] ID Generado: %s\n", deviceID.c_str());
    } else {
        Serial.println("\n[WIFI] Advertencia: No se pudo conectar al WiFi inicial. Reintentando en bucle...");
        deviceID = "ESP32_MAC_UNKNOWN";
    }

    // Inicializar tiempos de referencia
    unsigned long tiempoActual = millis();
    lastReadTime = tiempoActual;
    lastTransmitTime = tiempoActual;
}

void loop() {
    unsigned long currentTime = millis();

    // 1. TAREA DE LECTURA (Cada 10 segundos)
    if (currentTime - lastReadTime >= READ_INTERVAL) {
        lastReadTime = currentTime;
        
        Serial.println("[SENSOR] Solicitando lectura de temperatura...");
        sensors.requestTemperatures();
        float tempLeida = sensors.getTempCByIndex(0);
        
        // Validar si el sensor está realmente conectado y responde correctamente
        if (tempLeida == DEVICE_DISCONNECTED_C) {
            Serial.println("[ERROR] No se pudo leer el DS18B20 (Sensor desconectado)");
        } else {
            tempSum += tempLeida;
            tempCount++;
            Serial.printf("[SENSOR] Lectura exitosa: %.2f °C (Muestra #%d)\n", tempLeida, tempCount);
        }
    }

    // 2. TAREA DE TRANSMISIÓN (Cada 30 segundos)
    if (currentTime - lastTransmitTime >= TRANSMIT_INTERVAL) {
        lastTransmitTime = currentTime;
        
        // Asegurarse de que estamos conectados a internet antes de enviar
        verificarConexionWiFi();

        float promedioInterior = 0.0;
        float promedioExterior = 0.0;

        if (tempCount > 0) {
            promedioInterior = tempSum / tempCount;
            promedioExterior = promedioInterior + 5.0;
        } else {
            // Caso de falla: Si no hubo lecturas válidas acumuladas, hacer lectura forzada
            Serial.println("[WARN] Sin muestras en el ciclo. Realizando lectura de emergencia...");
            sensors.requestTemperatures();
            float tempForzada = sensors.getTempCByIndex(0);
            if (tempForzada != DEVICE_DISCONNECTED_C) {
                promedioInterior = tempForzada;
                promedioExterior = tempForzada + 5.0;
                tempCount = 1;
            } else {
                Serial.println("[ERROR] Falla total del sensor. Usando base de 24.0 °C para simulación.");
                promedioInterior = 24.0;
                promedioExterior = 29.0;
                tempCount = 1;
            }
        }

        // --- Estructura para Simular Múltiples Dispositivos ---
        int totalDispositivos = SIMULATION_MODE ? 4 : 1;
        
        // Configuración de los 4 dispositivos virtuales
        struct VirtualDevice {
            String suffixMAC;
            String userId;
            float offsetTemp;
        };

        VirtualDevice virtualDevices[4] = {
            {"_SIM_A", "USR_HACK_A_2026", -3.2},
            {"_SIM_B", "USR_HACK_B_2026", 1.8},
            {"_SIM_C", "USR_HACK_C_2026", -1.5},
            {"_SIM_D", "USR_HACK_D_2026", 4.3}
        };

        for (int i = 0; i < totalDispositivos; i++) {
            String currentDeviceID = deviceID;
            String currentUserID = TENANT_USER_ID;
            float interiorTempFinal = promedioInterior;
            float exteriorTempFinal = promedioExterior;

            if (SIMULATION_MODE) {
                // Generar ID único agregando el sufijo a la dirección MAC base
                currentDeviceID = deviceID + virtualDevices[i].suffixMAC;
                currentUserID = virtualDevices[i].userId;
                
                // Agregar el offset específico más un pequeño ruido aleatorio (+-0.4°C) para mayor realismo
                float ruidoRandom = (random(-40, 40) / 100.0);
                interiorTempFinal = promedioInterior + virtualDevices[i].offsetTemp + ruidoRandom;
                exteriorTempFinal = interiorTempFinal + 5.0; // Mantener el desfasaje exterior requerido
                
                Serial.printf("\n[SIMULACIÓN] Procesando dispositivo virtual %d/4 (%s)...\n", i + 1, currentDeviceID.c_str());
            }

            // Construir Payload JSON
            JsonDocument doc;
            
            // Estructura Hardware
            JsonObject hardware = doc["hardware"].to<JsonObject>();
            hardware["device_id"] = currentDeviceID.isEmpty() ? obtenerDeviceID() : currentDeviceID;
            hardware["firmware_version"] = FIRMWARE_VERSION;

            // Estructura Tenant
            JsonObject tenant = doc["tenant"].to<JsonObject>();
            tenant["user_id"] = currentUserID;

            // Estructura Telemetry
            JsonObject telemetry = doc["telemetry"].to<JsonObject>();
            telemetry["temp_interior_c"] = serialized(String(interiorTempFinal, 2));
            telemetry["temp_exterior_c"] = serialized(String(exteriorTempFinal, 2));
            telemetry["samples_averaged"] = tempCount;

            // Estructura Diagnostics
            JsonObject diagnostics = doc["diagnostics"].to<JsonObject>();
            diagnostics["uptime_s"] = currentTime / 1000;
            diagnostics["wifi_rssi_dbm"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

            // Serializar JSON a string
            String jsonPayload;
            serializeJson(doc, jsonPayload);

            Serial.println("[HTTP] Payload preparado para enviar:");
            Serial.println(jsonPayload);

            // Envío por HTTPS seguro omitiendo validación de certificados (setInsecure)
            if (WiFi.status() == WL_CONNECTED) {
                WiFiClientSecure client;
                client.setInsecure(); // Omitir verificación SSL para agilizar en Hackathon
                
                HTTPClient http;
                Serial.printf("[HTTP] Iniciando POST a: %s\n", BACKEND_URL);
                
                if (http.begin(client, BACKEND_URL)) {
                    http.addHeader("Content-Type", "application/json");
                    
                    int httpResponseCode = http.POST(jsonPayload);
                    
                    if (httpResponseCode > 0) {
                        Serial.printf("[HTTP] Código de Respuesta del Servidor: %d\n", httpResponseCode);
                        String response = http.getString();
                        Serial.println("[HTTP] Respuesta del Servidor: " + response);
                    } else {
                        Serial.printf("[HTTP] Error en POST. Código de error: %s\n", http.errorToString(httpResponseCode).c_str());
                    }
                    http.end();
                } else {
                    Serial.println("[HTTP] No se pudo establecer conexión con el servidor.");
                }
            } else {
                Serial.println("[HTTP] No se puede transmitir. Conexión WiFi no disponible.");
            }
            
            // Si estamos simulando, hacemos una pequeña pausa de 500ms entre solicitudes para no saturar
            if (SIMULATION_MODE) {
                delay(500);
            }
        }

        // Reiniciar acumuladores para el siguiente ciclo de 30 segundos
        tempSum = 0.0;
        tempCount = 0;
        Serial.println("\n[SISTEMA] Ciclo completado. Contadores de promedio reiniciados. Esperando siguiente ciclo...\n");
    }
}
