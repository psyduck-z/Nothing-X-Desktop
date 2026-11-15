#include "modelconfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

namespace {

// The official UI lists the presets as Balanced, More bass, More treble,
// Voice, Custom (IOTProductDevice). Bit i of the "eq" mask enables the
// preset whose wire value is EqBitToWire[i].
constexpr int EqBitToWire[] = {0, 1, 2, 3, 5, 6, 7};
constexpr int EqUiOrder[] = {0, 3, 2, 1, 5, 6, 7};

QVariantList parseGestureGroups(const QJsonObject &controls) {
    QVariantList out;
    for (const QJsonValue &groupValue : controls.value("earGestureGroups").toArray()) {
        const QJsonObject group = groupValue.toObject();
        const QString groupName = group.value("groupName").toString();
        for (const QJsonValue &gv : group.value("gestures").toArray()) {
            const QJsonObject g = gv.toObject();
            QVariantList options;
            for (const QJsonValue &ogv : g.value("operationGroups").toArray()) {
                const QJsonObject og = ogv.toObject();
                for (const QJsonValue &ov : og.value("operations").toArray()) {
                    const QJsonObject o = ov.toObject();
                    QStringList keys;
                    for (const QJsonValue &k : o.value("operationDescKey").toArray())
                        keys << k.toString();
                    options.append(QVariantMap{
                        {"action", o.value("operation").toInt()},
                        {"keys", keys},
                        {"custom", o.value("isCustom").toBool()},
                        // supportPlatform 1: Nothing phones only (News, Essential Space).
                        {"phoneOnly", o.value("supportPlatform").toInt() == 1},
                        {"chooser", o.value("childType").toInt() == 1},
                        {"tipKey", o.value("tipKey").toString()},
                        {"group", og.value("groupName").toString()},
                    });
                }
            }
            const QJsonObject defaults = g.value("defaultOperation").toObject();
            out.append(QVariantMap{
                {"device", g.value("deviceType").toInt()},
                {"button", g.value("button").toInt(1)},
                {"gesture", g.value("gesture").toInt()},
                {"custom", g.value("isCustom").toBool()},
                {"nameKey", g.value("gestureNameKey").toString()},
                {"imageKey", g.value("gestureImgKey").toString()},
                {"defaultAction", defaults.value("default").toInt(-1)},
                {"groupName", groupName},
                {"options", options},
            });
        }
    }
    return out;
}

// The Ear (3)'s entry from the official app's ear_white_list.json, reduced
// to what this app reads, for use before an official app is imported.
constexpr const char *Ear3Config = R"json({
 "id": "B173",
 "configs": [{
  "ancLevel": 63, "eq": 31, "advancedEq": 1, "advancedEqTotalGain": -6, "ultraBass": 1,
  "spatialAudio": 6, "highQualityAudio": 4, "audiodo": 1, "dualConnection": 1,
  "dualConnectionReboot": 0, "earTipFitTest": 1, "testFitDuration": 10, "superMic": 1,
  "callTransparency": 1, "walkieTalkieMode": 1, "spaceOrAdvance": 1, "supportExplore": 1,
  "defaultControl": "0902010209030102090201030803010308020107160301071602010901030109010401070B",
  "customEQ": {"freqLow": 140.0, "qLow": 0.8, "freqPeak": 980.0, "qPeak": 0.7, "freqHigh": 3400.0, "qHigh": 1.0},
  "controls": {"isShowBox": true, "isSupportEssentialSpace": true, "earGestureGroups": [
   {"gestures": [
    {"isCustom": true, "deviceType": 2, "button": 1, "gesture": 2, "gestureNameKey": "control_double_press",
     "gestureImgKey": "control_controlIcIndexTwo", "defaultOperation": {"default": 9}, "operationGroups": [
      {"groupName": "", "operations": [
       {"operation": 9, "isCustom": true, "operationDescKey": ["control_skip_forward"]},
       {"operation": 8, "isCustom": true, "operationDescKey": ["control_skip_back"]},
       {"operation": 11, "isCustom": true, "operationDescKey": ["control_voice_assistant"], "childType": 1},
       {"operation": 31, "isCustom": true, "operationDescKey": ["news_description"], "supportPlatform": 1}]},
      {"groupName": "gesture_not_customisation", "operations": [
       {"operation": 29, "isCustom": false, "operationDescKey": ["control_decline_incoming_call"]}]}]},
    {"isCustom": true, "deviceType": 2, "button": 1, "gesture": 3, "gestureNameKey": "control_triple_press",
     "gestureImgKey": "control_controlIcIndexThree", "defaultOperation": {"default": 8}, "operationGroups": [
      {"groupName": "", "operations": [
       {"operation": 9, "isCustom": true, "operationDescKey": ["control_skip_forward"]},
       {"operation": 8, "isCustom": true, "operationDescKey": ["control_skip_back"]},
       {"operation": 11, "isCustom": true, "operationDescKey": ["control_voice_assistant"], "childType": 1},
       {"operation": 31, "isCustom": true, "operationDescKey": ["news_description"], "supportPlatform": 1},
       {"operation": 1, "isCustom": true, "operationDescKey": ["control_no_action"]}]}]},
    {"isCustom": true, "deviceType": 2, "button": 1, "gesture": 7, "gestureNameKey": "Hold",
     "gestureImgKey": "control_controlIcIndexTapHold", "defaultOperation": {"default": 22}, "operationGroups": [
      {"groupName": "", "operations": [
       {"operation": 10, "isCustom": true, "operationDescKey": ["anc_noise_control"]},
       {"operation": 11, "isCustom": true, "operationDescKey": ["control_voice_assistant"], "childType": 1},
       {"operation": 31, "isCustom": true, "operationDescKey": ["news_description"], "supportPlatform": 1},
       {"operation": 18, "isCustom": true, "operationDescKey": ["control_volume_up"]},
       {"operation": 19, "isCustom": true, "operationDescKey": ["control_volume_down"]},
       {"operation": 29, "isCustom": true, "operationDescKey": ["control_mic_on_off"]},
       {"operation": 1, "isCustom": true, "operationDescKey": ["control_no_action"]}]}]},
    {"isCustom": true, "deviceType": 2, "button": 1, "gesture": 9, "gestureNameKey": "control_double_press_hold",
     "gestureImgKey": "control_controlIcIndexDoubleTapHold", "defaultOperation": {"default": 1}, "operationGroups": [
      {"groupName": "", "operations": [
       {"operation": 10, "isCustom": true, "operationDescKey": ["anc_noise_control"]},
       {"operation": 11, "isCustom": true, "operationDescKey": ["control_voice_assistant"], "childType": 1},
       {"operation": 31, "isCustom": true, "operationDescKey": ["news_description"], "supportPlatform": 1},
       {"operation": 18, "isCustom": true, "operationDescKey": ["control_volume_up"]},
       {"operation": 19, "isCustom": true, "operationDescKey": ["control_volume_down"]},
       {"operation": 1, "isCustom": true, "operationDescKey": ["control_no_action"], "childType": 1}]}]},
    {"isCustom": true, "deviceType": 4, "button": 1, "gesture": 7, "gestureNameKey": "hold",
     "gestureImgKey": "control_controlIcIndexTapHold", "defaultOperation": {"default": 11, "nothing": 33}, "operationGroups": [
      {"groupName": "talk_button_type", "operations": [
       {"operation": 29, "isCustom": false, "operationDescKey": ["talk_button_title"], "tipKey": "talk_button_switch"}]},
      {"groupName": "talk_button_type_2", "operations": [
       {"operation": 33, "isCustom": true, "operationDescKey": ["essential_space"], "supportPlatform": 1, "tipKey": "supermic_essential_space"},
       {"operation": 11, "isCustom": true, "operationDescKey": ["control_voice_assistant"], "childType": 1, "tipKey": "supermic_voice_assistant"},
       {"operation": 1, "isCustom": true, "operationDescKey": ["control_no_action"]}]}]},
    {"isCustom": true, "deviceType": 4, "button": 1, "gesture": 2, "gestureNameKey": "double_press",
     "gestureImgKey": "control_controlIcIndexTwo", "defaultOperation": {"default": 29}, "operationGroups": [
      {"groupName": "empty", "operations": [
       {"operation": 29, "isCustom": false, "operationDescKey": ["gesture_latch_supermic"], "tipKey": "gesture_latch_supermic_decs"}]}]}]},
   {"groupName": "gesture_not_customisation", "gestures": [
    {"isCustom": false, "deviceType": 2, "button": 1, "gesture": 3, "gestureNameKey": "control_single_press",
     "gestureImgKey": "control_controlIcIndexOne", "operationGroups": [
      {"groupName": "", "operations": [
       {"operation": -1, "isCustom": false, "operationDescKey": ["control_play_pause", "control_answer_hang_up"]}]}]}]}]}
 }]
})json";

} // namespace

