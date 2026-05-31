# **Plan de Implementación: Nodo de Temperatura ESP32-S3 (Versión Hackathon)**

## **1\. Arquitectura y Entorno de Desarrollo**

Se utilizará **PlatformIO** (integrado en VS Code o Google Antigravity) por su gestión declarativa de dependencias y mayor velocidad de iteración respecto al IDE de Arduino clásico.

### **Estructura de Archivos**

firmware/  
├── platformio.ini      \# Dependencias y configuración de la placa  
└── src/  
    ├── config.h        \# Credenciales WiFi y constantes (Hardcoded)  
    └── main.cpp        \# Lógica central: Conexión, lectura y HTTP POST

### **Configuración del Entorno (platformio.ini)**

Se eliminan todas las librerías no relacionadas con el sensor de temperatura y el manejo de JSON.

\[env:esp32-s3-devkitc-1\]  
platform \= espressif32  
board \= esp32-s3-devkitc-1  
framework \= arduino  
upload\_speed \= 921600  
monitor\_speed \= 115200  
build\_flags \=  
    \-DBOARD\_HAS\_PSRAM  
    \-DARDUINO\_USB\_MODE=1  
    \-DCORE\_DEBUG\_LEVEL=3  
lib\_deps \=  
    paulstoffregen/OneWire @ ^2.3.8  
    milesburton/DallasTemperature @ ^3.11.0  
    bblanchon/ArduinoJson @ ^7.1.0

## **2\. Hardware y Conexiones**

El sistema se simplifica a un único sensor DS18B20 conectado al ESP32-S3.

| Componente | Pin ESP32-S3 | Función |
| :---- | :---- | :---- |
| **DS18B20 (Señal 'S')** | GPIO 4 | Bus OneWire para datos. |
| **DS18B20 (VCC)** | 3.3V | Alimentación (Obligatorio 3.3V para proteger los pines del ESP32). |
| **DS18B20 (GND)** | GND | Tierra común. |

*Nota: El módulo DS18B20 específico mostrado en imágenes previas ya incluye la resistencia pull-up necesaria (4.7kΩ), por lo que se conecta directamente.*

## **3\. Lógica de Firmware (main.cpp y config.h)**

### **3.1. Gestión de Tiempo Asíncrona (El mandato de millis())**

El firmware utiliza millis() para gestionar dos tareas con frecuencias distintas sin bloquear el procesador ni interrumpir la conexión WiFi:

1. **Lectura (cada 10s):** Acumula lecturas del sensor.  
2. **Transmisión (cada 30s):** Promedia las lecturas acumuladas, simula el desfase exterior (+5°C) y envía el payload.

### **3.2. Estructura del Payload JSON**

Se estructuró para facilitar la ingesta en bases de datos orientadas a documentos (ej. Firestore), separando hardware, usuario y telemetría.

{  
  "hardware": {  
    "device\_id": "ESP32\_MAC\_A1B2C3",  
    "firmware\_version": "1.2.0-MINIMAL"  
  },  
  "tenant": {  
    "user\_id": "USR\_987654321"  
  },  
  "telemetry": {  
    "temp\_interior\_c": 24.50,  
    "temp\_exterior\_c": 29.50,  
    "samples\_averaged": 3  
  },  
  "diagnostics": {  
    "uptime\_s": 3600,  
    "wifi\_rssi\_dbm": \-65  
  }  
}

### **3.3. Transmisión HTTP Segura**

Se utiliza WiFiClientSecure con el método client.setInsecure() para saltar la validación de certificados SSL durante la hackathon y agilizar las pruebas contra el endpoint HTTPS (Cloud Run/FastAPI).

## **4\. Justificación de Cambios Arquitectónicos (Hackathon vs SDD Original)**

Para adaptar el Documento de Diseño de Software (SDD) original a los tiempos y recursos limitados de la hackathon, se implementaron los siguientes cambios clave:

| Aspecto | Diseño Original (SDD) | Implementación Hackathon | Razón del Cambio |
| :---- | :---- | :---- | :---- |
| **Aprovisionamiento** | BLE 5.0 (Aplicación móvil conecta y envía credenciales). | Credenciales WiFi hardcodeadas en config.h. | Elimina la necesidad de desarrollar/probar la app móvil y el stack BLE del ESP32. |
| **Sensores** | DS18B20, PZEM-004T (Energía), PIR (Presencia). | Únicamente DS18B20 (Temperatura). | Reducción de complejidad de hardware y puntos de fallo. Simulación de temperatura exterior por software (+5°C). |
| **Actuadores** | Emisor/Receptor IR para control de Aire Acondicionado. | Eliminado. | Simplificación extrema para enfocarse en la telemetría unidireccional (Sensor \-\> Nube). |
| **Display** | Pantalla OLED I2C para status local. | Eliminado (Solo Monitor Serial). | Ahorro de tiempo en cableado y programación de UI. |
| **Generación de Timestamp** | El ESP32 sincroniza la hora local vía servidor NTP (Network Time Protocol) antes de enviar datos. | **Delegada al Endpoint (Backend).** El ESP32 ya no envía el campo timestamp\_utc. | Ver detalle abajo. |

### **Detalle del "Hack" de Timestamp**

**Funcionamiento Original:**

El ESP32 debía conectarse a un servidor NTP (ej. pool.ntp.org) durante el arranque para obtener la hora UTC. Luego, en cada ciclo de telemetría, el microcontrolador formateaba la hora actual (ej. 2026-05-30T21:19:50Z) y la incluía en el JSON enviado al servidor.

**Problema:**

La sincronización NTP añade tiempo al ciclo de arranque, requiere manejo de errores si el servidor NTP falla y obliga a importar librerías adicionales para el formateo correcto de la fecha/hora en C++.

**Solución Implementada:**

Se eliminó la responsabilidad del tiempo del ESP32. El firmware ahora envía el JSON sin ninguna referencia temporal. El endpoint en la nube (FastAPI) está configurado para capturar la hora exacta del servidor en el instante en que recibe el payload HTTP POST y añadirla al registro antes de guardarlo en la base de datos.

* *Ventaja:* Simplifica drásticamente el código C++ y reduce el tamaño del payload.  
* *Limitación:* Si hay latencia de red, la hora reflejará el momento de *recepción*, no el momento exacto de *medición*. Dado el intervalo de 30s, esta desviación es irrelevante para este caso de uso.