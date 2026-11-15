.pragma library

// The official app's wording for the keys in its device configuration
// (gesture names, actions, tips), as its Flutter screens show them in
// British English. Keys this table doesn't know are shown humanised.

const text = {
    // Gestures
    "control_single_press": "Single pinch",
    "control_double_press": "Double pinch",
    "control_triple_press": "Triple pinch",
    "Hold": "Pinch and hold",
    "control_double_press_hold": "Double pinch and hold",
    "hold": "Press and hold",
    "double_press": "Double press",
    // Actions
    "control_skip_forward": "Skip forward",
    "control_skip_back": "Skip back",
    "control_voice_assistant": "Voice assistant",
    "news_description": "News Reporter",
    "control_no_action": "No action",
    "control_volume_up": "Volume up",
    "control_volume_down": "Volume down",
    "control_decline_incoming_call": "Decline incoming call",
    "control_play_pause": "Play / Pause",
    "control_answer_hang_up": "Answer / Hang up calls",
    "anc_noise_control": "Noise control",
    "control_mic_on_off": "Mic on/off",
    "essential_space": "Essential Space",
    "talk_button_title": "Talk button",
    "gesture_latch_supermic": "Latch Super Mic",
    // Groups
    "gesture_not_customisation": "Not customisable",
    "talk_button_type": "During calls",
    "talk_button_type_2": "Outside of calls",
    // Tips
    "talk_button_switch": "Press and hold the TALK button to switch earphone mic to Super Mic, release to revert to default.",
    "supermic_essential_space": "Press and hold the TALK button to start recording in Essential Space. Release to save the recording. To use this feature, at least one earbud must be in use.",
    "supermic_voice_assistant": "Press and hold the TALK button to activate voice assistant. To use this feature, at least one earbud must be in use.",
    "gesture_latch_supermic_decs": "Double-press the TALK button to use Super Mic for longer conversations. Return to the earbuds mic with a single press."
}

function t(key) {
    if (text[key] !== undefined)
        return text[key]
    const words = String(key).replace(/^control_/, "").replace(/_/g, " ")
    return words.charAt(0).toUpperCase() + words.slice(1)
}

// The current action of a gesture, by the number the earbuds report.
function action(code, device) {
    switch (code) {
    case 1: return "No action"
    case 2: return "Play / Pause"
    case 8: return "Skip back"
    case 9: return "Skip forward"
    case 10: return "Noise control"
    case 11: return "Voice assistant"
    case 18: return "Volume up"
    case 19: return "Volume down"
    case 20: return "Noise control"
    case 21: return "Noise control"
    case 22: return "Noise control"
    case 29: return device === 4 ? "Super Mic" : "Mic on/off"
    case 31: return "News Reporter"
    case 33: return "Essential Space"
    }
    return code < 0 ? "" : "Action " + code
}

function isNoiseCycle(code) { return code === 10 || (code >= 20 && code <= 22) }

// The modes a noise-control cycle goes through (ControlConfigurationEntity).
function cycleModes(code) {
    switch (code) {
    case 10: return { anc: true, transparency: true, off: true }
    case 20: return { anc: true, transparency: false, off: true }
    case 21: return { anc: false, transparency: true, off: true }
    case 22: return { anc: true, transparency: true, off: false }
    }
    return { anc: true, transparency: true, off: false }
}

// Back to the byte; 0 when fewer than two modes are left (not allowed).
function cycleCode(m) {
    if (m.anc && m.transparency && m.off) return 10
    if (m.anc && m.off) return 20
    if (m.transparency && m.off) return 21
    if (m.anc && m.transparency) return 22
    return 0
}

function cycleSummary(code) {
    const m = cycleModes(code)
    const parts = []
    if (m.anc) parts.push("Noise Cancellation")
    if (m.transparency) parts.push("Transparency")
    if (m.off) parts.push("Off")
    return parts.join(" · ")
}

// The official gesture icons, by the config's image key.
function gestureIcon(key) {
    const files = {
        "control_controlIcIndexOne": "control_ic_index_one",
        "control_controlIcIndexTwo": "control_ic_index_two",
        "control_controlIcIndexThree": "control_ic_index_three",
        "control_controlIcIndexTapHold": "control_ic_index_tap_hold",
        "control_controlIcIndexDoubleTapHold": "control_ic_index_double_tap_hold"
    }
    return files[key] ? "assets/images/ear/control/" + files[key] + ".svg" : ""
}

function presetName(p) {
    switch (p) {
    case 0: return "Balanced"
    case 1: return "Voice"
    case 2: return "More treble"
    case 3: return "More bass"
    case 5: return "Custom"
    case 6: return "New voice"
    case 7: return "New instrument"
    }
    return ""
}

function presetDescription(p) {
    switch (p) {
    case 0: return "Delivers a natural and balanced sound."
    case 1: return "Brings vocals forward in the mix."
    case 2: return "Enhances high-frequency detail for a brighter, clearer sound."
    case 3: return "Enhances bass for a deeper, more powerful sound."
    case 5: return "Shape the bass, mid and treble yourself."
    }
    return ""
}

function presetIcon(p) {
    switch (p) {
    case 0: return "assets/images/ear/eq/balance_icon_light.svg"
    case 1: return "assets/images/ear/eq/voice_icon_light.svg"
    case 2: return "assets/images/ear/eq/more_treble_icon_light.svg"
    case 3: return "assets/images/ear/eq/more_bass_icon_light.svg"
    case 5: return "assets/images/ear/eq/simple_custom_icon_light.svg"
    }
    return ""
}

function frequencyLabel(f) {
    return f >= 1000 ? (Math.round(f / 100) / 10) + " kHz" : Math.round(f) + " Hz"
}
