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

2.  **WifiPortal (Enhanced Captive Portal)**:
    *   Υπεύθυνος για την αρχική ρύθμιση WiFi και τη ρύθμιση παραμέτρων συσκευής σε πραγματικό χρόνο (runtime configuration).
    *   Σαρώνει αυτόματα διαθέσιμα δίκτυα WiFi και τα εμφανίζει σε dropdown menu.
    *   Παρέχει ενοποιημένη φόρμα για WiFi credentials και device configuration (Server URL, Read Interval, MQTT Enable/Disable).

3.  **AuthManager**:
    *   Υπεύθυνος για την αυθεντικοποίηση της συσκευής με το backend.
    *   Διαχειρίζεται τον κύκλο ζωής του **JWT (JSON Web Token)**.

3.  **DataSender (Smart Router)**:
    *   Ο πυρήνας της λογικής αποστολής δεδομένων.
    *   Κατασκευάζει τα JSON payloads με τις μετρήσεις.
    *   Αποφασίζει δυναμικά ποιο πρωτόκολλο θα χρησιμοποιηθεί (MQTT ή HTTP) βάσει διαθεσιμότητας και ρυθμίσεων.

4.  **Storage (NVS)**:
    *   Διαχειρίζεται τη μόνιμη μνήμη (Non-Volatile Storage) του ESP32.
    *   Αποθηκεύει κρυπτογραφημένα τα διαπιστευτήρια WiFi, το JWT token και τα διαπιστευτήρια MQTT.
    *   Υλοποιεί το **Config Struct Pattern** με lazy-loading cache για βελτιστοποίηση της πρόσβασης στο NVS και προστασία από φθορά της Flash μνήμης.

## Βελτιωμένο Captive Portal με Runtime Configuration

Το Captive Portal επεκτάθηκε σημαντικά για να υποστηρίζει δυναμική ρύθμιση παραμέτρων της συσκευής χωρίς ανάγκη επαναμεταγλώττισης του firmware.

### Χαρακτηριστικά

1.  **Αυτόματη Σάρωση WiFi**: Κατά την εκκίνηση του portal, η συσκευή σαρώνει τα διαθέσιμα δίκτυα WiFi και τα εμφανίζει σε dropdown menu, απαλείφοντας τα διπλότυπα SSID.

2.  **Ενοποιημένη Φόρμα Ρυθμίσεων**: Η φόρμα συνδυάζει:
    *   **WiFi Configuration**: Επιλογή δικτύου και εισαγωγή κωδικού πρόσβασης
    *   **Device Configuration**: Ρυθμίσεις συσκευής που προηγουμένως ήταν hardcoded στο `config.h`:
        *   **Server URL**: Η διεύθυνση του backend server (π.χ. `https://agronos.kalogeitonas.xyz`)
        *   **Read Interval**: Το διάστημα μεταξύ των μετρήσεων σε λεπτά
        *   **MQTT Enabled**: Checkbox για ενεργοποίηση/απενεργοποίηση του πρωτοκόλλου MQTT

3.  **Pre-population των Τιμών**: Η φόρμα εμφανίζει τις τρέχουσες αποθηκευμένες τιμές (ή τις προεπιλογές από `config.h`), επιτρέποντας εύκολη επεξεργασία χωρίς επανεισαγωγή όλων των πεδίων.

4.  **Πληροφορίες Συσκευής**: Το portal εμφανίζει:
    *   Device UUID
    *   Device Secret
    *   Λίστα ρυθμισμένων αισθητήρων με τα UUIDs τους

### Αρχιτεκτονική Υλοποίησης

Η κλάση `WifiPortal` δέχεται τις τρέχουσες τιμές ρύθμισης μέσω του constructor και τις χρησιμοποιεί για την παραγωγή της HTML φόρμας:

```cpp
WifiPortal(Storage &storage, const char* apSsid, const char* apPass, 
           const char* deviceUuid, const char* deviceSecret, 
           const SensorConfig* sensorConfigs, size_t sensorCount,
           const char* baseUrl, bool mqttEnabled, unsigned long readIntervalMs);
```

