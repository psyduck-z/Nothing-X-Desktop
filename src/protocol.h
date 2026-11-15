#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector>
#include <QtGlobal>
#include <optional>

// Nothing's earbuds control protocol, framed the way the official Nothing X
// app frames it (com.nothing.protocol.model.Message):
//
//   55 | control u16 | command u16 | length u16 | fsn u8 | payload | crc u16
//
// All integers are little-endian. The host always sends control 0x0160: CRC
// present (bit 5), the "multiFrames" bit 6 the app sets on every frame, and
// device type 1 (earbuds) in bits 8-11. Replies carry a status code in bits
// 0-4 (0 = OK). The CRC is CRC-16/MODBUS over everything before it.
//
// Commands: 0xC0xx reads, 0xF0xx writes, 0xE0xx notifications from the
// earbuds. The earbuds answer with bit 15 cleared (0xF00F -> 0x700F); the
// decoder puts it back, so a reply carries the code of its request.
namespace Proto {

constexpr quint16 HostControl = 0x0160;
constexpr quint16 CrcBit = 0x0020;

quint16 crc16(const QByteArray &bytes);
QByteArray encode(quint16 cmd, const QByteArray &payload, quint8 fsn);

// Frame sequence numbers as the official app issues them: 1 to 253, then 0.
class FsnCounter {
public:
    quint8 next() {
        m_value = m_value + 1 >= 254 ? 0 : m_value + 1;
        return quint8(m_value);
    }

private:
    int m_value = 0;
};

// Reply status codes (HeadsetSppConnector's table).
enum Status {
    Ok = 0,
    Failure = 1,
    NotActivated = 2,
    NotSupported = 3,
    InvalidParameter = 4,
};

struct Frame {
    quint16 control = 0;
    quint16 cmd = 0; // bit 15 restored: a reply carries its request's code
    quint8 fsn = 0;
    QByteArray payload;

