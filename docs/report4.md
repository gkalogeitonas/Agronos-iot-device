# Αναφορά 4

**Repository:** https://github.com/gkalogeitonas/Agronos-iot-device

**Φοιτητής:** Καλογείτονας Γεωργιος

**Επιβλέπων Καθηγητής:** Νικόλαος Σκλάβος

**Ημερομηνία:** Ιανουάριος 2026

---

## Περίληψη

Η παρούσα αναφορά καταγράφει την επέκταση της πλατφόρμας **Agronos** με την υποστήριξη νέου υλικού (hardware) και νέων αισθητήρων. Συγκεκριμένα, υλοποιήθηκε η υποστήριξη για την αναπτυξιακή πλακέτα **DFRobot FireBeetle 2 ESP32-C6**, η οποία προσφέρει σημαντικά πλεονεκτήματα διαχείρισης ενέργειας και συνδεσιμότητας. Παράλληλα, ενσωματώθηκε ο ψηφιακός αισθητήρας θερμοκρασίας και υγρασίας **DHT20**, ο οποίος λειτουργεί μέσω πρωτοκόλλου I2C, προσφέροντας μεγαλύτερη ακρίβεια και σταθερότητα. Η αρχιτεκτονική του λογισμικού προσαρμόστηκε ώστε να υποστηρίζει πολλαπλούς στόχους (multi-target build) από την ίδια βάση κώδικα.

## Σύνοψη Προηγούμενης Υλοποίησης (από Report 3)

Στην προηγούμενη αναφορά, είχε ολοκληρωθεί η ανάπτυξη του firmware για τον κλασικό μικροελεγκτή ESP32, εστιάζοντας  στο Captive Portal για δυναμική παραμετροποίηση και στον μηχανισμό Deep Sleep για εξοικονόμηση ενέργειας. Κεντρικό σημείο ήταν η χρήση Factory Pattern για τη διαχείριση αισθητήρων.

## Υποστήριξη Νέου Υλικού: DFRobot FireBeetle 2 ESP32-C6

Η μετάβαση από το κλασικό ESP32 DevKit στο **DFRobot FireBeetle 2 ESP32-C6** αποτελεί σημαντική αναβάθμιση για έναν κόμβο IoT που προορίζεται για αγροτικές εφαρμογές (Agronos), όπου η ενεργειακή αυτονομία είναι κρίσιμη.

### Πλεονεκτήματα της Πλακέτας (FireBeetle 2)
1.  **Διαχείριση Ενέργειας & Ηλιακή Φόρτιση**: Η πλακέτα ενσωματώνει εξειδικευμένο κύκλωμα διαχείρισης ενέργειας (με το chip CN3165) που επιτρέπει την απευθείας σύνδεση ηλιακών πάνελ (5V) και μπαταρίας Li-Ion. Αυτό βελτιστοποιεί τη φόρτιση και εξασφαλίζει την αυτόνομη λειτουργία στο χωράφι χωρίς εξωτερικούς φορτιστες.
2.  **Ultra-Low Power**: Σε κατάσταση Deep Sleep, η κατανάλωση της πλακέτας πέφτει στα **16µA**, σε σύγκριση με τα ~100-150µA των τυπικών ESP32 boards.
3.  **Παρακολούθηση Μπαταρίας**: Διαθέτει ενσωματωμένο διαιρέτη τάσης συνδεδεμένο σε αναλογική είσοδο, επιτρέποντας στο firmware να παρακολουθεί και να αποστέλλει το ποσοστό μπαταρίας στο backend.

### Πλεονεκτήματα του Μικροελεγκτή (ESP32-C6)
1.  **Αρχιτεκτονική RISC-V**: Ο ESP32-C6 βασίζεται σε πυρήνα RISC-V 32-bit (160MHz), προσφέροντας υψηλή απόδοση με χαμηλότερη κατανάλωση ενέργειας.



### Τεχνικές Λεπτομέρειες Υλοποίησης

Για να υποστηριχθούν και οι δύο πλακέτες (Standard ESP32 και ESP32-C6) από τον ίδιο κώδικα, έγιναν οι εξής αρχιτεκτονικές παρεμβάσεις:

1.  **Multi-Environment PlatformIO**: Το αρχείο `platformio.ini` αναδιοργανώθηκε ώστε να περιέχει κοινές ρυθμίσεις (`[env]`) και ξεχωριστά sections για κάθε πλακέτα (`[env:esp32dev]` και `[env:esp32-c6-devkitc-1]`). Ο προγραμματιστής μπορεί να επιλέξει πού θα κάνει compile/upload.

2.  **Conditional Compilation & Pin Mapping**:
    Στο `config.h`, χρησιμοποιήθηκαν preprocessor directives για τον ορισμό των Pins ανάλογα με τον στόχο (Target):
    ```cpp
    #if defined(CONFIG_IDF_TARGET_ESP32C6)
    constexpr int I2C_SDA = 19;
    constexpr int I2C_SCL = 20;
    #else
    constexpr int I2C_SDA = 21;
    constexpr int I2C_SCL = 22;
    #endif
    ```

