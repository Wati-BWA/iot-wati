# Nodo IoT de Telemetría de Temperatura - ESP32-S3

Este repositorio contiene el firmware y la configuración de hardware para un **Nodo IoT de Telemetría** de temperatura basado en el microcontrolador **ESP32-S3** y el sensor digital **DS18B20** sobre bus OneWire. El nodo recopila lecturas de temperatura, calcula un promedio de muestras y las envía en formato JSON a través de HTTPS POST a un backend en la nube de forma asíncrona.

---

## 1. Características Principales

*   **ESP32-S3 Nativo**: Configurado y optimizado para placas de desarrollo genéricas (por ejemplo, clones WROOM de 44 pines / Freenove) habilitando soporte para PSRAM.
*   **Gestión Asíncrona con `millis()`**: Diseño no bloqueante. Las lecturas de temperatura y las transmisiones HTTP ocurren a intervalos independientes mediante temporizadores de software, sin congelar la ejecución con `delay()`.
*   **Promedio de Muestreo**: Muestrea la temperatura cada 10 segundos y calcula el promedio de todas las lecturas recolectadas antes de cada transmisión (cada 30 segundos).
*   **Simulación Multi-Dispositivo**: Incluye un modo configurable (`SIMULATION_MODE`) que emula 4 dispositivos virtuales individuales con direcciones MAC únicas, inquilinos (`tenant`) diferenciados y variaciones micro-aleatorias de temperatura a partir de un offset base. Ideal para pruebas de carga en el backend sin requerir hardware físico masivo.
*   **Reconexión WiFi Automática**: Detecta caídas en el enlace inalámbrico antes de cada envío de telemetría y restablece la conexión de manera no bloqueante.
*   **HTTPS Seguro y Simplificado**: Diseñado para el contexto ágil de una hackathon. Realiza solicitudes HTTPS cifradas utilizando `client.setInsecure()`, lo que elimina la necesidad de gestionar certificados raíz (CA) que expiran constantemente.

---

## 2. Conexiones y Arquitectura de Hardware

La conexión física del sensor DS18B20 al ESP32-S3 requiere una resistencia pull-up de **4.7kΩ** conectada entre las líneas **VCC** y **Señal (Datos)**. Muchos módulos comerciales DS18B20 para prototipado ya la traen integrada en su PCB.

### Tabla de Pinout

| Pin del Sensor DS18B20 | Pin en ESP32-S3 | Función | Rango Eléctrico / Tipo |
| :--- | :--- | :--- | :--- |
| **S (Señal / Datos)** | **GPIO 4** | Bus OneWire bidireccional | 3.3V Digital I/O |
| **VCC (Alimentación)** | **3.3V** | Alimentación eléctrica | 3.0V - 5.5V (Recomendado 3.3V) |
| **GND (Tierra)** | **GND** | Tierra común de referencia | 0V |

---

## 3. Estructura del Firmware (PlatformIO)

El firmware está estructurado bajo los estándares de **PlatformIO**:

```text
firmware/
├── platformio.ini      # Configuración de placa, PSRAM, velocidad serial y librerías
└── src/
    ├── config.h        # Constantes de negocio, WiFi, intervalos y configuración del modo simulación
    └── main.cpp        # Lógica central del sistema (WiFi, OneWire, ArduinoJson y HTTPS client)
```

### Librerías Requeridas (instaladas automáticamente por PlatformIO):
*   `paulstoffregen/OneWire` (Bus OneWire)
*   `milesburton/DallasTemperature` (Abstracción del DS18B20)
*   `bblanchon/ArduinoJson` (Serialización JSON rápida y eficiente)

---

## 4. Estructura del Payload JSON Emitido

Las peticiones HTTP POST envían un JSON estructurado con el siguiente formato estricto:

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

*   `device_id`: Generado dinámicamente en tiempo de ejecución a partir de la dirección MAC física del adaptador WiFi (sin dos puntos).
*   `telemetry.temp_interior_c` y `telemetry.temp_exterior_c`: Valores promediados formateados a 2 decimales.
*   `telemetry.samples_averaged`: Cantidad de muestras del sensor integradas en el promedio enviado.
*   `diagnostics.uptime_s`: Segundos transcurridos desde que inició el microcontrolador.
*   `diagnostics.wifi_rssi_dbm`: Calidad y fuerza de señal RSSI medida al momento de la transmisión.

---

## 5. Manual de Operación y Desarrollo

### Preparación del Entorno
1. Instale **VS Code** junto con la extensión **PlatformIO IDE**.
2. Clone este repositorio localmente.
3. En la barra lateral de PlatformIO, seleccione **Open Project** y abra la carpeta `firmware/`.

### Configuración Rápida (`src/config.h`)
*   **SSID y Contraseña**: Modifique `SSID` y `WIFI_PASSWORD` según la red local.
*   **Endpoint del Backend**: Defina el URL destino en la constante `BACKEND_URL`:
    ```cpp
    const char* const BACKEND_URL = "https://tu-endpoint.cloud/api/telemetry";
    ```
*   **Modo Simulación**: Active o desactive el modo de pruebas virtuales:
    ```cpp
    #define SIMULATION_MODE true // true = Emula 4 sensores; false = Lee sensor físico real
    ```

### Ciclo de Desarrollo
*   **Compilar**: Presione `Ctrl + Alt + B` (o el icono de la palomita `✓` en la barra inferior).
*   **Cargar en Placa**: Conecte el ESP32-S3 por USB y presione la flecha derecha (`→`).
*   **Monitor Serial**: Presione el icono del enchufe en la barra inferior (configurado a `115200` baudios) para visualizar la conexión a la red, obtención de dirección MAC, lecturas internas, estructura del JSON a enviar y códigos de respuesta HTTP devueltos por el servidor.
