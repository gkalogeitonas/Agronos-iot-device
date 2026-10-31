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
         │ - topic_base        │
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
    │ {topic_base}/sensors     │
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
    ├── Key: "username"   → MQTT Username
    ├── Key: "password"   → MQTT Password
    └── Key: "topic_base" → Topic base, e.g. agronos/devices/{uuid}
```

## MQTT Topic Structure

Topics are not hard-coded in the firmware. The MQTT credentials response
(`GET /api/v1/device/mqtt-credentials`) returns a `topic_base`, e.g.
`agronos/devices/{uuid}`, where `agronos` is the Agronos topic prefix
(configurable on the backend, see agronos ADR-0007). The firmware appends a
suffix to it, so a prefix change needs no firmware release.

```
{topic_base} = {prefix}/devices/{device-uuid}     (default prefix: agronos)
├── sensors        [PUBLISH - QoS 1]
│   └── Sensor readings
│
├── status         [PUBLISH - QoS 0]
│   └── Device status/heartbeat
│
└── commands       [reserved - device is publish-only today]
    └── Commands from backend
```

A response without `topic_base` is treated as a provisioning failure: nothing is
stored, the device does not publish over MQTT and sends data via HTTP instead.
There is no fallback to the old `devices/{uuid}/...` topics.

**ACL**: Agronos writes a per-user broker ACL allowing `agronos/devices/<username>/#`.
If publishes are rejected, check that rule and that `topic_base` matches it.

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
  │  agronos/devices/123/    │
  │    sensors               │                       │
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