3.  **Διαχείριση Deep Sleep Wakeup**:
    Ο μηχανισμός αφύπνισης διαφέρει μεταξύ των γενιών ESP32. Ο κλασικός ESP32 χρησιμοποιεί το `esp_sleep_enable_ext0_wakeup`, το οποίο όμως δεν υποστηρίζεται στον C6.
    Ο κώδικας στο `main.cpp` προσαρμόστηκε ώστε να χρησιμοποιεί τη νεότερη μέθοδο `esp_sleep_enable_ext1_wakeup` όταν γίνεται compile για ESP32-C6, εξασφαλίζοντας ότι το κουμπί λειτουργεί ως πηγή αφύπνισης και στις δύο περιπτώσεις.

4.  **Δυναμική Αρχικοποίηση I2C**:
    Η κλάση του αισθητήρα DHT20 δεν χρησιμοποιεί τις default τιμές της βιβλιοθήκης `Wire`, αλλά καλεί ρητά την `Wire.begin(I2C_SDA, I2C_SCL)` με τις παραμέτρους που ορίστηκαν στο configuration, λύνοντας προβλήματα συνδεσιμότητας σε πλακέτες με μη τυπικό pinout (όπως η FireBeetle C6).

    

## Αναδιοργάνωση Ρυθμίσεων και Προφίλ Συσκευής

Πρόσφατα οργανώθηκε η διαχείριση ρυθμίσεων και προφίλ συσκευής για ευκολότερη ανάπτυξη και ασφαλέστερο workflow:

- **Συγκέντρωση προφίλ συσκευής**: Τα `DEFAULT_UUID`, `DEFAULT_SECRET` και ο πίνακας `SENSOR_CONFIGS` συγχωνεύτηκαν σε ένα αρχείο `device_profile.h` (και το αντίστοιχο `device_profile.h.example`). Αυτό το αρχείο είναι ειδικό για κάθε συσκευή και **αγνοείται από το git**.
- **Απλοποίηση `config.h`**: Το `config.h` τώρα περιέχει μόνο κοινές, μη ευαίσθητες ρυθμίσεις (pins, MQTT, calibration) και κάνει `#include "device_profile.h"` για την πρόσβαση στο προφίλ της συσκευής.
- **Έλεγχος build**: Το project χτίστηκε επιτυχώς μετά τις αλλαγές για το περιβάλλον `esp32-c6-devkitc-1`.


## Νέος Αισθητήρας: DHT20 (I2C)

Προστέθηκε υποστήριξη για τον αισθητήρα **DHT20**, ο οποίος αποτελεί την αναβαθμισμένη, βιομηχανική έκδοση του δημοφιλούς DHT11.

*   **Πρωτόκολλο I2C**: Σε αντίθεση με το ιδιότυπο πρωτόκολλο  του DHT11 που απαιτεί ακριβή χρονισμό (timing sensitive), ο DHT20 χρησιμοποιεί το στάνταρ πρωτόκολλο I2C. Αυτό εξαλείφει τα errors ανάγνωσης λόγω καθυστερήσεων του λειτουργικού συστήματος.
*   **Υλοποίηση (Guarded Read)**: Επειδή ο DHT20 απαιτεί τουλάχιστον 2 δευτερόλεπτα μεταξύ των μετρήσεων, υλοποιήθηκε ένας μηχανισμός "Guarded Read" (`readSharedDHT20`). Ανεξάρτητα από το πόσες φορές ή πόσο γρήγορα ζητηθεί μέτρηση από το κυρίως loop, η συνάρτηση εκτελεί I2C transaction μόνο αν έχει παρέλθει ο απαιτούμενος χρόνος, επιστρέφοντας cached τιμές ενδιάμεσα.

