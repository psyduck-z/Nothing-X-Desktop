#pragma once

#include <QJsonObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// What a model can do, read from the official app's per-model configuration
// (ear_white_list.json in the imported official app). The flags decode the
// way the official app decodes them (UnknownFunction/UnknownProduct):
// ancLevel and eq are bitmasks, spatialAudio and highQualityAudio carry one
// bit per mode or codec. When no official app has been imported yet, the
// Ear (3)'s configuration is built in, so the app works from the start.
struct ModelConfig {
    QString id;               // "B173"
    QString name;             // "Ear (3)"
    QVariantMap features;     // feature -> bool/number, for QML
    QVariantList eqPresets;   // wire values in the official UI order
    QVariantList gestureGroups;
    QString defaultControl;   // hex: the factory gesture table
    struct {
        double lowFreq = 140, lowQ = 0.8, peakFreq = 980, peakQ = 0.7, highFreq = 3400, highQ = 1.0;
    } customEq;
    float advancedTotalGain = -6.0f;

    bool has(const QString &feature) const { return features.value(feature).toBool(); }
    bool valid() const { return !id.isEmpty(); }

    // entry: one element of ear_white_list.json ({"id": ..., "configs": [...]}).
    static ModelConfig fromOfficial(const QJsonObject &entry, const QString &name = {});
    // The Ear (3), for when no official app is imported.
    static ModelConfig builtInEar3();
    // A model guess from the Bluetooth name ("Nothing Ear (3)" -> "B173").
    static QString idFromName(const QString &bluetoothName);
};
