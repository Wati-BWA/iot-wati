#ifndef CONFIG_H
#define CONFIG_H

// --- Credenciales WiFi ---
// Red abierta (sin contraseña)
#define WIFI_SSID "Build with AI"
#define WIFI_PASSWORD ""

// --- Configuración de Red e Endpoint HTTP ---
// URL del endpoint HTTPS. Modifica esta variable según tus necesidades.
const char* const BACKEND_URL = "https://us-central1-wati-497921.cloudfunctions.net/ingest-telemetry";

// --- Identificaciones y Constantes del Negocio ---
#define TENANT_USER_ID "USR_HACK_2026"
#define FIRMWARE_VERSION "1.2.0-MINIMAL"

// --- Configuración del Hardware ---
#define ONE_WIRE_BUS 4 // Pin GPIO 4 para la señal del DS18B20

// --- Intervalos de Tiempo (en milisegundos) ---
const unsigned long READ_INTERVAL = 10000;      // Lectura de temperatura cada 10 segundos
const unsigned long TRANSMIT_INTERVAL = 30000;  // Envío de telemetría cada 30 segundos

// --- Modo Simulación Multi-Dispositivo ---
// Cambiar a 'true' para simular 4 dispositivos virtuales con diferentes MAC, User ID y variación de temperatura.
// Cambiar a 'false' para el funcionamiento normal con el sensor y hardware real físico.
#define SIMULATION_MODE true

#endif // CONFIG_H
