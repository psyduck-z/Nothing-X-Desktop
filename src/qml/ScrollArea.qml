import QtQuick
import QtQuick.Controls

// Scrolling that keeps up with the rest of the desktop. Qt's own wheel
// handling moves a Flickable a few pixels per event, which feels sluggish on
// touchpads and wheels alike; here a touchpad moves the page 1:1 and a wheel
// notch about three lines, and a drag still flicks.
Flickable {
    id: flick
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickDeceleration: 3000
    maximumFlickVelocity: 5000
    pixelAligned: true
    ScrollBar.vertical: ScrollBar { width: 4; policy: ScrollBar.AsNeeded }

    function scrollBy(dy) {
        const max = Math.max(0, contentHeight - height)
        contentY = Math.max(0, Math.min(max, contentY + dy))
    }

    WheelHandler {
        target: null
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: (event) => {
            flick.cancelFlick()
            if (event.pixelDelta.y !== 0)
                flick.scrollBy(-event.pixelDelta.y)
            else
                flick.scrollBy(-event.angleDelta.y / 120 * 100)
            event.accepted = true
        }
    }
}
