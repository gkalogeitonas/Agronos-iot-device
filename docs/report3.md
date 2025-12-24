# Αναφορά 3

**Φοιτητής:** Καλογείτονας Γεωργιος

**Επιβλέπων Καθηγητής:** Νικόλαος Σκλάβος

**Ημερομηνία:** Δεκέμβριος 2025

---

## Περίληψη

Η παρούσα αναφορά εστιάζει αποκλειστικά στο υποσύστημα των IoT κόμβων (Device Nodes) της πλατφόρμας **Agronos**. Περιγράφει την αρχιτεκτονική του υλικολογισμικού (firmware) που αναπτύχθηκε για τον μικροελεγκτή ESP32, τις μεθόδους επικοινωνίας και τη στρατηγική "Smart Routing" για την αξιόπιστη αποστολή δεδομένων.

Η υλοποίηση του firmware βασίζεται στο **PlatformIO** και στη γλώσσα **C++**, ακολουθώντας αντικειμενοστραφή σχεδίαση για ευελιξία και επεκτασιμότητα.

## Σύνοψη Προηγούμενης Υλοποίησης (από Report 1)

Στην πρώτη αναφορά προόδου (`report.md`), είχε παρουσιαστεί η αρχική υλοποίηση του firmware για τον μικροελεγκτή ESP32. Συγκεκριμένα, είχαν καλυφθεί:

*   **Βασική Λειτουργία**: Ανάπτυξη σε PlatformIO (C++) με δυνατότητα ανάγνωσης αισθητήρων και αποστολής JSON payloads στο backend (`/api/v1/device/data`).
*   **Provisioning**: Υλοποίηση Captive Portal για την αρχική ρύθμιση των στοιχείων WiFi χωρίς hardcoding κωδικών.
*   **Αρχιτεκτονική Λογισμικού**:
    *   Χρήση του **Strategy Pattern** (`SensorBase`) για την ομοιόμορφη διαχείριση διαφορετικών αισθητήρων.
    *   Χρήση του **Factory Pattern** για τη δυναμική δημιουργία αντικειμένων αισθητήρων βάσει του `config.h`.
*   **Αυθεντικοποίηση**: Μηχανισμός αυτόματης σύνδεσης στο backend (Login) για την απόκτηση και αποθήκευση του JWT token σε μόνιμη μνήμη (Persistent Storage).

Η παρούσα αναφορά επεκτείνει την παραπάνω λειτουργικότητα, εισάγοντας προηγμένες δυνατότητες όπως το πρωτόκολλο MQTT, τη διαχείριση ενέργειας (Deep Sleep) και τη βελτιωμένη αρχιτεκτονική "Smart Routing".

## Αρχιτεκτονική Υλικολογισμικού (Firmware Architecture)

Το λογισμικό της συσκευής έχει σχεδιαστεί με αρθρωτή (modular) αρχιτεκτονική, διαχωρίζοντας τις ευθύνες σε διακριτά υποσυστήματα.

### Κύρια Συστατικά (Components)

1.  **Sensor Abstraction Layer**:
    *   Χρήση του προτύπου σχεδίασης **Factory Pattern** (`SensorFactory`) για τη δυναμική δημιουργία αντικειμένων αισθητήρων βάσει των ρυθμίσεων (`config.h`).
    *   Όλοι οι αισθητήρες κληρονομούν από τη βασική κλάση `SensorBase`, επιτρέποντας την ομοιόμορφη διαχείριση διαφορετικών τύπων αισθητήρων (π.χ. DHT11, Soil Moisture).


## Νέος Αισθητήρας: Υγρασία Εδάφους (SoilMoistureSensor)

Στο τελευταίο στάδιο της υλοποίησης προστέθηκε υποστήριξη για **αισθητήρα υγρασίας εδάφους** (capacitive soil moisture sensor, DFRobot SEN0193). Η υλοποίηση βρίσκεται στο `src/soil_moisture.cpp` και ενσωματώνεται στον υπάρχοντα μηχανισμό αισθητήρων χωρίς αλλαγές στο factory.

Βασικά τεχνικά σημεία:

*   **Είσοδος**: αναλογική ανάγνωση από pin ADC του ESP32 (στο παράδειγμα χρησιμοποιείται GPIO32 / ADC1).
*   **Σταθερότητα μέτρησης**: γίνεται averaging πολλαπλών αναγνώσεων (π.χ. 10 samples) για μείωση θορύβου.
*   **Βαθμονόμηση (Calibration)**: η μετατροπή σε ποσοστό απαιτεί δύο τιμές αναφοράς:
    *   `SOIL_MOISTURE_AIR_VALUE` → μέτρηση στον αέρα/ξηρό (dry)
    *   `SOIL_MOISTURE_WATER_VALUE` → μέτρηση σε νερό/πολύ υγρό (wet)
    Οι τιμές αυτές ορίζονται στο `include/config.h` και πρέπει να προκύψουν εμπειρικά για το συγκεκριμένο setup (τροφοδοσία, καλωδίωση, αισθητήρας).
*   **Έξοδος**: υπολογίζεται ποσοστό υγρασίας $0\%$ (dry) έως $100\%$ (wet) με clamping στο βαθμονομημένο εύρος.
*   **Εγγραφή στον Factory Registry**: ο αισθητήρας καταχωρείται αυτόματα με `registerSensorFactory("SoilMoistureSensor", create_sensor_impl<SoilMoistureSensor>)`, ώστε να μπορεί να ενεργοποιηθεί αποκλειστικά μέσω `SENSOR_CONFIGS`.

```c++
#include "sensor.h"
#include "config.h"
#include "sensor_creator.h"
#include <Arduino.h>

/**
 * DFRobot Capacitive Soil Moisture Sensor (SEN0193)
 * - Analog output: 0-3V
 * - Connected to GPIO32 (ADC1_CH4)
 * - Requires calibration (air value = dry, water value = wet)
 * - Returns moisture percentage: 0% (dry) to 100% (wet)
 */
class SoilMoistureSensor : public SensorBase {
public:
    SoilMoistureSensor(int pin, const char* uid)
    : pin_(pin), uuid_(uid) {}
    
    const char* uuid() const override { return uuid_; }
    
    bool read(float &out) override {
        // Average multiple readings for stability
        const int numSamples = 10;
        long sum = 0;
        
        for (int i = 0; i < numSamples; i++) {
            sum += analogRead(pin_);
            delay(10);
        }
        
        int rawValue = sum / numSamples;
        
        // Calibration values (MUST be determined empirically)
        // These are estimates scaled from Arduino 10-bit to ESP32 12-bit ADC
        // To calibrate:
        //   1. Expose sensor to air, note the raw value -> update airValue
        //   2. Submerge in water (to limit line), note raw value -> update waterValue
        const int airValue = SOIL_MOISTURE_AIR_VALUE;    // Dry soil (high value, low capacitance)
        const int waterValue = SOIL_MOISTURE_WATER_VALUE;  // Wet soil (low value, high capacitance)
        
        // Clamp to calibrated range
        if (rawValue > airValue) rawValue = airValue;
        if (rawValue < waterValue) rawValue = waterValue;
        
        // Convert to percentage: 100% = wet (waterValue), 0% = dry (airValue)
        float moisturePercent = 100.0f * (float)(airValue - rawValue) / (float)(airValue - waterValue);
        
        out = moisturePercent;
        return true;
    }
    
private:
    int pin_;
    const char* uuid_;
};

// Auto-register with factory using standard creator template
static bool _reg = registerSensorFactory("SoilMoistureSensor", create_sensor_impl<SoilMoistureSensor>);
```

Παράδειγμα ενεργοποίησης στο configuration:

```c++
{ "SoilMoistureSensor", 32, "Soil-Moisture-1" }
```

2.  **AuthManager**:
    *   Υπεύθυνος για την αυθεντικοποίηση της συσκευής με το backend.
    *   Διαχειρίζεται τον κύκλο ζωής του **JWT (JSON Web Token)**.

3.  **DataSender (Smart Router)**:
    *   Ο πυρήνας της λογικής αποστολής δεδομένων.
    *   Κατασκευάζει τα JSON payloads με τις μετρήσεις.
    *   Αποφασίζει δυναμικά ποιο πρωτόκολλο θα χρησιμοποιηθεί (MQTT ή HTTP) βάσει διαθεσιμότητας και ρυθμίσεων.

4.  **Storage (NVS)**:
    *   Διαχειρίζεται τη μόνιμη μνήμη (Non-Volatile Storage) του ESP32.
    *   Αποθηκεύει κρυπτογραφημένα τα διαπιστευτήρια WiFi, το JWT token και τα διαπιστευτήρια MQTT.

