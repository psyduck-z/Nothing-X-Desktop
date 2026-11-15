// The byte-level expectations below are worked out from the official Nothing
// X app's code (frame layout, CRC, payload builders), not from the app's own
// output, so they check this implementation against the official rules.

#include "demotransport.h"
#include "device.h"
#include "modelconfig.h"
#include "officialpack.h"
#include "popupplacer.h"
#include "protocol.h"
#include "settings.h"
#include "theme.h"
#include "trayicon.h"
#include "updater.h"

#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Proto;

static QByteArray hex(const char *h) { return QByteArray::fromHex(QByteArray(h).replace(' ', "")); }

static Frame decodeOne(const QByteArray &bytes, bool flush = false) {
    Decoder d;
    d.feed(bytes);
    auto f = d.next(flush);
    if (!f)
        qFatal("frame did not decode");
    return *f;
}

// A connected Device on the simulated Ear (3), with no official app imported.
struct Rig {
    QTemporaryDir dir;
    Settings settings{dir.filePath("test.conf"), nullptr};
    OfficialPack pack{dir.filePath("official")};
    DemoTransport *transport = new DemoTransport;
    Device device{transport, &settings, &pack};
    Rig() {
        transport->setLatency(0);
        transport->setFitDuration(10);
        device.connectToDevice("00:11:22:33:44:55", "Nothing Ear (3)");
    }
    bool ready() const { return device.state() == Device::Connected && device.gestures().size() == 9; }
    QList<Frame> sentWith(quint16 cmd) const {
        QList<Frame> out;
        for (const Frame &f : transport->sent())
            if (f.cmd == cmd)
                out << f;
        return out;
    }
};

class Tests : public QObject {
    Q_OBJECT

private slots:
    // --- Framing -------------------------------------------------------------
    void crcCheckValue() { QCOMPARE(crc16("123456789"), quint16(0x4B37)); }

    void encodesOfficialFrames_data() {
        QTest::addColumn<int>("cmd");
        QTest::addColumn<QByteArray>("payload");
        QTest::addColumn<int>("fsn");
        QTest::addColumn<QByteArray>("expected");
        QTest::newRow("firmware") << int(Cmd::FirmwareRead) << QByteArray() << 1 << hex("55 6001 42c0 0000 01 6110");
        QTest::newRow("battery") << int(Cmd::BatteryRead) << QByteArray() << 2 << hex("55 6001 07c0 0000 02 ecde");
        QTest::newRow("anc read") << int(Cmd::AncRead) << QByteArray(1, 3) << 3 << hex("55 6001 1ec0 0100 03 03 9de9");
        QTest::newRow("anc off") << int(Cmd::AncWrite) << ancPayload(AncOff) << 4 << hex("55 6001 0ff0 0300 04 010500 fbdb");
        QTest::newRow("eq more bass") << int(Cmd::EqModeWrite) << le16Bytes(3) << 5 << hex("55 6001 10f0 0200 05 0300 6698");
        QTest::newRow("time") << int(Cmd::TimeWrite) << timePayload(1763208000) << 6 << hex("55 6001 0af0 0400 06 406b1869 151c");
    }
    void encodesOfficialFrames() {
        QFETCH(int, cmd);
        QFETCH(QByteArray, payload);
        QFETCH(int, fsn);
        QFETCH(QByteArray, expected);
        QCOMPARE(encode(quint16(cmd), payload, quint8(fsn)).toHex(' '), expected.toHex(' '));
    }

    void fsnRunsOneTo253ThenZero() {
        FsnCounter c;
        QCOMPARE(c.next(), quint8(1));
        for (int i = 2; i <= 253; ++i)
            c.next();
        QCOMPARE(c.next(), quint8(0));
        QCOMPARE(c.next(), quint8(1));
    }

    void decodesRepliesWithStatus() {
        // A reply carries the request's code with bit 15 cleared.
        Frame ok = decodeOne(hex("55 6001 0f70 0000 04 abdd"));
        QCOMPARE(ok.cmd, quint16(Cmd::AncWrite));
        QVERIFY(ok.ok());
        Frame refused = decodeOne(hex("55 6301 0f70 0000 04 98dd"));
        QCOMPARE(refused.status(), int(NotSupported));
        QVERIFY(!refused.ok());
    }

