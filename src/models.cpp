#include "models.h"

namespace Models {

QStringList allFeatures() {
    return {"battery", "anc", "eq", "customEq", "advancedEq", "bass", "spatial", "personalSound",
            "gestures", "caseButton", "fitTest", "findBuds", "inEar", "lowLatency", "ldac",
            "dualConnection", "superMic", "micMode", "callTransparency", "walkieTalkie", "deviceInfo"};
}

static Model ear3() {
    Model m;
    m.id = "B173";
    m.name = "Nothing Ear (3)";
    for (const auto &f : allFeatures())
        m.features.insert(f);
    m.ancLevels = true;
    m.ancAdaptive = true;
    return m;
}

Model generic(const QString &name) {
    Model m;
    m.name = name.isEmpty() ? QStringLiteral("Nothing earbuds") : name;
    m.features = {"battery", "anc", "eq", "inEar", "lowLatency", "findBuds", "deviceInfo"};
    return m;
}

Model byId(const QString &id) {
    if (id == "B173")
        return ear3();

    struct Known { const char *id; const char *name; bool levels; bool adaptive; QSet<QString> extra; };
    static const QList<Known> known{
        {"B181", "Nothing Ear (1)", false, false, {}},
        {"B155", "Nothing Ear (2)", true, true, {"customEq", "fitTest", "dualConnection", "ldac"}},
        {"B171", "Nothing Ear", true, true, {"customEq", "advancedEq", "bass", "fitTest", "dualConnection", "ldac"}},
        {"B162", "Nothing Ear (a)", true, true, {"customEq", "bass", "fitTest", "dualConnection"}},
        {"B174", "Nothing Ear (open)", false, false, {"customEq", "advancedEq", "dualConnection"}},
        {"B170", "Nothing Headphone (1)", true, true, {"customEq", "advancedEq", "bass", "spatial", "dualConnection", "ldac"}},
        {"B172", "CMF Buds Pro 2", true, true, {"dualConnection"}},
        {"B168", "CMF Buds", false, false, {"dualConnection"}},
        {"B179", "CMF Buds 2", true, true, {"dualConnection"}},
        {"B175", "CMF Headphone Pro", true, true, {"dualConnection", "ldac", "spatial"}},
    };
    for (const auto &k : known) {
        if (id == QLatin1String(k.id)) {
            Model m = generic(QString::fromLatin1(k.name));
            m.id = id;
            m.ancLevels = k.levels;
            m.ancAdaptive = k.adaptive;
            m.features.unite(k.extra);
            if (id == "B174" || id == "B168")
                m.features.remove("anc");
            if (id == "B170" || id == "B175")
                m.hasCase = false;
            return m;
        }
    }
    Model m = generic({});
    m.id = id;
    return m;
}

Model byName(const QString &name) {
    const QString n = name.toLower();
    if (n.contains("ear (3)") || n.contains("ear(3)"))
        return byId("B173");
    if (n.contains("ear (2)"))
        return byId("B155");
    if (n.contains("ear (a)"))
        return byId("B162");
    if (n.contains("ear (open)"))
        return byId("B174");
    if (n.contains("ear (1)"))
        return byId("B181");
    if (n.contains("headphone (1)"))
        return byId("B170");
    return generic(name);
}

} // namespace Models
