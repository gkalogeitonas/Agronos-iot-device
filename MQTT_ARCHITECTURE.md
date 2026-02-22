# MQTT Architecture Diagram

## Complete System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         ESP32 Device                             │
│                                                                   │
│  ┌────────────┐      ┌────────────┐      ┌────────────┐        │
│  │   Sensors  │─────▶│  main.cpp  │◀────▶│   Storage  │        │
│  │ (DHT11/22) │      │            │      │    (NVS)   │        │
│  └────────────┘      └─────┬──────┘      └─────┬──────┘        │
│                            │                    │                │
│                            ▼                    │                │
│                     ┌──────────────┐            │                │
│                     │ AuthManager  │            │                │
│                     │  (HTTP/JWT)  │            │                │
│                     └──────┬───────┘            │                │
│                            │                    │                │
│                            ▼                    │                │
│              ┌─────────────────────────┐       │                │
│              │  JWT Token Obtained     │       │                │
│              └──────────┬──────────────┘       │                │
│                         │                      │                │
│                         ▼                      │                │
│              ┌──────────────────────┐          │                │
│              │ MQTT Enabled?        │          │                │
│              └──────┬───────────────┘          │                │
│                     │ YES                      │                │
│                     ▼                          │                │
│           ┌────────────────────┐               │                │
│           │ MqttProvisioner    │               │                │
│           │ GET /mqtt-creds    │               │                │
│           │ (Bearer JWT)       │               │                │
│           └────────┬───────────┘               │                │
│                    │                           │                │
│                    ▼                           │                │
│           ┌────────────────────┐               │                │
│           │ Store MQTT Creds   │──────────────▶│                │
│           │ server, user, pass │               │                │
│           └────────┬───────────┘               │                │
│                    │                           │                │
│                    ▼                           │                │
│         ┌──────────────────────┐               │                │
│         │   DataSender         │               │                │
│         │       │               │                │
│         └──────┬────────┬──────┘               │                │
│                │        │                      │                │
│       Has MQTT?│        │No MQTT               │                │
│                ▼        ▼                      │                │
│      ┌──────────┐  ┌──────────┐               │                │
│      │  MQTT    │  │   HTTP   │               │                │
│      │ Client   │  │  Sender  │               │                │
│      └────┬─────┘  └────┬─────┘               │                │
│           │             │                      │                │
└───────────┼─────────────┼──────────────────────┘                │
            │             │                                        │
            │             │                                        │
            ▼             ▼                                        │
     ┌──────────┐   ┌──────────┐                                  │
     │   MQTT   │   │   HTTP   │                                  │
     │  Broker  │   │  Server  │                                  │
     └────┬─────┘   └────┬─────┘                                  │
          │              │                                         │
          └──────┬───────┘                                         │
                 ▼                                                 │
         ┌────────────────┐                                        │
         │    Backend     │                                        │
         │   Platform     │                                        │
         └────────────────┘                                        │