    void decoderIsLenient() {
        Decoder d;
        // Split across reads, after garbage and a stray 0x55.
        const QByteArray a = encode(0x4007, hex("03 025a 03d8 0446"), 9);
        d.feed(hex("00 55 ff") + a.left(5));
        QVERIFY(!d.next());
        d.feed(a.mid(5));
        auto f = d.next();
        QVERIFY(f);
        QCOMPARE(f->cmd, quint16(Cmd::BatteryRead));
        // CRC bit set but no CRC, with the next frame right behind it.
        QByteArray noCrc = encode(0x6003, hex("010100"), 0);
        noCrc.chop(2);
        d.feed(noCrc + encode(0x6001, hex("01 0250"), 0));
        f = d.next();
        QVERIFY(f);
        QCOMPARE(f->cmd, quint16(Cmd::AncEvent));
        QCOMPARE(f->payload, hex("010100"));
        f = d.next();
        QVERIFY(f);
        QCOMPARE(f->cmd, quint16(Cmd::BatteryEvent));
        // A lone CRC-less frame is taken once the line goes quiet.
        d.feed(noCrc);
        QVERIFY(!d.next());
        QVERIFY(d.next(true));
    }

    // --- Payloads --------------------------------------------------------------
    void customEqMatchesOfficialLayout() {
        // Mid +2, treble -3, bass +4: total gain -4, bands mid/treble/bass, 9 zero bytes.
        CustomEqShape shape;
        shape.highQ = 0.7;
        const QByteArray expected = hex("03 000080c0"
                                        "01 00000040 00007544 3333333f"
                                        "02 000040c0 00805445 3333333f"
                                        "00 00008040 00000c43 cdcc4c3f"
                                        "000000000000000000");
        QCOMPARE(customEqPayload({4, 2, -3}, shape).toHex(' '), expected.toHex(' '));
        const auto back = parseCustomEq(expected);
        QVERIFY(back);
        QCOMPARE(back->bass, 4.0);
        QCOMPARE(back->mid, 2.0);
        QCOMPARE(back->treble, -3.0);
        // Nothing boosted: the total gain is negative zero, as in the official app.
        QCOMPARE(customEqPayload({0, -1, 0}, {}).mid(1, 4), hex("00000080"));
    }

    void advancedEqRoundTrip() {
        QVector<EqBand> bands = defaultAdvancedBands();
        bands[2].gain = 3.5;
        bands[6].q = 2.5;
        const QByteArray p = advancedEqPayload(bands, -6.0f);
        QCOMPARE(p.size(), 2 + 4 + 8 * 13);
        QCOMPARE(p.left(6), hex("00 08 0000c0c0"));
        QCOMPARE(parseAdvancedEq(p), bands);
    }

    void soundPayloads() {
        QCOMPARE(bassPayload(true, 3), hex("0106"));
        QCOMPARE(bassPayload(false, 5), hex("000a"));
        QCOMPARE(parseBass(hex("0108")).level, 4);
        QCOMPARE(spatialPayload(0), hex("0000"));
        QCOMPARE(spatialPayload(1), hex("0100"));
        QCOMPARE(spatialPayload(2), hex("0101"));
        QCOMPARE(parseSpatial(hex("0101")), 2);
        QCOMPARE(parseSpatial(hex("01")), 1);
        const AncState anc = parseAnc(hex("010700 020300"));
        QCOMPARE(anc.mode, int(AncTransparency));
        QCOMPARE(anc.lastLevel, int(AncLow));
    }

    void devicePayloads() {
        Battery b;
        mergeBattery(b, hex("03 025a 03d8 0446"));
        QCOMPARE(b.left.level, 90);
        QVERIFY(!b.left.charging);
        QCOMPARE(b.right.level, 88);
        QVERIFY(b.right.charging);
        QCOMPARE(b.caseUnit.level, 70);
        // A later report without the case keeps the case's last level.
        mergeBattery(b, hex("02 0250 0350"));
        QCOMPARE(b.caseUnit.level, 70);

        QCOMPARE(parseModelId(hex("73b1")), QString("B173"));
        QCOMPARE(parseColorId(hex("02")), QString("02"));
        QCOMPARE(parseFirmware("1.0.1.62"), QString("1.0.1.62"));
        QVERIFY(parseFirmware("garbage").isEmpty());
        QCOMPARE(parseCaseFirmware(hex("0100001200000005")), QString("1.0.0.18"));
        const DeviceInfo info = parseDeviceInfo(QByteArray(1, 3) + "2,2,1.0.1.62\n4,2,1.0.0.18\n2,4,SH123\n");
        QCOMPARE(info.serial, QString("SH123"));
        QCOMPARE(info.caseFirmware, QString("1.0.0.18"));

        QCOMPARE(inEarPayload(true), hex("010101"));
        QCOMPARE(parseInEar(hex("02 0200 0101")), std::optional<bool>(true));
        QCOMPARE(findPayload(PartLeft, true), hex("0201"));
        QCOMPARE(findPayload(PartWhole, false), hex("00"));

        // The official default gesture table for the Ear (3).
        const auto g = parseGestures(hex("0902010209030102090201030803010308020107160301071602010901030109010401070B"));
        QCOMPARE(g.size(), 9);
        QCOMPARE(g[4], (Gesture{PartLeft, 1, 7, 22}));
        QCOMPARE(g[8], (Gesture{PartCase, 1, 7, 11}));
        QCOMPARE(gesturesPayload({{PartRight, 1, 2, 8}}), hex("01 03010208"));
    }

