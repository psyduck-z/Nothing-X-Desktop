#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

// What each Nothing model supports, so the popup only shows controls the
// earbuds understand. The Ear (3) entry is complete (it follows the Nothing X
// app's own B173 configuration as published by ear-web); other models get a
// conservative common subset.
struct Model {
    QString id;      // "B173"
    QString name;    // "Nothing Ear (3)"
    QSet<QString> features;
    // Noise cancellation strengths offered besides Off/Transparency.
    bool ancLevels = false;   // High / Mid / Low
    bool ancAdaptive = false;
    bool hasCase = true;

    bool has(const QString &feature) const { return features.contains(feature); }
};

namespace Models {
Model byId(const QString &id);
// Best guess from the Bluetooth name, used until the model id is read.
Model byName(const QString &name);
Model generic(const QString &name);
QStringList allFeatures();
}
