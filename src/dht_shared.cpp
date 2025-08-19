#include "dht_shared.h"
#include <Arduino.h>
#include <stdlib.h>

DHT* getSharedDht(int pin) {
    struct Entry { int pin; DHT* dht; Entry* next; };
    static Entry* head = nullptr;
    for (Entry* e = head; e != nullptr; e = e->next) {
        if (e->pin == pin) return e->dht;
    }
    Entry* ne = (Entry*)malloc(sizeof(Entry));
    if (!ne) return nullptr;
    ne->pin = pin;
    ne->dht = new DHT(pin, DHT11);
    ne->dht->begin();
    ne->next = head;
    head = ne;
    return ne->dht;
}
