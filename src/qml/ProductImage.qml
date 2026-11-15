import QtQuick

// A product photo of the connected model in its colour, the way the official
// app shows it: downloaded from Nothing's image server once, with the
// offline photos from the imported app as a fallback.
Image {
    id: root
    // global, left, right, box, setting, control
    property string kind: "global"
    property int revision: 0
    readonly property string colorName: device.colorId === "02" ? "white" : "black"
    readonly property string offline: {
        if (device.modelId !== "B173")
            return ""
        switch (kind) {
        case "left": return "native/drawable/ear_three_" + colorName + "_left.png"
        case "right": return "native/drawable/ear_three_" + colorName + "_right.png"
        case "box": return "native/drawable/os_ear_three_" + colorName + "_case.png"
        default: return "native/drawable/os_ear_three_" + colorName + "_disconnect.png"
        }
    }
    source: {
        revision
        const online = official.productImage(device.modelId, device.colorId, kind)
        if (online.toString() !== "")
            return online
        return offline !== "" && official.hasAsset(offline) ? official.asset(offline) : ""
    }
    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: true
    asynchronous: true
    Connections {
        target: official
        function onImageReady() { root.revision++ }
        function onChanged() { root.revision++ }
    }
}
