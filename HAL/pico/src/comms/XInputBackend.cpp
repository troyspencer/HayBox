#include "comms/XInputBackend.hpp"

#include "core/CommunicationBackend.hpp"
#include "core/state.hpp"

#include <Adafruit_USBD_XInput.hpp>

#define ANALOG_STICK_MIN 1
#define ANALOG_STICK_NEUTRAL 128
#define ANALOG_STICK_RANGE 256
#define ANALOG_STICK_MAX 255

XInputBackend::XInputBackend(
    InputState &inputs,
    InputSource **input_sources,
    size_t input_source_count
)
    : CommunicationBackend(inputs, input_sources, input_source_count),
      _xinput() {
    Serial.end();
    _xinput.begin();
    Serial.begin(115200);

    TinyUSBDevice.setID(0x0738, 0x4726);
}

CommunicationBackendId XInputBackend::BackendId() {
    return COMMS_BACKEND_XINPUT;
}

int16_t XInputBackend::ScaleValue(uint8_t input) {

    // 128 -> 0
    // 1 -> -32768
    // 255 -> 32767
    // 2 -> -32512
    // 3 -> 32511

    int8_t buffer = 0;
    const uint8_t lowBuffer = 63;
    const uint8_t highBuffer = 126;
    if (input < 128 - highBuffer) {
        buffer = -2;
    } else if (input < 128 - lowBuffer) {
        buffer = -1;
    } else if (input > 128 + highBuffer) {
        buffer = 1; 
    }

    return input * 258 - 33024 + buffer;
}

void XInputBackend::SendReport() {
    ScanInputs(InputScanSpeed::SLOW);
    ScanInputs(InputScanSpeed::MEDIUM);

    while (!_xinput.ready()) {
        tight_loop_contents();
    }

    ScanInputs(InputScanSpeed::FAST);

    UpdateOutputs();

    // Digital outputs
    _report.a = _outputs.a;
    _report.b = _outputs.b;
    _report.x = _outputs.x;
    _report.y = _outputs.y;
    _report.lb = _outputs.buttonL;
    _report.rb = _outputs.buttonR;
    _report.lt = _outputs.triggerLDigital ? 255 : _outputs.triggerLAnalog;
    _report.rt = _outputs.triggerRDigital ? 255 : _outputs.triggerRAnalog;
    _report.start = _outputs.start;
    _report.back = _outputs.select;
    _report.home = _outputs.home;
    _report.dpad_up = _outputs.dpadUp;
    _report.dpad_down = _outputs.dpadDown;
    _report.dpad_left = _outputs.dpadLeft;
    _report.dpad_right = _outputs.dpadRight;
    _report.ls = _outputs.leftStickClick;
    _report.rs = _outputs.rightStickClick;

    _report.lx = ScaleValue(_outputs.leftStickX);
    _report.ly = ScaleValue(_outputs.leftStickY);
    _report.rx = ScaleValue(_outputs.rightStickX);
    _report.ry = ScaleValue(_outputs.rightStickY);

    _xinput.sendReport(&_report);
}