# **Documentación del Proyecto: Nodo IoT de Temperatura ESP32-S3**

Este archivo sirve como contexto exhaustivo y manual de operaciones para que cualquier desarrollador o agente de IA pueda entender, compilar, modificar y dar soporte al nodo de telemetría IoT implementado.

---

## **1. Contexto y Objetivos del Proyecto**
El proyecto consiste en un **Nodo IoT de Telemetría** diseñado para ejecutarse en un microcontrolador **ESP32-S3** (compatible con clones genéricos y placas tipo Freenove/WROOM). Su única tarea física es leer la temperatura ambiente de un sensor digital DS18B20 y enviar los datos a un backend HTTPS en la nube mediante solicitudes HTTP POST estructuradas en formato JSON.

### **Restricciones de Hackathon Implementadas**
Para garantizar la máxima velocidad de desarrollo y estabilidad en el contexto competitivo de una hackathon, se realizaron simplificaciones de arquitectura:
1. **Credenciales WiFi Hardcodeadas:** En lugar de aprovisionamiento por Bluetooth (BLE), las credenciales se definen directamente en código.
2. **Transmisión Segura Simplificada (Insecure):** Para evitar la tediosa gestión de certificados SSL/TLS raíz (que suelen caducar o requerir actualizaciones constantes en C++), el firmware utiliza `client.setInsecure()`. La comunicación viaja encriptada vía HTTPS, pero se omite la validación de la firma del certificado.
3. **Gestión Asíncrona con `millis()`:** Se evita el uso de sistemas operativos en tiempo real complejos (FreeRTOS) o bloqueos forzados con `delay()`. Las lecturas de sensores y transmisiones ocurren en intervalos independientes sin interferir con la pila de red.
4. **Timestamp Delegado al Backend:** El ESP32-S3 no sincroniza su reloj interno por NTP. El backend (FastAPI/Cloud Run) captura el momento exacto en el que recibe el JSON y le inyecta la estampa de tiempo UTC al registrarlo.

---

## **2. Arquitectura de Hardware y Conexiones**

El sensor utilizado es el **DS18B20** en bus OneWire. La conexión es directa ya que los módulos específicos de sensor DS18B20 para prototipado ya integran la resistencia de pull-up de 4.7kΩ necesaria en la línea de datos.

| Pin del Sensor | Pin en ESP32-S3 | Función | Rango Eléctrico |
| :--- | :--- | :--- | :--- |
| **S (Señal / Datos)** | **GPIO 4** | Bus OneWire para transmisión de datos bidireccional | 3.3V Digital I/O |
| **VCC (Alimentación)** | **3.3V** | Alimentación eléctrica del integrado | 3.0V - 5.5V (Usar siempre 3.3V) |
| **GND (Tierra)** | **GND** | Tierra común de referencia eléctrica | 0V |

---

## **3. Estructura del Firmware (PlatformIO)**

El firmware se ubica en el directorio `firmware/` y sigue los estándares estrictos de PlatformIO:

```text
firmware/
├── platformio.ini      # Archivo central de configuración de PlatformIO
└── src/
    ├── config.h        # Constantes de negocio, credenciales WiFi y red
    └── main.cpp        # Lógica central (Setup, Loop asíncrono, HTTP POST)
```

### **Detalles de Archivos Clave:**

