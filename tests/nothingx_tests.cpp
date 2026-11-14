// Protocol frames below come from community captures of a real Nothing Ear (3)
// (EarA-linux issue #29) and from nothingctl's per-model frame tables.

#include "demotransport.h"
#include "device.h"
#include "popupplacer.h"
#include "protocol.h"
#include "settings.h"
#include "theme.h"

#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Proto;

static QByteArray hex(const char *h) { return QByteArray::fromHex(h); }

static Frame decodeOne(const QByteArray &bytes) {
    Decoder d;
    d.feed(bytes);
    auto f = d.next();
    if (!f)
        qFatal("frame did not decode");
    return *f;
}

class Tests : public QObject {
    Q_OBJECT

private slots:
    // --- Framing -----------------------------------------------------------
    void crcCheckValue() { QCOMPARE(crc16("123456789"), quint16(0x4B37)); }

    void encodesKnownFrames_data() {
        QTest::addColumn<int>("cmd");
        QTest::addColumn<QByteArray>("payload");
        QTest::addColumn<int>("fsn");
        QTest::addColumn<QByteArray>("expected");
        QTest::newRow("battery read") << int(BatteryRead) << QByteArray() << 1 << hex("55600107c0000001acdf");
        QTest::newRow("anc read") << int(AncRead) << QByteArray() << 8 << hex("5560011ec0000008711b");
        QTest::newRow("anc adaptive") << int(AncWrite) << ancPayload(AncAdaptiveByte) << 1 << hex("5560010ff0030001010400fa87");
        QTest::newRow("anc off") << int(AncWrite) << ancPayload(AncOffByte) << 1 << hex("5560010ff0030001010500fb17");
        QTest::newRow("eq voice") << int(EqPresetWrite) << hex("0100") << 1 << hex("55600110f002000101002639");
        QTest::newRow("advanced eq off") << int(AdvancedEqStatusWrite) << hex("0000") << 1 << hex("5560014ff0020001000088ac");
        QTest::newRow("advanced bands") << int(AdvancedEqBandsRead) << hex("ff") << 1 << hex("5560014dc0010001ff90ab");
        QTest::newRow("bass 3") << int(BassWrite) << hex("0106") << 1 << hex("55600151f00200010106f73f");
        QTest::newRow("low latency on") << int(LowLatencyWrite) << hex("0100") << 1 << hex("55600140f00200010100763c");
        QTest::newRow("in-ear on") << int(InEarWrite) << hex("010101") << 1 << hex("55600104f003000101010179a4");
        QTest::newRow("super mic on") << int(SuperMicWrite) << hex("01") << 1 << hex("5560015ff001000101525d");
        QTest::newRow("walkie off") << int(WalkieTalkieWrite) << hex("00") << 1 << hex("55600161f0010001009743");
        QTest::newRow("mic voice focus") << int(MicModeWrite) << hex("01") << 1 << hex("55600165f0010001015707");
        QTest::newRow("spatial fixed") << int(SpatialWrite) << hex("0100") << 1 << hex("55600152f00200010100443d");
        QTest::newRow("dual list") << int(DualListRead) << hex("00") << 12 << hex("55600128c001000c00dd8e");
        QTest::newRow("time") << int(TimeWrite) << timePayload(0x6abc5100) << 4 << hex("5560010af00400046abc51002342");
    }
    void encodesKnownFrames() {
        QFETCH(int, cmd);
        QFETCH(QByteArray, payload);
        QFETCH(int, fsn);
        QFETCH(QByteArray, expected);
        QCOMPARE(encode(quint16(cmd), payload, quint8(fsn)).toHex(), expected.toHex());
    }

    void customEqMatchesNothingX() {
        // `eq-custom 3 0 -3` = bass +3, mid 0, treble -3.
        const auto bytes = encode(CustomEqWrite, customEqPayload({3, 0, -3}), 1);
        QCOMPARE(bytes.toHex(), QByteArray("55600141f035000103000040c00100000000000075443333333f02000040c0008054450000803f0000"
                                           "00404000000c43cdcc4c3f0000000000000000002aa2"));
    }

    void decoderHandlesSplitsAndGarbage() {
        Decoder d;
        const QByteArray battery = hex("5560010740050002020250035f32a7");
        const QByteArray anc = hex("5560011e40060003010100020100642a");
        d.feed(hex("00ff55") + battery.left(5)); // garbage, a stray 0x55, half a frame
        QVERIFY(!d.next());
        d.feed(battery.mid(5) + anc.left(3));
        auto f1 = d.next();
        QVERIFY(f1);
        QCOMPARE(f1->cmd, quint16(0x4007));
        QCOMPARE(f1->fsn, quint8(2));
        QVERIFY(!d.next());
        d.feed(anc.mid(3));
        auto f2 = d.next();
        QVERIFY(f2);
        QCOMPARE(f2->cmd, quint16(0x401E));
    }