    void dualConnectionList() {
        const QByteArray p = hex("01 00 02"
                                 "11 a0b1c2d3e4f5 07") + "Desktop" +
                             hex("01 1c2b3a495867 85") + "Phone";
        const DualPage page = parseDualList(p);
        QCOMPARE(page.pages, 1);
        QCOMPARE(page.devices.size(), 2);
        QCOMPARE(page.devices[0].address, QString("A0:B1:C2:D3:E4:F5"));
        QVERIFY(page.devices[0].thisHost && page.devices[0].connected);
        QCOMPARE(page.devices[1].name, QString("Phone…"));
        QVERIFY(!page.devices[1].thisHost);
        QCOMPARE(dualConnectPayload("1C:2B:3A:49:58:67", false), hex("00 1c2b3a495867"));
    }

    // --- Model configuration -------------------------------------------------------
    void builtInEar3MatchesOfficialFlags() {
        const ModelConfig m = ModelConfig::builtInEar3();
        QCOMPARE(m.id, QString("B173"));
        for (const char *f : {"anc", "ancMid", "ancLow", "ancAdaptive", "transparency", "customEq", "advancedEq", "ultraBass",
                              "spatial", "spatialHead", "ldac", "lowLag", "inEar", "find", "personalSound", "dual",
                              "fitTest", "superMic", "callTransparency", "walkieTalkie", "gestures", "caseButton"})
            QVERIFY2(m.has(f), f);
        QVERIFY(!m.has("lhdc"));
        // Balanced, More bass, More treble, Voice, Custom.
        QCOMPARE(m.eqPresets, (QVariantList{0, 3, 2, 1, 5}));
        QCOMPARE(m.advancedTotalGain, -6.0f);
        QCOMPARE(m.gestureGroups.size(), 7);
        QCOMPARE(ModelConfig::idFromName("Nothing Ear (3)"), QString("B173"));
    }

    void officialConfigWhenImported() {
        // Only on a machine with the official app unpacked next to the repo.
        const QString file = QFINDTESTDATA("../official/base/assets/flutter_assets/assets/config/ear_white_list.json");
        if (file.isEmpty())
            QSKIP("The official app isn't unpacked here");
        QFile f(file);
        QVERIFY(f.open(QIODevice::ReadOnly));
        for (const QJsonValue &v : QJsonDocument::fromJson(f.readAll()).array()) {
            if (v.toObject().value("id").toString() != "B173")
                continue;
            const ModelConfig official = ModelConfig::fromOfficial(v.toObject());
            const ModelConfig builtIn = ModelConfig::builtInEar3();
            QCOMPARE(official.features, builtIn.features);
            QCOMPARE(official.eqPresets, builtIn.eqPresets);
            QCOMPARE(official.gestureGroups, builtIn.gestureGroups);
            QCOMPARE(official.defaultControl, builtIn.defaultControl);
            return;
        }
        QFAIL("B173 missing from the official list");
    }