*   **[platformio.ini](file:///c:/Users/Nexuz/Desktop/HACKATON/IOT/firmware/platformio.ini):** Configura la placa `esp32-s3-devkitc-1` (apropiada para chips WROOM genéricos de 44 pines / tipo Freenove), habilita el soporte para PSRAM, define las velocidades de carga (`921600`) y monitoreo (`115200`), y descarga automáticamente las siguientes librerías desde el registro de PlatformIO:
    *   `paulstoffregen/OneWire` (Lectura del bus de datos OneWire).
    *   `milesburton/DallasTemperature` (Capa de abstracción para el sensor DS18B20).
    *   `bblanchon/ArduinoJson` (Serialización y estructuración eficiente del JSON de telemetría).

*   **[config.h](file:///c:/Users/Nexuz/Desktop/HACKATON/IOT/firmware/src/config.h):** Centraliza la configuración de red y variables globales.
    *   **SSID:** `"Build with AI"` (Configurada como red inalámbrica abierta/sin contraseña).
    *   **Variables de Red:** Contiene la variable modificable `BACKEND_URL` para redefinir el endpoint HTTPS fácilmente.
    *   **Variables de Negocio:** ID de Inquilino (`TENANT_USER_ID = "USR_HACK_2026"`).
    *   **Pines de Hardware:** Pin de datos OneWire (`ONE_WIRE_BUS = 4`).
    *   **Tiempos:** `READ_INTERVAL` (Lectura cada 10,000ms / 10s) y `TRANSMIT_INTERVAL` (Transmisión cada 30,000ms / 30s).
    *   **SIMULATION_MODE:** Bandera de control (`true` o `false`) que activa la simulación de 4 dispositivos virtuales.

*   **[main.cpp](file:///c:/Users/Nexuz/Desktop/HACKATON/IOT/firmware/src/main.cpp):** Contiene la lógica ejecutada por el ESP32-S3:
    *   **Identificación Única de MAC:** En tiempo de ejecución lee los bytes físicos de la interfaz WiFi usando `WiFi.macAddress()`, los formatea en un buffer hexadecimal para generar el `device_id` de forma dinámica con la nomenclatura `ESP32_MAC_<DIRECCIÓN_SIN_PUNTOS>`.
    *   **Frecuencia Asíncrona (millis):** El `loop()` evalúa constantemente el tiempo transcurrido desde el último evento.
    *   **Promediado de Muestras:** Cada 10s pide una lectura al sensor y la acumula en un sumador.
    *   **Bucle de Transmisión (Multi-Dispositivo):** Cada 30s, si `SIMULATION_MODE` es `true`, recorre secuencialmente 4 iteraciones simulando 4 dispositivos con identificadores de dispositivo únicos (añadiendo sufijos `_SIM_A`, `_SIM_B`, `_SIM_C`, `_SIM_D` a la MAC), IDs de inquilinos individuales y desfases de temperatura fijos con micro-fluctuaciones aleatorias (+-0.4°C) para modelar un entorno real y dinámico de múltiples sensores. Si está en `false`, transmite de forma nativa la lectura del sensor real único.
    *   **Reconexión Dinámica:** Antes de realizar el POST HTTP, valida que el WiFi siga en estado conectado. De lo contrario, ejecuta una subrutina no bloqueante para restablecer la conexión.

---

## **4. Estructura del Payload JSON Emitido**

El JSON enviado en el cuerpo de la petición HTTPS POST al backend tiene la siguiente estructura estricta:

```json
{
  "hardware": {
    "device_id": "ESP32_MAC_F46ADDE4D129",
    "firmware_version": "1.2.0-MINIMAL"
  },
  "tenant": {
    "user_id": "USR_HACK_2026"
  },
  "telemetry": {
    "temp_interior_c": 24.50,
    "temp_exterior_c": 29.50,
    "samples_averaged": 3
  },
  "diagnostics": {
    "uptime_s": 30,
    "wifi_rssi_dbm": -65
  }
}
```

*   `temp_interior_c` y `temp_exterior_c` son formateados dinámicamente con 2 decimales.
*   `samples_averaged` indica cuántas muestras válidas del sensor DS18B20 entraron en el promedio durante los 30 segundos.
*   `uptime_s` contabiliza el tiempo de encendido del procesador en segundos.
*   `wifi_rssi_dbm` mide la fuerza de la señal de red recibida en decibelios en el momento exacto del envío.

---

## **5. Manual de Operación y Desarrollo**

Para compilar o subir este firmware, siga estos pasos recomendados en VS Code con PlatformIO instalado:

### **Cargar el proyecto en VS Code**
1. Abra VS Code.
2. Haga clic en el botón de la extensión de PlatformIO (icono de hormiga).
3. Seleccione **Open** -> **Open Project**.
4. Navegue y seleccione la carpeta `firmware/` del proyecto.

### **Cambiar el Endpoint del Servidor**
Abra el archivo `firmware/src/config.h` y modifique la línea de la constante:
```cpp
const char* const BACKEND_URL = "https://tu-endpoint-especifico.com/api/telemetry";
```

### **Acciones del Ciclo de Vida**
*   **Compilar:** Presione `Ctrl + Alt + B` (o haga clic en el botón **✓** en la barra inferior).
*   **Subir Firmware:** Conecte el ESP32-S3 a un puerto USB de la computadora y presione el botón de la flecha derecha (**→**). PlatformIO detectará el puerto COM automáticamente e iniciará la carga a 921600 baudios.
*   **Depuración Serial:** Abra el monitor serie presionando el botón del enchufe en la barra inferior. Verifique los logs que imprimen la IP obtenida, el ID dinámico basado en MAC, las lecturas cada 10 segundos, la serialización del JSON y el código de estado HTTPS devuelto por tu servidor.
