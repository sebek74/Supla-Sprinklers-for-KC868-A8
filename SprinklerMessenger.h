#include <SuplaDevice.h>
#include <supla/sensor/general_purpose_measurement.h>

#define QUEUE_SIZE 20 // Rozmiar kolejki (maksymalnie 20 zdarzeń w pamięci)
#define INTERVAL_STEP 10000 

enum Msg {
  PUMP_ON = 1,
  PUMP_OFF = 2,
  SCHEDULE_ON = 3,
  SCHEDULE_OFF = 4,
  PROGRAM_ON = 5,
  PROGRAM_OFF = 6,
  ALL_OFF = 7,
  TANK_FULL = 8,
  TANK_EMPTY = 9,
  TANK_NOMINAL = 10,
  SCHEDULE_FAILED = 11,
  DOOR_OPEN = 12,
  DAILY_MESSAGE_ENABLED = 13,
  DAILY_MESSAGE_DISABLED = 14,
  DAILY_MESSAGE_NO_WORK = 15,
  PUMP_ON_60MIN = 16,
  LIGHT_ON_30MIN = 17
};

class SprinklerMessenger {
private: 
  // Wskaźnik do oficjalnego kanału KPOP
  Supla::Sensor::GeneralPurposeMeasurement *triggerChannel = nullptr;

  // Definicja prostej kolejki FIFO
  double eventQueue[QUEUE_SIZE];
  int queueHead = 0; // Wskaźnik do zapisu
  int queueTail = 0; // Wskaźnik do odczytu

  // Zmienne do zarządzania czasem wysyłki
  unsigned long lastActionTime = 0;

  // Definicja stanów automatu
  enum TransmissionState {
    STATE_IDLE,
    STATE_SENDING_CODE,
    STATE_SENDING_ZERO
  };
  TransmissionState currentState = STATE_IDLE;

public: 
  void Initialize() {
    triggerChannel = new Supla::Sensor::GeneralPurposeMeasurement();
    triggerChannel->setInitialCaption("Komunikaty ze sterownika");
    //triggerChannel->setKeepHistory(1);
    triggerChannel->getChannel()->setChannelNumber(RelayId::_LastProgram*2);
    triggerChannel->setValue(0.0);
  }


  // Funkcja dodająca zdarzenie do kolejki (wywoływana w dowolnym miejscu kodu)
  void sendMessage(Msg code) {
    if (triggerChannel == nullptr) return;
    int nextHead = (queueHead + 1) % QUEUE_SIZE;
    // Sprawdzenie, czy kolejka nie jest pełna
    if (nextHead != queueTail) {
      eventQueue[queueHead] = (double)code;
      queueHead = nextHead;
      Serial.print("Dodano do kolejki kod: ");
      Serial.println(code);
    } else {
      Serial.println("Błąd: Kolejka zdarzeń jest pełna!");
    }
  }

  void pushMessages() {
    unsigned long currentMillis = millis();

    // Automat stanów zarządzający kolejką i wysyłaniem
    switch (currentState) {
      case STATE_IDLE:
        // Jeśli kolejka nie jest pusta, pobierz kod i go wyślij
        if (queueTail != queueHead) {
          double codeToSend = eventQueue[queueTail];
          queueTail = (queueTail + 1) % QUEUE_SIZE; // Przesunięcie ogona kolejki

          if (triggerChannel) {
            triggerChannel->setValue(codeToSend);
            Serial.print(">>> SUPLA: Wysyłam kod zdarzenia: ");
            Serial.println(codeToSend);
          }
          
          lastActionTime = currentMillis;
          currentState = STATE_SENDING_CODE;
        }
        break;

      case STATE_SENDING_CODE:
        // Odczekaj zadany czas, a następnie zresetuj kanał do zera
        if (currentMillis - lastActionTime >= INTERVAL_STEP) {
          if (triggerChannel) {
            triggerChannel->setValue(0.0);
            Serial.println(">>> SUPLA: Reset kanału do 0.0");
          }
          lastActionTime = currentMillis;
          currentState = STATE_SENDING_ZERO;
        }
        break;

      case STATE_SENDING_ZERO:
        // Odczekaj czas w stanie zero, zanim pozwolisz pobrać kolejny kod
        if (currentMillis - lastActionTime >= INTERVAL_STEP) {
          currentState = STATE_IDLE;
        }
        break;
    }
  };
};