    int status() const { return control & 0x1F; }
    bool ok() const { return status() == Ok; }
    bool isNotification() const { return (cmd & 0xFF00) == 0xE000; }
    bool isRead() const { return (cmd & 0xFF00) == 0xC000; }
    bool isWrite() const { return (cmd & 0xFF00) == 0xF000; }
};

// Splits a byte stream into frames. Lenient like the official app: frames may
// arrive split or several in one read, and the firmware sometimes sets the
// CRC bit without sending a CRC. A frame whose CRC bytes haven't arrived is
// held back until more bytes come or flush() says the stream went quiet.
class Decoder {
public:
    void feed(const QByteArray &bytes) { m_buffer.append(bytes); }
    std::optional<Frame> next(bool flush = false);
    bool pending() const { return !m_buffer.isEmpty(); }
    void clear() { m_buffer.clear(); }

private:
    QByteArray m_buffer;
};

// --- Commands ----------------------------------------------------------------
// The codes the official app uses for the Ear (3) family. Where a code's
// payload comes from community captures rather than the official Java code
// (it only exists in the app's compiled Dart), it says so.
namespace Cmd {
enum : quint16 {
    // Identity and status
    FirmwareRead = 0xC042,       // ASCII "a.b.c.d"
    CaseFirmwareRead = 0xC05C,   // [v0..v3][mcu0..mcu3]
    InfoRead = 0xC006,           // [n] + "device,type,value" lines; type 4 = serial
    ModelRead = 0xC01C,          // bytes reversed, as hex: 73 B1 -> "B173"
    ColorRead = 0xC00C,          // [index] 1 black, 2 white, ...
    BatteryRead = 0xC007,        // [n]([part][level | 0x80 charging])
    StatusRead = 0xC00A,         // [n]([part][flags]) bit0 in case, bit2 in ear
    BatteryEvent = 0xE001,
    StatusEvent = 0xE002,
    TimeRequest = 0xC03F,        // the earbuds ask the host for the time
    TimeWrite = 0xF00A,          // 4-byte little-endian Unix seconds
    MutualExclusionRead = 0xC062,
    // Sound
    AncRead = 0xC01E,            // payload 03
    AncWrite = 0xF00F,           // 01 mode 00
    AncEvent = 0xE003,
    EqModeRead = 0xC01F,
    EqModeWrite = 0xF010,        // mode 00
    CustomEqRead = 0xC044,
    CustomEqWrite = 0xF041,
    AdvancedEqModeRead = 0xC04C,
    AdvancedEqModeWrite = 0xF04F, // 00 00 off / 01 00 on
    AdvancedEqRead = 0xC04D,     // payload: profile LE16
    AdvancedEqWrite = 0xF050,
    BassRead = 0xC04E,
    BassWrite = 0xF051,          // on, level*2
    SpatialRead = 0xC04F,
    SpatialWrite = 0xF052,       // on, head tracking
    LdacRead = 0xC029,           // 2 = LDAC on
    LdacWrite = 0xF01C,          // 02 00 on / 00 00 off
    LowLagRead = 0xC041,         // 2 = off
    LowLagWrite = 0xF040,        // 01 00 on / 02 00 off
    // Personal sound: community captures (Dart-only in the official app)
    PersonalSoundRead = 0xC05A,
    PersonalSoundWrite = 0xF05C,
    PersonalSoundEvent = 0xE019,
    // Controls
    InEarRead = 0xC00E,          // [n]([feature][0/1]); feature 1 = in-ear detection
    InEarWrite = 0xF004,         // 01 01 on
    GesturesRead = 0xC018,       // [n]([device][button][gesture][action])
    GesturesWrite = 0xF003,
    FindWrite = 0xF002,          // [side][01 ring / 00 stop]
    EssentialEvent = 0xE018,     // case TALK button: Essential Space request
    GestureEvent = 0xE014,       // "magic button": a gesture set to action 31 fired
    EssentialReply = 0xF062,     // 03 = not available on this host
    // Dual connection
    DualRead = 0xC027,
    DualWrite = 0xF01A,          // 01 / 00
    DualListRead = 0xC028,       // payload: page LE16
    DualConnectWrite = 0xF01B,   // [01 connect / 00 disconnect][mac 6]
    DualSwitchEvent = 0xE006,
    DualDeviceEvent = 0xE00E,
    // Microphone: community captures (Dart-only in the official app); the
    // codes do appear in the app's compiled command tables.
    SuperMicRead = 0xC05E,
    SuperMicWrite = 0xF05F,
    CallTransparencyRead = 0xC05F,
    CallTransparencyWrite = 0xF060,
    WalkieTalkieRead = 0xC060,
    WalkieTalkieWrite = 0xF061,
    // Ear tip fit test: community captures (Dart-only in the official app)
    FitTestWrite = 0xF014,       // 01 start
    FitTestEvent = 0xE00D,       // [left][right] result per ear
};
}

// --- Values -----------------------------------------------------------------
// Payload layouts below are the official app's (core/ext/TWSDeviceExtKt.java
// and the entity classes it uses). Integers are little-endian; a plain
// integer setting goes out as two bytes ("03 00").

QByteArray le16Bytes(int value);
// A little-endian integer over the whole payload (Message.obtainPayload).
int leInt(const QByteArray &payload);
float floatLe(const QByteArray &bytes, qsizetype at);
void appendFloatLe(QByteArray &out, float value);

// Noise control. The read (C01E, payload 03), its reply and the E003
// notification carry 3-byte records: type 1 = current mode, type 2 = the
// last noise-cancelling level, used when ANC is switched back on.
enum AncMode : quint8 {
    AncHigh = 1,
    AncMid = 2,
    AncLow = 3,
    AncAdaptive = 4,
    AncOff = 5,
    AncTransparency = 7,
};
struct AncState {
    int mode = -1;
    int lastLevel = -1;
};
AncState parseAnc(const QByteArray &payload);
QByteArray ancPayload(int mode);

// The 3-band custom equaliser (C044/F041): bands in the order mid, treble,
// bass, each { type u8, gain f32, frequency f32, Q f32 }, preceded by the
// band count and a total gain that is minus the largest boost.
enum FilterType : quint8 { LowShelf = 0, Peak = 1, HighShelf = 2 };
struct EqBand {
    int type = Peak;
    double gain = 0;
    double frequency = 1000;
    double q = 1;
    bool operator==(const EqBand &) const = default;
};
struct CustomEqShape {
    double lowFreq = 140, lowQ = 0.8;
    double peakFreq = 980, peakQ = 0.7;
    double highFreq = 3400, highQ = 1.0;
};
struct CustomEq {
    double bass = 0, mid = 0, treble = 0;
};
QByteArray customEqPayload(const CustomEq &eq, const CustomEqShape &shape);
std::optional<CustomEq> parseCustomEq(const QByteArray &payload);

// The advanced (8-band) equaliser: F04F switches it on or off, F050 sends
// the curve as profile 0 with the model's fixed total gain (-6 dB on the
// Ear (3)); C04D reads it back.
QVector<EqBand> defaultAdvancedBands();
QByteArray advancedEqPayload(const QVector<EqBand> &bands, float totalGain);
QVector<EqBand> parseAdvancedEq(const QByteArray &payload);

// Ultra Bass (C04E/F051): on/off and a level 1-5, sent as 2, 4, ... 10.
struct Bass {
    bool on = false;
    int level = 1;
};
Bass parseBass(const QByteArray &payload);
QByteArray bassPayload(bool on, int level);

// Spatial audio (C04F/F052): [on][head tracking]. 0 off, 1 fixed, 2 head
// tracked, as the official app numbers the modes.
int parseSpatial(const QByteArray &payload);
QByteArray spatialPayload(int mode);

// Parts of the device, as payloads number them.
enum Part : quint8 { PartLeft = 2, PartRight = 3, PartCase = 4, PartWhole = 5 };

struct BatteryLevel {
    int level = -1;
    bool charging = false;
};
struct Battery {
    BatteryLevel left, right, caseUnit;
};
// Merges a battery report into `into`; parts not in the report keep their
// last value (the case is only reported while it can be reached).
void mergeBattery(Battery &into, const QByteArray &payload);

struct Wear {
    int left = -1, right = -1, caseUnit = -1; // raw flags, -1 unknown
    static bool inEar(int f) { return f >= 0 && (f & 0x04); }
    static bool inCase(int f) { return f >= 0 && (f & 0x01); }
};
void mergeWear(Wear &into, const QByteArray &payload);

QString parseFirmware(const QByteArray &payload);
QString parseCaseFirmware(const QByteArray &payload);
struct DeviceInfo {
    QString serial;
    QString leftFirmware, rightFirmware, caseFirmware;
};
DeviceInfo parseDeviceInfo(const QByteArray &payload);
QString parseModelId(const QByteArray &payload);
// "01" black, "02" white: the colour ids the official device list uses.
QString parseColorId(const QByteArray &payload);

std::optional<bool> parseInEar(const QByteArray &payload);
QByteArray inEarPayload(bool on);

struct Gesture {
    int device = 0, button = 1, gesture = 0, action = 0;
    bool operator==(const Gesture &) const = default;
};
QList<Gesture> parseGestures(const QByteArray &payload);
QByteArray gesturesPayload(const QList<Gesture> &gestures);

QByteArray findPayload(int part, bool ring);

struct PairedDevice {
    QString address;
    QString name;
    bool connected = false;
    bool thisHost = false;
};
struct DualPage {
    int pages = 0, page = 0;
    QList<PairedDevice> devices;
};
DualPage parseDualList(const QByteArray &payload);
QByteArray dualConnectPayload(const QString &address, bool connect);
QString macFromBytes(const QByteArray &six);
QByteArray macToBytes(const QString &address);

QByteArray timePayload(qint64 unixSeconds);

} // namespace Proto
