#pragma once

// Nothing's earbud control protocol over RFCOMM, as documented by the
// community projects credited in the README (ear-web, nothingctl, PairFree,
// BudsLink, Gadgetbridge, EarA-linux). This is an independent implementation:
// pure functions with no Bluetooth or UI dependency, so it is fully testable.
//
// Frame: 55 | ctrl u16 LE | cmd u16 LE | len u16 LE | fsn u8 | payload | crc16 LE
// The CRC (CRC-16/MODBUS) is present when ctrl bit 0x20 is set and covers
// everything from the 0x55 to the end of the payload. Reads are 0xC0xx,
// writes 0xF0xx, device pushes 0xE0xx; a reply is the request with bit 15
// cleared (0xC007 -> 0x4007, 0xF00F -> 0x700F).

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector>
#include <optional>

namespace Proto {

enum Command : quint16 {
    ProtocolVersionRead = 0xC001,
    Activate = 0xF001,
    FindBuds = 0xF002,
    GesturesWrite = 0xF003,
    InEarWrite = 0xF004,
    DeviceInfoRead = 0xC006,
    BatteryRead = 0xC007,
    TimeWrite = 0xF00A,
    WearStateRead = 0xC00A,
    InEarRead = 0xC00E,
    AncWrite = 0xF00F,
    EqPresetWrite = 0xF010,
    FitTestStart = 0xF014,
    GesturesRead = 0xC018,
    DualEnableWrite = 0xF01A,
    DualConnectWrite = 0xF01B,
    ModelIdRead = 0xC01C,
    CodecWrite = 0xF01C,
    AncRead = 0xC01E,
    EqPresetRead = 0xC01F,
    DualEnableRead = 0xC027,
    DualListRead = 0xC028,
    CodecRead = 0xC029,
    LowLatencyWrite = 0xF040,
    CustomEqWrite = 0xF041,
    LowLatencyRead = 0xC041,
    FirmwareRead = 0xC042,
    CustomEqRead = 0xC044,
    AdvancedEqStatusRead = 0xC04C,
    AdvancedEqBandsRead = 0xC04D,
    BassRead = 0xC04E,
    SpatialRead = 0xC04F,
    AdvancedEqStatusWrite = 0xF04F,
    AdvancedEqBandsWrite = 0xF050,
    BassWrite = 0xF051,
    SpatialWrite = 0xF052,
    PersonalSoundRead = 0xC05A,
    PersonalSoundWrite = 0xF05C,
    SuperMicRead = 0xC05E,
    SuperMicWrite = 0xF05F,
    CallTransparencyRead = 0xC05F,
    CallTransparencyWrite = 0xF060,
    WalkieTalkieRead = 0xC060,
    WalkieTalkieWrite = 0xF061,
    MicModeRead = 0xC064,
    MicModeWrite = 0xF065,

    // Pushed by the earbuds without being asked.
    BatteryPush = 0xE001,
    WearStatePush = 0xE002,
    AncPush = 0xE003,
    DualEnablePush = 0xE006,
    FitTestPush = 0xE00D,
    DualDevicePush = 0xE00E,
    PersonalSoundPush = 0xE019,
};

// The reply code for a request: bit 15 cleared.
constexpr quint16 replyTo(quint16 cmd) { return cmd & 0x7FFF; }

constexpr quint16 DefaultCtrl = 0x0160;

struct Frame {
    quint16 ctrl = DefaultCtrl;
    quint16 cmd = 0;
    quint8 fsn = 0;
    QByteArray payload;