## Διαδικασία Provisioning & Authentication

Η διαδικασία ένταξης μιας νέας συσκευής στο δίκτυο (Provisioning) είναι πλήρως αυτοματοποιημένη και ασφαλής.

### Βήμα 1: WiFi Provisioning (Captive Portal)
Εάν η συσκευή δεν έχει αποθηκευμένα στοιχεία σύνδεσης WiFi, ενεργοποιεί αυτόματα ένα Access Point (AP) με όνομα `ESP_Config`. Ο χρήστης συνδέεται σε αυτό και μέσω μιας ιστοσελίδας (Captive Portal) εισάγει το SSID και τον κωδικό του δικτύου του.

### Βήμα 2: Device Login (HTTP)
Μόλις η συσκευή συνδεθεί στο διαδίκτυο, εκτελεί ένα HTTP POST αίτημα στο `/api/v1/device/login` χρησιμοποιώντας το μοναδικό `UUID` και `Secret` της. Το backend επιστρέφει ένα JWT token, το οποίο αποθηκεύεται τοπικά.

### Βήμα 3: MQTT Credential Provisioning
Εφόσον το MQTT είναι ενεργοποιημένο (`MQTT_ENABLED = true`), η συσκευή χρησιμοποιεί το JWT token για να ζητήσει τα ειδικά διαπιστευτήρια MQTT (Broker URL, Username, Password) από το backend. Αυτά τα στοιχεία είναι δυναμικά και αποθηκεύονται στη μνήμη NVS, ώστε η συσκευή να μπορεί να συνδέεται απευθείας στον Broker σε επόμενες εκκινήσεις.

## Στρατηγική Επικοινωνίας: Smart Routing

Για να εξασφαλιστεί η μέγιστη αξιοπιστία και εξοικονόμηση ενέργειας, η συσκευή υλοποιεί μια υβριδική στρατηγική επικοινωνίας.

### Ροή Απόφασης (Decision Flow)

1.  **Συλλογή Δεδομένων**: Η συσκευή ξυπνά από Deep Sleep, διαβάζει τους αισθητήρες και δημιουργεί το πακέτο δεδομένων.
2.  **Προσπάθεια MQTT (Προτιμώμενο)**:
    *   Η συσκευή ελέγχει αν έχει αποθηκευμένα διαπιστευτήρια MQTT.
    *   Επιχειρεί σύνδεση στον MQTT Broker (θύρα 1883 ή 8883 για TLS).
    *   Δημοσιεύει τα δεδομένα στο θέμα `devices/{uuid}/sensors` με **QoS 1** (At least once).
3.  **HTTP Fallback (Εφεδρικό)**:
    *   Εάν η σύνδεση MQTT αποτύχει ή δεν υπάρχουν διαπιστευτήρια, το `DataSender` μεταπίπτει αυτόματα σε HTTP λειτουργία.
    *   Στέλνει τα δεδομένα μέσω HTTP POST στο `/api/v1/device/data` χρησιμοποιώντας το JWT για αυθεντικοποίηση.

Αυτή η προσέγγιση συνδυάζει την ταχύτητα και το χαμηλό overhead του MQTT με την καθολική συμβατότητα και αξιοπιστία του HTTP ως δικλείδα ασφαλείας.

## Διαχείριση Ενέργειας (Deep Sleep)

Λόγω της φύσης των γεωργικών εφαρμογών (απομακρυσμένα σημεία, πιθανή χρήση μπαταρίας), η συσκευή έχει βελτιστοποιηθεί για χαμηλή κατανάλωση.

*   Μετά από κάθε κύκλο μέτρησης και αποστολής, η συσκευή εισέρχεται σε κατάσταση **Deep Sleep**.
*   Όλα τα περιφερειακά (WiFi, Bluetooth, Sensors) απενεργοποιούνται.
*   Η διάρκεια ύπνου καθορίζεται από την παράμετρο `SENSORS_READ_INTERVAL_MS` (π.χ. 2 λεπτά).
*   Η μνήμη RTC χρησιμοποιείται για τη διατήρηση κρίσιμων μετρητών μεταξύ των κύκλων ύπνου.
*   **Wake-up από κουμπί**: Η συσκευή μπορεί να ξυπνήσει άμεσα από το Deep Sleep με πάτημα του φυσικού κουμπιού (GPIO 14), επιτρέποντας μετρήσεις κατά παραγγελία.

