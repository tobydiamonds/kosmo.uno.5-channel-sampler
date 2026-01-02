#ifndef AnalogMuxScanner_h
#define AnalogMuxScanner_h

typedef void (*Change)(int inputNumber, uint16_t value);

class AnalogMuxScanner {
private:
  uint8_t _s0, _s1, _s2;
  uint8_t _a;
  uint8_t _numberOfInputs;

  Change  _onChange = nullptr;

  uint8_t  _mux = 0;
  uint32_t _lastScanMs = 0;
  uint16_t _scanIntervalMs = 5;
  uint16_t _hysteresis     = 3;
  uint8_t  _samplesPerRead = 1;

  uint16_t* _stable;
  bool*     _hasStable;

  uint16_t readInvertedAnalog(uint8_t pin) {
    if (_samplesPerRead <= 1) {
      return (uint16_t)(1023-analogRead(pin));
    }
    uint32_t sum = 0;
    for (uint8_t i = 0; i < _samplesPerRead; ++i) sum += analogRead(pin);
    uint16_t avg = (uint16_t)(sum / _samplesPerRead);
    return (uint16_t)(1023-avg);
  }

public:
  AnalogMuxScanner(uint8_t s0, uint8_t s1, uint8_t s2,
                   uint8_t a0,
                   uint8_t numberOfInputs)
  : _s0(s0), _s1(s1), _s2(s2),
    _a(a0),
    _numberOfInputs(numberOfInputs)
  {
    // allocate per-channel/pot stable storage
    _stable = new uint16_t[_numberOfInputs];
    _hasStable = new bool[_numberOfInputs];
    for (uint16_t i = 0; i < _numberOfInputs; ++i) {
      _stable[i] = 0;
      _hasStable[i] = false;
    }
  }

  ~AnalogMuxScanner() {
    delete[] _stable;
    delete[] _hasStable;
  }

  void begin() {
    pinMode(_s0, OUTPUT);
    pinMode(_s1, OUTPUT);
    pinMode(_s2, OUTPUT);
  }

  void setScanInterval(uint16_t ms) { _scanIntervalMs = ms; }
  void setHysteresis(uint16_t counts) { _hysteresis = counts; }    // e.g., 2–5 counts
  void setSamplesPerRead(uint8_t n) { _samplesPerRead = (n == 0) ? 1 : n; }

  void onChange(Change onChangeHandler) {
    _onChange = onChangeHandler;
  }

  // Call regularly (e.g., in loop) when you want scanning active
  void scan(unsigned long now) {
    if (now - _lastScanMs < _scanIntervalMs) return;
    _lastScanMs = now;

    // Select mux address
    digitalWrite(_s0, (_mux & 0x01));
    digitalWrite(_s1, (_mux & 0x02) >> 1);
    digitalWrite(_s2, (_mux & 0x04) >> 2);

    // Read inverted value
    uint16_t value = readInvertedAnalog(_a);
    // char s[100];
    // sprintf(s, "%d: %d", _mux, value);
    // Serial.println(s);

    // Use a simplified formula for single input
    int inputNumber = _mux;

    //if (inputNumber < 0 || inputNumber >= _numberOfInputs) return;

    if (!_hasStable[inputNumber]) {
      _stable[inputNumber] = value;
      _hasStable[inputNumber] = true;
      if (_onChange) _onChange(inputNumber, _stable[inputNumber]); // first report
    } else {
      int diff = (int)value - (int)_stable[inputNumber];
      if (diff < 0) diff = -diff;
      if (diff >= _hysteresis) {
        _stable[inputNumber] = value;
        if (_onChange) _onChange(inputNumber, _stable[inputNumber]); // report significant change
      }
    }

    // Advance mux (0..7)
    _mux = (_mux + 1) & 0x07;
  }
};

#endif