ModelConfig ModelConfig::fromOfficial(const QJsonObject &entry, const QString &name) {
    ModelConfig m;
    m.id = entry.value("id").toString().toUpper();
    m.name = name;
    const QJsonObject c = entry.value("configs").toArray().first().toObject();
    auto num = [&c](const char *key, int fallback = 0) { return c.contains(key) ? c.value(key).toInt() : fallback; };
    QVariantMap f;

    // Noise control: bit 1 Mid, 2 Low, 3 Adaptive, 4 Transparency; High is
    // always offered when ANC is (getSupportANCLevel counts it as the +1).
    const int anc = num("ancLevel");
    f["anc"] = anc != 0;
    f["ancMid"] = bool(anc & 2);
    f["ancLow"] = bool(anc & 4);
    f["ancAdaptive"] = bool(anc & 8);
    f["transparency"] = bool(anc & 16);

    const int eq = num("eq");
    QVariantList presets;
    for (int wire : EqUiOrder)
        for (int bit = 0; bit < 7; ++bit)
            if (EqBitToWire[bit] == wire && (eq & (1 << bit)))
                presets << wire;
    m.eqPresets = presets;
    f["eq"] = !presets.isEmpty();
    f["customEq"] = bool(eq & (1 << 4));
    f["advancedEq"] = num("advancedEq") == 1;
    if (c.contains("advancedEqTotalGain"))
        m.advancedTotalGain = float(c.value("advancedEqTotalGain").toDouble());
    f["eqExplore"] = num("supportExplore") == 1;
    f["advancedTotalGain"] = double(m.advancedTotalGain);
    f["ultraBass"] = num("ultraBass") == 1;

    const int spatial = num("spatialAudio");
    f["spatial"] = spatial != 0;
    f["spatialHead"] = bool(spatial & 4);
    const int hqa = num("highQualityAudio");
    f["ldac"] = bool(hqa & 4);
    f["lhdc"] = bool(hqa & 2);
    // Earbuds never set these keys; headphones and speakers set 0 to hide.
    f["lowLag"] = num("lowLagMode", 1) != 0;
    f["inEar"] = num("earDetection", 1) != 0;
    f["find"] = num("findDevice", 1) != 0;

    f["personalSound"] = num("audiodo") == 1 || num("mimi") == 1;
    f["dual"] = num("dualConnection") == 1;
    f["dualReboot"] = num("dualConnectionReboot") == 1;
    f["fitTest"] = num("earTipFitTest") == 1;
    f["fitDuration"] = num("testFitDuration", 10);
    f["superMic"] = num("superMic") == 1;
    f["callTransparency"] = num("callTransparency") == 1;
    f["walkieTalkie"] = num("walkieTalkieMode") == 1;
    f["spaceOrAdvance"] = num("spaceOrAdvance") == 1;

    const QJsonObject controls = c.value("controls").toObject();
    m.gestureGroups = parseGestureGroups(controls);
    f["gestures"] = !m.gestureGroups.isEmpty();
    f["caseButton"] = controls.value("isShowBox").toBool();
    m.defaultControl = c.value("defaultControl").toString();

    const QJsonObject custom = c.value("customEQ").toObject();
    if (!custom.isEmpty()) {
        m.customEq.lowFreq = custom.value("freqLow").toDouble(140);
        m.customEq.lowQ = custom.value("qLow").toDouble(0.8);
        m.customEq.peakFreq = custom.value("freqPeak").toDouble(980);
        m.customEq.peakQ = custom.value("qPeak").toDouble(0.7);
        m.customEq.highFreq = custom.value("freqHigh").toDouble(3400);
        m.customEq.highQ = custom.value("qHigh").toDouble(1.0);
    }
    m.features = f;
    return m;
}

ModelConfig ModelConfig::builtInEar3() {
    return fromOfficial(QJsonDocument::fromJson(Ear3Config).object(), QStringLiteral("Ear (3)"));
}

QString ModelConfig::idFromName(const QString &bluetoothName) {
    static const QRegularExpression ear3(R"(\bEar\s*\(3\))", QRegularExpression::CaseInsensitiveOption);
    return ear3.match(bluetoothName).hasMatch() ? QStringLiteral("B173") : QString();
}