## Λειτουργία Κουμπιού Επαναφοράς (Reset Button)

Για την ευκολία της συντήρησης και της αρχικής ρύθμισης, η συσκευή διαθέτει φυσικό κουμπί επαναφοράς.

### Υλικό (Hardware)
*   **Σύνδεση**: Ένα momentary push button συνδέεται μεταξύ του GPIO 14 και GND.
*   **Pull-up Resistor**: Χρησιμοποιείται ο εσωτερικός pull-up αντιστάτης του ESP32 (`INPUT_PULLUP`).
*   **Λογική**: Το κουμπί είναι ενεργό σε χαμηλή στάθμη (active-low) — όταν πατηθεί, το pin διαβάζεται ως `LOW`.

### Λειτουργίες

1.  **Factory Reset (Επαναφορά Εργοστασιακών Ρυθμίσεων)**:
    *   Κατά την εκκίνηση της συσκευής, εάν το κουμπί παραμείνει πατημένο για **10 δευτερόλεπτα** (ρυθμίζεται από την παράμετρο `BUTTON_LONG_PRESS_MS` στο `config.h`), η συσκευή διαγράφει όλα τα αποθηκευμένα δεδομένα:
        *   Διαπιστευτήρια WiFi (SSID, password)
        *   JWT authentication token
        *   MQTT credentials (broker, username, password)
    *   Μετά τη διαγραφή, η συσκευή επανεκκινείται αυτόματα και εισέρχεται σε λειτουργία provisioning (Captive Portal).

2.  **Wake from Deep Sleep (Αφύπνιση από Deep Sleep)**:
    *   Το κουμπί λειτουργεί ως wake-up source όταν η συσκευή βρίσκεται σε κατάσταση Deep Sleep.
    *   Ένα πάτημα του κουμπιού ξυπνά άμεσα τη συσκευή, επιτρέποντας μετρήσεις και αποστολή δεδομένων κατά παραγγελία, χωρίς αναμονή του προγραμματισμένου timer.

### Υλοποίηση

Η λειτουργία υλοποιείται στο `src/main.cpp`:

```cpp
// Button pin definition
constexpr int BUTTON_PIN = 14; // GPIO 14 for button

// Check if button is held for more than 10 seconds to reset all storage
static void checkButtonReset() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Button pressed, checking for long press...");
        unsigned long pressStart = millis();
        
        while (digitalRead(BUTTON_PIN) == LOW) {
            if (millis() - pressStart >= BUTTON_LONG_PRESS_MS) {
                Serial.println("Long press detected! Wiping all storage data...");
                storage.clearAll();
                Serial.println("Storage cleared. Restarting device...");
                delay(1000);
                ESP.restart();
            }
            delay(100);
        }
        Serial.println("Button released before 10 seconds");
    }
}
```

Η ρουτίνα `checkButtonReset()` καλείται νωρίς στο `setup()`, πριν από την προσπάθεια σύνδεσης WiFi, ώστε να παρέχει άμεση δυνατότητα επαναφοράς. Για το wake-up από Deep Sleep, χρησιμοποιείται η συνάρτηση `esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0)` πριν από την είσοδο σε ύπνο.

### Πλεονεκτήματα
*   **Χωρίς επαναφλασάρισμα (No Reflashing)**: Επαναφορά χωρίς ανάγκη επαναπρογραμματισμού του firmware.
*   **Απλό Hardware**: Μόνο ένα φθηνό momentary button.
*   **Ευελιξία**: Το χρονικό όριο (`BUTTON_LONG_PRESS_MS`) είναι ρυθμιζόμενο μέσω του `config.h`.
*   **On-demand Readings**: Δυνατότητα άμεσης αφύπνισης για έλεγχο ή αποστολή δεδομένων.

## Μελλοντικές Επεκτάσεις

*   **OTA Updates**: Υποστήριξη Over-The-Air αναβαθμίσεων του firmware.
*   **LoRaWAN**: Διερεύνηση ενσωμάτωσης LoRa για περιοχές χωρίς κάλυψη WiFi.
*   **Local Buffering**: Αποθήκευση μετρήσεων στην flash μνήμη όταν δεν υπάρχει καθόλου δίκτυο και μαζική αποστολή κατά την επανασύνδεση.