    // --- The device against the simulated Ear (3) ------------------------------------
    void connectsAndReadsEverything() {
        Rig r;
        QTRY_VERIFY(r.ready());
        // The firmware version is the first thing asked, as in the official app.
        QCOMPARE(r.transport->sent().first().cmd, quint16(Cmd::FirmwareRead));
        QTRY_COMPARE(r.device.values().value("firmware").toString(), QString("1.0.1.62"));
        QCOMPARE(r.device.modelId(), QString("B173"));
        QCOMPARE(r.device.batteryLeft(), 86);
        QCOMPARE(r.device.batteryCase(), 64);
        QVERIFY(r.device.chargingCase());
        QVERIFY(r.device.leftInEar());
        QCOMPARE(r.device.ancMode(), int(AncAdaptive));
        QTRY_COMPARE(r.device.values().value("serial").toString(), QString("SH10DEM0000173"));
        QCOMPARE(r.device.values().value("caseFirmware").toString(), QString("1.0.0.18"));
        QCOMPARE(r.device.values().value("customBass").toDouble(), 2.0);
        QCOMPARE(r.device.values().value("bassLevel").toInt(), 3);
        QCOMPARE(r.device.values().value("lowLag").toBool(), false);
        QCOMPARE(r.device.values().value("superMic").toBool(), true);
        QTRY_COMPARE(r.device.pairedDevices().size(), 2);
        QCOMPARE(r.device.advancedBands().size(), 8);
        // The ANC read carries the payload 03.
        QCOMPARE(r.sentWith(Cmd::AncRead).first().payload, hex("03"));
        // The earbuds asked for the time: answered with 4-byte little-endian seconds.
        QTRY_COMPARE(r.sentWith(Cmd::TimeWrite).size(), 1);
        const qint64 sent = qFromLittleEndian<quint32>(r.sentWith(Cmd::TimeWrite).first().payload.constData());
        QVERIFY(qAbs(sent - QDateTime::currentSecsSinceEpoch()) < 5);
    }

    void writesUseOfficialValues() {
        Rig r;
        QTRY_VERIFY(r.ready());
        r.device.setLowLag(false);
        r.device.setLdac(true);
        r.device.setAncMode(AncTransparency);
        r.device.setEqPreset(3);
        r.device.setBass(true, 5);
        r.device.setDualConnection(false);
        r.device.setInEarDetection(false);
        QTRY_COMPARE(r.sentWith(Cmd::InEarWrite).size(), 1);
        QCOMPARE(r.sentWith(Cmd::LowLagWrite).last().payload, hex("0200"));
        QCOMPARE(r.sentWith(Cmd::LdacWrite).last().payload, hex("0200"));
        QCOMPARE(r.sentWith(Cmd::AncWrite).last().payload, hex("010700"));
        QCOMPARE(r.sentWith(Cmd::EqModeWrite).last().payload, hex("0300"));
        QCOMPARE(r.sentWith(Cmd::BassWrite).last().payload, hex("010a"));
        QCOMPARE(r.sentWith(Cmd::DualWrite).last().payload, hex("00"));
        QCOMPARE(r.sentWith(Cmd::InEarWrite).last().payload, hex("010100"));
        // The simulated earbuds now report what was written.
        QCOMPARE(r.transport->state(Cmd::LdacRead), hex("0200"));
        QTRY_COMPARE(r.device.ancMode(), int(AncTransparency));
    }

    void exclusiveSettingsTurnEachOtherOff() {
        Rig r;
        QTRY_VERIFY(r.ready());
        // Adaptive ANC is on: LDAC turns it back to High, as the official app does.
        QVERIFY(!r.device.conflict("ldac", true).isEmpty());
        r.device.setLdac(true);
        QTRY_COMPARE(r.device.ancMode(), int(AncHigh));
        // Spatial audio turns LDAC and (spaceOrAdvance) Ultra Bass off.
        QVERIFY(r.device.conflict("spatial", 1).contains("LDAC"));
        r.device.setSpatial(2);
        QTRY_COMPARE(r.transport->state(Cmd::SpatialRead), hex("0101"));
        QCOMPARE(r.transport->state(Cmd::LdacRead), hex("0000"));
        QCOMPARE(r.transport->state(Cmd::BassRead).left(1), hex("00"));
        // A preset other than Balanced turns spatial audio off (C062 says exclusive).
        r.device.setEqPreset(2);
        QTRY_COMPARE(r.transport->state(Cmd::SpatialRead), hex("0000"));
    }

    void rejectedWriteIsUndone() {
        Rig r;
        QTRY_VERIFY(r.ready());
        QSignalSpy notices(&r.device, &Device::notice);
        r.transport->rejectNextWrite(NotSupported);
        r.device.setLowLag(true);
        QTRY_COMPARE(notices.size(), 1);
        // Read back from the earbuds: still off.
        QTRY_COMPARE(r.device.values().value("lowLag").toBool(), false);
    }