    void decoderRejectsBadCrc() {
        Decoder d;
        QByteArray bad = hex("5560011e40060003010100020100642b");
        d.feed(bad + hex("55600142400800 05312e302e312e36399490").replace(' ', ""));
        auto f = d.next();
        QVERIFY(f);
        QCOMPARE(f->cmd, quint16(0x4042));
    }

    // --- Real Ear (3) replies --------------------------------------------------
    void parsesBattery() {
        const auto b = parseBattery(decodeOne(hex("5560010740050002020250035f32a7")).payload);
        QCOMPARE(b.left.level, 80);
        QCOMPARE(b.right.level, 95);
        QCOMPARE(b.caseUnit.level, -1); // lid closed: not reported
        const auto c = parseBattery(hex("0302d203400494"));
        QCOMPARE(c.left.level, 82);
        QVERIFY(c.left.charging);
        QCOMPARE(c.caseUnit.level, 20);
        QVERIFY(c.caseUnit.charging);
    }

    void parsesAnc() {
        const auto a = parseAnc(decodeOne(hex("5560011e40060003010100020100642a")).payload);
        QVERIFY(a);
        QCOMPARE(int(a->mode), int(AncHighByte));
        QCOMPARE(Device::ancFromByte(AncTransparencyByte), int(Device::AncTransparency));
        QCOMPARE(int(Device::ancToByte(Device::AncAdaptive)), int(AncAdaptiveByte));
    }

    void parsesFirmwareAndDeviceInfo() {
        QCOMPARE(QString::fromLatin1(decodeOne(hex("5560014240080005312e302e312e36399490")).payload), QString("1.0.1.69"));
        const auto info = parseDeviceInfo(hex("09") + "2,1,\n2,2,1.0.1.69\n2,4,SH10212538005632\n3,1,\n4,2,1.0.1.70\n");
        QCOMPARE(info.serial, QString("SH10212538005632"));
        QCOMPARE(info.leftFirmware, QString("1.0.1.69"));
        QCOMPARE(info.caseFirmware, QString("1.0.1.70"));
    }

    void parsesGestures() {
        const auto g = parseGestures(decodeOne(hex("55600118402500080902010212030102090201030803010308020107160301071602010901030109010401070be67f")).payload);
        QCOMPARE(g.size(), 9);
        QCOMPARE(g[0], (Gesture{2, 1, 2, 0x12}));
        QCOMPARE(g[8], (Gesture{4, 1, 7, 0x0B}));
        QCOMPARE(gesturePayload({3, 1, 7, 0x16}).toHex(), QByteArray("0103010716"));
    }

    void parsesCustomEqReply() {
        const auto f = decodeOne(hex("55600144406d000903000000c001abaaaa3f00007544c3f5283f020000803f00c05a450000803f000000004000000c43cdcc4c3f000000000000000000af87f1949658202a0001094c0043004c0080000100000000000000d9c5582038c6582000000000acc55820acc55820000000000000000000a20c"));
        const auto eq = parseCustomEq(f.payload);
        QVERIFY(eq);
        QCOMPARE(eq->bass, 2.0);
        QCOMPARE(eq->mid, 1.3);
        QCOMPARE(eq->treble, 1.0);
        const auto round = parseCustomEq(customEqPayload({-4.5, 2, 6}));
        QCOMPARE(round->bass, -4.5);
        QCOMPARE(round->treble, 6.0);
    }

    void parsesSettingsReplies() {
        QCOMPARE(parseInEar(decodeOne(hex("5560010e4013000e0901010201070109010a010b000e0112011501166a")).payload), std::optional<bool>(true));
        QCOMPARE(parseModelId(hex("73b1")), QString("B173"));
        const auto w = parseWearState(hex("0202050304"));
        QVERIFY(!w.leftInEar() == false);
        QVERIFY(w.rightInEar());
    }

    void advancedEqRoundTrip() {
        auto bands = defaultAdvancedEq();
        bands[0].gain = 4.5;
        bands[7].gain = -2;
        const auto parsed = parseAdvancedEq(advancedEqPayload(bands));
        QCOMPARE(parsed.size(), 8);
        QCOMPARE(parsed[0].gain, 4.5);
        QCOMPARE(parsed[0].frequency, 55.0);
        QCOMPARE(parsed[7].gain, -2.0);
        QCOMPARE(parsed[7].frequency, 13200.0);
    }

    void dualList() {
        QByteArray p = hex("000002") + char(0x11) + hex("a1b2c3d4e5f6") + char(4) + "Omar" + char(0x00) + hex("0a1b2c3d4e5f") +
                       char(char(0x85)) + "Phone";
        int n = 0;
        const auto list = parseDualList(p, &n);
        QCOMPARE(n, 2);
        QCOMPARE(list.size(), 2);
        QVERIFY(list[0].thisHost && list[0].connected);
        QCOMPARE(list[0].address, QString("A1:B2:C3:D4:E5:F6"));
        QCOMPARE(list[1].name, QString("Phone"));
        QVERIFY(!list[1].connected);
        QCOMPARE(dualConnectPayload("0A:1B:2C:3D:4E:5F", true).toHex(), QByteArray("010a1b2c3d4e5f"));
    }