    // 0 ok, 1 failed, 2 not activated, 3 unsupported, 4 bad parameter.
    int status() const { return ctrl & 0x1F; }
};

quint16 crc16(const QByteArray &bytes);
QByteArray encode(quint16 cmd, const QByteArray &payload, quint8 fsn);
QByteArray fromHex(const char *hex);

// Splits an RFCOMM byte stream into frames. Partial frames wait for more
// bytes; garbage and frames with a bad CRC are skipped by resyncing on the
// next 0x55.
class Decoder {
public:
    void feed(const QByteArray &bytes);
    std::optional<Frame> next();
    int buffered() const { return int(m_buffer.size()); }

private:
    QByteArray m_buffer;
};

// --- Values -----------------------------------------------------------------

// Noise control mode bytes as the earbuds use them.
enum AncByte : quint8 {
    AncHighByte = 0x01,
    AncMidByte = 0x02,
    AncLowByte = 0x03,
    AncAdaptiveByte = 0x04,
    AncOffByte = 0x05,
    AncTransparencyByte = 0x07,
};

enum EqPreset : quint8 {
    EqBalanced = 0,
    EqVoice = 1,
    EqTreble = 2,
    EqBass = 3,
    EqCustom = 5,
};

enum Side : quint8 { Left = 0x02, Right = 0x03, Case = 0x04, Single = 0x06 };

struct BatteryLevel {
    int level = -1;     // 0-100, or -1 when the part is not reported
    bool charging = false;
};
struct Battery {
    BatteryLevel left, right, caseUnit, single;
};
Battery parseBattery(const QByteArray &payload);

struct Anc {
    quint8 mode = AncOffByte;
};
std::optional<Anc> parseAnc(const QByteArray &payload);
QByteArray ancPayload(quint8 mode);

// Gestures: device 2 = left, 3 = right, 4 = case "Talk" button. Gesture:
// 1 single, 2 double, 3 triple, 7 hold, 9 double-and-hold.
struct Gesture {
    quint8 device = 0;
    quint8 button = 1;
    quint8 type = 0;
    quint8 action = 0;
    bool operator==(const Gesture &) const = default;
};
QList<Gesture> parseGestures(const QByteArray &payload);
QByteArray gesturePayload(const Gesture &g);

// The three-band custom EQ, in dB (-6..+6), as the Nothing X sliders show it.
struct CustomEq {
    double bass = 0, mid = 0, treble = 0;
};
// Ear (3) curve: low shelf 140 Hz Q0.8, peak 980 Hz Q0.7, high shelf 3400 Hz Q1.0.
struct CustomEqShape {
    double lowFreq = 140, lowQ = 0.8, peakFreq = 980, peakQ = 0.7, highFreq = 3400, highQ = 1.0;
};
QByteArray customEqPayload(const CustomEq &eq, const CustomEqShape &shape = {});
std::optional<CustomEq> parseCustomEq(const QByteArray &payload);

// The advanced (parametric) EQ: up to 8 peak filters.
struct EqBand {
    quint8 type = 1; // 0 low shelf, 1 peak, 2 high shelf
    double gain = 0;
    double frequency = 1000;
    double q = 1.0;
};
QByteArray advancedEqPayload(const QVector<EqBand> &bands);
QVector<EqBand> parseAdvancedEq(const QByteArray &payload);
QVector<EqBand> defaultAdvancedEq();

struct PairedDevice {
    QString address;
    QString name;
    bool thisHost = false;
    bool connected = false;
};
// The reply to C028 (paged), and the number of devices the earbuds reported.
QList<PairedDevice> parseDualList(const QByteArray &payload, int *reportedCount = nullptr);
QByteArray dualConnectPayload(const QString &address, bool connect);
// E00E: a remembered device (dis)connected.
std::optional<PairedDevice> parseDualDevicePush(const QByteArray &payload);

struct DeviceInfo {
    QString serial;
    QString leftFirmware, rightFirmware, caseFirmware;
};
DeviceInfo parseDeviceInfo(const QByteArray &payload);

// In-ear detection setting (feature 1 in the C00E reply).
std::optional<bool> parseInEar(const QByteArray &payload);

// Wear state (C00A / E002): per part, bit 0 in case, bit 2 in ear.
struct WearState {
    int left = -1, right = -1; // raw flags or -1
    bool leftInEar() const { return left >= 0 && (left & 0x04); }
    bool rightInEar() const { return right >= 0 && (right & 0x04); }
};
WearState parseWearState(const QByteArray &payload);

// Fit test result per side: 0 good seal, 1 adjust / try another tip, 2 not detected.
struct FitResult {
    int left = -1, right = -1;
};
std::optional<FitResult> parseFitResult(const QByteArray &payload);

QString parseModelId(const QByteArray &payload); // "B173"
QByteArray timePayload(qint64 unixSeconds);

QString macFromBytes(const QByteArray &six);
QByteArray macToBytes(const QString &address);

} // namespace Proto