    void gesturesCustomEqAndAdvanced() {
        Rig r;
        QTRY_VERIFY(r.ready());
        r.device.setGesture(PartLeft, 2, 11);
        QTRY_COMPARE(r.sentWith(Cmd::GesturesWrite).size(), 1);
        QCOMPARE(r.sentWith(Cmd::GesturesWrite).first().payload, hex("01 0201020b"));
        // Noise-control cycles stay the same on both earbuds.
        r.device.setGesture(PartRight, 7, 20);
        QTRY_COMPARE(r.sentWith(Cmd::GesturesWrite).size(), 2);
        QCOMPARE(r.sentWith(Cmd::GesturesWrite).last().payload, hex("02 03010714 02010714"));
        r.device.resetGestures();
        QTRY_COMPARE(r.sentWith(Cmd::GesturesWrite).size(), 3);
        QCOMPARE(r.sentWith(Cmd::GesturesWrite).last().payload.size(), 37);

        r.device.setCustomEq(1, -2.5, 6);
        QTRY_COMPARE(r.sentWith(Cmd::CustomEqWrite).size(), 1);
        QCOMPARE(r.sentWith(Cmd::EqModeWrite).last().payload, hex("0500"));
        QCOMPARE(r.sentWith(Cmd::CustomEqWrite).first().payload.size(), 53);

        r.device.setAdvancedBand(0, 60, 4, 1.2);
        QTRY_COMPARE(r.sentWith(Cmd::AdvancedEqWrite).size(), 1);
        QCOMPARE(r.sentWith(Cmd::AdvancedEqModeWrite).last().payload, hex("0100"));
        const auto bands = parseAdvancedEq(r.sentWith(Cmd::AdvancedEqWrite).first().payload);
        QCOMPARE(bands.first().gain, 4.0);
        QCOMPARE(bands.first().frequency, 60.0);
    }

    void fitTestFindAndDual() {
        Rig r;
        QTRY_VERIFY(r.ready());
        QSignalSpy fit(&r.device, &Device::fitTestFinished);
        r.device.startFitTest();
        QCOMPARE(r.device.values().value("fitRunning").toBool(), true);
        QTRY_COMPARE(fit.size(), 1);
        QCOMPARE(r.device.values().value("fitLeft").toInt(), 0);
        QCOMPARE(r.device.values().value("fitRight").toInt(), 1);

        r.device.ring(PartRight, true);
        QTRY_COMPARE(r.sentWith(Cmd::FindWrite).size(), 1);
        QCOMPARE(r.sentWith(Cmd::FindWrite).first().payload, hex("0301"));
        r.device.ring(PartRight, false);
        QTRY_COMPARE(r.sentWith(Cmd::FindWrite).last().payload, hex("0300"));

        r.device.connectPairedDevice("1C:2B:3A:49:58:67", false);
        QTRY_COMPARE(r.sentWith(Cmd::DualConnectWrite).size(), 1);
        QTRY_VERIFY(!r.device.pairedDevices().at(1).toMap().value("connected").toBool());
    }