```

## Detailed Flow Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│ 1. DEVICE STARTUP                                                 │
└──────────────────────────────────────────────────────────────────┘
                         │
                         ▼
              ┌──────────────────┐
              │  WiFi Connect    │
              └────────┬─────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────────┐
│ 2. HTTP AUTHENTICATION                                            │
└──────────────────────────────────────────────────────────────────┘
                       │
      POST /api/v1/device/login
      {uuid, secret}
                       │
                       ▼
         ┌─────────────────────┐
         │ JWT Token Received  │
         │ Stored in NVS       │
         └──────────┬──────────┘
                    │
                    ▼
┌──────────────────────────────────────────────────────────────────┐
│ 3. MQTT CREDENTIAL PROVISIONING (if MQTT_ENABLED)                 │
└──────────────────────────────────────────────────────────────────┘
                    │
       GET /api/v1/device/mqtt-credentials
       Authorization: Bearer {JWT}
                    │
                    ▼
         ┌─────────────────────┐
         │ MQTT Credentials    │
         │ - server            │
         │ - username          │
         │ - password          │
         │ Stored in NVS       │
         └──────────┬──────────┘
                    │
                    ▼
┌──────────────────────────────────────────────────────────────────┐
│ 4. SENSOR DATA COLLECTION                                         │
└──────────────────────────────────────────────────────────────────┘
                    │
         ┌──────────▼──────────┐
         │  Read Sensors       │
         │  DHT11 Temp & Hum   │
         └──────────┬──────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │  Create Readings     │
         │  [{uuid, value}, ...]│
         └──────────┬───────────┘
                    │
                    ▼
┌──────────────────────────────────────────────────────────────────┐
│ 5. DATA TRANSMISSION                              │
└──────────────────────────────────────────────────────────────────┘
                    │
         ┌──────────▼──────────┐
         │ DataSender::        │
         │ sendReadings()      │
         └──────────┬──────────┘
                    │
         ┌──────────▼──────────┐
         │ MQTT Enabled? AND   │
         │ Has MQTT Creds?     │
         └──────┬────────┬─────┘
             YES│        │NO
                │        │
    ┌───────────▼──┐     │
    │ Try MQTT     │     │
    │ Connect      │     │
    └───────┬──────┘     │
            │            │
    ┌───────▼──────┐     │
    │ MQTT         │     │
    │ Connected?   │     │
    └───┬──────────┘     │
     YES│  NO            │
        │  │             │
    ┌───▼──▼─────────────▼─────┐
    │ Publish to MQTT          │
    │ agronos/{uuid}/data      │
    └───────┬──────────────────┘
            │
    ┌───────▼──────┐
    │ Success?     │
    └───┬──────┬───┘
     YES│      │NO
        │      │
    ┌───▼──┐   │
    │ Done │   │
    └──────┘   │
               │
    ┌──────────▼──────────┐
    │ HTTP Fallback       │
    │ POST /device/data   │
    │ Bearer {JWT}        │
    └──────────┬──────────┘
               │
    ┌──────────▼──────────┐
    │ Success? → Done     │
    │ Failure? → Retry    │
    └─────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│ 6. DEEP SLEEP                                                     │
└──────────────────────────────────────────────────────────────────┘
               │
    ┌──────────▼──────────┐
    │ Disconnect WiFi     │
    │ Disconnect MQTT     │
    └──────────┬──────────┘
               │
    ┌──────────▼──────────┐
    │ esp_deep_sleep()    │
    │ for N minutes       │
    └──────────┬──────────┘
               │
               ▼
         (Back to Step 1)
```

## Component Interaction Diagram

```
┌─────────────────┐
│   main.cpp      │
│   (loop)        │
└────────┬────────┘
         │
         ├─────────────────┐
         │                 │
         ▼                 ▼
┌─────────────────┐  ┌──────────────┐
│  AuthManager    │  │  Storage     │
│  - loop()       │  │  - NVS       │
│  - JWT auth     │  │  - WiFi      │
└────────┬────────┘  │  - Token     │
         │           │  - MQTT      │
         │           └──────────────┘
         │                 ▲
         ▼                 │
┌─────────────────┐        │
│ MqttProvisioner │────────┘
│ - fetch creds   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  MqttClient     │
│  - connect()    │
│  - publish()    │
│  - loop()       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  DataSender     │◀───────── Sensors
│  - try MQTT     │
│  - fallback HTTP│
└─────────────────┘
```

## Storage Structure

```
ESP32 NVS (Non-Volatile Storage)
├── Namespace: "wifi"
│   ├── Key: "ssid"     → WiFi SSID
│   └── Key: "pass"     → WiFi Password
│
├── Namespace: "auth"
│   └── Key: "token"    → JWT Token
│
└── Namespace: "mqtt"
    ├── Key: "server"   → MQTT Broker Address
    ├── Key: "username" → MQTT Username
    └── Key: "password" → MQTT Password
```