Η μέθοδος `generateHtmlPage()` δημιουργεί δυναμικά την HTML φόρμα, ενσωματώνοντας:
*   Dropdown με τα σαρωμένα WiFi δίκτυα
*   Text input για το Server URL με pre-filled τιμή
*   Number input για το Read Interval (σε λεπτά) με pre-filled τιμή
*   Checkbox για το MQTT με κατάσταση checked/unchecked βάσει της τρέχουσας ρύθμισης

### Ροή Αποθήκευσης

Όταν ο χρήστης υποβάλει τη φόρμα:

1.  Η μέθοδος `onSave()` εξάγει όλες τις παραμέτρους από το POST request
2.  Αποθηκεύει τα WiFi credentials στο NVS namespace `"wifi"`
3.  Αποθηκεύει τις ρυθμίσεις συσκευής στο NVS namespace `"config"`:
    *   `base_url` → Server URL
    *   `interval_ms` → Read Interval (μετατρέπεται από λεπτά σε milliseconds)
    *   `mqtt_enabled` → Boolean flag (checkbox presence detection)
4.  Στέλνει επιβεβαίωση στον χρήστη και επανεκκινεί τη συσκευή
5.  Μετά την επανεκκίνηση, η συσκευή φορτώνει τις νέες ρυθμίσεις από το NVS

### Πλεονεκτήματα

*   **Ευελιξία στην Ανάπτυξη**: Μια εικόνα firmware μπορεί να χρησιμοποιηθεί σε διαφορετικά περιβάλλοντα (development, staging, production) χωρίς επαναμεταγλώττιση
*   **Εύκολη Συντήρηση**: Αλλαγή του backend URL ή του read interval χωρίς reflashing
*   **User-Friendly**: Όλες οι ρυθμίσεις σε μία ενοποιημένη φόρμα με προεπιλεγμένες τιμές
*   **Network Scanning**: Αποφυγή λαθών πληκτρολόγησης του SSID μέσω dropdown επιλογής

### Τεχνικά Χαρακτηριστικά του Portal

*   **DNS Hijacking**: Ο DNS server ανακατευθύνει όλα τα queries στο ESP32 (192.168.4.1) για αυτόματη εμφάνιση του portal
*   **Multi-Platform Detection**: Υποστηρίζει captive portal detection για Android (`/generate_204`), iOS (`/hotspot-detect.html`), Windows (`/ncsi.txt`)
*   **Auto-Stop**: Το portal σταματά αυτόματα όταν επιτευχθεί σύνδεση WiFi
*   **Responsive Design**: HTML φόρμα με CSS styling για καλή εμφάνιση σε mobile και desktop συσκευές

*(Εικόνα του Captive Portal UI θα προστεθεί)*

## Refactoring του Storage Layer: Config Struct Pattern

Το Storage subsystem αναδιαμορφώθηκε πλήρως για να υλοποιήσει το **Config Struct Pattern** με **Lazy-Loading Cache**, βελτιώνοντας σημαντικά την απόδοση και τη συντηρησιμότητα του κώδικα.

### Προβλήματα της Προηγούμενης Υλοποίησης

Η αρχική υλοποίηση χρησιμοποιούσε ξεχωριστές getter/setter μεθόδους για κάθε παράμετρο ρύθμισης:
*   Κάθε `get` επιχείρηση άνοιγε το NVS, διάβαζε μία τιμή, και έκλεινε το NVS
*   Πολλαπλές προσβάσεις NVS για διάφορες παραμέτρους (performance overhead)
*   Fallback logic διασκορπισμένη στο `main.cpp`
*   Πιθανές εγγραφές NVS ακόμα και όταν οι τιμές δεν άλλαζαν (φθορά Flash)

### Νέα Αρχιτεκτονική

#### 1. DeviceConfig Struct
Όλες οι παράμετροι ρύθμισης ομαδοποιούνται σε ένα ενιαίο `struct`:

```cpp
struct DeviceConfig {
    String baseUrl;
    unsigned long readIntervalMs;
    bool mqttEnabled;
};
```

#### 2. State Management με Cache
Η κλάση `Storage` διατηρεί τρεις ιδιωτικές μεταβλητές κατάστασης:

```cpp
private:
    DeviceConfig _cache;           // Cached values in RAM
    DeviceConfig _defaults;        // Fallback defaults from config.h
    bool _configLoaded = false;    // Lazy-loading flag
```

#### 3. Dependency Injection για Defaults
Η μέθοδος `loadDefaults()` επιτρέπει την ένεση των προεπιλεγμένων τιμών από το `config.h`:

```cpp
DeviceConfig defaults = {
    .baseUrl = BASE_URL,
    .readIntervalMs = SENSORS_READ_INTERVAL_MS,
    .mqttEnabled = MQTT_ENABLED
};
storage.loadDefaults(defaults);
```

Επιστρέφει `this` για method chaining.

#### 4. Lazy-Loading με ensureConfigLoaded()
Η ιδιωτική μέθοδος `ensureConfigLoaded()` εκτελείται μόνο στην πρώτη πρόσβαση:

```cpp
void Storage::ensureConfigLoaded() {
    if (_configLoaded) return;  // Already loaded, skip
    
    prefs.begin("config", true); // Read-only mode
    
    // Load entire config with defaults as fallback
    _cache.baseUrl = prefs.getString("base_url", _defaults.baseUrl);
    _cache.readIntervalMs = prefs.getULong("interval_ms", _defaults.readIntervalMs);
    _cache.mqttEnabled = prefs.getBool("mqtt_enabled", _defaults.mqttEnabled);
    
    prefs.end();
    _configLoaded = true;
}
```

Χαρακτηριστικά:
*   **Μία μόνο φορά**: Η μέθοδος εκτελείται μόνο την πρώτη φορά που ζητηθεί κάποια παράμετρος
*   **Bulk Loading**: Όλες οι παράμετροι φορτώνονται μαζί σε ένα άνοιγμα/κλείσιμο NVS
*   **Read-Only Mode**: Το NVS ανοίγει σε read-only mode για ασφάλεια
*   **Automatic Fallback**: Χρήση `_defaults` όταν το NVS δεν έχει αποθηκευμένη τιμή

#### 5. Transparent Getters
Οι getter μέθοδοι απλοποιούνται δραστικά:

```cpp
String Storage::getBaseUrl() {
    ensureConfigLoaded();
    return _cache.baseUrl;
}

unsigned long Storage::getReadIntervalMs() {
    ensureConfigLoaded();
    return _cache.readIntervalMs;
}

bool Storage::getMqttEnabled() {
    ensureConfigLoaded();
    return _cache.mqttEnabled;
}
```

Δεν υπάρχει πλέον NVS access σε κάθε κλήση — μόνο ανάγνωση από RAM cache.

#### 6. Atomic Configuration Saving
Η μέθοδος `saveConfig()` παρέχει ατομική αποθήκευση ολόκληρου του configuration struct:

```cpp
void Storage::saveConfig(const DeviceConfig& cfg) {
    ensureConfigLoaded(); // Ensure cache is valid
    
    prefs.begin("config", false);
    
    // Only write changed fields (Flash wear prevention)
    if (cfg.baseUrl != _cache.baseUrl) {
        prefs.putString("base_url", cfg.baseUrl);
    }
    
    if (cfg.readIntervalMs != _cache.readIntervalMs) {
        prefs.putULong("interval_ms", cfg.readIntervalMs);
    }
    
    if (cfg.mqttEnabled != _cache.mqttEnabled) {
        prefs.putBool("mqtt_enabled", cfg.mqttEnabled);
    }
    
    prefs.end();
    
    // Update cache to match
    _cache = cfg;
}
```

Κρίσιμα σημεία:
*   **Change Detection**: Σύγκριση νέων τιμών με cache πριν την εγγραφή
*   **Conditional Writes**: Εγγραφή στο NVS μόνο αν η τιμή άλλαξε
*   **Cache Synchronization**: Ενημέρωση του cache μετά την επιτυχή εγγραφή
*   **Single Transaction**: Όλες οι αλλαγές σε ένα άνοιγμα/κλείσιμο NVS

#### 7. Individual Setters με Delegation
Οι ξεχωριστές setter μεθόδοι διατηρούνται για backwards compatibility και delegάρουν στην `saveConfig()`:

```cpp
void Storage::setBaseUrl(const String &url) {
    DeviceConfig cfg = _cache;
    cfg.baseUrl = url;
    saveConfig(cfg);
}

void Storage::setReadIntervalMs(unsigned long ms) {
    DeviceConfig cfg = _cache;
    cfg.readIntervalMs = ms;
    saveConfig(cfg);
}
```

### Βελτιώσεις Απόδοσης

Σύγκριση πρόσβασης NVS (old vs new approach):

**Προηγούμενη Υλοποίηση**:
```
getBaseUrl()         → NVS open, read, close
getReadIntervalMs()  → NVS open, read, close
getMqttEnabled()     → NVS open, read, close
Total: 3 NVS transactions
```

**Νέα Υλοποίηση**:
```
getBaseUrl()         → ensureConfigLoaded() → NVS open, read all, close
                    → return from cache
getReadIntervalMs()  → return from cache (already loaded)
getMqttEnabled()     → return from cache (already loaded)
Total: 1 NVS transaction
```

**Βελτίωση**: ~66% λιγότερες NVS προσβάσεις

### Προστασία της Flash Μνήμης

Το ESP32 NVS χρησιμοποιεί Flash memory με περιορισμένο αριθμό write cycles (~100,000). Η νέα υλοποίηση:

*   **Μηδενικές εγγραφές για αμετάβλητες τιμές**: Το `saveConfig()` ελέγχει κάθε πεδίο πριν την εγγραφή
*   **Bulk updates**: Όλες οι αλλαγές σε μία NVS transaction αντί για πολλαπλές
*   **Cache invalidation**: Το `clearAll()` σηματοδοτεί ότι το cache δεν είναι πλέον έγκυρο

### Απλοποίηση του main.cpp

Η φόρτωση ρυθμίσεων στο `setup()` απλοποιήθηκε σημαντικά:

**Πριν**:
```cpp
baseUrl = storage.getBaseUrl();
if (baseUrl.length() == 0) {
    baseUrl = BASE_URL;
    storage.setBaseUrl(baseUrl);
}

readIntervalMs = storage.getReadIntervalMs();
if (readIntervalMs == 0) {
    readIntervalMs = SENSORS_READ_INTERVAL_MS;
    storage.setReadIntervalMs(readIntervalMs);
}

mqttEnabled = storage.getMqttEnabled(MQTT_ENABLED);
// ~20 lines total
```

**Μετά**:
```cpp
DeviceConfig defaults = {
    .baseUrl = BASE_URL,
    .readIntervalMs = SENSORS_READ_INTERVAL_MS,
    .mqttEnabled = MQTT_ENABLED
};
storage.loadDefaults(defaults);

baseUrl = storage.getBaseUrl();
readIntervalMs = storage.getReadIntervalMs();
mqttEnabled = storage.getMqttEnabled();
// ~6 lines total
```

### Επίλυση του ESP32 NVS Key Length Bug

Κατά τη διάρκεια της υλοποίησης, ανακαλύφθηκε ότι το ESP32 NVS έχει όριο **15 χαρακτήρων** για τα ονόματα των keys. Το αρχικό key `"read_interval_ms"` (16 χαρακτήρες) προκαλούσε σφάλμα `KEY_TOO_LONG` και αποτυχία αποθήκευσης.

**Λύση**: Το key μετονομάστηκε σε `"interval_ms"` (11 χαρακτήρες), λύνοντας το πρόβλημα και επιτρέποντας επιτυχή αποθήκευση της τιμής.

### Πλεονεκτήματα του Config Struct Pattern

1.  **Απόδοση**: 66% λιγότερες NVS προσβάσεις μετά την αρχική φόρτωση
2.  **Προστασία Flash**: Μηδενικές περιττές εγγραφές, παρατεταμένη διάρκεια ζωής συσκευής
3.  **Συντηρησιμότητα**: Όλες οι παράμετροι σε ένα struct, εύκολη επέκταση
4.  **Consistency**: Atomic updates μέσω `saveConfig()`, αποφυγή μερικώς ενημερωμένης κατάστασης
5.  **Απλότητα**: Καθαρότερος κώδικας στο `main.cpp`, centralized defaults
6.  **Type Safety**: Το struct παρέχει type checking σε compile time

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