    // --- Official app import, updates, tray -------------------------------------------
    void importsAnOfficialApp() {
        const QString tar = QStandardPaths::findExecutable("bsdtar");
        if (tar.isEmpty())
            QSKIP("bsdtar is needed to build a test .xapk");
        QTemporaryDir dir;
        const QString apk = dir.filePath("apk");
        QDir().mkpath(apk + "/assets/flutter_assets/assets/config");
        QDir().mkpath(apk + "/assets/flutter_assets/assets/images/ear");
        QDir().mkpath(apk + "/assets/flutter_assets/assets/images/watch");
        auto put = [](const QString &path, const QByteArray &data) {
            QFile f(path);
            f.open(QIODevice::WriteOnly);
            f.write(data);
        };
        put(apk + "/assets/flutter_assets/assets/config/ear_white_list.json",
            R"([{"id":"B173","supportId":"B201","configs":[{"ancLevel":63}]},{"id":"B201","configs":[{"ancLevel":63}]}])");
        put(apk + "/assets/flutter_assets/assets/config/devices_info_list.json",
            "{\"deviceSkuResps\":[{\"colorId\":\"01\",\"deviceSpu\":{\"modelId\":\"B173\",\"name\":\"Ear (3)\"},"
            "\"globalImageUrl\":\"https://example.invalid/x.png\"}]}");
        put(apk + "/assets/flutter_assets/assets/images/ear/card_anc_on.svg", "<svg/>");
        put(apk + "/assets/flutter_assets/assets/images/watch/skip.svg", "<svg/>");
        put(dir.filePath("manifest.json"), R"({"package_name":"com.nothing.smartcenter","version_name":"3.11.0"})");
        auto zip = [&](const QString &out, const QString &from, const QStringList &what) {
            return QProcess::execute(tar, QStringList{"--format", "zip", "-cf", out, "-C", from} + what) == 0;
        };
        QVERIFY(zip(dir.filePath("com.nothing.smartcenter.apk"), apk, {"assets"}));
        QVERIFY(zip(dir.filePath("Nothing X_3.11.0.xapk"), dir.path(), {"manifest.json", "com.nothing.smartcenter.apk"}));

        const QString cache = dir.filePath("cache");
        QString version, error;
        QVERIFY2(OfficialPack::unpack(dir.filePath("Nothing X_3.11.0.xapk"), cache, &version, &error), qPrintable(error));
        QCOMPARE(version, QString("3.11.0"));
        OfficialPack pack(cache);
        QVERIFY(pack.available());
        QCOMPARE(pack.version(), QString("3.11.0"));
        QVERIFY(pack.hasAsset("assets/images/ear/card_anc_on.svg"));
        QVERIFY(!pack.hasAsset("assets/images/watch/skip.svg"));
        QVERIFY(!pack.hasAsset("../../etc/passwd"));
        QCOMPARE(pack.modelConfig("b173").value("configs").toArray().first().toObject().value("ancLevel").toInt(), 63);
        // A real Ear (3) reports B201, which the official list files under B173.
        QCOMPARE(pack.productId("B201"), QString("B173"));
        QCOMPARE(pack.productId("B173"), QString("B173"));
        QCOMPARE(pack.productId("XXXX"), QString("XXXX"));
        // A file that isn't the official app is refused.
        put(dir.filePath("other.apk"), "not a zip");
        QVERIFY(!OfficialPack::unpack(dir.filePath("other.apk"), cache, &version, &error));
        QVERIFY(OfficialPack(cache).available());
    }

    void versions() {
        QCOMPARE(OfficialPack::versionFromName("Nothing+X_3.10.3_APKPure.xapk"), QString("3.10.3"));
        QCOMPARE(OfficialPack::compareVersions("3.10.3", "3.9.12"), 1);
        QCOMPARE(OfficialPack::compareVersions("0.1.0", "0.1"), 0);
        QCOMPARE(OfficialPack::compareVersions("0.1.0", "0.2.0"), -1);
    }

    void updaterPicksTheRightFile() {
        const QJsonArray assets = QJsonDocument::fromJson(
            "[{\"name\":\"nothingx-0.2.0-1-x86_64.pkg.tar.zst\",\"browser_download_url\":\"https://example.invalid/a\"},"
            "{\"name\":\"NothingX-Setup-0.2.0.exe\",\"browser_download_url\":\"https://example.invalid/b\"}]").array();
        QCOMPARE(Updater::pickAsset(assets, true).value("name").toString(), QString("NothingX-Setup-0.2.0.exe"));
        QCOMPARE(Updater::pickAsset(assets, false).value("name").toString(), QString("nothingx-0.2.0-1-x86_64.pkg.tar.zst"));
        QVERIFY(Updater::releasesUrl().contains("psyduck-z/Nothing-X-Desktop"));
    }

    void trayShowsBatteryRoundedToFive() {
        QCOMPARE(TrayIcon::roundedLevel(86), 85);
        QCOMPARE(TrayIcon::roundedLevel(88), 90);
        QCOMPARE(TrayIcon::roundedLevel(3), 5);
        QCOMPARE(TrayIcon::roundedLevel(100), 100);
        QVERIFY(!TrayIcon::renderImage(TrayIcon::Battery, 42, false, 22).isNull());
    }

    // --- Platform plumbing ---------------------------------------------------------------
    void dockedPopupPosition() {
        const QRect screen(0, 0, 1920, 1080);
        const QRect bottomBar(0, 0, 1920, 1032);
        QCOMPARE(PopupPlacer::dockedPosition(screen, bottomBar, QRect(1800, 1040, 24, 24), QSize(384, 660), 10),
                 QPoint(1920 - 384 - 10, 1032 - 660 - 10));
        QCOMPARE(PopupPlacer::dockedPosition(screen, bottomBar, QRect(), QSize(384, 660), 10), QPoint(1526, 362));
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
    }
};

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    Tests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "nothingx_tests.moc"