## MQTT Topic Structure

```
agronos/
└── {device-uuid}/
    ├── data           [PUBLISH - QoS 1]
    │   └── Sensor readings
    │
    ├── status         [PUBLISH - QoS 0]
    │   └── Device status/heartbeat
    │
    └── command        [SUBSCRIBE - QoS 1]
        └── Commands from backend
```

## Message Flow Examples

### Example 1: MQTT Success Path

```
Device                  MQTT Broker              Backend
  │                          │                       │
  │──── Connect ────────────▶│                       │
  │                          │                       │
  │◀─── Connected ──────────│                       │
  │                          │                       │
  │──── PUBLISH ────────────▶│                       │
  │  agronos/123/data        │                       │
  │  {sensors:[...]}         │                       │
  │                          │                       │
  │                          │──── Forward ─────────▶│
  │                          │                       │
  │◀─── PUBACK ─────────────│                       │
  │                          │                       │
  │──── Disconnect ─────────▶│                       │
  │                          │                       │
  │──── Deep Sleep           │                       │
```

### Example 2: MQTT Failure → HTTP Fallback

```
Device              MQTT Broker       HTTP Server
  │                      │                 │
  │──── Connect ────────▶│                 │
  │                      X (timeout)       │
  │                                        │
  │──── POST /device/data ────────────────▶│
  │      Bearer {JWT}                      │
  │      {sensors:[...]}                   │
  │                                        │
  │◀─── 200 OK ───────────────────────────│
  │                                        │
  │──── Deep Sleep                         │
```

## Class Hierarchy

```
Storage
  ├── getWifiCreds()
  ├── setWifiCreds()
  ├── getToken()
  ├── setToken()
  ├── getMqttCredentials()      [NEW]
  ├── setMqttCredentials()      [NEW]
  ├── hasMqttCredentials()      [NEW]
  └── clearMqttCredentials()    [NEW]

AuthManager
  ├── loop()
  ├── tryAuthenticateOnce()
  └── performAuthRequest()

MqttProvisioner                 [NEW]
  ├── fetchMqttCredentials()
  ├── buildUrl()
  └── parseCredentialsResponse()

MqttClient                      [NEW]
  ├── connect()
  ├── isConnected()
  ├── disconnect()
  ├── loop()
  ├── publishSensorData()
  ├── publishStatus()
  ├── loadCredentials()
  ├── reconnect()
  └── mqttCallback()

DataSender                      [UPDATED]
  ├── setMqttClient()           [NEW]
  ├── sendReadings()            [UPDATED]
  ├── sendViaPreferredProtocol() [NEW]
  ├── sendValuesWithUuids()
  ├── sendValues()
  ├── buildUrl()
  └── postJson()
```

---

## Decision Tree: When to Use MQTT vs HTTP

```
                Start
                  │
                  ▼
        ┌─────────────────┐
        │ MQTT_ENABLED?   │
        └────┬─────────┬──┘
         NO  │         │ YES
             │         │
             │         ▼
             │  ┌──────────────────┐
             │  │ Has MQTT Creds?  │
             │  └────┬──────────┬──┘
             │   NO  │          │ YES
             │       │          │
             │       │          ▼
             │       │   ┌──────────────┐
             │       │   │ MQTT Connect │
             │       │   └────┬─────┬───┘
             │       │    OK  │     │ FAIL
             │       │        │     │
             │       │        ▼     │
             │       │   ┌──────────▼────┐
             │       │   │ MQTT Publish  │
             │       │   └────┬──────┬───┘
             │       │    OK  │      │ FAIL
             │       │        │      │
             │       │        ▼      │
             │       │   ┌───────────┐
             │       │   │   Done    │
             │       │   └───────────┘
             │       │              │
             └───────┴──────────────┘
                     │
                     ▼
              ┌─────────────┐
              │ Use HTTP    │
              └─────────────┘
```

This architecture ensures maximum reliability with automatic fallback!