    // --- Backend against the simulated earbuds -----------------------------------
    void deviceEndToEnd() {
        QTemporaryDir dir;
        Settings settings(dir.filePath("test.conf"), nullptr);
        auto *demo = new DemoTransport;
        demo->setLatency(0);
        Device device(demo, &settings);
        device.connectToDevice("00:11:22:33:44:55", "Nothing Ear (3)");
        QTRY_COMPARE(device.state(), int(Device::Connected));
        QTRY_COMPARE(device.values().value("walkieTalkie").isValid(), true);

        QCOMPARE(device.modelId(), QString("B173"));
        QVERIFY(device.hasFeature("superMic"));
        QCOMPARE(device.batteryLeft(), 86);
        QCOMPARE(device.batteryCase(), 64);
        QVERIFY(device.chargingCase());
        QCOMPARE(device.ancMode(), int(Device::AncHigh));
        QCOMPARE(device.values().value("firmware").toString(), QString("1.0.1.69"));
        QCOMPARE(device.gestureAction(4, 7), 0x0B);
        QCOMPARE(device.pairedDevices().size(), 2);
        QCOMPARE(settings.lastDeviceAddress(), QString("00:11:22:33:44:55"));

        device.setAncMode(Device::AncTransparency);
        QTRY_COMPARE(QByteArray(demo->state(AncRead)).toHex(), QByteArray("010700020100"));
        QCOMPARE(device.ancMode(), int(Device::AncTransparency));

        device.setGesture(2, 7, 18);
        QTRY_COMPARE(parseGestures(demo->state(GesturesRead))[4].action, quint8(18));

        device.setBass(true, 5);
        QTRY_COMPARE(QByteArray(demo->state(BassRead)).toHex(), QByteArray("010a"));

        device.setCustomEq(3, 0, -3);
        QTRY_COMPARE(device.values().value("eqPreset").toInt(), int(EqCustom));
        QTRY_COMPARE(parseCustomEq(demo->state(CustomEqRead))->treble, -3.0);

        QSignalSpy fit(&device, &Device::fitTestFinished);
        device.startFitTest();
        QTRY_COMPARE(fit.count(), 1);
        QCOMPARE(device.values().value("fitRight").toInt(), 1);

        device.disconnectFromDevice();
        QCOMPARE(device.state(), int(Device::Disconnected));
        QCOMPARE(device.batteryLeft(), -1);
    }

    // --- Desktop integration -----------------------------------------------------
    void dockedPopupPosition() {
        const QRect screen(0, 0, 1920, 1080);
        // Windows taskbar at the bottom, tray icon near the right.
        const QRect bottomBar(0, 0, 1920, 1032);
        QCOMPARE(PopupPlacer::dockedPosition(screen, bottomBar, QRect(1800, 1040, 24, 24), QSize(384, 660), 10),
                 QPoint(1920 - 384 - 10, 1032 - 660 - 10));
        // Unknown icon geometry: the taskbar corner.
        QCOMPARE(PopupPlacer::dockedPosition(screen, bottomBar, QRect(), QSize(384, 660), 10), QPoint(1526, 362));
        // Taskbar at the top.
        const QRect topBar(0, 40, 1920, 1040);
        QCOMPARE(PopupPlacer::dockedPosition(screen, topBar, QRect(), QSize(384, 660), 10).y(), 50);
    }

    void omarchyTrayCorner() {
        QTemporaryDir dir;
        auto write = [&](const char *json) {
            QFile f(dir.filePath("shell.json"));
            f.open(QIODevice::WriteOnly);
            f.write(json);
        };
        QCOMPARE(PopupPlacer::omarchyTrayCorner(dir.filePath("missing.json")), QString("top-right"));
        write(R"({"bar":{"position":"top","layout":{"right":[{"id":"omarchy.tray"}]}}})");
        QCOMPARE(PopupPlacer::omarchyTrayCorner(dir.filePath("shell.json")), QString("top-right"));
        write(R"({"bar":{"position":"bottom","layout":{"left":[{"id":"omarchy.tray"}]}}})");
        QCOMPARE(PopupPlacer::omarchyTrayCorner(dir.filePath("shell.json")), QString("bottom-left"));
    }

    void themeReadsOmarchy() {
        QTemporaryDir dir;
        QFile f(dir.filePath("colors.toml"));
        f.open(QIODevice::WriteOnly);
        f.write("mode = \"light\"\naccent = \"#7aa2f7\"\n");
        f.close();
        QCOMPARE(Theme::readOmarchyAccent(f.fileName()), QColor("#7aa2f7"));
        QCOMPARE(Theme::readOmarchyDark(f.fileName(), true), false);
        QVERIFY(!Theme::readOmarchyAccent(dir.filePath("none")).isValid());
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Tests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "nothingx_tests.moc"
