#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#define SECOND 1000
#define MOVE_INTERVAL 30 * SECOND

NimBLEHIDDevice *hid;
NimBLECharacteristic *input;
bool deviceConnected = false;

static NimBLEServer *pServer;

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        Serial.printf("Client connected: %s\n", connInfo.getAddress().toString().c_str());
        deviceConnected = true;
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        deviceConnected = false;
        Serial.printf("Client disconnected: %s\n", connInfo.getAddress().toString().c_str());
        NimBLEDevice::startAdvertising();
    }
} serverCallbacks;

void logError(const char *errorMessage)
{
    Serial.print("[ERROR] ");
    Serial.println(errorMessage);
}

void setup()
{
    Serial.begin(115200);
    
    NimBLEDevice::init(DEVICE_NAME);
    Serial.printf("BLE Device initialized %s\n", DEVICE_NAME);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
    NimBLEDevice::setSecurityAuth(true, false, true);

    Serial.println("Security settings configured");

    hid = new NimBLEHIDDevice(pServer);
    if (!hid)
    {
        logError("Failed to create HID device");
    }

    input = hid->getInputReport(1);
    hid->setManufacturer(MANUFACTURER);
    hid->setPnp(0x02, VID, PID, 0x0110);

    // HID-Version 1.11, häufig bei modernen Geräten
    hid->setHidInfo(0x01, 0x11);

    // HID-Berichtmappe festlegen
    const uint8_t reportMap[] = {
        0x05, 0x01, // UsagePage(Generic Desktop[1])
        0x09, 0x02, // UsageId(Mouse[2])
        0xA1, 0x01, // Collection(Application)
        0x85, 0x01, //     ReportId(1)
        0x09, 0x01, //     UsageId(Pointer[1])
        0xA1, 0x00, //     Collection(Physical)
        0x09, 0x30, //         UsageId(X[48])
        0x09, 0x31, //         UsageId(Y[49])
        0x15, 0x80, //         LogicalMinimum(-128)
        0x25, 0x7F, //         LogicalMaximum(127)
        0x95, 0x02, //         ReportCount(2)
        0x75, 0x08, //         ReportSize(8)
        0x81, 0x06, //         Input(Data, Variable, Relative, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
        0x05, 0x09, //         UsagePage(Button[9])
        0x19, 0x01, //         UsageIdMin(Button 1[1])
        0x29, 0x03, //         UsageIdMax(Button 3[3])
        0x15, 0x00, //         LogicalMinimum(0)
        0x25, 0x01, //         LogicalMaximum(1)
        0x95, 0x03, //         ReportCount(3)
        0x75, 0x01, //         ReportSize(1)
        0x81, 0x02, //         Input(Data, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
        0xC0,       //     EndCollection()
        0x95, 0x01, //     ReportCount(1)
        0x75, 0x05, //     ReportSize(5)
        0x81, 0x03, //     Input(Constant, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
        0xC0,       // EndCollection()
    };

    hid->setReportMap((uint8_t *)reportMap, sizeof(reportMap));
    hid->startServices();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(DEVICE_NAME);
    pAdvertising->addServiceUUID(hid->getHidService()->getUUID());
    pAdvertising->setAppearance(0x03C2);
    if (!pAdvertising->start())
    {
        logError("Failed to start advertising");
    }

    hid->setBatteryLevel(100);
}

void loop()
{
    if (deviceConnected)
    {
        Serial.println("Moving mouse...");

        // Maus nach rechts bewegen
        int8_t mouseMoveRight[] = {0x00, -1, 0}; // keine Buttons, X=-1, Y=0
        input->setValue((uint8_t *)mouseMoveRight, sizeof(mouseMoveRight));
        input->notify();

        delay(SECOND);

        // Maus nach links bewegen
        int8_t mouseMoveLeft[] = {0x00, 1, 0}; // keine Buttons, X=-1, Y=0
        input->setValue((uint8_t *)mouseMoveLeft, sizeof(mouseMoveLeft));
        input->notify();
    }

    delay(MOVE_INTERVAL);
}