```cpp
#include "sensor.h"
#include "config.h"
#include "sensor_creator.h"
#include <Arduino.h>
#include <DHT20.h>
#include <Wire.h>

/**
 * DHT20 Temperature and Humidity Sensor (I2C)
 * - Uses robtillaart/DHT20 library
 * - Default I2C address is 0x38
 * - Multiple readers share the same DHT20 instance to avoid I2C conflicts
 */

static DHT20* getSharedDHT20() {
    static DHT20* dht = nullptr;
    if (!dht) {
        // Initialize I2C with pins defined in config.h
        Wire.begin(I2C_SDA, I2C_SCL); 
        dht = new DHT20(&Wire);
        if (!dht->begin()) {
            Serial.println("DHT20: Failed to initialize sensor!");
            return nullptr;
        }
        Serial.println("DHT20: Sensor initialized");
    }
    return dht;
}

// Guarded read wrapper: only performs I2C read if 2+ seconds have passed
// Both temperature and humidity readers call this, but only the first triggers the actual read
static bool readSharedDHT20() {
    static unsigned long lastReadTime = 0;
    DHT20* dht = getSharedDHT20();
    if (!dht) return false;
    
    unsigned long now = millis();
    const unsigned long READ_INTERVAL = 2000; // DHT20 requires 2 seconds between reads
    
    // Only perform an actual read if 2+ seconds have passed
    if (now - lastReadTime >= READ_INTERVAL) {
        int status = dht->read();
        if (status != DHT20_OK) {
            Serial.print("DHT20 Read Error: ");
            Serial.println(status);
            return false;
        }
        lastReadTime = now;
    }
    
    return true;
}

class DHT20TemperatureReader : public SensorBase {
public:
    DHT20TemperatureReader(int pin, const char* uid)
    : uuid_(uid), dht_(getSharedDHT20()) {}
    
    const char* uuid() const override { return uuid_; }
    
    bool read(float &out) override {
        if (!dht_) return false;
        
        // Use guarded read (only performs I2C transaction once per 2 seconds)
        if (!readSharedDHT20()) return false;
        
        out = dht_->getTemperature();
        return true;
    }
    
private:
    const char* uuid_;
    DHT20* dht_;
};

class DHT20HumidityReader : public SensorBase {
public:
    DHT20HumidityReader(int pin, const char* uid)
    : uuid_(uid), dht_(getSharedDHT20()) {}
    
    const char* uuid() const override { return uuid_; }
    
    bool read(float &out) override {
        if (!dht_) return false;
        
        // Use guarded read (only performs I2C transaction once per 2 seconds)
        if (!readSharedDHT20()) return false;
        
        out = dht_->getHumidity();
        return true;
    }
    
private:
    const char* uuid_;
    DHT20* dht_;
};

// Register the factory creators
static bool _reg_temp = registerSensorFactory("DHT20TemperatureReader", create_sensor_impl<DHT20TemperatureReader>);
static bool _reg_hum = registerSensorFactory("DHT20HumidityReader", create_sensor_impl<DHT20HumidityReader>);
```

## Νέος Αισθητήρας: Παρακολούθηση Επιπέδου Μπαταρίας (Battery Level Sensor)

Προστέθηκε αισθητήρας για την παρακολούθηση του επιπέδου μπαταρίας στο **DFRobot FireBeetle 2 ESP32-C6**, ο οποίος είναι κρίσιμος για την αυτόνομη λειτουργία σε αγροτικές εφαρμογές.

### Τεχνικά Χαρακτηριστικά

*   **Υλικό (Hardware)**:
    - Η πλακέτα FireBeetle 2 διαθέτει ενσωματωμένο διαιρέτη τάσης (voltage divider) με αναλογία 2:1
    - Η τάση της μπαταρίας μετράται μέσω του **ADC0 (GPIO 0)**
    - 12-bit ADC resolution (0-4095) για ακριβείς μετρήσεις
    - Συμβατό με **Polymer Lithium Ion Battery 3.7V 1200mAh**

*   **Υλοποίηση**:
    - Μέση τιμή 10 μετρήσεων για σταθερότητα και μείωση θορύβου
    - Αυτόματη διόρθωση με βάση τον διαιρέτη τάσης (πραγματική τάση = 2x ADC reading)
    - Χρήση **piecewise linear approximation** της καμπύλης εκφόρτισης LiPo (discharge curve)

*   **Χαρτογράφηση Τάσης σε Ποσοστό**:
    Η μετατροπή της τάσης σε ποσοστό μπαταρίας ακολουθεί τη ρεαλιστική καμπύλη εκφόρτισης μιας μπαταρίας Li-Ion:
    - **4.2V = 100%** (πλήρως φορτισμένη)
    - **4.0V = 90%** (γρήγορη πτώση στην κορυφή)
    - **3.7V = 50%** (ονομαστική τάση)
    - **3.4V = 20%** (μέτρια πτώση)
    - **3.0V = 0%** (cutoff voltage - ελάχιστη ασφαλής τάση)

*   **Πλεονεκτήματα**:
    - Ακριβέστερη εκτίμηση από απλή γραμμική χαρτογράφηση
    - Επιτρέπει στο backend να προβλέπει πότε θα χρειαστεί φόρτιση ή συντήρηση
    - Συνεργάζεται με το MQTT/HTTP routing για αποστολή δεδομένων
    - Αυτόματη καταχώρηση στο sensor factory pattern

### Παράδειγμα Χρήσης

```cpp
// Στο device_profile.h:
constexpr SensorConfig SENSOR_CONFIGS[] = {
    { "DHT20TemperatureReader", -1, "DHT20-Temp-1", "DHT20 Temperature" },
    { "DHT20HumidityReader", -1, "DHT20-Hum-1", "DHT20 Humidity" },
    { "BatteryLevelSensor", 0, "Battery-Level-1", "Battery Level" }
};
```

Ο αισθητήρας επιστρέφει το ποσοστό μπαταρίας (0-100%) και ενσωματώνεται απρόσκοπτα με την υπόλοιπη αρχιτεκτονική του firmware.



