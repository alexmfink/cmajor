//
//     ,ad888ba,                              88
//    d8"'    "8b
//   d8            88,dba,,adba,   ,aPP8A.A8  88     The Cmajor Toolkit
//   Y8,           88    88    88  88     88  88
//    Y8a.   .a8P  88    88    88  88,   ,88  88     (C)2022 Sound Stacks Ltd
//     '"Y888Y"'   88    88    88  '"8bbP"Y8  88     https://cmajor.dev
//                                           ,88
//                                        888P"
//
//  Cmajor may be used under the terms of the ISC license:
//
//  Permission to use, copy, modify, and/or distribute this software for any purpose with or
//  without fee is hereby granted, provided that the above copyright notice and this permission
//  notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//  WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//  AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//  CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

#include <array>
#include <string_view>

namespace cmaj
{

/// This contains the javascript and other asset files availble to patch views running in
/// a browser envirtonment.
///
/// It contains modules that provide the generic GUI view, and other helper classes that
/// can be used by custom views.
///
struct EmbeddedWebAssets
{
    static std::string_view findResource (std::string_view path)
    {
        for (auto& file : files)
            if (path == file.name)
                return file.content;

        return {};
    }

private:
    struct File { std::string_view name, content; };

    static constexpr const char* cmajgenericpatchview_js =
        R"(//  //
//  //     ,ad888ba,                                88
//  //    d8"'    "8b
//  //   d8            88,dPba,,adPba,   ,adPPYba,  88      The Cmajor Language
//  //   88            88P'  "88"   "8a        '88  88
//  //   Y8,           88     88     88  ,adPPPP88  88      (c)2022 Sound Stacks Ltd
//  //    Y8a.   .a8P  88     88     88  88,   ,88  88      https://cmajor.dev
//  //     '"Y888Y"'   88     88     88  '"8bbP"Y8  88
//  //                                             ,88
//  //                                           888P"

export default function createPatchView (patchConnection)
{
    return new PatchView (patchConnection);
}

class PatchView extends HTMLElement
{
    constructor (patchConnection)
    {
        super();

        this.patchConnection = patchConnection;
        this.state = {};
        this.parameterChangedListeners = {};

        this.attachShadow ({ mode: "open" });
        this.shadowRoot.innerHTML = this.getHTML();

        this.titleElement      = this.shadowRoot.getElementById ("patch-title");
        this.parametersElement = this.shadowRoot.getElementById ("patch-parameters");
    }

    connectedCallback()
    {
        this.connection = this.createPatchConnection();
        this.connection.requestStatusUpdate();

        const font = document.createElement ("link");
        font.href = "/cmaj_api/assets/ibmplexmono/v12/ibmplexmono.css";
        font.rel = "stylesheet"
        document.head.appendChild (font);
    }

    createPatchConnection()
    {
        this.patchConnection.onPatchStatusChanged       = this.onPatchStatusChanged.bind (this);
        this.patchConnection.onSampleRateChanged        = this.onSampleRateChanged.bind (this);
        this.patchConnection.onParameterEndpointChanged = this.onParameterEndpointChanged.bind (this);
        this.patchConnection.onOutputEvent              = this.onOutputEvent.bind (this);)"
R"(

        return {
            requestStatusUpdate:       this.patchConnection.requestStatusUpdate.bind (this.patchConnection),
            sendParameterGestureStart: this.patchConnection.sendParameterGestureStart.bind (this.patchConnection),
            sendEventOrValue:          this.patchConnection.sendEventOrValue.bind (this.patchConnection),
            sendParameterGestureEnd:   this.patchConnection.sendParameterGestureEnd.bind (this.patchConnection),

            performSingleEdit: (endpointID, value) =>
            {
                this.patchConnection.sendParameterGestureStart (endpointID);
                this.patchConnection.sendEventOrValue (endpointID, value);
                this.patchConnection.sendParameterGestureEnd (endpointID);
            },
        };
    }

    notifyParameterChangedListeners (endpointID, parameter)
    {
        this.parameterChangedListeners[endpointID]?.forEach (notify => notify (parameter));
    }

    onPatchStatusChanged (buildError, manifest, inputEndpoints, outputEndpoints)
    {
        this.parametersElement.innerHTML = "";

        const initialState = {
            title: manifest?.name ?? "Cmajor",
            status: buildError ?? "",
            inputs:  this.toInputControls (inputEndpoints),
            outputs: this.toOutputControls (outputEndpoints),
        };

        const clear = object => Object.keys (object).forEach (key => delete object[key]);
        clear (this.parameterChangedListeners);
        clear (this.state);
        Object.assign (this.state, initialState);)"
R"(

        this.renderInitialState ({
                onBeginEdit: (endpointID)              => this.connection.sendParameterGestureStart (endpointID),
                onEdit: (endpointID, value)            => this.connection.sendEventOrValue (endpointID, value),
                onEndEdit: (endpointID)                => this.connection.sendParameterGestureEnd (endpointID),
                performSingleEdit: (endpointID, value) => this.connection.performSingleEdit (endpointID, value),
            });

        this.state.inputs.forEach (({ endpointID }) => this.patchConnection.requestEndpointValue (endpointID))
    }

    onParameterEndpointChanged (endpointID, value)
    {
        const currentInputs = this.state.inputs;

        if (currentInputs)
        {
            const index = currentInputs.findIndex (p => p.endpointID === endpointID);

            if (index >= 0)
            {
                const currentParameter = currentInputs[index];
                currentParameter.value = value;
                this.notifyParameterChangedListeners (endpointID, currentParameter);
            }
        }
    }

    onOutputEvent (endpointID, value) {}
    onSampleRateChanged() {}

    addParameterChangedListener (endpointID, update)
    {
        let listeners = this.parameterChangedListeners[endpointID];

        if (! listeners)
            listeners = this.parameterChangedListeners[endpointID] = [];

        listeners.push (update);
    }

    toInputControls (inputEndpoints)
    {
        return (inputEndpoints || []).filter (e => e.purpose === "parameter").map (this.toParameterControl);
    }

    toParameterControl ({ endpointID, annotation })
    {
        const common =
        {
            name: annotation?.name,
            endpointID,
            annotation,
            unit: annotation.unit,
            defaultValue: annotation.init,
        };

        if (annotation.boolean)
        {
            return { type: "switch", ...common, };
        })"
R"(

        const textOptions = annotation.text?.split?.("|");
        if (textOptions?.length > 1)
        {
            const hasUserDefinedRange = annotation.min != null && annotation.max != null;
            const { min, step } = hasUserDefinedRange
                ? { min: annotation.min, step: (annotation.max - annotation.min) / (textOptions.length - 1) }
                : { min: 0, step: 1 };

            return {
                type: "options",
                options: textOptions.map ((text, index) => ({ value: min + (step * index), text })),
                ...common,
            };
        }

        return {
            type: "slider",
            min: annotation.min ?? 0,
            max: annotation.max ?? 1,
            ...common,
        };
    }

    toOutputControls (endpoints)
    {
        return [];
    }

    renderInitialState (backend)
    {
        this.titleElement.innerText = this.state.title;

        this.state.inputs.forEach (({ type, value, name, ...other }, index) =>
        {
            const control = this.makeControl (backend, { type, value, name, index, ...other });

            if (control)
            {
                const mapValue = control.mapValue ?? (v => v);
                const wrapped = this.makeLabelledControl (control.control, {
                    initialValue: mapValue (other.defaultValue),
                    name,
                    toDisplayValue: control.toDisplayValue,
                });

                this.addParameterChangedListener (other.endpointID, parameter => wrapped.update (mapValue (parameter.value)));
                this.parametersElement.appendChild (wrapped.element);
            }
        });
    })"
R"(

    makeControl (backend, { type, min, max, defaultValue, index, ...other })
    {
        switch (type)
        {
            case "slider": return {
                control: this.makeKnob ({
                    initialValue: defaultValue,
                    min,
                    max,
                    onBeginEdit: () => backend.onBeginEdit (other.endpointID),
                    onEdit: nextValue => backend.onEdit (other.endpointID, nextValue),
                    onEndEdit: () => backend.onEndEdit (other.endpointID),
                    onReset: () => backend.performSingleEdit (other.endpointID, defaultValue),
                }),
                toDisplayValue: v => `${v.toFixed (2)} ${other.unit ?? ""}`
            }
            case "switch":
            {
                const mapValue = v => v > 0.5;
                return {
                    control: this.makeSwitch ({
                        initialValue: mapValue (defaultValue),
                        onEdit: nextValue => backend.performSingleEdit (other.endpointID, nextValue),
                    }),
                    toDisplayValue: v => `${v ? "On" : "Off"}`,
                    mapValue,
                };
            }
            case "options":
            {
                const toDisplayValue = index => other.options[index].text;

                const toIndex = (value, options) =>
                {
                    const binarySearch = (arr, toValue, target) =>
                    {
                        let low = 0;
                        let high = arr.length - 1;

                        while (low <= high)
                        {
                            const mid = low + ((high - low) >> 1);
                            const value = toValue (arr[mid]);

                            if (value < target) low = mid + 1;
                            else if (value > target) high = mid - 1;
                            else return mid;
                        })"
R"(

                        return high;
                    };

                    return Math.max(0, binarySearch (options, v => v.value, value));
                };

                return {
                    control: this.makeOptions ({
                        initialSelectedIndex: toIndex (defaultValue, other.options),
                        options: other.options,
                        onEdit: (optionsIndex) =>
                        {
                            const nextValue = other.options[optionsIndex].value;
                            backend.performSingleEdit (other.endpointID, nextValue);
                        },
                        toDisplayValue,
                    }),
                    toDisplayValue,
                    mapValue: (v) => toIndex (v, other.options),
                };
            }
        }

        return undefined;
    }

    makeKnob ({
        initialValue = 0,
        min = 0,
        max = 1,
        onBeginEdit = () => {},
        onEdit = (nextValue) => {},
        onEndEdit = () => {},
        onReset = () => {},
     } = {})
    {
        // core drawing code derived from https://github.com/Kyle-Shanks/react_synth (MIT licensed)

        const isBipolar = min + max === 0;
        const type = isBipolar ? 2 : 1;

        const maxKnobRotation = 132;
        const typeDashLengths = { 1: 184, 2: 251.5 };
        const typeValueOffsets = { 1: 132, 2: 0 };
        const typePaths =
        {
            1: "M20,76 A 40 40 0 1 1 80 76",
            2: "M50.01,10 A 40 40 0 1 1 50 10"
        };

        const createSvgElement = ({ document = window.document, tag = "svg" } = {}) => document.createElementNS ("http://www.w3.org/2000/svg", tag);

        const svg = createSvgElement();
        svg.setAttribute ("viewBox", "0 0 100 100");)"
R"(

        const trackBackground = createSvgElement ({ tag: "path" });
        trackBackground.setAttribute ("d", "M20,76 A 40 40 0 1 1 80 76");
        trackBackground.classList.add ("knob-path");
        trackBackground.classList.add ("knob-track-background");

        const getDashOffset = (val, type) => typeDashLengths[type] - 184 / (maxKnobRotation * 2) * (val + typeValueOffsets[type]);

        const trackValue = createSvgElement ({ tag: "path" });
        trackValue.setAttribute ("d", typePaths[type]);
        trackValue.setAttribute ("stroke-dasharray", typeDashLengths[type]);
        trackValue.classList.add ("knob-path");
        trackValue.classList.add ("knob-track-value");

        const dial = document.createElement ("div");
        dial.className = "knob-dial";

        const dialTick = document.createElement ("div");
        dialTick.className = "knob-dial-tick";
        dial.appendChild (dialTick);

        const container = document.createElement ("div");
        container.className = "knob-container";

        svg.appendChild (trackBackground);
        svg.appendChild (trackValue);

        container.appendChild (svg);
        container.appendChild (dial);

        const remap = (source, sourceFrom, sourceTo, targetFrom, targetTo) =>
        {
            return targetFrom + (source - sourceFrom) * (targetTo - targetFrom) / (sourceTo - sourceFrom);
        };

        const toValue = (knobRotation) => remap (knobRotation, -maxKnobRotation, maxKnobRotation, min, max);
        const toRotation = (value) => remap (value, min, max, -maxKnobRotation, maxKnobRotation);

        const state =
        {
            rotation: toRotation (initialValue),
        };

        const update = (degrees, force) =>
        {
            if (! force && state.rotation === degrees) return;

            state.rotation = degrees;)"
R"(

            trackValue.setAttribute ("stroke-dashoffset", getDashOffset (state.rotation, type));
            dial.style.transform = `translate(-50%,-50%) rotate(${state.rotation}deg)`
        };

        const force = true;
        update (state.rotation, force);

        let accumlatedRotation = undefined;
        let previousScreenY = undefined;

        const onMouseMove = (event) =>
        {
            event.preventDefault(); // avoid scrolling whilst dragging

            const nextRotation = (rotation, delta) =>
            {
                const clamp = (v, min, max) => Math.min (Math.max (v, min), max);

                return clamp (rotation - delta, -maxKnobRotation, maxKnobRotation);
            };

            const workaroundBrowserIncorrectlyCalculatingMovementY = event.movementY === event.screenY;
            const movementY = workaroundBrowserIncorrectlyCalculatingMovementY
                ? event.screenY - previousScreenY
                : event.movementY;
            previousScreenY = event.screenY;

            const speedMultiplier = event.shiftKey ? 0.25 : 1.5;
            accumlatedRotation = nextRotation (accumlatedRotation, movementY * speedMultiplier);
            onEdit (toValue (accumlatedRotation));
        };

        const onMouseUp = (event) =>
        {
            previousScreenY = undefined;
            accumlatedRotation = undefined;
            window.removeEventListener ("mousemove", onMouseMove);
            window.removeEventListener ("mouseup", onMouseUp);
            onEndEdit();
        };

        const onMouseDown = (event) =>
        {
            previousScreenY = event.screenY;
            accumlatedRotation = state.rotation;
            onBeginEdit();
            window.addEventListener ("mousemove", onMouseMove);
            window.addEventListener ("mouseup", onMouseUp);
        };)"
R"(

        container.addEventListener ("mousedown", onMouseDown);
        container.addEventListener ("mouseup", onMouseUp);
        container.addEventListener ("dblclick", () => onReset());

        return {
            element: container,
            update: nextValue => update (toRotation (nextValue)),
        };
    }

    makeSwitch ({
        initialValue = false,
        onEdit = () => {},
    } = {})
    {
        const outer = document.createElement ("div");
        outer.classList = "switch-outline";

        const inner = document.createElement ("div");
        inner.classList = "switch-thumb";

        const state =
        {
            value: initialValue,
        };

        outer.appendChild (inner);

        const container = document.createElement ("div");
        container.classList.add ("switch-container");

        container.addEventListener ("click", () => onEdit (! state.value));

        container.appendChild (outer);

        const update = (nextValue, force) =>
        {
            if (! force && state.value === nextValue) return;

            state.value = nextValue;
            container.classList.remove ("switch-off", "switch-on");
            container.classList.add (`${nextValue ? "switch-on" : "switch-off"}`);
        };

        const force = true;
        update (initialValue, force);

        return {
            element: container,
            update,
        };
    }

    makeOptions ({
        initialSelectedIndex = -1,
        options = [],
        onEdit = () => {},
        toDisplayValue = () => {},
    } = {})
    {
        const select = document.createElement ("select");

        options.forEach ((option, index) =>
        {
            const optionElement = document.createElement ("option");
            optionElement.innerText = toDisplayValue (index);
            select.appendChild (optionElement);
        });

        const state =
        {
            selectedIndex: initialSelectedIndex,
        };)"
R"(

        select.addEventListener ("change", (e) =>
        {
            const incomingIndex = e.target.selectedIndex;

            // prevent local state change. the caller will update us when the backend actually applies the update
            e.target.selectedIndex = state.selectedIndex;

            onEdit?.(incomingIndex);
        });

        const update = (incomingIndex, force) =>
        {
            if (! force && state.selectedIndex === incomingIndex) return;

            state.selectedIndex = incomingIndex;
            select.selectedIndex = state.selectedIndex;
        };

        const force = true;
        update (initialSelectedIndex, force);

        const wrapper = document.createElement ("div");
        wrapper.className = "select-container";

        wrapper.appendChild (select);

        const icon = document.createElement ("span");
        icon.className = "select-icon";
        wrapper.appendChild (icon);

        return {
            element: wrapper,
            update,
        };
    }

    makeLabelledControl (childControl, {
        name = "",
        initialValue = 0,
        toDisplayValue = () => "",
    } = {})
    {
        const container = document.createElement ("div");
        container.className = "labelled-control";

        const centeredControl = document.createElement ("div");
        centeredControl.className = "labelled-control-centered-control";

        centeredControl.appendChild (childControl.element);

        const titleValueHoverContainer = document.createElement ("div");
        titleValueHoverContainer.className = "labelled-control-label-container";

        const nameText = document.createElement ("div");
        nameText.classList.add ("labelled-control-name");
        nameText.innerText = name;

        const valueText = document.createElement ("div");
        valueText.classList.add ("labelled-control-value");
        valueText.innerText = toDisplayValue (initialValue);)"
R"(

        titleValueHoverContainer.appendChild (nameText);
        titleValueHoverContainer.appendChild (valueText);

        container.appendChild (centeredControl);
        container.appendChild (titleValueHoverContainer);

        return {
            element: container,
            update: (nextValue) =>
            {
                childControl.update (nextValue);
                valueText.innerText = toDisplayValue (nextValue);
            },
        };
    }

    getHTML()
    {
        return `
<style>
* {
    box-sizing: border-box;
    user-select: none;
    -webkit-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;

    margin: 0;
    padding: 0;
}

:host {
    --header-height: 40px;
    --foreground: #ffffff;
    --background: #1a1a1a;

    /* knob */
    --knob-track-background-color: var(--background);
    --knob-track-value-color: var(--foreground);

    --knob-dial-border-color: var(--foreground);
    --knob-dial-background-color: var(--background);
    --knob-dial-tick-color: var(--foreground);

    /* switch */
    --switch-outline-color: var(--foreground);
    --switch-thumb-color: var(--foreground);
    --switch-on-background-color: var(--background);
    --switch-off-background-color: var(--background);

    /* control value + name display */
    --labelled-control-font-color: var(--foreground);
    --labelled-control-font-size: 12px;

    display: block;
    height: 100%;
    font-family: 'IBM Plex Mono', monospace;
    background-color: var(--background);
}

.main {
    background: var(--background);
}

.header {
    width: 100%;
    height: var(--header-height);
    border-bottom: 1px solid var(--foreground);

    display: flex;
    justify-content: space-between;
    align-items: center;
}

.header-title {
    color: var(--foreground);
    text-overflow: ellipsis;
    white-space: nowrap;
    overflow: hidden;

    cursor: default;
})"
R"(

.logo {
    flex: 1;
    height: 100%;
    background-color: var(--foreground);
    mask: url(cmaj_api/assets/sound-stacks-logo.svg);
    mask-repeat: no-repeat;
    -webkit-mask: url(cmaj_api/assets/sound-stacks-logo.svg);
    -webkit-mask-repeat: no-repeat;
    min-width: 100px;
}

.header-filler {
    flex: 1;
}

.app-body {
    height: calc(100% - var(--header-height));
    overflow: auto;
    padding: 1rem;
    text-align: center;
}

/* options / select */
.select-container {
    position: relative;
    display: block;
    font-size: 0.8rem;
    width: 100%;
    color: var(--foreground);
    border: 2px solid var(--foreground);
    border-radius: 0.6rem;
}

select {
    background: none;
    appearance: none;
    -webkit-appearance: none;
    font-family: inherit;
    font-size: var(--labelled-control-font-size);

    overflow: hidden;
    text-overflow: ellipsis;

    padding: 0 1.5rem 0 0.6rem;

    outline: none;
    color: var(--foreground);
    height: 2rem;
    box-sizing: border-box;
    margin: 0;
    border: none;

    width: 100%;
}

select option {
    background: var(--background);
    color: var(--foreground);
}

.select-icon {
    position: absolute;
    right: 0.3rem;
    top: 0.5rem;
    pointer-events: none;
    background-color: var(--foreground);
    width: 1.4em;
    height: 1.4em;
    mask: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath d='M17,9.17a1,1,0,0,0-1.41,0L12,12.71,8.46,9.17a1,1,0,0,0-1.41,0,1,1,0,0,0,0,1.42l4.24,4.24a1,1,0,0,0,1.42,0L17,10.59A1,1,0,0,0,17,9.17Z'/%3E%3C/svg%3E");
    mask-repeat: no-repeat;
    -webkit-mask: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Cpath d='M17,9.17a1,1,0,0,0-1.41,0L12,12.71,8.46,9.17a1,1,0,0,0-1.41,0,1,1,0,0,0,0,1.42l4.24,4.24a1,1,0,0,0,1.42,0L17,10.59A1,1,0,0,0,17,9.17Z'/%3E%3C/svg%3E");
    -webkit-mask-repeat: no-repeat;
}

/* knob */
.knob-container {
    position: relative;
    display: inline-block;)"
R"(

    height: 80px;
    width: 80px;
}

.knob-path {
    fill: none;
    stroke-linecap: round;
    stroke-width: 3;
}

.knob-track-background {
    stroke: var(--knob-track-background-color);
}

.knob-track-value {
    stroke: var(--knob-track-value-color);
}

.knob-dial {
    position: absolute;
    text-align: center;
    height: 50px;
    width: 50px;
    top: 50%;
    left: 50%;
    border: 2px solid var(--knob-dial-border-color);
    border-radius: 100%;
    box-sizing: border-box;
    transform: translate(-50%,-50%);
    background-color: var(--knob-dial-background-color);
}

.knob-dial-tick {
    position: absolute;
    display: inline-block;

    height: 14px;
    width: 2px;
    background-color: var(--knob-dial-tick-color);
}

/* switch */
.switch-outline {
    position: relative;
    display: inline-block;
    height: 1.25rem;
    width: 2.5rem;
    border-radius: 10rem;
    box-shadow: 0 0 0 2px var(--switch-outline-color);
    transition: background-color 0.1s cubic-bezier(0.5, 0, 0.2, 1);
}

.switch-thumb {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%,-50%);
    height: 1rem;
    width: 1rem;
    background-color: var(--switch-thumb-color);
    border-radius: 100%;
    transition: left 0.1s cubic-bezier(0.5, 0, 0.2, 1);
}

.switch-off .switch-thumb {
    left: 25%
}
.switch-on .switch-thumb {
    left: 75%;
}

.switch-off .switch-outline {
    background-color: var(--switch-on-background-color);
}
.switch-on .switch-outline {
    background-color: var(--switch-off-background-color);
}

.switch-container {
    position: relative;
    display: flex;
    align-items: center;
    justify-content: center;

    height: 100%;
    width: 100%;
}

/* control value + name display */
.labelled-control {
    position: relative;
    display: inline-block;

    margin: 0 0.5rem 0.5rem;
    vertical-align: top;
    text-align: left;
})"
R"(

.labelled-control-centered-control {
    position: relative;
    display: flex;
    align-items: center;
    justify-content: center;

    width: 80px;
    height: 80px;
}

.labelled-control-label-container {
    position: relative;
    display: block;

    max-width: 80px;
    margin: -0.5rem auto 0.5rem;
    text-align: center;
    font-size: var(--labelled-control-font-size);
    color: var(--labelled-control-font-color);

    cursor: default;
}

.labelled-control-name {
    overflow: hidden;
    text-overflow: ellipsis;
}

.labelled-control-value {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;

    overflow: hidden;
    text-overflow: ellipsis;

    opacity: 0;
}

.labelled-control:hover .labelled-control-name,
.labelled-control:active .labelled-control-name {
    opacity: 0;
}
.labelled-control:hover .labelled-control-value,
.labelled-control:active .labelled-control-value {
    opacity: 1;
}

</style>

<div class="main">
  <div class="header">
   <span class="logo"></span>
   <h2 id="patch-title" class="header-title"></h2>
   <div class="header-filler"></div>
  </div>
  <div id="patch-parameters" class="app-body"></div>
</div>
`;
    }
}

if (! window.customElements.get ("cmaj-generic-patch-view"))
    window.customElements.define ("cmaj-generic-patch-view", PatchView);
)";
    static constexpr const char* assets_cmajorlogo_svg =
        R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="150 140 1620 670">
   <g id="fe54d652-402d-4071-afab-d8a6918b9a1f" data-name="Logo">
      <path
         d="M944.511,462.372V587.049H896.558V469.165c0-27.572-13.189-44.757-35.966-44.757-23.577,0-39.958,19.183-39.958,46.755V587.049H773.078V469.165c0-27.572-13.185-44.757-35.962-44.757-22.378,0-39.162,19.581-39.162,46.755V587.049H650.4v-201.4h47.551v28.77c8.39-19.581,28.771-32.766,54.346-32.766,27.572,0,46.353,11.589,56.343,35.166,11.589-23.577,33.57-35.166,65.934-35.166C918.937,381.652,944.511,412.42,944.511,462.372Zm193.422-76.724h47.953v201.4h-47.953V557.876c-6.794,19.581-31.167,33.567-64.335,33.567q-42.558,0-71.928-29.969c-19.183-20.381-28.771-45.155-28.771-75.128s9.588-54.743,28.771-74.726c19.581-20.377,43.556-30.366,71.928-30.366,33.168,0,57.541,13.985,64.335,33.566Zm3.6,100.7c0-17.579-5.993-32.368-17.981-43.953-11.589-11.59-26.374-17.583-43.559-17.583s-31.167,5.993-42.756,17.583c-11.187,11.585-16.783,26.374-16.783,43.953s5.6,32.369,16.783,43.958c11.589,11.589,25.575,17.583,42.756,17.583s31.97-5.994,43.559-17.583C1135.537,518.715,1141.53,503.929,1141.53,486.346Zm84.135,113.49c0,21.177-7.594,29.571-25.575,29.571-2.8,0-7.192-.4-13.185-.8v42.357c4.393.8,11.187,1.2,19.979,1.2,44.355,0,66.734-22.776,66.734-67.932V385.648h-47.953Zm23.978-294.108c-15.987,0-28.774,12.385-28.774,28.372s12.787,28.369,28.774,28.369a28.371,28.371,0,0,0,0-56.741Zm239.674,104.694c21.177,20.381,31.966,45.956,31.966,75.924s-10.789,55.547-31.966,75.928-47.154,30.769-77.926,30.769-56.741-10.392-77.922-30.769-31.966-45.955-31.966-75.928,10.789-55.543,31.966-75.924,47.154-30.768,77.922-30.768S1468.136,390.041,1489.317,410.422Zm-15.585,75.924c0-17.981-5.994-32.766-17.985-44.753-11.988-12.39-26.773-18.383-44.356-18.383-17.981,0-32.766,5.993-44.754,18.383-11.589,11.987-17.583,26.772-17.583,44.753s5.994,32.77,17.583,45.156c11.988,11.987,26.773,17.985,44.754,17.985q26.374,0,44.356-17.985C1467.738,519.116,1473.732,504.331,1473.732,486.346Zm184.122-104.694c-28.373,0-50.349,12.787-59.941,33.964V38)"
R"(5.648h-47.551v201.4h47.551v-105.9c0-33.169,21.177-53.948,54.345-53.948a102.566,102.566,0,0,1,19.979,2V382.85A74.364,74.364,0,0,0,1657.854,381.652ZM580.777,569.25l33.909,30.087c-40.644,47.027-92.892,70.829-156.173,70.829-58.637,0-108.567-19.737-149.788-59.8C268.082,570.31,247.763,519.8,247.763,460s20.319-109.726,60.962-149.786c41.221-40.059,91.151-60.38,149.788-60.38,62.119,0,113.789,22.643,154.432,68.507l-33.864,30.134c-16.261-19.069-35.272-32.933-56.978-41.783V486.346H496.536V621.1Q546.954,610.231,580.777,569.25Zm-237.74,9.1A150.247,150.247,0,0,0,396.5,614.04V486.346H370.929V319.387a159.623,159.623,0,0,0-27.892,22.829Q297.187,389.16,297.186,460C297.186,507.229,312.47,547.06,343.037,578.354Zm115.476,46.66a187.178,187.178,0,0,0,27.28-1.94V486.346H474.548V295.666c-5.236-.426-10.567-.677-16.035-.677a177.387,177.387,0,0,0-40.029,4.4V486.346H407.239v131.4A175.161,175.161,0,0,0,458.513,625.014Z"
         fill="#fff" />
   </g>
</svg>)";
    static constexpr const char* assets_soundstackslogo_svg =
        R"(<?xml version="1.0" encoding="utf-8"?>
<svg version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
     viewBox="0 0 1200 560" fill="#ffffff" xml:space="preserve">
<g id="Exclusion_Zone">
    <path d="M532.3,205.9c4.4,5.9,6.6,13.4,6.6,22.7c0,9.2-2.3,17.3-6.9,24.2c-4.6,6.9-11.2,12.3-19.7,16.2c-8.5,3.9-18.6,5.8-30.1,5.8
        c-11.6,0-21.9-2.1-30.8-6.2c-8.9-4.1-15.9-10.2-21-18.1c-5.1-8-7.6-17.7-7.6-29.1v-5.5h23.5v5.5c0,10.9,3.3,19,9.8,24.4
        c6.5,5.4,15.2,8.1,26.1,8.1c11,0,19.3-2.3,24.9-6.9c5.6-4.6,8.4-10.6,8.4-17.9c0-4.8-1.3-8.8-4-11.9c-2.6-3.1-6.4-5.5-11.2-7.4
        c-4.8-1.8-10.6-3.6-17.4-5.2l-8.1-2c-9.8-2.3-18.4-5.2-25.6-8.7c-7.3-3.4-12.9-8-16.8-13.8s-5.9-13.1-5.9-22
        c0-9.1,2.2-16.9,6.7-23.3c4.5-6.5,10.7-11.4,18.6-15s17.2-5.3,27.9-5.3c10.7,0,20.3,1.8,28.7,5.5c8.4,3.7,15.1,9.1,19.9,16.3
        c4.8,7.2,7.3,16.2,7.3,27.1v7.9h-23.5v-7.9c0-6.6-1.4-12-4.1-16.1c-2.7-4.1-6.5-7.1-11.3-9c-4.8-1.9-10.5-2.9-16.9-2.9
        c-9.4,0-16.7,1.9-21.9,5.8c-5.2,3.9-7.8,9.3-7.8,16.2c0,4.7,1.1,8.5,3.4,11.6c2.3,3,5.6,5.5,10,7.4c4.4,1.9,9.9,3.6,16.5,5.1l8.1,2
        c10,2.2,18.8,5,26.4,8.5C521.9,195.4,527.9,200.1,532.3,205.9z M637.7,268.1c-8.5,4.5-18.2,6.7-29,6.7c-10.9,0-20.5-2.2-28.9-6.7
        c-8.4-4.5-15.1-10.9-19.9-19.2c-4.8-8.4-7.3-18.3-7.3-29.7v-3.3c0-11.4,2.4-21.3,7.3-29.6c4.8-8.3,11.5-14.7,19.9-19.2
        c8.4-4.5,18.1-6.8,28.9-6.8c10.9,0,20.5,2.3,29,6.8c8.5,4.5,15.2,11,20,19.2c4.8,8.3,7.3,18.2,7.3,29.6v3.3
        c0,11.4-2.4,21.3-7.3,29.7C652.9,257.2,646.2,263.6,637.7,268.1z M642.3,216.5c0-11.3-3.1-20.1-9.2-26.5
        c-6.2-6.4-14.3-9.6-24.4-9.6c-9.8,0-17.9,3.2-24.1,9.6c-6.2,6.4-9.4,15.2-9.4,26.5v2c0,11.3,3.1,20.1,9.4,26.5
        c6.2,6.4,14.3,9.6,24.1,9.6c10,0,18.1-3.2,24.3-9.6c6.2-6.4,9.4-15.2,9.4-26.5V216.5z M699.5,268.2c6.5,3.7,13.8,5.5,22,5.5
        c10.3,0,18-1.9,23.3-5.8s8.9-8,11-12.4h3.5v16.3h22.2V163.2h-22.7V218c0,11.7-2.8,20.7-8.5,27c-5.6,6.2-13.1,9.4-22.3,9.4)"
R"(
        c-8.4,0-14.9-2.2-19.6-6.7c-4.7-4.5-7-11.4-7-20.8v-63.6h-22.7v65.1c0,9.4,1.8,17.5,5.5,24.2C688,259.3,693,264.5,699.5,268.2z
         M822.3,216.9c0-11.7,2.8-20.7,8.5-26.8c5.6-6.2,13.2-9.2,22.5-9.2c8.2,0,14.7,2.2,19.4,6.7c4.7,4.5,7,11.4,7,20.8v63.4h22.7v-65.1
        c0-9.4-1.8-17.4-5.5-24.1c-3.7-6.7-8.7-11.8-15.1-15.5c-6.4-3.7-13.7-5.5-21.9-5.5c-10.4,0-18.3,1.9-23.5,5.7
        c-5.3,3.8-8.9,7.9-11,12.3h-3.5v-16.3h-22.2v108.5h22.7V216.9z M923.3,249.2c-4.6-8.3-6.9-18.3-6.9-30v-3.3
        c0-11.6,2.3-21.6,6.8-29.9c4.5-8.4,10.6-14.7,18.3-19.1c7.6-4.4,16-6.6,25.1-6.6c7,0,12.9,0.9,17.7,2.6c4.8,1.8,8.7,4,11.8,6.7
        c3.1,2.7,5.4,5.5,7,8.5h3.5v-60.3h22.7v154H1007v-15.4h-3.5c-2.8,4.7-7,8.9-12.6,12.8c-5.6,3.8-13.8,5.7-24.3,5.7
        c-8.9,0-17.2-2.2-24.9-6.6C934.1,263.8,928,257.4,923.3,249.2z M939.3,218.5c0,11.7,3.2,20.8,9.6,27.1c6.4,6.3,14.4,9.5,24.1,9.5
        c9.8,0,17.9-3.2,24.3-9.5c6.4-6.3,9.6-15.3,9.6-27.1v-2c0-11.6-3.2-20.5-9.5-26.8c-6.3-6.3-14.4-9.5-24.4-9.5
        c-9.7,0-17.7,3.2-24.1,9.5c-6.4,6.3-9.6,15.3-9.6,26.8V218.5z M514.3,366.9c-7.6-3.4-16.4-6.3-26.4-8.5l-8.1-2
        c-6.6-1.5-12.1-3.2-16.5-5.1c-4.4-1.9-7.7-4.4-10-7.4c-2.3-3-3.4-6.9-3.4-11.6c0-6.9,2.6-12.3,7.8-16.2c5.2-3.9,12.5-5.8,21.9-5.8
        c6.5,0,12.1,1,16.9,2.9c4.8,1.9,8.6,4.9,11.3,9c2.7,4.1,4.1,9.5,4.1,16.1v7.9h23.5v-7.9c0-10.9-2.4-19.9-7.3-27.1
        c-4.8-7.2-11.5-12.6-19.9-16.3c-8.4-3.7-18-5.5-28.7-5.5c-10.7,0-20,1.8-27.9,5.3s-14.1,8.5-18.6,15c-4.5,6.5-6.7,14.2-6.7,23.3
        c0,8.9,2,16.3,5.9,22c4,5.7,9.6,10.3,16.8,13.8c7.3,3.4,15.8,6.3,25.6,8.7l8.1,2c6.7,1.6,12.5,3.3,17.4,5.2
        c4.8,1.8,8.6,4.3,11.2,7.4c2.6,3.1,4,7,4,11.9c0,7.3-2.8,13.3-8.4,17.9c-5.6,4.6-13.9,6.9-24.9,6.9c-10.9,0-19.5-2.7-26.1-8.1
        c-6.5-5.4-9.8-13.6-9.8-24.4v-5.5h-23.5v5.5c0,11.4,2.5,21.2,7.6,29.1c5.1,8,12.1,14,21,18.2c8.9,4.1,19.2,6.2,30.8,6.2
        c11.6,0,21.6-1.9,30.1-5.8c8.5-3.9,15.1-9.3,19.7-16.2c4.6-6.9,6.9-15,6.9-24.2s-2.2-16.8-6.6-22.7)"
R"(
        C527.9,375.1,521.9,370.4,514.3,366.9z M597.4,302.4h-22.7v35.9h-29.9v19.1h29.9v67.5c0,6.6,1.9,11.9,5.8,15.8
        c3.9,4,9.1,5.9,15.7,5.9h29.9v-19.1h-22.7c-4.1,0-6.2-2.2-6.2-6.6v-63.6h32.3v-19.1h-32.3V302.4z M757.4,427.8h9.2v18.9h-19.1
        c-5.1,0-9.3-1.5-12.5-4.4c-3.2-2.9-4.8-6.9-4.8-11.9v-1.5h-3.3c-2.8,5.7-7.1,10.6-13,14.7c-5.9,4.1-14.3,6.2-25.3,6.2
        c-9.1,0-17.5-2.2-25.1-6.6c-7.6-4.4-13.7-10.7-18.3-19c-4.5-8.3-6.8-18.3-6.8-30v-3.3c0-11.7,2.3-21.7,6.9-30
        c4.6-8.3,10.7-14.6,18.4-19c7.6-4.4,15.9-6.6,24.9-6.6c10.6,0,18.7,1.9,24.3,5.7c5.6,3.8,9.9,8.1,12.6,13h3.5v-15.6h22.2v82.9
        C751.2,425.6,753.2,427.8,757.4,427.8z M728.8,391.5c0-11.6-3.2-20.5-9.6-26.8c-6.4-6.3-14.5-9.5-24.3-9.5
        c-9.7,0-17.7,3.2-24.1,9.5c-6.4,6.3-9.6,15.3-9.6,26.8v2c0,11.7,3.2,20.8,9.6,27.1c6.4,6.3,14.4,9.5,24.1,9.5
        c10,0,18.1-3.2,24.4-9.5c6.3-6.3,9.5-15.3,9.5-27.1V391.5z M848.4,422.2c-5,4.9-12.2,7.4-21.8,7.4c-6.3,0-12-1.4-17-4.2
        c-5.1-2.8-9.1-6.9-12-12.3c-2.9-5.4-4.4-12-4.4-19.6v-2c0-7.6,1.5-14.1,4.4-19.5c2.9-5.4,6.9-9.5,12-12.3c5.1-2.9,10.7-4.3,17-4.3
        c6.5,0,11.8,1.2,16.1,3.5c4.3,2.3,7.6,5.5,9.9,9.5c2.3,4,3.9,8.4,4.6,13.2l22-4.6c-1.3-7.6-4.2-14.6-8.7-20.9
        c-4.5-6.3-10.4-11.4-17.7-15.2c-7.3-3.8-16.2-5.7-26.6-5.7c-10.4,0-19.8,2.2-28.3,6.7c-8.4,4.5-15.1,10.9-20,19.1
        c-4.9,8.3-7.4,18.3-7.4,30v2.9c0,11.7,2.5,21.8,7.4,30.1c4.9,8.4,11.6,14.7,20,19.1c8.4,4.4,17.9,6.6,28.3,6.6
        c10.4,0,19.3-1.9,26.6-5.6c7.3-3.7,13.2-8.8,17.7-15.1c4.5-6.3,7.6-13.2,9.3-20.7l-22-5.1C856.6,411,853.4,417.3,848.4,422.2z
         M991.9,338.2h-30.1l-42.7,42.2h-3.5v-87.8h-22.7v154h22.7v-45.1h3.5l44.7,45.1h29.9l-56.5-55.9L991.9,338.2z M1081.5,397.9
        c-3.7-4.4-8.8-7.8-15.3-10.1c-6.5-2.3-13.6-4.2-21.3-5.5l-7.7-1.3c-5.9-1-10.5-2.6-14-4.6c-3.4-2.1-5.2-5.3-5.2-9.7
        c0-4.1,1.8-7.3,5.3-9.6c3.5-2.3,8.4-3.4,14.5-3.4c6.3,0,11.6,1.4,15.8,4.1c4.3,2.7,7,7.4,8.4,14l21.1-5.9)"
R"(
        c-2.3-9.4-7.4-16.8-15.3-22.3c-7.8-5.5-17.9-8.2-30-8.2c-12.6,0-22.7,2.8-30.4,8.5c-7.6,5.6-11.4,13.6-11.4,23.9
        c0,6.9,1.8,12.5,5.3,16.9c3.5,4.4,8.3,7.8,14.3,10.3c6,2.5,12.7,4.4,20,5.7l7.5,1.3c7.2,1.3,12.6,3,16.3,5.1
        c3.7,2.1,5.5,5.3,5.5,9.7c0,4.4-1.9,8-5.8,10.8c-3.9,2.8-9.4,4.2-16.6,4.2c-4.8,0-9.3-0.7-13.5-2.2c-4.2-1.5-7.7-4-10.5-7.5
        c-2.8-3.5-4.8-8.3-5.9-14.3l-21.1,5.1c2.1,12.5,7.6,21.8,16.7,27.9c9.1,6.2,20.5,9.2,34.3,9.2c13.6,0,24.5-3,32.6-9
        c8.1-6,12.1-14.4,12.1-25.3C1087.1,408.1,1085.3,402.3,1081.5,397.9z M112.4,151v37.9l82.2,39.7l151.2-73v-37.9l-151.2,73
        L112.4,151z M112.4,369.1V407l82.2,39.7l151.2-73v-37.9l-151.2,73L112.4,369.1z M112.4,226.7v37.9l151.2,73l82.2-39.7v-37.9
        l-82.2,39.7L112.4,226.7z"/>
</g>
<g id="Logo">
</g>
</svg>
)";
    static constexpr const char assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1iAq131njotFQ_woff2[] = {
        119,79,70,50,0,1,0,0,0,0,13,(char)-80,0,14,0,0,0,0,40,0,0,0,13,89,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,40,27,30,28,46,6,96,0,(char)-126,80,17,16,10,(char)-78,60,(char)-87,87,11,(char)-126,
    48,0,1,54,2,36,3,(char)-124,76,4,32,5,(char)-125,58,7,(char)-117,105,27,115,33,51,3,(char)-62,(char)-58,1,60,(char)-37,26,58,(char)-63,127,(char)-103,(char)-120,(char)-58,97,(char)-34,92,(char)-79,(char)-88,
    59,(char)-116,(char)-70,34,(char)-111,76,(char)-128,(char)-91,(char)-123,87,(char)-90,124,(char)-10,(char)-20,(char)-77,101,87,93,(char)-21,(char)-101,9,(char)-99,(char)-81,(char)-56,65,(char)-99,(char)-1,
    (char)-88,28,(char)-110,15,79,(char)-37,(char)-22,(char)-3,(char)-103,(char)-63,(char)-104,1,(char)-116,67,(char)-42,5,79,(char)-79,16,35,18,43,(char)-48,29,(char)-64,78,(char)-42,(char)-61,(char)-120,
    (char)-38,(char)-120,60,54,(char)-45,(char)-11,50,(char)-61,101,112,(char)-34,16,26,120,(char)-104,(char)-120,48,(char)-74,(char)-113,54,(char)-47,(char)-127,121,(char)-106,(char)-68,22,41,(char)-99,
    60,(char)-68,(char)-35,(char)-34,(char)-65,(char)-19,(char)-60,35,110,(char)-127,29,96,(char)-44,22,91,(char)-104,101,18,28,(char)-73,90,92,(char)-5,77,79,69,(char)-105,117,113,78,(char)-75,(char)-20,
    68,70,63,73,(char)-53,(char)-110,(char)-115,63,(char)-93,(char)-1,(char)-97,(char)-82,(char)-41,86,111,(char)-75,87,(char)-58,15,(char)-78,(char)-71,35,(char)-88,(char)-16,(char)-12,26,19,38,69,(char)-105,
    (char)-94,78,53,43,(char)-51,(char)-126,(char)-2,(char)-20,(char)-77,(char)-42,102,25,3,8,82,(char)-10,31,103,100,(char)-30,13,48,26,(char)-72,10,32,53,(char)-83,(char)-85,54,77,(char)-107,(char)-66,
    72,(char)-107,(char)-94,(char)-82,2,97,(char)-90,90,22,84,(char)-79,123,20,18,(char)-106,97,(char)-120,42,117,(char)-14,(char)-7,(char)-27,(char)-18,(char)-78,32,0,(char)-124,(char)-8,118,54,5,6,(char)-2,
    (char)-91,75,16,(char)-21,18,84,(char)-69,4,6,34,(char)-3,1,22,78,(char)-17,(char)-121,(char)-92,(char)-90,(char)-25,(char)-14,(char)-80,2,3,0,102,(char)-77,0,(char)-90,(char)-48,15,77,115,(char)-20,
    (char)-114,(char)-13,(char)-128,(char)-79,29,(char)-110,(char)-75,(char)-90,(char)-37,0,9,0,(char)-82,(char)-35,49,84,49,(char)-39,2,1,64,45,42,(char)-89,1,(char)-56,(char)-87,62,15,(char)-87,68,92,
    (char)-124,3,20,39,(char)-80,22,80,52,40,90,1,(char)-120,0,80,(char)-8,(char)-10,61,101,(char)-115,121,10,(char)-110,4,40,54,90,104,(char)-10,32,126,12,33,(char)-9,40,64,118,3,0,(char)-59,80,(char)-53,
    0,20,104,39,(char)-47,(char)-95,36,(char)-46,36,(char)-72,12,(char)-119,(char)-5,124,(char)-114,108,(char)-47,(char)-80,2,32,(char)-126,34,104,98,71,(char)-68,73,11,57,74,46,57,(char)-1,(char)-93,56,
    59,73,(char)-33,(char)-80,111,(char)-118,(char)-40,18,21,105,38,71,(char)-56,69,(char)-25,35,(char)-122,(char)-89,101,(char)-2,(char)-61,(char)-36,125,27,(char)-12,81,3,8,(char)-2,111,(char)-74,39,33,
    (char)-27,72,(char)-58,(char)-119,51,0,72,99,(char)-89,(char)-120,(char)-77,10,62,58,(char)-60,(char)-88,(char)-26,(char)-49,32,86,58,123,25,(char)-18,(char)-111,103,(char)-83,2,114,26,78,10,(char)-35,
    43,(char)-97,76,38,(char)-119,108,82,57,(char)-42,(char)-56,(char)-27,40,(char)-117,(char)-125,117,92,(char)-16,20,(char)-76,92,(char)-23,(char)-72,41,(char)-26,(char)-95,(char)-124,(char)-89,82,74,
    122,(char)-18,(char)-54,121,43,(char)-93,82,(char)-55,87,21,63,(char)-115,66,53,11,(char)-41,34,66,(char)-85,72,77,(char)-62,(char)-44,8,80,39,72,(char)-67,96,13,66,(char)-44,10,(char)-44,38,74,(char)-69,
    104,(char)-21,(char)-59,(char)-71,79,2,(char)-93,68,93,(char)-110,116,(char)-118,(char)-41,35,69,(char)-73,100,(char)-67,(char)-44,(char)-6,(char)-92,34,(char)-96,97,5,22,(char)-74,4,102,(char)-77,13,
    0,(char)-40,13,0,9,0,80,8,(char)-52,(char)-109,96,33,3,4,73,0,45,1,(char)-114,(char)-114,(char)-87,43,(char)-111,84,(char)-76,109,(char)-76,57,108,89,105,89,103,(char)-90,100,53,30,(char)-85,109,(char)-72,
    (char)-101,(char)-87,(char)-38,18,(char)-115,113,57,18,(char)-43,126,(char)-45,91,101,24,55,(char)-79,(char)-77,72,(char)-52,74,(char)-60,107,(char)-19,(char)-83,24,(char)-95,80,34,(char)-107,9,(char)-27,
    (char)-10,76,(char)-14,36,(char)-42,73,(char)-84,104,(char)-33,(char)-77,(char)-109,(char)-44,85,38,99,24,75,71,(char)-95,88,(char)-62,4,105,(char)-85,(char)-56,(char)-126,(char)-108,86,(char)-95,(char)-91,
    76,(char)-75,(char)-5,69,(char)-95,(char)-104,35,(char)-31,(char)-21,(char)-25,44,109,44,106,69,2,(char)-95,104,(char)-15,65,103,(char)-10,(char)-36,(char)-21,(char)-20,(char)-30,(char)-101,100,(char)-30,
    45,(char)-114,35,19,(char)-81,(char)-78,(char)-117,(char)-81,(char)-5,(char)-27,(char)-17,127,(char)-24,(char)-71,(char)-25,(char)-72,(char)-91,55,64,113,(char)-88,93,122,(char)-22,(char)-50,69,66,(char)-66,
    (char)-50,2,(char)-58,95,(char)-88,(char)-7,(char)-96,(char)-19,(char)-51,90,126,43,(char)-68,39,(char)-7,91,4,(char)-124,24,(char)-65,(char)-4,(char)-64,51,(char)-36,(char)-46,91,(char)-127,121,(char)-105,
    94,47,93,124,122,(char)-27,18,32,34,19,87,30,124,(char)-74,109,(char)-15,12,33,19,103,(char)-71,(char)-91,(char)-45,(char)-64,(char)-8,(char)-87,102,(char)-114,102,(char)-119,(char)-120,(char)-30,32,
    (char)-20,(char)-86,54,(char)-2,58,21,(char)-35,(char)-93,(char)-32,(char)-87,(char)-66,(char)-39,(char)-37,37,(char)-33,(char)-88,99,(char)-9,(char)-100,(char)-69,117,109,106,75,104,(char)-114,74,(char)-97,
    (char)-51,45,(char)-35,(char)-68,115,49,117,22,(char)-61,(char)-74,113,(char)-27,6,(char)-21,(char)-44,103,98,9,(char)-89,47,(char)-27,(char)-76,90,78,47,(char)-46,(char)-79,60,91,(char)-59,85,(char)-76,
    (char)-78,(char)-59,28,39,16,65,(char)-120,102,(char)-115,35,(char)-22,107,(char)-64,18,17,88,(char)-122,67,(char)-49,(char)-13,95,122,(char)-84,(char)-82,61,(char)-6,98,61,(char)-1,(char)-7,(char)-121,
    (char)-21,(char)-109,30,121,(char)-95,(char)-118,(char)-118,106,(char)-105,(char)-97,49,47,123,(char)-18,(char)-22,91,99,40,60,53,(char)-37,(char)-55,114,90,68,15,(char)-5,16,61,87,76,(char)-43,(char)-83,
    (char)-36,(char)-25,89,122,(char)-61,(char)-97,35,44,(char)-31,8,7,14,44,(char)-59,(char)-22,(char)-106,110,(char)-98,(char)-67,72,(char)-56,(char)-60,(char)-123,51,55,(char)-106,116,18,79,68,(char)-8,
    (char)-80,92,(char)-104,(char)-38,34,(char)-57,(char)-40,119,(char)-100,(char)-27,72,(char)-65,94,(char)-105,(char)-19,(char)-61,(char)-8,57,(char)-106,125,96,(char)-36,38,59,(char)-105,(char)-69,(char)-121,
    115,(char)-92,124,(char)-35,(char)-38,(char)-6,62,(char)-72,13,20,75,54,47,38,92,(char)-103,84,(char)-24,(char)-76,(char)-97,81,(char)-38,89,(char)-33,(char)-123,102,71,(char)-66,(char)-114,125,67,109,
    47,87,(char)-66,86,(char)-13,9,55,(char)-39,(char)-44,(char)-122,(char)-19,(char)-66,109,113,89,122,(char)-1,13,118,(char)-15,(char)-124,36,97,(char)-1,67,(char)-10,(char)-109,110,59,(char)-100,(char)-68,
    (char)-2,56,(char)-53,(char)-76,38,126,(char)-49,(char)-118,(char)-63,(char)-70,(char)-47,(char)-31,126,(char)-21,(char)-70,115,(char)-14,(char)-99,103,(char)-21,(char)-75,85,122,109,102,(char)-101,
    54,74,(char)-57,(char)-57,(char)-24,(char)-37,22,(char)-97,125,(char)-16,(char)-118,44,(char)-57,127,(char)-17,(char)-23,(char)-45,(char)-58,(char)-67,(char)-100,61,43,15,(char)-52,(char)-38,115,105,
    (char)-27,(char)-23,(char)-54,(char)-33,(char)-44,73,(char)-29,(char)-105,84,(char)-86,(char)-21,(char)-26,44,126,101,(char)-70,(char)-46,121,82,(char)-84,(char)-9,(char)-7,(char)-37,114,(char)-54,(char)-99,
    64,(char)-33,104,87,(char)-101,(char)-82,(char)-13,122,(char)-19,(char)-107,(char)-46,98,(char)-19,(char)-103,36,7,(char)-87,107,(char)-117,(char)-12,51,20,2,44,(char)-1,(char)-13,(char)-86,(char)-78,
    (char)-11,90,126,14,(char)-107,65,101,65,(char)-90,(char)-66,52,(char)-121,(char)-14,(char)-106,76,41,119,(char)-122,21,(char)-42,(char)-34,(char)-38,(char)-73,(char)-118,19,62,68,67,19,49,(char)-28,
    29,92,(char)-83,(char)-21,(char)-106,(char)-54,49,62,(char)-19,(char)-70,13,(char)-83,(char)-47,90,121,(char)-41,63,35,(char)-4,43,40,(char)-124,(char)-114,(char)-62,(char)-9,98,(char)-113,(char)-20,
    56,(char)-126,(char)-97,(char)-70,49,(char)-75,64,(char)-104,29,(char)-110,88,88,(char)-75,46,43,(char)-77,57,(char)-81,(char)-57,16,26,(char)-3,119,108,88,(char)-78,42,68,(char)-65,(char)-35,(char)-63,
    (char)-23,103,(char)-81,(char)-78,(char)-42,(char)-78,34,(char)-89,(char)-52,116,(char)-113,8,(char)-105,(char)-128,(char)-30,20,(char)-119,(char)-42,(char)-112,(char)-103,(char)-20,(char)-91,116,80,
    (char)-51,35,(char)-116,(char)-117,42,(char)-10,76,72,119,(char)-83,(char)-115,79,112,(char)-85,(char)-117,79,(char)-41,121,(char)-122,(char)-121,(char)-15,(char)-98,(char)-79,41,46,101,(char)-111,(char)-54,
    81,71,(char)-73,(char)-110,(char)-123,(char)-5,61,(char)-62,114,2,(char)-93,77,117,(char)-83,41,(char)-123,(char)-17,(char)-72,(char)-10,45,52,(char)-69,89,(char)-86,56,(char)-73,(char)-116,(char)-52,
    52,(char)-28,(char)-12,(char)-72,(char)-81,72,109,(char)-91,43,(char)-18,63,(char)-33,58,19,(char)-68,60,(char)-68,(char)-100,(char)-54,62,124,33,(char)-11,22,124,(char)-71,8,(char)-99,103,92,(char)-70,
    75,109,66,(char)-68,75,93,44,(char)-75,106,(char)-124,62,(char)-96,46,62,(char)-63,(char)-91,(char)-10,42,(char)-31,(char)-87,(char)-22,(char)-52,(char)-123,5,124,54,(char)-1,99,(char)-114,58,(char)-15,
    (char)-45,(char)-32,100,99,(char)-116,70,111,(char)-77,(char)-94,(char)-80,(char)-90,(char)-79,50,71,19,(char)-97,17,(char)-97,26,(char)-25,(char)-8,94,36,47,107,(char)-44,48,26,(char)-68,74,11,(char)-26,
    (char)-49,104,94,(char)-33,119,111,110,106,127,(char)-12,(char)-58,(char)-105,(char)-2,(char)-111,116,111,(char)-27,62,(char)-58,(char)-36,(char)-6,(char)-31,(char)-109,(char)-104,39,(char)-1,(char)-24,
    (char)-93,127,(char)-29,(char)-10,(char)-6,79,99,93,109,93,(char)-93,121,1,(char)-91,7,(char)-90,23,(char)-78,74,(char)-65,(char)-53,(char)-49,(char)-54,(char)-1,(char)-67,52,11,(char)-68,(char)-6,66,
    (char)-40,5,(char)-11,(char)-91,(char)-24,48,(char)-16,(char)-27,(char)-110,(char)-54,(char)-126,98,(char)-53,(char)-28,51,58,(char)-83,124,(char)-74,(char)-106,90,53,73,31,48,(char)-85,(char)-43,(char)-55,
    103,(char)-114,(char)-106,(char)-104,(char)-23,(char)-77,(char)-91,(char)-52,(char)-106,15,55,(char)-122,(char)-39,106,(char)-53,(char)-74,(char)-8,36,79,(char)-46,(char)-122,25,(char)-61,(char)-17,
    (char)-110,(char)-35,(char)-29,(char)-8,(char)-19,(char)-29,(char)-95,(char)-126,(char)-126,61,29,(char)-63,(char)-63,29,123,(char)-30,(char)-35,(char)-121,30,(char)-25,55,(char)-25,(char)-117,(char)-69,
    2,(char)-14,116,(char)-54,(char)-116,(char)-116,(char)-90,68,106,(char)-121,(char)-53,(char)-33,12,(char)-54,(char)-2,103,83,117,(char)-35,117,(char)-2,16,(char)-93,59,116,(char)-87,84,94,15,99,(char)-113,
    125,47,49,(char)-38,116,(char)-81,16,109,(char)-107,(char)-95,(char)-71,(char)-27,81,(char)-45,(char)-122,(char)-124,(char)-113,(char)-82,101,(char)-26,(char)-9,(char)-58,(char)-17,(char)-115,(char)-53,
    (char)-115,75,(char)-117,79,(char)-5,56,(char)-73,59,(char)-62,(char)-15,(char)-64,95,(char)-127,67,(char)-97,(char)-125,(char)-21,95,7,28,19,(char)-44,9,17,105,17,(char)-41,(char)-61,(char)-17,124,
    36,(char)-11,90,42,78,(char)-83,99,(char)-126,(char)-85,58,(char)-86,90,55,(char)-114,(char)-113,(char)-117,76,(char)-56,(char)-16,113,(char)-103,75,123,19,(char)-10,(char)-90,107,(char)-46,(char)-109,
    19,(char)-110,(char)-85,53,83,49,(char)-14,(char)-43,(char)-104,116,(char)-39,(char)-78,44,61,102,85,(char)-98,(char)-98,(char)-53,(char)-89,23,(char)-121,93,(char)-48,(char)-94,62,(char)-114,3,(char)-55,
    (char)-69,(char)-22,(char)-41,78,41,(char)-91,69,(char)-86,(char)-106,(char)-44,29,88,99,(char)-60,14,61,11,(char)-33,(char)-40,(char)-46,(char)-50,83,49,(char)-121,114,82,(char)-74,72,54,(char)-99,
    (char)-31,(char)-74,(char)-68,(char)-92,(char)-31,119,(char)-25,(char)-80,(char)-107,(char)-99,25,104,40,(char)-48,36,105,124,86,(char)-58,71,46,(char)-98,63,(char)-33,(char)-4,(char)-100,(char)-29,
    43,55,51,121,(char)-66,(char)-85,(char)-75,(char)-67,(char)-107,(char)-17,(char)-30,(char)-21,63,(char)-39,(char)-104,(char)-76,(char)-47,18,30,79,(char)-56,(char)-90,(char)-107,78,(char)-17,121,(char)-19,
    104,(char)-39,(char)-26,53,38,29,(char)-61,11,(char)-55,47,(char)-74,(char)-74,51,(char)-29,(char)-17,(char)-17,55,(char)-1,(char)-43,84,116,102,52,(char)-54,(char)-90,(char)-107,118,(char)-91,(char)-113,
    99,(char)-23,(char)-82,(char)-109,21,(char)-2,53,(char)-52,(char)-118,(char)-20,14,126,111,(char)-7,(char)-38,(char)-53,(char)-18,(char)-55,(char)-35,(char)-94,(char)-75,105,(char)-115,(char)-8,77,(char)-17,
    1,(char)-79,(char)-19,(char)-7,(char)-9,(char)-21,(char)-51,30,86,(char)-80,59,(char)-124,100,70,(char)-14,(char)-89,51,(char)-35,(char)-80,(char)-4,(char)-43,15,(char)-103,(char)-72,90,36,42,(char)-2,
    55,108,122,(char)-79,(char)-5,91,(char)-27,66,(char)-4,27,(char)-64,(char)-73,72,(char)-43,(char)-111,(char)-53,55,122,38,95,(char)-111,(char)-69,54,(char)-37,(char)-39,(char)-10,(char)-69,(char)-54,
    (char)-81,36,123,54,(char)-14,(char)-71,29,88,(char)-9,127,(char)-121,(char)-46,44,(char)-59,(char)-10,(char)-10,(char)-111,55,(char)-93,(char)-101,122,113,(char)-50,101,(char)-81,68,44,(char)-39,(char)-21,
    82,124,60,32,83,53,(char)-40,92,(char)-35,(char)-16,101,65,(char)-41,70,87,(char)-41,(char)-115,93,5,95,(char)-10,77,52,(char)-11,59,15,90,(char)-16,(char)-89,(char)-45,92,(char)-33,58,(char)-127,87,
    127,(char)-45,68,(char)-33,69,26,(char)-86,(char)-101,7,(char)-9,(char)-82,53,(char)-56,(char)-21,58,88,(char)-56,46,35,65,(char)-107,(char)-67,(char)-82,67,(char)-109,117,(char)-79,(char)-13,(char)-51,
    (char)-71,91,(char)-97,101,(char)-36,(char)-109,(char)-53,(char)-80,(char)-9,(char)-87,9,92,(char)-12,(char)-107,(char)-82,(char)-114,(char)-51,15,116,(char)-9,54,42,7,(char)-29,83,97,(char)-85,110,
    26,120,(char)-45,(char)-25,124,94,54,(char)-108,64,(char)-81,27,(char)-99,(char)-33,(char)-90,84,(char)-98,(char)-27,(char)-74,49,83,(char)-61,3,(char)-99,116,96,91,(char)-57,(char)-52,7,21,67,67,(char)-14,
    24,103,(char)-80,(char)-90,(char)-119,(char)-9,(char)-28,(char)-74,74,119,(char)-5,50,(char)-1,46,(char)-25,(char)-82,110,43,27,28,(char)-70,(char)-87,(char)-100,(char)-96,(char)-81,81,(char)-96,54,
    (char)-107,(char)-37,(char)-116,95,(char)-113,97,66,127,109,16,(char)-16,30,91,(char)-66,53,(char)-19,111,114,(char)-33,(char)-64,76,(char)-99,(char)-67,110,(char)-75,(char)-27,(char)-90,(char)-13,(char)-42,
    (char)-84,(char)-113,27,(char)-9,98,122,(char)-109,(char)-35,12,(char)-14,(char)-86,89,48,(char)-83,60,24,(char)-91,4,3,(char)-64,2,35,(char)-113,(char)-10,20,(char)-106,24,11,57,106,113,(char)-44,59,
    30,42,(char)-35,117,124,(char)-30,68,118,82,(char)-60,(char)-87,(char)-52,116,(char)-50,(char)-23,25,22,103,(char)-60,89,(char)-52,(char)-79,96,(char)-22,118,56,4,81,(char)-128,(char)-36,(char)-59,2,
    (char)-3,(char)-59,(char)-32,76,(char)-85,(char)-95,(char)-41,82,114,56,51,(char)-118,(char)-76,(char)-118,57,(char)-74,(char)-64,(char)-50,(char)-123,87,25,(char)-97,(char)-36,(char)-64,(char)-106,
    118,(char)-37,15,(char)-106,74,(char)-76,63,60,(char)-88,41,82,6,12,(char)-43,(char)-15,(char)-102,3,89,49,93,(char)-75,92,86,(char)-67,97,(char)-77,51,(char)-86,56,(char)-117,57,(char)-74,18,(char)-45,
    27,(char)-121,112,84,(char)-111,(char)-69,(char)-85,99,53,12,(char)-90,(char)-68,(char)-54,(char)-32,29,18,71,(char)-19,1,(char)-74,(char)-58,126,62,111,(char)-51,64,45,55,(char)-119,(char)-35,(char)-81,
    89,13,27,83,(char)-53,84,(char)-86,(char)-72,(char)-55,(char)-69,(char)-7,(char)-42,(char)-108,(char)-40,(char)-24,19,(char)-73,100,(char)-73,34,110,(char)-53,108,(char)-49,57,61,108,16,119,(char)-120,
    59,(char)-79,(char)-117,57,(char)-65,25,44,(char)-34,77,119,(char)-79,(char)-64,(char)-68,(char)-64,(char)-32,108,87,67,103,5,57,20,105,85,(char)-36,(char)-119,93,44,(char)-60,(char)-12,(char)-61,(char)-39,
    (char)-92,(char)-56,33,43,(char)-78,(char)-117,34,(char)-69,(char)-38,94,68,117,(char)-54,(char)-128,(char)-91,58,94,115,33,43,(char)-74,(char)-81,77,(char)-76,(char)-107,(char)-94,72,20,(char)-119,
    34,(char)-120,83,(char)-86,(char)-77,73,(char)-111,(char)-21,71,76,(char)-61,(char)-44,69,(char)-125,(char)-41,32,26,(char)-68,(char)-122,(char)-84,(char)-115,(char)-41,107,3,(char)-101,(char)-78,65,
    (char)-123,(char)-35,2,99,(char)-1,(char)-35,(char)-1,(char)-52,119,(char)-102,(char)-49,84,(char)-10,110,22,0,72,73,(char)-108,(char)-110,(char)-82,(char)-84,58,81,(char)-74,(char)-124,87,(char)-37,
    (char)-13,51,(char)-117,105,(char)-24,10,30,4,(char)-9,(char)-47,(char)-126,(char)-50,(char)-49,18,21,120,84,(char)-30,97,(char)-88,109,(char)-79,3,101,27,19,(char)-84,(char)-38,111,(char)-121,33,62,
    37,(char)-35,86,(char)-107,82,15,112,(char)-91,(char)-12,(char)-88,79,60,(char)-116,86,(char)-106,(char)-72,81,(char)-33,(char)-57,(char)-99,0,(char)-85,(char)-6,(char)-19,(char)-31,89,58,125,72,(char)-69,
    57,(char)-47,(char)-100,(char)-108,(char)-59,84,(char)-73,30,(char)-48,68,(char)-83,(char)-38,(char)-81,115,80,32,41,(char)-27,(char)-112,66,74,82,(char)-85,(char)-4,100,(char)-14,103,97,32,88,(char)-51,
    (char)-67,(char)-35,(char)-105,(char)-54,(char)-95,57,(char)-85,69,84,119,80,52,81,(char)-85,99,(char)-65,46,60,(char)-93,40,119,(char)-118,(char)-23,74,115,77,(char)-44,(char)-22,(char)-56,111,115,
    1,42,(char)-122,62,(char)-45,49,(char)-7,(char)-123,(char)-44,50,85,19,11,(char)-104,(char)-64,118,105,43,7,37,(char)-20,1,(char)-28,28,106,(char)-121,9,(char)-95,80,(char)-59,74,(char)-108,(char)-38,
    (char)-106,88,81,(char)-53,113,71,104,(char)-94,77,(char)-10,(char)-28,(char)-78,112,38,20,(char)-65,61,(char)-25,47,(char)-11,(char)-128,(char)-42,(char)-75,(char)-116,42,32,86,(char)-126,(char)-73,
    45,97,(char)-88,(char)-27,(char)-72,35,52,(char)-47,(char)-90,120,18,81,33,86,80,79,98,(char)-3,46,9,112,61,(char)-23,(char)-21,(char)-60,71,62,(char)-70,2,38,123,(char)-44,13,98,66,53,(char)-17,39,
    (char)-107,86,(char)-120,47,80,(char)-83,80,66,51,125,(char)-119,(char)-122,(char)-62,(char)-98,116,(char)-117,(char)-23,(char)-21,84,(char)-28,(char)-93,43,96,(char)-118,71,97,23,(char)-84,12,73,71,
    104,(char)-94,(char)-81,(char)-61,(char)-12,87,(char)-104,4,1,64,33,121,(char)-15,(char)-4,(char)-1,(char)-75,42,(char)-101,(char)-72,63,(char)-84,(char)-84,(char)-23,(char)-81,1,120,119,(char)-33,(char)-84,
    3,0,(char)-68,(char)-1,(char)-11,(char)-49,(char)-61,(char)-35,(char)-1,(char)-51,(char)-107,(char)-102,103,(char)-86,0,88,(char)-125,2,0,16,(char)-128,33,119,47,(char)-128,42,54,(char)-55,90,(char)-46,
    8,(char)-126,111,53,108,54,(char)-100,2,5,24,(char)-53,77,74,33,53,(char)-73,106,84,78,(char)-31,(char)-28,66,(char)-91,(char)-60,83,20,105,(char)-56,(char)-114,(char)-118,54,68,10,(char)-26,26,40,110,
    109,106,76,31,(char)-42,(char)-44,(char)-54,(char)-92,72,49,85,(char)-84,111,57,(char)-29,10,(char)-97,(char)-44,46,36,29,95,(char)-87,(char)-23,(char)-113,(char)-55,51,41,(char)-94,76,21,(char)-21,
    (char)-85,97,92,118,52,77,(char)-37,(char)-56,76,119,114,75,125,(char)-63,124,(char)-114,(char)-98,(char)-14,(char)-106,82,(char)-119,85,(char)-67,(char)-32,(char)-18,(char)-1,(char)-79,(char)-44,(char)-37,
    (char)-99,71,58,(char)-125,70,101,(char)-56,74,86,(char)-123,28,(char)-5,38,0,118,(char)-45,(char)-78,64,11,11,(char)-11,(char)-13,2,44,64,(char)-127,92,3,13,2,(char)-64,10,(char)-115,104,16,(char)-58,
    26,64,(char)-87,127,(char)-85,(char)-63,4,50,31,15,(char)-90,32,(char)-58,(char)-85,(char)-125,105,68,(char)-39,55,(char)-104,(char)-127,(char)-113,(char)-50,(char)-63,2,72,(char)-107,14,(char)-74,(char)-128,
    82,88,40,(char)-82,(char)-50,6,(char)-102,117,55,(char)-6,119,(char)-118,17,120,(char)-15,46,117,(char)-116,90,(char)-12,(char)-80,59,(char)-72,1,(char)-70,(char)-62,(char)-10,(char)-8,(char)-41,91,
    (char)-49,(char)-88,73,(char)-96,2,(char)-23,114,101,73,(char)-111,71,(char)-93,93,(char)-125,126,121,(char)-42,51,88,(char)-49,95,(char)-111,6,77,122,(char)-76,(char)-85,97,(char)-92,(char)-45,(char)-64,
    (char)-104,(char)-55,(char)-73,93,94,33,68,(char)-128,(char)-32,28,(char)-111,(char)-94,(char)-114,20,(char)-11,(char)-72,20,(char)-54,23,(char)-59,(char)-42,27,34,118,(char)-8,(char)-76,(char)-38,(char)-34,
    97,67,(char)-75,44,(char)-21,52,(char)-80,108,(char)-35,(char)-76,29,(char)-2,84,41,104,101,36,(char)-123,63,48,(char)-121,(char)-58,58,12,(char)-48,(char)-50,78,(char)-42,(char)-82,93,18,78,(char)-35,
    (char)-91,(char)-122,13,49,53,(char)-124,(char)-67,(char)-43,(char)-91,(char)-22,5,(char)-96,(char)-16,(char)-19,(char)-60,(char)-102,(char)-69,17,7,32,(char)-8,(char)-81,(char)-66,27,64,8,5,26,(char)-87,
    50,(char)-27,42,82,(char)-84,(char)-100,7,21,63,33,(char)-94,(char)-60,61,15,(char)-42,116,(char)-61,(char)-76,28,78,(char)-105,(char)-37,(char)-29,(char)-75,125,(char)-96,31,65,49,(char)-100,32,(char)-87,
    (char)-68,5,96,88,108,14,(char)-105,(char)-57,23,8,69,98,(char)-119,84,38,87,40,85,106,(char)-115,86,(char)-89,55,24,77,102,(char)-117,(char)-43,102,119,56,93,110,(char)-113,(char)-41,(char)-25,7,(char)-126,
    (char)-64,16,40,12,(char)-114,64,(char)-94,(char)-48,24,44,14,79,32,(char)-110,(char)-56,20,42,(char)-115,(char)-50,96,(char)-78,(char)-40,28,46,(char)-113,47,16,(char)-118,(char)-60,18,(char)-87,76,
    (char)-82,(char)-56,59,(char)-54,108,4,123,59,(char)-17,(char)-87,(char)-44,26,(char)-83,78,111,48,(char)-102,(char)-52,22,(char)-85,(char)-51,(char)-18,112,(char)-70,(char)-36,30,47,111,31,95,63,(char)-1,
    (char)-87,101,(char)-19,78,68,(char)-108,101,111,(char)-126,2,4,97,(char)-12,(char)-34,(char)-3,119,(char)-23,(char)-45,(char)-71,(char)-128,12,100,(char)-66,54,65,22,12,(char)-103,(char)-41,14,(char)-120,
    12,11,(char)-97,(char)-113,12,93,29,8,57,95,23,(char)-62,22,48,36,52,(char)-30,(char)-11,60,(char)-108,(char)-53,(char)-23,(char)-70,(char)-35,5,(char)-50,92,(char)-74,54,125,(char)-15,120,(char)-33,
    85,70,62,90,(char)-120,74,(char)-99,115,(char)-32,(char)-38,14,46,1,15,9,(char)-99,(char)-73,77,(char)-88,94,102,97,29,(char)-59,38,(char)-51,(char)-38,43,(char)-93,(char)-99,44,(char)-60,88,47,5,(char)-50,
    112,(char)-75,53,106,56,(char)-1,14,32,(char)-17,46,108,(char)-29,6,15,(char)-32,113,(char)-128,32,80,96,64,16,80,37,(char)-112,96,(char)-32,(char)-82,23,0,(char)-119,(char)-33,(char)-113,(char)-43,
    (char)-9,2,(char)-121,(char)-64,9,46,(char)-64,(char)-128,32,(char)-96,56,(char)-48,(char)-32,(char)-128,(char)-36,23,(char)-92,(char)-29,(char)-106,(char)-122,71,(char)-128,64,66,1,(char)-107,(char)-128,
    (char)-125,(char)-128,98,3,84,15,3,(char)-86,87,(char)-123,3,33,32,(char)-95,28,79,(char)-127,9,51,(char)-33,(char)-11,(char)-69,30,97,(char)-71,106,(char)-66,(char)-60,(char)-89,(char)-111,36,47,(char)-53,
    (char)-124,(char)-20,29,(char)-40,121,(char)-85,(char)-51,(char)-17,(char)-69,75,5,(char)-103,116,(char)-105,(char)-45,(char)-114,(char)-113,(char)-57,(char)-27,101,(char)-48,81,118,(char)-4,(char)-25,
    2,74,(char)-1,(char)-59,(char)-119,(char)-17,75,(char)-23,(char)-63,(char)-57,(char)-58,8,63,67,(char)-14,(char)-67,(char)-96,27,(char)-35,(char)-117,(char)-4,127,101,(char)-48,(char)-100,(char)-116,
    44,120,(char)-95,123,29,(char)-85,(char)-108,(char)-97,83,(char)-111,(char)-52,(char)-28,(char)-21,106,(char)-92,74,120,48,31,101,79,60,(char)-110,122,34,95,52,90,84,(char)-14,(char)-67,(char)-60,(char)-92,
    (char)-42,(char)-80,88,(char)-34,77,(char)-65,76,(char)-116,108,13,(char)-13,(char)-123,6,109,(char)-98,69,(char)-30,2,0,0 };
    static constexpr const char assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1iEq131njotFQ_woff2[] = {
        119,79,70,50,0,1,0,0,0,0,31,100,0,14,0,0,0,0,76,64,0,0,31,13,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,46,27,30,28,46,6,96,0,(char)-124,68,17,16,10,(char)-17,36,(char)-39,65,11,(char)-125,122,0,
    1,54,2,36,3,(char)-121,112,4,32,5,(char)-125,58,7,(char)-112,88,27,106,62,53,(char)-29,88,(char)-109,120,28,64,(char)-87,(char)-43,42,(char)-118,58,53,57,(char)-51,24,69,(char)-80,113,0,81,(char)-8,
    70,(char)-10,(char)-1,(char)-97,(char)-111,116,(char)-116,(char)-31,(char)-96,59,0,84,(char)-78,(char)-2,(char)-125,(char)-118,25,76,58,35,(char)-61,(char)-117,115,85,85,98,(char)-61,42,80,(char)-23,
    51,(char)-111,(char)-94,35,(char)-17,76,(char)-98,72,122,(char)-101,(char)-12,126,86,56,(char)-91,(char)-67,(char)-10,(char)-15,(char)-121,87,125,(char)-19,(char)-118,27,(char)-37,36,(char)-63,35,(char)-120,
    (char)-44,(char)-26,(char)-106,28,(char)-46,68,107,68,104,(char)-20,81,94,(char)-7,(char)-91,(char)-78,89,(char)-54,(char)-49,(char)-99,84,8,74,(char)-114,48,(char)-27,(char)-125,(char)-43,(char)-5,
    (char)-4,11,(char)-122,80,15,7,52,(char)-67,84,114,(char)-68,(char)-42,57,(char)-38,17,26,(char)-5,36,23,74,26,(char)-53,(char)-55,(char)-26,(char)-98,(char)-88,(char)-84,8,28,16,10,83,(char)-42,85,
    (char)-70,(char)-62,84,(char)-103,106,52,(char)-110,(char)-124,(char)-84,4,(char)-50,15,(char)-4,54,123,(char)-108,(char)-123,58,(char)-84,4,81,16,11,65,(char)-46,0,(char)-7,(char)-64,39,(char)-14,(char)-85,
    31,3,(char)-116,(char)-88,101,(char)-71,(char)-120,114,(char)-23,(char)-18,(char)-50,88,(char)-108,(char)-100,(char)-69,59,23,85,(char)-82,(char)-82,122,(char)-47,(char)-6,(char)-67,107,85,85,58,75,
    (char)-1,(char)-89,(char)-34,(char)-88,91,125,(char)-58,(char)-80,(char)-48,(char)-69,(char)-100,116,(char)-122,123,9,(char)-98,63,(char)-101,(char)-74,(char)-45,(char)-10,24,(char)-123,1,(char)-110,
    (char)-92,78,(char)-47,(char)-84,(char)-68,(char)-18,(char)-15,(char)-124,11,26,(char)-39,(char)-36,82,32,96,(char)-125,(char)-1,(char)-25,(char)-71,(char)-31,(char)-3,(char)-25,4,22,90,(char)-92,105,
    32,(char)-111,14,124,(char)-128,51,(char)-97,(char)-40,123,110,8,33,86,(char)-16,(char)-98,65,81,67,72,78,(char)-39,85,(char)-35,(char)-11,89,(char)-9,3,60,72,(char)-47,57,90,(char)-99,126,106,(char)-93,
    (char)-57,(char)-42,(char)-57,66,42,(char)-81,56,43,(char)-8,110,(char)-48,32,30,0,33,23,15,29,(char)-120,78,108,0,56,(char)-94,38,61,(char)-25,14,(char)-22,121,126,59,(char)-2,(char)-88,(char)-48,(char)-92,
    2,74,(char)-31,0,88,(char)-44,41,(char)-87,114,98,80,1,49,78,(char)-7,43,(char)-97,59,(char)-18,50,12,(char)-44,57,(char)-54,(char)-71,(char)-55,(char)-124,85,(char)-75,13,10,(char)-72,(char)-39,(char)-104,
    29,(char)-66,112,20,0,32,56,(char)-57,66,(char)-120,(char)-126,58,(char)-28,31,55,(char)-105,40,(char)-119,22,72,(char)-103,20,(char)-15,79,68,54,(char)-11,(char)-7,(char)-26,(char)-54,(char)-81,76,
    94,55,69,(char)-106,0,100,(char)-111,(char)-107,(char)-83,(char)-102,(char)-99,(char)-39,116,51,(char)-112,(char)-93,(char)-39,(char)-37,35,(char)-64,108,10,64,(char)-110,(char)-119,93,(char)-21,16,
    62,(char)-95,81,95,8,117,95,(char)-40,(char)-6,79,75,27,50,(char)-113,(char)-1,(char)-25,122,(char)-111,(char)-48,101,105,(char)-10,101,67,(char)-85,14,(char)-119,(char)-111,115,(char)-103,(char)-99,
    36,119,(char)-69,115,(char)-91,53,71,(char)-83,106,(char)-95,107,20,18,(char)-108,6,97,17,30,(char)-101,(char)-90,(char)-22,(char)-116,40,118,(char)-58,34,115,17,63,(char)-34,(char)-11,109,(char)-81,
    122,15,(char)-50,37,(char)-69,(char)-35,25,68,89,68,(char)-63,42,40,54,(char)-87,(char)-3,(char)-15,(char)-37,(char)-79,31,71,(char)-10,107,76,55,35,116,84,98,12,(char)-96,(char)-52,32,76,80,112,112,
    105,2,(char)-51,(char)-91,9,(char)-52,44,77,96,(char)-88,52,(char)-127,107,34,(char)-95,10,80,36,124,(char)-97,(char)-124,(char)-51,109,19,14,110,(char)-112,1,(char)-120,81,(char)-104,2,56,(char)-47,
    81,18,62,62,(char)-8,(char)-98,(char)-40,(char)-34,8,(char)-2,79,89,103,51,(char)-8,3,96,110,41,41,(char)-33,(char)-106,(char)-93,0,(char)-70,(char)-100,2,1,14,(char)-82,46,(char)-58,(char)-50,(char)-55,
    (char)-104,8,80,(char)-119,(char)-30,78,33,(char)-110,(char)-128,(char)-24,(char)-35,(char)-96,2,80,(char)-28,(char)-26,26,(char)-19,93,102,6,42,(char)-128,(char)-98,(char)-109,(char)-28,57,26,83,98,
    34,16,(char)-4,(char)-70,(char)-127,112,5,64,(char)-119,(char)-24,4,116,35,(char)-83,113,(char)-64,84,107,65,(char)-126,0,30,68,30,94,(char)-108,(char)-5,75,38,28,80,(char)-127,49,(char)-98,101,(char)-39,
    52,91,(char)-74,(char)-107,(char)-37,(char)-66,125,27,37,46,(char)-119,10,58,(char)-91,(char)-83,(char)-67,79,97,108,(char)-73,(char)-53,30,(char)-33,(char)-24,119,(char)-64,97,78,(char)-57,125,127,
    63,(char)-11,(char)-90,(char)-56,106,64,24,55,120,(char)-68,(char)-34,110,(char)-62,0,(char)-31,3,49,88,(char)-100,81,(char)-16,(char)-65,86,(char)-9,50,(char)-6,24,(char)-5,25,(char)-121,24,(char)-57,
    18,24,(char)-99,(char)-15,2,110,106,(char)-116,(char)-70,(char)-66,(char)-84,(char)-121,27,(char)-13,37,(char)-26,(char)-13,(char)-21,(char)-25,(char)-79,0,79,(char)-35,70,22,(char)-114,(char)-52,31,
    (char)-103,55,(char)-46,53,50,119,100,(char)-10,(char)-56,(char)-84,(char)-111,25,35,(char)-45,70,(char)-90,(char)-114,4,(char)-118,1,(char)-31,102,(char)-1,(char)-30,127,(char)-73,0,(char)-128,16,(char)-93,
    (char)-96,68,(char)-5,21,40,12,16,50,(char)-62,119,(char)-114,101,(char)-14,111,(char)-78,31,(char)-103,127,84,(char)-1,(char)-48,(char)-1,(char)-102,(char)-65,106,(char)-79,86,(char)-94,83,(char)-104,
    (char)-92,122,(char)-71,81,105,(char)-106,25,52,(char)-104,(char)-38,(char)-86,(char)-53,(char)-42,102,25,112,125,(char)-114,(char)-91,(char)-64,(char)-108,71,(char)-26,(char)-102,(char)-13,(char)-115,
    (char)-33,(char)-40,109,(char)-123,(char)-10,(char)-94,(char)-121,(char)-114,18,87,(char)-103,(char)-77,(char)-44,93,(char)-18,(char)-87,(char)-16,86,(char)-6,(char)-85,125,85,(char)-113,2,(char)-75,
    (char)-31,(char)-122,96,93,(char)-88,62,(char)-34,(char)-110,108,75,(char)-76,(char)-90,40,107,51,38,(char)-51,(char)-91,88,12,5,(char)-33,28,(char)-105,106,37,55,122,(char)-75,(char)-44,(char)-37,(char)-31,
    (char)-66,(char)-83,119,(char)-1,121,(char)-4,30,(char)-35,(char)-95,(char)-26,(char)-42,92,(char)-95,(char)-8,(char)-83,(char)-116,54,(char)-27,92,57,81,114,125,(char)-35,12,(char)-33,28,(char)-9,(char)-52,
    119,(char)-57,60,(char)-9,45,(char)-16,(char)-64,66,4,72,(char)-72,(char)-31,1,13,37,70,107,0,(char)-16,75,(char)-64,62,17,30,64,117,26,104,(char)-4,6,(char)-103,79,(char)-60,(char)-73,3,65,32,(char)-82,
    32,64,(char)-124,(char)-60,18,(char)-95,79,34,4,123,(char)-94,(char)-47,58,125,14,37,(char)-35,27,(char)-120,8,(char)-108,60,(char)-105,(char)-103,112,15,(char)-114,10,16,80,52,(char)-107,45,(char)-28,
    (char)-102,125,(char)-21,98,118,47,(char)-77,105,50,(char)-14,(char)-106,102,82,82,(char)-44,121,(char)-26,27,91,108,(char)-3,109,93,99,(char)-47,(char)-67,118,(char)-53,28,107,(char)-10,(char)-38,61,
    (char)-74,(char)-39,6,27,32,93,3,(char)-43,(char)-23,23,31,44,5,73,(char)-61,73,8,(char)-34,(char)-27,(char)-55,3,100,40,(char)-101,80,67,69,29,82,97,(char)-30,97,31,(char)-7,(char)-37,(char)-66,(char)-80,
    (char)-53,(char)-61,13,(char)-56,(char)-102,92,86,(char)-92,12,16,74,95,(char)-128,120,(char)-51,(char)-120,120,36,(char)-105,57,17,40,115,(char)-90,99,0,67,(char)-34,62,(char)-19,30,79,(char)-44,3,
    (char)-66,80,18,1,(char)-77,(char)-46,28,22,56,24,(char)-119,84,(char)-112,(char)-89,56,(char)-17,82,(char)-105,76,(char)-73,7,(char)-62,(char)-98,(char)-98,74,(char)-88,52,79,18,(char)-104,68,(char)-126,
    60,97,72,(char)-60,76,51,70,127,49,110,42,14,46,80,(char)-108,78,40,31,(char)-92,125,64,(char)-34,(char)-120,82,(char)-79,(char)-64,(char)-62,(char)-46,116,58,2,68,4,85,92,(char)-58,66,62,2,31,41,82,
    109,(char)-17,(char)-71,11,82,48,39,(char)-82,(char)-44,(char)-105,(char)-54,(char)-120,(char)-49,18,55,(char)-111,(char)-115,66,99,96,89,124,(char)-38,30,(char)-94,81,125,107,(char)-78,100,(char)-75,
    (char)-118,(char)-91,(char)-127,(char)-93,(char)-20,117,(char)-59,(char)-88,19,(char)-122,67,(char)-127,(char)-108,(char)-70,(char)-16,21,97,28,(char)-88,(char)-116,119,63,(char)-15,(char)-16,(char)-118,
    (char)-110,65,43,(char)-59,92,64,113,(char)-57,(char)-53,88,118,112,(char)-43,69,29,6,110,10,32,33,(char)-64,118,(char)-71,(char)-103,76,91,79,(char)-94,(char)-14,(char)-84,36,13,77,78,66,(char)-46,
    33,(char)-2,(char)-51,73,(char)-115,68,(char)-119,(char)-51,(char)-77,(char)-102,(char)-122,31,(char)-99,(char)-8,4,(char)-14,(char)-114,6,(char)-123,(char)-86,81,104,(char)-28,(char)-59,63,122,(char)-67,
    24,(char)-12,(char)-29,36,45,(char)-89,109,32,126,(char)-127,(char)-46,94,69,(char)-80,(char)-69,(char)-35,17,84,(char)-19,(char)-40,2,(char)-121,(char)-31,17,(char)-118,45,(char)-25,56,6,120,(char)-25,
    (char)-82,(char)-55,120,(char)-76,113,(char)-6,16,59,(char)-62,25,8,58,62,112,70,(char)-125,(char)-91,72,(char)-117,(char)-86,101,(char)-48,124,33,(char)-37,12,119,110,33,103,28,7,104,24,1,(char)-78,
    (char)-58,(char)-26,(char)-96,(char)-26,41,97,(char)-36,20,(char)-74,(char)-115,(char)-94,(char)-64,(char)-25,4,(char)-107,27,(char)-81,99,115,(char)-58,(char)-76,109,28,(char)-102,(char)-49,10,26,(char)-87,
    (char)-68,0,63,(char)-39,118,(char)-93,(char)-124,47,126,11,(char)-21,113,(char)-38,109,9,86,74,(char)-110,29,60,(char)-78,80,(char)-55,72,17,111,51,(char)-34,(char)-4,(char)-58,63,75,(char)-92,112,
    (char)-36,30,(char)-118,(char)-60,(char)-32,127,(char)-64,53,(char)-28,(char)-53,105,80,(char)-88,19,118,2,(char)-21,113,(char)-17,(char)-84,80,106,(char)-101,2,71,(char)-39,(char)-57,93,91,(char)-56,
    (char)-121,(char)-10,(char)-70,(char)-47,21,(char)-72,5,(char)-93,(char)-50,57,(char)-95,71,(char)-61,1,71,(char)-9,(char)-79,(char)-57,24,(char)-94,(char)-113,(char)-13,2,(char)-104,4,(char)-45,121,
    (char)-93,(char)-52,67,(char)-100,18,(char)-50,(char)-44,(char)-29,(char)-96,38,7,38,86,35,(char)-94,(char)-31,(char)-28,12,(char)-11,89,100,(char)-104,65,(char)-70,122,(char)-6,74,(char)-108,58,34,
    17,71,68,84,(char)-120,(char)-6,94,(char)-29,(char)-118,83,(char)-60,(char)-120,(char)-77,(char)-54,(char)-42,11,(char)-111,(char)-117,(char)-104,9,(char)-49,57,(char)-109,32,75,(char)-108,30,(char)-79,
    (char)-92,(char)-1,15,(char)-9,116,29,127,(char)-79,109,40,29,(char)-114,(char)-125,(char)-61,28,(char)-99,101,(char)-107,83,97,118,20,72,(char)-8,(char)-71,47,84,(char)-53,(char)-126,(char)-30,(char)-15,
    (char)-78,(char)-60,110,(char)-10,(char)-65,44,60,(char)-73,(char)-29,8,60,(char)-46,(char)-115,(char)-48,7,40,68,(char)-81,52,108,60,(char)-110,(char)-55,(char)-55,112,(char)-64,8,(char)-68,(char)-103,
    (char)-19,58,100,26,98,(char)-83,(char)-91,18,(char)-73,48,(char)-105,126,(char)-88,99,(char)-16,(char)-109,49,66,48,(char)-93,(char)-25,(char)-54,118,90,35,(char)-66,(char)-121,(char)-68,(char)-70,
    16,(char)-63,60,(char)-39,(char)-9,88,(char)-34,112,24,50,(char)-76,(char)-76,(char)-71,41,(char)-97,64,(char)-76,29,(char)-117,(char)-56,99,14,(char)-38,(char)-109,(char)-58,21,(char)-38,93,(char)-58,
    86,(char)-67,108,(char)-7,67,(char)-123,20,(char)-91,2,(char)-84,(char)-108,41,(char)-47,(char)-44,86,115,(char)-101,91,5,58,76,(char)-67,(char)-125,(char)-21,(char)-27,(char)-28,(char)-68,(char)-97,
    (char)-8,(char)-60,118,(char)-25,(char)-96,(char)-44,38,(char)-15,(char)-125,(char)-99,(char)-25,(char)-90,(char)-23,(char)-4,(char)-118,49,(char)-38,123,(char)-20,35,(char)-101,(char)-84,102,49,(char)-107,
    (char)-108,52,(char)-10,104,116,(char)-107,(char)-46,(char)-59,74,15,103,41,32,(char)-1,11,116,(char)-114,9,14,71,(char)-56,(char)-52,47,(char)-114,(char)-16,69,68,70,(char)-12,(char)-97,(char)-74,16,
    (char)-69,(char)-81,59,56,40,(char)-1,30,66,28,(char)-5,(char)-79,51,92,(char)-24,(char)-81,(char)-42,39,(char)-70,63,74,29,29,(char)-5,(char)-63,(char)-20,(char)-83,36,118,(char)-59,74,99,17,53,(char)-77,
    79,(char)-25,(char)-9,13,(char)-29,(char)-60,63,82,80,(char)-106,71,33,10,(char)-12,(char)-126,(char)-34,37,0,(char)-28,125,(char)-27,(char)-43,65,0,44,123,(char)-108,123,105,37,(char)-127,(char)-30,
    10,6,(char)-72,87,79,(char)-1,48,117,95,108,31,2,77,(char)-94,(char)-91,51,83,17,80,125,39,24,14,(char)-111,(char)-37,101,5,2,(char)-67,(char)-33,(char)-43,100,(char)-113,99,(char)-109,(char)-80,82,
    (char)-87,(char)-27,98,(char)-113,37,3,107,101,127,(char)-9,(char)-48,63,82,48,109,71,34,49,95,(char)-72,(char)-16,(char)-41,108,86,(char)-128,(char)-54,35,116,(char)-92,70,110,100,(char)-124,121,76,
    (char)-121,(char)-112,12,101,(char)-67,116,103,(char)-78,(char)-30,112,65,29,(char)-31,58,44,(char)-113,(char)-48,(char)-64,0,(char)-3,54,67,23,(char)-76,58,(char)-31,(char)-89,67,15,(char)-59,(char)-60,
    (char)-84,(char)-19,(char)-84,75,114,88,2,8,(char)-65,79,(char)-118,32,83,(char)-5,(char)-45,(char)-116,(char)-4,(char)-120,(char)-93,(char)-2,(char)-114,(char)-28,37,(char)-39,78,(char)-21,(char)-28,
    (char)-88,9,81,41,(char)-107,(char)-60,(char)-25,(char)-120,125,10,(char)-84,37,(char)-31,(char)-66,64,(char)-93,(char)-26,37,52,127,24,36,75,4,122,79,(char)-2,(char)-97,91,71,115,(char)-27,66,111,125,
    0,63,(char)-100,(char)-57,0,127,40,68,101,(char)-113,27,(char)-120,93,13,(char)-110,(char)-70,(char)-26,(char)-83,0,(char)-14,(char)-65,44,(char)-112,(char)-49,(char)-70,9,125,21,(char)-74,(char)-37,
    (char)-81,(char)-120,127,(char)-90,(char)-93,(char)-113,(char)-83,22,81,89,8,7,53,(char)-73,39,124,58,(char)-7,76,(char)-36,120,(char)-21,(char)-81,112,(char)-58,28,70,(char)-19,84,117,9,65,102,13,106,
    119,100,(char)-43,59,(char)-33,74,122,(char)-101,(char)-65,27,20,43,36,(char)-47,27,(char)-7,77,(char)-71,(char)-25,70,(char)-79,(char)-117,(char)-101,49,42,49,122,29,49,46,40,71,31,(char)-1,117,107,
    (char)-112,(char)-111,14,76,(char)-88,(char)-108,(char)-70,120,13,52,(char)-119,(char)-82,64,85,76,108,8,(char)-62,78,60,30,51,(char)-56,77,3,(char)-28,85,(char)-4,70,(char)-79,(char)-68,57,3,62,20,
    42,97,0,48,(char)-66,84,(char)-78,66,47,39,(char)-93,(char)-111,49,(char)-94,86,(char)-44,1,7,(char)-42,(char)-102,(char)-114,71,13,69,(char)-62,92,97,10,(char)-105,(char)-17,104,49,109,98,(char)-91,
    106,99,(char)-126,(char)-95,(char)-55,(char)-48,(char)-76,122,82,(char)-50,111,77,(char)-17,99,(char)-52,36,6,13,(char)-40,(char)-34,121,113,8,(char)-83,1,123,(char)-76,(char)-79,(char)-4,62,(char)-14,
    (char)-79,(char)-67,(char)-120,(char)-57,112,(char)-4,(char)-77,79,(char)-17,8,(char)-103,66,(char)-79,(char)-21,(char)-123,(char)-99,81,(char)-1,122,63,(char)-99,(char)-27,(char)-24,1,36,0,(char)-105,
    (char)-34,104,(char)-75,94,91,19,(char)-37,98,37,84,(char)-90,(char)-125,(char)-102,90,(char)-23,(char)-94,86,85,(char)-16,20,107,(char)-36,(char)-2,(char)-120,(char)-48,65,(char)-86,50,63,114,(char)-105,
    75,58,(char)-13,83,(char)-4,(char)-31,(char)-3,(char)-117,(char)-111,61,21,(char)-120,(char)-92,16,49,98,31,(char)-10,55,56,(char)-73,37,122,(char)-101,(char)-81,18,97,(char)-114,22,(char)-108,(char)-65,
    124,83,(char)-113,(char)-94,111,(char)-24,31,(char)-109,41,116,(char)-66,(char)-55,13,3,(char)-35,11,(char)-61,114,12,(char)-89,(char)-57,39,8,0,(char)-43,(char)-5,101,46,(char)-118,(char)-78,36,(char)-3,
    98,73,(char)-43,(char)-92,23,38,(char)-80,26,(char)-22,(char)-113,80,(char)-121,2,25,55,23,(char)-67,76,101,109,13,(char)-16,82,107,(char)-120,99,125,53,(char)-77,(char)-64,(char)-76,86,(char)-84,25,
    40,38,(char)-126,30,(char)-119,115,(char)-12,38,(char)-73,113,123,(char)-56,19,103,(char)-58,23,30,65,(char)-83,59,(char)-40,126,85,47,68,(char)-48,66,108,(char)-95,68,(char)-48,104,122,36,52,(char)-47,
    120,(char)-55,(char)-104,50,34,(char)-15,66,27,(char)-73,(char)-31,111,125,46,(char)-51,1,99,76,119,(char)-98,(char)-113,118,48,60,122,(char)-30,94,(char)-87,7,(char)-36,(char)-40,75,8,48,(char)-10,
    (char)-126,55,5,(char)-15,(char)-25,82,108,(char)-120,111,(char)-128,(char)-55,(char)-33,5,(char)-114,99,56,(char)-74,67,(char)-77,(char)-95,(char)-66,(char)-59,(char)-111,(char)-117,(char)-117,127,
    (char)-51,88,35,(char)-84,101,(char)-94,(char)-125,(char)-49,4,120,95,(char)-50,(char)-59,11,70,31,63,14,(char)-102,76,(char)-66,83,22,(char)-126,108,67,19,(char)-15,77,(char)-53,97,(char)-101,(char)-35,
    (char)-113,7,(char)-96,20,32,55,50,63,92,(char)-18,(char)-55,2,40,27,78,(char)-2,5,69,1,62,(char)-9,100,95,28,80,83,(char)-116,(char)-106,115,77,(char)-5,(char)-72,26,57,(char)-78,89,6,32,34,29,(char)-84,
    (char)-103,(char)-22,(char)-1,(char)-109,(char)-123,64,(char)-51,99,97,25,(char)-102,104,(char)-78,(char)-44,(char)-11,58,104,(char)-53,29,26,34,53,18,8,38,76,(char)-38,(char)-103,(char)-109,(char)-32,
    (char)-22,(char)-38,62,(char)-77,(char)-123,(char)-2,(char)-7,(char)-1,24,34,(char)-57,33,(char)-14,(char)-121,(char)-29,7,58,(char)-12,66,(char)-123,35,(char)-54,(char)-114,(char)-59,40,29,(char)-56,
    124,14,(char)-62,(char)-43,20,(char)-45,8,60,(char)-39,118,(char)-36,(char)-58,(char)-121,(char)-9,86,(char)-34,106,37,(char)-34,45,(char)-102,43,123,(char)-106,10,33,(char)-85,3,102,(char)-79,2,(char)-75,
    87,5,(char)-26,(char)-14,115,(char)-5,24,55,(char)-57,2,68,7,41,3,(char)-123,121,104,17,18,78,(char)-104,68,1,12,(char)-86,12,25,(char)-40,54,(char)-45,116,(char)-102,64,115,(char)-16,56,(char)-90,110,
    (char)-107,49,106,88,(char)-108,42,(char)-79,82,83,47,67,33,19,(char)-122,52,(char)-105,(char)-99,(char)-103,110,(char)-122,(char)-14,(char)-34,(char)-26,74,79,(char)-48,(char)-80,(char)-100,115,56,
    (char)-97,69,46,(char)-110,1,52,(char)-3,(char)-7,5,89,18,27,(char)-66,96,(char)-16,99,(char)-115,(char)-105,(char)-26,(char)-59,49,12,12,(char)-27,(char)-100,4,(char)-76,103,109,(char)-30,48,(char)-30,
    122,(char)-26,114,(char)-75,(char)-112,104,93,33,(char)-102,11,89,21,113,(char)-89,106,(char)-50,76,38,120,(char)-77,(char)-119,63,29,69,(char)-97,42,9,(char)-49,(char)-43,(char)-115,(char)-2,123,102,
    (char)-77,(char)-127,(char)-116,(char)-107,65,22,92,(char)-32,(char)-22,88,34,22,7,(char)-84,(char)-45,26,(char)-59,(char)-122,(char)-51,118,(char)-48,102,10,(char)-115,56,(char)-118,83,107,(char)-31,
    20,27,81,49,(char)-65,(char)-66,(char)-98,(char)-73,4,111,(char)-92,99,68,(char)-125,(char)-52,105,70,42,117,(char)-11,50,(char)-29,112,(char)-58,(char)-12,(char)-128,(char)-84,97,(char)-20,113,(char)-8,
    38,23,83,(char)-100,(char)-36,(char)-26,(char)-28,(char)-114,49,89,(char)-100,108,(char)-104,(char)-36,(char)-87,39,119,(char)-96,109,(char)-16,73,(char)-59,(char)-53,(char)-125,24,(char)-98,(char)-63,
    (char)-98,15,(char)-45,(char)-69,87,119,(char)-61,(char)-33,(char)-55,(char)-45,85,20,79,67,74,102,86,73,(char)-114,94,87,107,30,(char)-37,44,72,125,(char)-105,46,68,56,41,121,(char)-85,2,(char)-62,
    (char)-1,(char)-119,117,(char)-44,59,(char)-78,(char)-61,117,(char)-102,24,49,(char)-99,(char)-101,(char)-81,(char)-12,(char)-73,53,(char)-21,(char)-112,88,118,0,103,62,(char)-56,(char)-88,(char)-46,
    92,118,58,26,89,33,(char)-109,(char)-47,43,(char)-46,53,54,(char)-106,(char)-60,88,(char)-124,(char)-32,(char)-1,(char)-34,85,36,(char)-16,(char)-94,(char)-85,11,(char)-30,(char)-104,(char)-7,(char)-52,
    (char)-72,(char)-126,113,49,66,85,114,106,87,69,(char)-87,10,(char)-124,84,105,62,75,(char)-82,(char)-119,42,(char)-105,(char)-55,(char)-103,21,50,77,46,75,36,(char)-60,89,(char)-23,74,(char)-70,67,
    (char)-62,(char)-98,22,(char)-52,44,88,(char)-80,(char)-25,(char)-99,96,124,(char)-2,122,42,(char)-22,(char)-107,89,(char)-9,(char)-93,(char)-58,47,(char)-88,101,(char)-70,114,(char)-88,76,(char)-83,
    14,5,94,(char)-102,101,(char)-90,97,(char)-42,(char)-44,70,68,30,127,124,(char)-110,(char)-89,(char)-67,45,(char)-108,(char)-93,(char)-99,72,126,37,(char)-52,(char)-110,(char)-60,(char)-122,33,22,125,
    72,82,(char)-38,33,(char)-9,(char)-56,(char)-93,(char)-52,18,77,103,(char)-12,(char)-47,8,36,46,54,28,(char)-111,27,66,(char)-110,(char)-68,(char)-38,(char)-10,(char)-17,95,95,124,(char)-45,43,(char)-24,
    9,(char)-85,(char)-67,(char)-78,(char)-64,96,80,(char)-46,(char)-3,(char)-27,7,(char)-123,124,38,(char)-106,105,(char)-47,(char)-104,(char)-29,(char)-121,(char)-78,51,(char)-26,113,35,(char)-72,93,25,
    25,(char)-29,(char)-6,25,28,15,(char)-119,55,127,(char)-67,1,18,18,14,43,(char)-118,(char)-93,(char)-55,17,63,93,106,(char)-90,101,(char)-90,97,(char)-18,113,(char)-104,(char)-48,(char)-104,(char)-17,
    56,105,(char)-18,40,(char)-108,(char)-67,(char)-64,(char)-124,(char)-28,(char)-10,(char)-10,(char)-72,81,70,86,89,117,(char)-79,17,(char)-109,105,101,(char)-22,(char)-116,(char)-32,(char)-121,18,91,
    72,53,6,(char)-75,47,(char)-80,(char)-101,(char)-28,2,(char)-4,(char)-101,45,(char)-27,(char)-73,(char)-49,(char)-1,(char)-107,90,113,(char)-114,(char)-102,(char)-113,125,(char)-117,(char)-127,85,117,
    (char)-88,(char)-16,(char)-112,(char)-102,(char)-36,(char)-83,43,(char)-28,24,(char)-108,(char)-109,49,(char)-4,(char)-44,61,64,2,85,(char)-100,(char)-53,(char)-54,(char)-48,(char)-48,(char)-53,(char)-27,
    50,122,(char)-59,0,22,38,(char)-90,(char)-82,21,50,(char)-7,17,34,(char)-107,72,(char)-83,(char)-54,(char)-78,(char)-30,6,(char)-68,(char)-75,83,45,(char)-98,30,48,(char)-87,(char)-42,77,89,(char)-89,
    (char)-2,121,22,(char)-27,83,(char)-66,26,(char)-43,97,101,40,110,69,70,(char)-50,(char)-87,7,(char)-128,79,(char)-51,(char)-22,20,47,41,(char)-42,(char)-28,(char)-59,(char)-55,51,76,(char)-15,92,(char)-1,
    (char)-33,(char)-74,(char)-67,(char)-13,(char)-59,(char)-88,59,(char)-1,(char)-108,71,(char)-57,(char)-57,(char)-2,50,(char)-50,(char)-24,(char)-101,(char)-92,(char)-87,78,(char)-48,(char)-16,22,(char)-18,
    41,79,103,(char)-54,20,113,(char)-100,(char)-89,66,110,(char)-60,(char)-62,7,31,31,(char)-87,121,92,14,(char)-96,(char)-23,98,68,104,(char)-24,85,(char)-49,8,85,(char)-121,(char)-50,51,31,(char)-7,93,
    (char)-3,(char)-5,20,(char)-99,38,(char)-20,(char)-50,(char)-55,124,(char)-49,120,104,53,70,(char)-58,32,(char)-126,(char)-92,77,59,(char)-18,(char)-98,119,60,63,47,7,(char)-29,21,64,45,(char)-92,(char)-38,
    (char)-84,1,(char)-73,(char)-100,(char)-127,90,(char)-72,65,(char)-62,70,(char)-37,(char)-71,(char)-40,(char)-83,(char)-115,17,38,(char)-11,(char)-124,(char)-44,37,87,(char)-33,103,(char)-25,(char)-34,
    53,(char)-99,36,23,105,(char)-64,102,54,(char)-75,43,(char)-43,27,92,(char)-20,60,53,60,75,59,(char)-13,26,(char)-107,(char)-14,110,15,(char)-74,(char)-20,(char)-63,(char)-67,(char)-44,(char)-67,12,
    (char)-69,12,74,(char)-22,(char)-116,41,(char)-125,3,3,124,68,51,120,104,72,88,(char)-32,69,45,(char)-93,107,(char)-126,(char)-9,48,35,7,(char)-101,90,(char)-41,94,(char)-57,71,(char)-104,(char)-61,
    24,(char)-10,(char)-98,(char)-48,(char)-91,101,92,10,8,67,67,(char)-96,120,(char)-16,(char)-28,(char)-116,(char)-87,3,125,125,(char)-40,(char)-62,(char)-116,(char)-109,(char)-33,(char)-23,112,(char)-68,
    (char)-93,(char)-66,(char)-79,30,79,25,(char)-34,(char)-64,1,(char)-69,(char)-77,95,111,(char)-1,(char)-35,(char)-94,(char)-73,(char)-68,(char)-78,(char)-21,(char)-31,(char)-37,66,(char)-54,(char)-20,
    (char)-79,38,126,76,123,(char)-13,(char)-47,(char)-34,67,19,7,(char)-89,(char)-13,(char)-75,33,9,(char)-127,(char)-33,104,25,93,(char)-75,(char)-34,(char)-41,(char)-23,(char)-72,101,106,93,(char)-93,
    104,76,(char)-82,117,71,(char)-126,(char)-38,(char)-104,(char)-123,(char)-24,86,(char)-87,(char)-98,21,88,(char)-32,96,6,77,(char)-122,59,113,33,77,(char)-56,(char)-51,0,(char)-82,(char)-22,23,(char)-10,
    (char)-85,(char)-10,61,37,48,50,72,73,(char)-116,(char)-48,(char)-106,25,67,(char)-109,(char)-80,(char)-102,(char)-20,49,6,43,77,77,(char)-66,(char)-80,45,34,85,(char)-82,64,(char)-5,62,(char)-23,(char)-63,
    (char)-45,86,49,(char)-10,41,(char)-18,(char)-60,27,73,(char)-114,(char)-114,(char)-53,111,(char)-85,94,(char)-79,62,(char)-82,42,(char)-121,9,78,(char)-68,(char)-51,(char)-88,(char)-121,29,22,87,(char)-85,
    (char)-55,57,(char)-2,79,(char)-99,(char)-85,(char)-98,(char)-9,0,9,84,(char)-123,(char)-125,(char)-105,(char)-18,8,(char)-101,(char)-101,107,11,(char)-21,98,(char)-80,48,5,117,(char)-19,(char)-78,(char)-27,
    30,33,(char)-10,(char)-107,(char)-87,(char)-117,95,(char)-18,(char)-96,(char)-31,(char)-94,118,33,(char)-51,(char)-26,88,30,63,(char)-88,(char)-51,38,108,23,(char)-47,(char)-16,103,31,(char)-6,113,(char)-25,
    (char)-105,(char)-78,(char)-86,(char)-70,18,91,94,85,(char)-114,(char)-21,(char)-2,(char)-92,(char)-30,(char)-81,(char)-30,90,122,52,(char)-94,93,124,33,127,(char)-105,104,47,64,28,(char)-75,120,(char)-68,
    (char)-70,106,120,(char)-106,(char)-99,(char)-97,105,(char)-78,(char)-94,(char)-103,(char)-126,(char)-11,90,(char)-86,(char)-39,(char)-109,(char)-93,(char)-114,94,25,30,85,58,(char)-75,(char)-84,1,111,
    (char)-105,48,119,(char)-100,(char)-50,74,70,(char)-76,(char)-58,(char)-12,(char)-116,(char)-28,(char)-123,122,79,(char)-125,39,7,(char)-115,(char)-34,30,(char)-63,44,(char)-101,(char)-42,4,(char)-74,
    (char)-42,54,(char)-68,(char)-19,87,61,91,15,59,7,119,10,40,(char)-42,(char)-11,77,124,126,(char)-45,122,(char)-118,85,(char)-80,19,95,102,(char)-15,(char)-18,(char)-32,(char)-102,115,(char)-39,90,(char)-83,
    (char)-126,1,44,123,(char)-78,44,3,(char)-69,120,(char)-48,24,(char)-66,(char)-52,(char)-5,(char)-122,(char)-101,(char)-93,(char)-73,(char)-72,81,(char)-36,10,(char)-80,82,(char)-12,123,(char)-33,(char)-119,
    120,(char)-73,27,109,25,(char)-120,(char)-66,(char)-1,(char)-12,(char)-108,86,28,(char)-58,(char)-73,(char)-110,115,(char)-73,(char)-10,74,49,29,(char)-75,47,112,(char)-34,25,23,(char)-78,38,(char)-23,
    (char)-17,24,23,(char)-79,(char)-74,(char)-128,(char)-37,(char)-39,(char)-59,39,21,39,(char)-97,106,(char)-98,38,79,(char)-37,98,80,80,(char)-79,(char)-84,98,83,(char)-16,(char)-125,(char)-18,95,40,
    69,15,21,28,(char)-79,(char)-65,(char)-95,(char)-71,50,10,(char)-43,(char)-38,(char)-78,42,26,122,(char)-85,10,(char)-110,(char)-44,103,(char)-30,19,107,14,(char)-84,28,34,(char)-66,60,(char)-52,63,
    70,(char)-81,(char)-44,124,70,(char)-108,(char)-22,(char)-39,42,31,65,42,68,(char)-50,(char)-66,(char)-103,(char)-3,122,(char)-48,67,(char)-27,12,90,57,123,40,(char)-3,40,85,125,(char)-117,(char)-2,
    93,94,122,(char)-48,120,46,(char)-17,57,2,(char)-79,30,(char)-1,(char)-51,(char)-89,(char)-74,(char)-55,(char)-67,(char)-105,79,20,109,(char)-21,88,(char)-99,127,(char)-64,30,76,109,(char)-110,123,84,
    122,120,(char)-75,109,(char)-9,(char)-88,(char)-107,123,78,(char)-103,(char)-53,83,83,84,17,93,(char)-75,(char)-34,(char)-87,(char)-116,11,(char)-113,(char)-34,(char)-88,34,7,(char)-81,(char)-81,95,
    92,(char)-23,81,45,103,(char)-3,75,113,87,(char)-92,9,(char)-106,50,51,(char)-112,21,(char)-54,(char)-61,(char)-123,117,(char)-115,(char)-126,(char)-65,(char)-17,39,54,91,33,(char)-12,106,93,18,(char)-86,
    (char)-86,68,42,88,61,21,22,36,21,(char)-55,13,(char)-32,(char)-46,(char)-51,103,115,35,(char)-59,88,(char)-83,(char)-100,(char)-8,(char)-57,49,67,98,(char)-86,(char)-46,(char)-124,(char)-104,(char)-3,
    85,(char)-68,39,(char)-56,(char)-123,(char)-118,(char)-93,1,(char)-121,(char)-111,35,83,(char)-1,(char)-100,78,(char)-103,1,113,111,(char)-25,(char)-65,(char)-45,(char)-68,66,(char)-86,66,24,50,(char)-107,
    87,83,113,(char)-92,(char)-86,77,110,(char)-108,(char)-71,(char)-54,61,95,(char)-24,51,(char)-48,9,124,(char)-25,(char)-52,66,107,(char)-87,93,(char)-99,(char)-108,(char)-121,(char)-27,93,117,57,(char)-55,
    (char)-114,105,(char)-115,(char)-14,(char)-82,(char)-68,(char)-111,114,(char)-119,57,(char)-70,(char)-71,(char)-26,(char)-53,(char)-111,(char)-12,(char)-112,47,(char)-91,(char)-81,(char)-49,63,(char)-12,
    9,(char)-39,24,76,(char)-51,62,33,(char)-6,(char)-21,(char)-81,(char)-47,(char)-102,(char)-11,80,72,(char)-122,73,(char)-105,(char)-107,(char)-106,(char)-99,52,(char)-87,(char)-16,101,(char)-94,50,110,
    (char)-98,96,(char)-124,(char)-53,(char)-27,(char)-114,8,(char)-30,127,(char)-125,(char)-4,(char)-73,(char)-103,(char)-120,(char)-116,104,75,67,(char)-92,46,(char)-65,26,(char)-119,127,113,(char)-120,
    127,(char)-23,(char)-57,61,72,(char)-111,52,(char)-94,77,(char)-122,100,(char)-70,(char)-66,(char)-99,(char)-91,88,69,75,26,(char)-97,101,(char)-43,(char)-117,(char)-12,102,(char)-118,62,84,49,35,(char)-81,
    (char)-128,(char)-109,108,(char)-45,(char)-49,120,(char)-64,(char)-28,(char)-113,(char)-119,81,100,(char)-13,(char)-85,(char)-108,48,(char)-67,15,26,20,(char)-41,(char)-67,(char)-64,(char)-71,35,78,
    29,68,8,(char)-45,(char)-89,(char)-108,(char)-103,(char)-38,(char)-94,18,127,(char)-97,28,96,(char)-101,4,27,(char)-81,(char)-110,109,(char)-102,(char)-40,20,91,73,115,109,93,73,(char)-77,45,69,19,107,
    35,(char)-49,(char)-33,32,(char)-37,(char)-112,97,(char)-54,64,101,(char)-24,83,83,(char)-89,56,120,(char)-13,(char)-37,(char)-88,(char)-128,(char)-15,1,81,111,55,7,(char)-53,85,114,49,42,86,(char)-55,
    (char)-41,12,(char)-85,15,(char)-87,(char)-31,24,(char)-82,(char)-43,(char)-95,58,46,(char)-50,21,(char)-33,74,42,(char)-71,(char)-101,116,15,(char)-67,(char)-2,84,83,(char)-3,80,35,(char)-123,(char)-36,
    123,(char)-9,109,(char)-5,23,41,80,80,(char)-57,48,(char)-27,(char)-61,95,(char)-12,44,(char)-3,47,7,(char)-73,(char)-46,(char)-64,(char)-18,108,(char)-101,4,(char)-111,96,122,36,(char)-85,(char)-2,
    (char)-75,(char)-28,(char)-103,105,106,21,(char)-110,(char)-22,(char)-18,71,(char)-91,(char)-6,(char)-71,(char)-89,34,(char)-86,13,112,(char)-1,59,(char)-106,109,21,(char)-44,(char)-71,5,(char)-55,91,
    (char)-68,(char)-97,(char)-30,99,(char)-16,(char)-51,(char)-117,(char)-109,11,(char)-26,82,49,(char)-72,78,54,(char)-47,77,125,11,(char)-9,77,57,54,(char)-35,(char)-43,11,(char)-13,(char)-87,(char)-54,
    (char)-12,107,(char)-33,48,(char)-11,(char)-42,(char)-124,57,(char)-86,(char)-120,43,(char)-92,43,(char)-68,(char)-96,(char)-17,(char)-86,(char)-92,33,69,34,85,22,75,(char)-22,14,63,(char)-97,(char)-22,
    (char)-52,(char)-71,(char)-1,(char)-16,60,18,(char)-2,(char)-39,(char)-30,111,19,(char)-73,(char)-101,(char)-29,(char)-102,54,102,5,(char)-86,117,(char)-114,44,(char)-31,52,(char)-10,19,(char)-59,15,
    (char)-112,76,77,54,(char)-124,(char)-15,(char)-62,(char)-108,(char)-100,(char)-80,61,(char)-45,(char)-117,(char)-111,21,(char)-53,114,73,81,72,(char)-53,(char)-28,122,95,(char)-15,29,41,91,19,(char)-15,
    92,(char)-47,87,29,43,78,(char)-48,4,(char)-123,31,(char)-113,13,(char)-100,86,88,(char)-122,(char)-84,(char)-82,(char)-36,10,49,(char)-57,(char)-38,127,48,40,13,(char)-55,(char)-52,(char)-120,51,49,
    55,50,(char)-105,(char)-75,(char)-16,(char)-32,(char)-82,(char)-69,62,74,(char)-5,117,87,69,119,62,52,(char)-97,(char)-54,76,(char)-65,(char)-106,99,91,40,11,(char)-41,(char)-88,34,(char)-82,105,62,
    87,74,(char)-2,15,(char)-120,122,70,(char)-97,(char)-40,(char)-91,(char)-118,(char)-20,108,(char)-109,23,38,(char)-16,(char)-83,(char)-51,(char)-100,(char)-5,111,18,(char)-62,(char)-24,(char)-53,(char)-52,
    94,117,(char)-84,(char)-70,70,72,(char)-20,68,(char)-65,(char)-96,(char)-73,(char)-74,(char)-34,(char)-63,(char)-17,(char)-36,30,(char)-81,40,(char)-84,122,(char)-52,(char)-100,(char)-118,(char)-10,
    74,(char)-44,34,41,7,101,25,(char)-108,92,126,47,(char)-6,31,(char)-6,61,69,42,(char)-103,61,13,(char)-99,64,70,(char)-84,69,(char)-88,(char)-41,(char)-13,88,(char)-40,(char)-8,(char)-90,60,9,109,(char)-88,
    (char)-93,31,(char)-21,9,126,(char)-12,48,(char)-114,(char)-126,58,(char)-22,103,119,91,24,(char)-81,98,60,12,(char)-74,(char)-119,(char)-78,21,(char)-116,(char)-26,(char)-52,(char)-20,82,(char)-82,
    98,9,(char)-36,(char)-100,(char)-37,(char)-123,119,(char)-27,(char)-96,57,14,(char)-67,10,(char)-42,(char)-105,101,85,13,(char)-91,(char)-91,(char)-43,53,101,(char)-53,28,47,(char)-48,(char)-25,47,(char)-105,
    (char)-117,118,(char)-14,(char)-28,(char)-68,(char)-99,(char)-94,(char)-27,(char)-112,(char)-119,(char)-74,(char)-55,(char)-37,42,(char)-99,(char)-107,48,78,(char)-1,(char)-107,(char)-57,127,(char)-42,
    (char)-38,81,121,(char)-102,(char)-122,70,(char)-70,(char)-84,55,(char)-21,98,(char)-125,84,(char)-69,(char)-74,11,59,(char)-28,43,63,(char)-82,16,(char)-11,(char)-13,98,121,(char)-35,(char)-94,21,(char)-15,
    (char)-37,(char)-46,20,66,113,(char)-100,72,33,76,(char)-37,6,(char)-20,(char)-102,(char)-109,(char)-11,57,(char)-82,1,(char)-37,104,116,(char)-3,(char)-124,(char)-27,103,(char)-39,(char)-36,(char)-97,
    57,49,(char)-101,23,18,(char)-107,83,(char)-103,(char)-126,(char)-90,107,25,2,103,(char)-54,46,33,(char)-61,82,(char)-104,(char)-51,(char)-109,15,(char)-117,(char)-87,(char)-87,122,(char)-93,(char)-63,
    98,(char)-73,(char)-28,101,(char)-118,84,(char)-69,(char)-40,(char)-20,111,110,40,58,118,(char)-101,9,(char)-111,73,119,41,(char)-92,52,5,51,(char)-42,98,29,55,86,76,12,(char)-47,(char)-14,10,(char)-110,
    (char)-3,(char)-2,(char)-86,29,(char)-125,109,28,(char)-41,91,(char)-63,(char)-94,11,(char)-12,(char)-23,40,(char)-23,(char)-127,82,(char)-20,21,(char)-52,(char)-101,114,40,0,23,74,(char)-50,114,(char)-57,
    (char)-60,125,(char)-26,(char)-19,(char)-28,(char)-14,(char)-43,58,(char)-26,15,76,29,95,13,127,80,29,(char)-107,75,102,(char)-52,(char)-48,(char)-85,(char)-17,(char)-88,(char)-26,(char)-106,(char)-112,
    (char)-123,(char)-40,75,12,39,115,75,(char)-116,(char)-102,(char)-112,(char)-57,(char)-15,(char)-102,80,103,(char)-88,38,(char)-2,113,(char)-120,18,(char)-55,(char)-42,(char)-27,(char)-21,(char)-112,
    108,(char)-57,(char)-34,26,(char)-34,(char)-112,12,(char)-10,(char)-26,(char)-112,(char)-7,37,77,37,(char)-11,17,0,96,4,(char)-53,113,(char)-112,(char)-9,109,(char)-112,111,(char)-48,96,26,68,(char)-114,
    (char)-108,98,(char)-77,(char)-45,(char)-62,62,(char)-89,125,(char)-107,38,(char)-19,115,(char)-104,(char)-58,(char)-124,107,(char)-14,(char)-37,127,5,(char)-88,(char)-67,(char)-62,(char)-3,(char)-103,
    (char)-93,110,21,(char)-6,(char)-82,122,(char)-23,47,(char)-76,(char)-79,100,26,102,(char)-71,92,(char)-58,(char)-84,(char)-112,(char)-95,(char)-71,108,(char)-82,(char)-75,(char)-117,(char)-106,(char)-91,
    79,(char)-88,(char)-97,(char)-54,61,(char)-59,(char)-77,(char)-87,(char)-48,(char)-15,(char)-28,119,(char)-100,(char)-117,49,(char)-28,(char)-41,84,(char)-32,31,(char)-64,(char)-52,38,55,75,10,4,92,
    (char)-59,60,(char)-106,112,116,(char)-101,(char)-26,(char)-101,47,(char)-120,3,(char)-27,(char)-91,(char)-120,28,(char)-47,96,(char)-102,13,(char)-14,13,(char)-5,(char)-92,(char)-53,65,(char)-22,84,
    (char)-50,41,78,(char)-75,(char)-57,15,(char)-3,(char)-22,99,112,(char)-1,(char)-118,(char)-42,(char)-84,99,(char)-43,55,(char)-76,38,(char)-35,1,85,(char)-123,70,15,5,(char)-46,2,(char)-121,(char)-94,
    55,63,41,(char)-44,(char)-110,48,50,102,(char)-99,(char)-3,(char)-45,124,44,87,40,70,(char)-46,79,17,103,(char)-53,31,14,(char)-59,(char)-17,(char)-43,(char)-124,85,(char)-77,63,(char)-88,73,(char)-22,
    (char)-100,(char)-34,49,(char)-34,(char)-125,(char)-48,79,(char)-91,121,(char)-69,49,50,70,33,23,23,126,(char)-43,83,126,(char)-5,(char)-6,95,(char)-4,10,(char)-50,(char)-27,(char)-5,104,61,95,(char)-43,
    40,(char)-78,(char)-100,(char)-109,(char)-21,(char)-9,(char)-28,127,(char)-2,(char)-43,125,101,(char)-92,23,(char)-58,108,47,(char)-118,4,(char)-57,68,(char)-80,127,(char)-10,(char)-121,(char)-31,59,
    90,(char)-71,(char)-58,(char)-74,(char)-88,75,(char)-2,(char)-93,58,(char)-89,61,(char)-60,46,22,105,(char)-119,19,(char)-75,33,103,(char)-28,(char)-90,(char)-31,27,(char)-120,(char)-85,(char)-103,(char)-21,
    (char)-4,(char)-22,127,(char)-93,37,(char)-91,60,(char)-3,45,18,(char)-55,92,113,(char)-36,37,(char)-124,117,84,(char)-122,(char)-52,102,7,102,115,(char)-22,(char)-44,111,(char)-37,100,127,(char)-53,
    (char)-111,(char)-112,110,111,(char)-35,(char)-101,38,(char)-8,126,(char)-17,(char)-117,(char)-82,(char)-60,(char)-3,(char)-42,(char)-105,(char)-65,(char)-16,90,(char)-96,(char)-33,82,(char)-102,(char)-9,
    (char)-22,53,7,61,(char)-107,42,114,(char)-123,80,42,(char)-12,(char)-114,(char)-59,(char)-48,(char)-108,83,68,(char)-100,(char)-63,100,(char)-7,71,(char)-108,49,2,(char)-24,47,38,(char)-126,(char)-25,
    89,(char)-39,55,(char)-41,(char)-52,38,121,(char)-45,34,(char)-23,(char)-73,(char)-34,(char)-117,(char)-6,(char)-28,125,7,127,(char)-123,55,(char)-66,57,94,94,56,43,(char)-36,80,96,(char)-16,10,(char)-99,
    (char)-61,14,(char)-68,(char)-118,61,13,47,110,(char)-43,(char)-62,43,(char)-71,(char)-89,(char)-1,83,22,(char)-56,8,25,77,10,(char)-70,74,(char)-50,(char)-83,111,46,9,(char)-65,60,104,65,(char)-9,(char)-2,
    (char)-69,25,(char)-35,(char)-4,97,47,(char)-6,46,114,106,117,75,87,43,84,89,49,5,22,63,(char)-60,(char)-2,58,27,58,(char)-70,(char)-90,(char)-54,103,75,20,75,92,(char)-31,59,(char)-56,0,(char)-11,(char)-43,
    12,(char)-34,32,(char)-65,(char)-82,104,(char)-32,(char)-78,40,33,(char)-51,(char)-42,(char)-12,15,55,96,112,(char)-79,(char)-24,17,(char)-54,8,(char)-52,29,107,119,(char)-88,(char)-118,(char)-7,(char)-23,
    (char)-123,(char)-95,115,(char)-16,(char)-34,(char)-50,(char)-29,(char)-45,117,(char)-15,(char)-35,70,31,92,(char)-28,33,(char)-11,16,(char)-11,22,99,55,63,76,(char)-29,(char)-94,81,104,58,(char)-15,
    78,(char)-102,26,(char)-89,53,(char)-71,(char)-32,93,51,104,51,32,71,(char)-55,21,(char)-11,(char)-109,(char)-68,98,77,(char)-16,121,(char)-16,99,50,(char)-35,48,(char)-37,26,(char)-50,(char)-53,110,
    (char)-54,26,99,(char)-56,(char)-15,23,(char)-112,47,(char)-19,(char)-113,(char)-110,(char)-85,80,(char)-61,31,24,(char)-66,30,(char)-121,(char)-96,109,99,(char)-84,33,104,83,91,53,110,(char)-71,(char)-95,
    70,(char)-125,(char)-98,24,1,19,(char)-115,(char)-60,19,(char)-117,(char)-89,(char)-6,94,77,(char)-97,(char)-109,(char)-16,(char)-89,(char)-59,(char)-43,10,9,84,71,11,(char)-41,122,48,(char)-67,106,
    75,(char)-16,(char)-12,44,22,(char)-23,98,(char)-97,126,41,79,119,(char)-16,32,(char)-103,105,(char)-97,(char)-16,(char)-74,(char)-101,(char)-91,(char)-115,(char)-26,(char)-112,(char)-50,27,(char)-13,
    101,(char)-91,124,107,103,16,115,58,116,14,59,(char)-4,97,(char)-20,(char)-22,(char)-70,(char)-73,(char)-121,(char)-86,16,(char)-90,43,50,49,(char)-67,(char)-68,85,(char)-5,(char)-18,15,57,58,84,(char)-95,
    125,(char)-82,(char)-62,72,22,(char)-71,30,73,17,(char)-96,75,44,(char)-22,104,(char)-88,35,(char)-81,(char)-104,92,123,(char)-7,(char)-32,(char)-3,(char)-20,(char)-67,55,(char)-56,21,(char)-124,(char)-42,
    (char)-38,(char)-118,(char)-14,(char)-42,(char)-58,(char)-78,84,7,(char)-25,(char)-29,(char)-14,102,5,(char)-125,45,30,(char)-107,(char)-60,72,70,37,(char)-59,(char)-26,(char)-34,(char)-69,(char)-68,
    54,28,108,86,12,(char)-57,(char)-6,20,125,(char)-49,(char)-72,(char)-85,(char)-61,(char)-81,(char)-49,38,61,(char)-2,122,(char)-80,84,74,94,0,(char)-70,55,(char)-115,90,31,(char)-34,66,(char)-13,(char)-99,
    (char)-31,(char)-93,(char)-107,(char)-97,(char)-15,47,(char)-20,(char)-24,117,11,(char)-4,14,(char)-76,121,(char)-93,120,108,(char)-110,(char)-39,(char)-69,(char)-61,(char)-68,(char)-20,(char)-77,(char)-11,
    41,106,(char)-43,86,(char)-121,(char)-50,97,(char)-5,(char)-40,25,(char)-16,(char)-57,116,86,(char)-124,102,(char)-67,(char)-99,(char)-30,(char)-31,(char)-27,(char)-77,(char)-27,15,(char)-41,(char)-101,
    (char)-41,(char)-94,79,(char)-42,5,(char)-74,(char)-3,16,(char)-84,14,(char)-122,(char)-124,10,42,(char)-53,124,(char)-91,(char)-114,(char)-66,(char)-63,(char)-33,(char)-37,127,3,125,51,(char)-28,38,
    127,(char)-56,118,(char)-32,(char)-71,(char)-2,53,(char)-2,(char)-107,(char)-102,33,(char)-38,10,95,72,(char)-47,73,56,84,(char)-57,4,(char)-25,(char)-49,30,(char)-116,105,36,80,(char)-12,(char)-87,
    121,(char)-23,(char)-107,(char)-50,74,(char)-23,39,87,120,(char)-97,75,(char)-28,(char)-55,(char)-102,(char)-72,(char)-11,(char)-84,19,(char)-78,57,77,38,(char)-68,(char)-102,(char)-123,28,8,(char)-117,
    (char)-86,(char)-11,(char)-95,77,(char)-120,10,59,(char)-128,(char)-80,(char)-86,113,83,19,36,100,6,119,(char)-1,21,66,62,127,117,7,(char)-49,50,(char)-75,(char)-122,(char)-3,17,26,24,(char)-6,71,(char)-40,
    (char)-45,91,10,126,29,(char)-36,5,(char)-122,(char)-114,67,(char)-75,(char)-91,(char)-57,(char)-3,77,(char)-2,119,(char)-109,(char)-123,13,53,(char)-63,7,(char)-74,(char)-52,101,(char)-88,6,114,62,
    53,(char)-79,71,(char)-125,2,87,53,78,(char)-67,(char)-109,90,51,14,122,(char)-49,101,67,(char)-54,(char)-102,(char)-116,67,50,8,(char)-54,(char)-15,46,(char)-112,(char)-69,7,(char)-87,(char)-95,(char)-102,
    (char)-118,(char)-17,73,(char)-17,106,(char)-6,61,(char)-1,(char)-9,(char)-81,(char)-38,(char)-14,(char)-65,(char)-31,63,79,(char)-22,(char)-63,(char)-107,(char)-43,(char)-28,1,47,122,(char)-86,(char)-55,
    (char)-58,(char)-20,36,16,(char)-112,(char)-74,(char)-33,(char)-89,14,(char)-3,117,99,(char)-18,124,(char)-8,92,(char)-41,(char)-118,6,55,85,85,83,82,84,61,109,(char)-13,34,68,54,(char)-112,(char)-31,
    3,46,(char)-96,(char)-1,(char)-15,(char)-93,49,(char)-120,21,(char)-92,(char)-114,(char)-125,82,(char)-56,(char)-117,21,75,65,113,(char)-71,68,19,89,49,10,(char)-120,(char)-37,37,(char)-6,(char)-117,
    72,113,101,39,8,(char)-9,123,93,65,(char)-114,18,112,68,82,91,36,24,(char)-34,48,8,17,71,(char)-99,44,76,60,(char)-80,47,17,84,0,9,(char)-43,30,(char)-31,0,112,(char)-28,(char)-112,(char)-121,63,0,(char)-118,
    44,78,(char)-36,(char)-121,112,(char)-25,(char)-53,(char)-86,(char)-44,(char)-77,28,32,121,(char)-81,(char)-54,(char)-13,37,57,(char)-11,(char)-46,(char)-11,(char)-74,97,38,32,(char)-115,(char)-62,(char)-43,
    (char)-101,68,(char)-86,(char)-68,(char)-96,120,(char)-54,(char)-104,(char)-17,(char)-8,40,88,(char)-87,(char)-12,108,97,7,90,(char)-109,90,79,(char)-125,(char)-65,(char)-95,97,(char)-4,123,73,(char)-58,
    (char)-25,101,(char)-16,(char)-98,25,3,27,33,(char)-30,123,9,7,(char)-106,76,69,(char)-120,95,(char)-75,(char)-4,(char)-43,21,34,(char)-31,(char)-46,94,22,(char)-73,6,66,(char)-94,(char)-112,(char)-60,
    31,(char)-52,89,9,74,0,31,46,(char)-14,100,101,(char)-35,40,(char)-46,64,10,(char)-25,(char)-51,75,(char)-86,4,(char)-43,(char)-19,(char)-33,36,(char)-8,(char)-110,67,(char)-31,(char)-51,85,(char)-118,
    (char)-124,92,114,92,(char)-52,42,(char)-9,(char)-71,(char)-78,(char)-111,(char)-102,(char)-122,50,(char)-36,68,(char)-62,(char)-78,(char)-57,115,(char)-27,34,89,(char)-62,(char)-21,(char)-106,87,(char)-120,
    16,23,101,18,57,(char)-19,30,(char)-93,(char)-29,109,62,(char)-26,43,(char)-83,(char)-25,(char)-80,118,31,(char)-62,(char)-68,47,61,66,95,26,0,(char)-77,(char)-33,81,46,(char)-116,(char)-20,1,119,41,
    89,(char)-89,72,26,5,114,(char)-45,(char)-19,5,69,40,(char)-52,(char)-17,70,102,35,(char)-16,91,125,(char)-29,(char)-39,82,101,(char)-93,(char)-43,81,(char)-12,(char)-5,73,54,70,43,(char)-57,80,(char)-116,
    (char)-9,(char)-4,94,(char)-2,22,22,(char)-65,(char)-30,55,(char)-4,(char)-97,(char)-68,22,(char)-74,(char)-66,(char)-15,125,95,(char)-38,44,(char)-101,77,108,(char)-49,(char)-96,(char)-113,(char)-27,
    (char)-76,20,(char)-21,61,(char)-65,(char)-99,(char)-4,(char)-41,(char)-51,63,103,111,94,47,39,(char)-70,62,(char)-33,6,17,123,(char)-21,124,97,(char)-17,127,(char)-100,(char)-2,11,2,(char)-84,(char)-9,
    10,(char)-37,(char)-25,19,26,(char)-90,(char)-83,67,60,(char)-10,105,(char)-100,10,(char)-8,16,25,74,105,36,126,110,125,64,(char)-43,25,(char)-32,(char)-96,(char)-108,70,61,(char)-64,(char)-16,(char)-63,
    68,124,7,27,(char)-29,(char)-115,113,(char)-5,(char)-5,(char)-125,28,96,(char)-65,126,80,61,80,(char)-81,(char)-125,(char)-79,(char)-50,(char)-29,(char)-34,123,(char)-118,(char)-110,(char)-118,(char)-61,
    (char)-97,78,(char)-93,(char)-112,(char)-21,37,12,40,79,26,(char)-58,64,7,(char)-19,(char)-69,50,110,(char)-3,107,72,(char)-54,(char)-91,2,117,(char)-98,(char)-93,(char)-44,68,25,19,(char)-83,73,11,
    (char)-127,14,80,(char)-113,103,52,44,(char)-71,123,(char)-86,(char)-84,57,(char)-13,(char)-108,70,(char)-112,(char)-93,(char)-44,68,25,19,(char)-39,46,120,(char)-110,(char)-88,80,(char)-118,107,66,
    63,5,(char)-57,(char)-46,(char)-120,5,(char)-86,18,(char)-33,(char)-43,70,(char)-87,118,86,10,45,27,(char)-67,113,(char)-128,122,76,104,(char)-104,26,(char)-10,(char)-53,(char)-9,68,2,(char)-51,59,21,
    (char)-115,(char)-6,60,116,107,(char)-33,62,81,82,(char)-47,49,(char)-108,(char)-4,126,28,125,(char)-113,13,(char)-56,61,(char)-108,(char)-78,70,124,(char)-128,46,17,(char)-66,(char)-122,54,(char)-12,
    (char)-90,18,(char)-4,(char)-49,30,(char)-11,(char)-61,52,(char)-67,18,106,6,76,(char)-49,(char)-67,115,(char)-108,0,117,83,98,47,121,(char)-6,(char)-75,41,83,51,103,(char)-90,1,(char)-87,(char)-75,
    (char)-30,(char)-15,(char)-28,45,122,114,(char)-108,9,(char)-30,25,107,45,(char)-126,29,(char)-35,3,7,74,(char)-34,(char)-104,57,(char)-90,(char)-31,(char)-34,89,12,(char)-67,(char)-22,41,(char)-114,
    (char)-73,69,(char)-95,(char)-108,(char)-45,(char)-72,(char)-35,118,63,108,2,106,(char)-12,(char)-78,(char)-17,10,65,49,13,78,(char)-55,112,(char)-40,(char)-122,(char)-127,(char)-66,(char)-112,56,83,
    6,(char)-122,22,75,(char)-26,(char)-48,(char)-67,13,36,119,75,(char)-107,21,(char)-10,(char)-95,1,(char)-40,24,(char)-40,(char)-63,66,82,37,(char)-14,(char)-54,27,51,(char)-128,(char)-98,28,(char)-27,
    41,(char)-99,17,2,(char)-76,2,(char)-117,(char)-102,40,(char)-51,114,86,48,(char)-117,43,77,108,(char)-52,33,(char)-45,96,(char)-82,(char)-99,62,75,52,(char)-48,(char)-108,31,54,61,67,(char)-45,(char)-10,
    (char)-63,39,(char)-73,99,81,19,62,33,39,13,(char)-45,80,14,121,(char)-76,(char)-50,(char)-62,(char)-21,28,(char)-10,(char)-61,102,(char)-32,127,(char)-104,(char)-125,0,32,(char)-112,(char)-17,64,(char)-90,
    (char)-125,24,(char)-109,(char)-15,102,(char)-72,(char)-109,126,1,(char)-16,(char)-8,(char)-47,11,0,(char)-16,(char)-4,(char)-1,(char)-89,(char)-11,(char)-1,83,124,(char)-13,(char)-113,18,(char)-128,
    2,68,0,48,32,(char)-39,(char)-105,87,(char)-48,97,102,(char)-99,(char)-59,15,16,(char)-72,(char)-79,(char)-57,(char)-74,124,24,118,40,(char)-33,67,9,(char)-100,85,40,71,93,(char)-126,121,23,110,(char)-60,
    (char)-116,88,20,34,18,34,(char)-40,64,(char)-121,6,37,(char)-80,7,105,28,(char)-16,96,64,(char)-104,(char)-80,29,(char)-92,64,82,8,1,(char)-94,(char)-31,3,94,(char)-86,(char)-71,(char)-97,(char)-120,
    (char)-37,(char)-66,95,8,(char)-124,(char)-127,(char)-113,80,(char)-92,35,27,(char)-31,33,116,30,(char)-92,97,104,66,18,13,82,(char)-113,(char)-4,(char)-113,41,(char)-17,(char)-115,71,20,(char)-114,
    36,100,92,8,85,46,(char)-86,38,126,(char)-69,26,(char)-11,(char)-56,54,69,(char)-95,(char)-50,126,10,3,(char)-105,15,65,(char)-87,(char)-53,104,36,25,74,54,23,82,69,(char)-59,(char)-98,107,97,84,100,
    33,17,(char)-37,(char)-94,(char)-24,(char)-58,63,74,(char)-125,18,4,22,(char)-78,23,(char)-110,(char)-62,3,88,33,60,107,(char)-21,59,24,(char)-105,(char)-99,0,112,(char)-94,48,(char)-106,(char)-102,
    (char)-28,(char)-113,73,(char)-45,127,42,9,(char)-56,(char)-116,10,57,87,44,42,(char)-6,92,124,(char)-104,(char)-121,57,(char)-24,(char)-58,(char)-97,88,(char)-117,(char)-125,24,(char)-59,(char)-73,
    (char)-68,(char)-56,28,(char)-61,(char)-1,(char)-5,(char)-66,3,(char)-47,(char)-117,(char)-77,(char)-8,(char)-111,(char)-70,47,30,(char)-59,109,(char)-97,0,(char)-16,43,113,(char)-95,(char)-114,11,(char)-39,
    30,20,1,65,(char)-72,70,114,(char)-18,121,(char)-86,(char)-105,89,(char)-110,59,0,(char)-57,62,(char)-72,26,4,66,61,(char)-83,65,(char)-60,(char)-101,27,53,72,72,109,(char)-84,65,38,94,107,13,10,(char)-127,
    (char)-20,53,92,96,19,98,111,(char)-124,1,23,58,(char)-21,42,(char)-83,(char)-46,36,(char)-57,(char)-19,80,(char)-95,93,(char)-35,22,119,82,105,(char)-82,14,(char)-36,56,(char)-36,(char)-65,22,(char)-19,
    106,36,(char)-77,(char)-46,48,(char)-47,83,50,(char)-61,52,(char)-86,50,(char)-127,89,(char)-117,102,45,(char)-110,100,(char)-85,82,99,(char)-84,70,101,(char)-38,(char)-27,(char)-86,(char)-46,30,13,
    67,73,25,82,112,(char)-15,(char)-29,(char)-20,25,(char)-50,(char)-98,(char)-31,(char)-38,(char)-62,(char)-112,(char)-71,(char)-10,(char)-23,110,100,(char)-92,(char)-89,(char)-44,(char)-110,(char)-2,
    77,(char)-123,85,(char)-50,(char)-93,(char)-43,(char)-60,(char)-90,(char)-110,(char)-102,18,32,125,34,(char)-49,91,(char)-126,17,(char)-125,24,(char)-72,93,48,119,89,50,(char)-94,81,35,6,(char)-101,
    118,76,113,(char)-43,32,86,(char)-31,113,63,92,105,(char)-30,68,110,(char)-6,54,58,65,6,(char)-64,(char)-8,126,(char)-4,75,48,35,2,9,53,13,29,(char)-61,72,96,100,97,(char)-107,37,(char)-101,77,(char)-82,
    60,(char)-7,(char)-20,10,21,(char)-119,(char)-63,22,(char)-117,35,94,(char)-126,68,73,(char)-72,(char)-110,(char)-15,(char)-91,16,17,(char)-109,(char)-112,74,(char)-109,65,102,(char)-42,(char)-56,(char)-16,
    (char)-54,72,(char)-64,48,45,(char)-37,(char)-31,116,(char)-71,61,94,31,8,(char)-63,8,(char)-118,(char)-31,4,73,(char)-43,13,(char)-10,5,(char)-64,24,(char)-51,0,126,(char)-114,23,68,(char)-87,(char)-9,
    88,114,(char)-123,82,(char)-43,93,(char)-15,26,(char)-83,78,111,48,(char)-102,(char)-52,22,(char)-85,(char)-51,(char)-34,11,18,(char)-121,(char)-45,(char)-27,(char)-10,120,125,109,(char)-121,(char)-4,
    0,8,(char)-63,(char)-3,(char)-40,39,40,6,(char)-117,(char)-61,19,(char)-120,36,50,(char)-59,(char)-39,(char)-59,(char)-43,(char)-51,(char)-35,(char)-61,(char)-45,(char)-53,(char)-37,(char)-57,(char)-41,
    (char)-49,(char)-65,(char)-97,21,(char)-46,(char)-24,12,102,(char)-53,(char)-31,62,23,108,14,(char)-105,(char)-57,23,8,69,98,73,(char)-17,52,(char)-54,(char)-28,10,(char)-91,(char)-86,15,(char)-38,53,
    90,84,(char)-89,55,24,77,(char)-26,(char)-98,(char)-101,98,(char)-75,(char)-39,29,8,(char)-38,10,12,39,72,(char)-118,(char)-42,(char)-53,62,90,64,103,48,89,108,14,(char)-105,(char)-57,23,8,69,98,(char)-119,
    84,38,87,40,(char)-5,86,(char)-91,(char)-42,104,(char)-5,(char)-55,(char)-10,(char)-18,(char)-9,(char)-96,71,45,109,(char)-92,37,(char)-87,(char)-18,(char)-11,(char)-84,(char)-121,61,(char)-74,67,(char)-89,
    55,24,77,102,(char)-117,(char)-43,102,119,56,93,110,(char)-113,(char)-41,(char)-25,(char)-25,(char)-50,104,126,108,(char)-75,90,83,22,(char)-100,(char)-106,(char)-105,79,75,(char)-45,0,109,89,(char)-10,
    (char)-68,124,107,100,73,109,(char)-87,43,(char)-11,(char)-91,(char)-127,52,(char)-110,(char)-58,(char)-46,(char)-44,48,43,62,(char)-74,36,45,(char)-75,(char)-91,(char)-114,(char)-44,(char)-105,6,(char)-46,
    72,(char)-102,6,(char)-51,(char)-47,(char)-78,(char)-1,18,(char)-78,23,(char)-77,(char)-78,(char)-1,88,37,31,41,94,(char)-57,(char)-22,126,(char)-26,(char)-42,27,72,65,89,(char)-22,(char)-1,(char)-77,
    (char)-91,30,(char)-58,20,(char)-50,(char)-49,32,(char)-21,(char)-9,119,19,67,(char)-71,(char)-29,(char)-34,63,37,124,(char)-67,(char)-85,31,(char)-70,(char)-50,(char)-14,19,71,79,(char)-94,(char)-40,
    (char)-10,(char)-114,76,113,47,52,(char)-63,60,81,(char)-11,92,(char)-85,(char)-4,(char)-94,59,3,17,(char)-44,53,41,(char)-125,111,(char)-17,21,(char)-69,41,30,(char)-60,77,67,104,23,(char)-33,(char)-119,
    (char)-16,(char)-88,113,(char)-116,(char)-108,(char)-102,72,35,123,(char)-10,82,123,(char)-68,108,(char)-74,63,81,44,124,(char)-89,(char)-35,19,49,39,118,(char)-27,94,3,(char)-22,(char)-1,(char)-31,
    (char)-110,(char)-71,53,(char)-90,(char)-115,(char)-78,(char)-67,(char)-53,(char)-8,123,(char)-103,114,30,(char)-122,(char)-44,(char)-67,127,48,15,105,(char)-50,121,69,(char)-100,23,(char)-33,66,106,
    (char)-49,(char)-106,31,(char)-121,(char)-19,(char)-17,118,110,(char)-113,(char)-123,(char)-86,101,85,(char)-124,(char)-1,(char)-123,53,46,(char)-1,95,47,(char)-53,(char)-88,113,86,104,(char)-84,8,(char)-109,
    93,(char)-113,(char)-67,15,53,(char)-67,(char)-28,40,(char)-62,(char)-5,(char)-21,(char)-122,(char)-75,(char)-88,(char)-42,92,(char)-113,99,(char)-45,(char)-42,(char)-39,70,61,55,(char)-45,(char)-48,
    (char)-38,(char)-7,24,(char)-74,39,(char)-71,115,(char)-99,(char)-73,(char)-70,(char)-10,57,(char)-81,91,99,(char)-92,10,(char)-14,105,(char)-96,(char)-123,82,58,(char)-22,(char)-96,47,(char)-85,125,
    (char)-25,(char)-54,101,87,46,(char)-105,(char)-76,79,71,(char)-3,(char)-17,102,(char)-17,30,(char)-73,(char)-33,74,(char)-39,(char)-34,(char)-68,(char)-5,(char)-3,(char)-28,(char)-9,(char)-48,88,76,
    (char)-11,(char)-128,53,(char)-101,119,112,(char)-100,(char)-8,(char)-11,(char)-48,(char)-101,(char)-49,13,112,(char)-86,(char)-21,59,62,(char)-11,(char)-112,20,92,52,3,2,(char)-24,98,4,26,0,97,68,72,
    113,63,(char)-2,90,89,110,82,(char)-46,(char)-45,(char)-108,16,49,(char)-124,(char)-55,(char)-35,86,127,46,(char)-40,60,(char)-106,(char)-40,(char)-67,28,115,(char)-54,27,(char)-34,(char)-16,(char)-54,
    (char)-1,(char)-52,11,66,16,100,113,28,(char)-91,75,(char)-85,18,75,(char)-93,32,117,42,85,84,97,(char)-27,(char)-95,95,(char)-110,(char)-17,17,82,(char)-105,48,95,65,29,(char)-119,37,82,(char)-85,(char)-13,
    (char)-84,(char)-31,112,(char)-30,(char)-23,(char)-22,96,6,(char)-19,80,121,53,(char)-70,99,(char)-124,(char)-28,99,48,8,(char)-118,(char)-96,(char)-29,85,79,23,125,(char)-73,(char)-75,(char)-15,76,
    (char)-73,(char)-93,(char)-113,95,(char)-3,67,102,(char)-57,(char)-119,(char)-13,(char)-100,24,(char)-113,(char)-25,50,(char)-23,(char)-80,91,(char)-125,70,109,(char)-113,117,121,110,(char)-59,(char)-17,
    126,(char)-97,10,(char)-19,(char)-50,15,81,125,(char)-18,122,80,101,(char)-24,(char)-56,7,(char)-18,115,118,91,9,79,23,(char)-81,92,(char)-98,14,25,103,35,28,77,120,75,(char)-58,(char)-42,(char)-75,
    (char)-98,(char)-125,(char)-45,27,63,99,89,(char)-71,67,(char)-112,(char)-7,(char)-70,17,0,0 };
    static constexpr const char assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1iIq131njotFQ_woff2[] = {
        119,79,70,50,0,1,0,0,0,0,16,96,0,13,0,0,0,0,35,(char)-76,0,0,16,11,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,30,28,46,6,96,0,(char)-126,60,17,16,10,(char)-78,24,(char)-89,117,11,(char)-127,86,0,
    1,54,2,36,3,(char)-127,90,4,32,5,(char)-125,58,7,(char)-121,85,27,(char)-49,28,21,(char)-20,(char)-40,35,(char)-80,113,0,(char)-50,30,27,(char)-117,(char)-88,(char)-108,(char)-20,23,(char)-4,127,(char)-99,
    96,(char)-114,(char)-95,(char)-82,7,86,(char)-49,(char)-48,41,(char)-80,65,(char)-125,(char)-78,84,(char)-38,(char)-108,(char)-63,14,(char)-99,124,(char)-24,(char)-110,(char)-50,53,62,(char)-30,(char)-80,
    (char)-125,(char)-84,(char)-78,(char)-83,43,(char)-113,56,(char)-102,32,80,(char)-110,(char)-90,(char)-8,(char)-54,(char)-4,(char)-113,117,(char)-6,(char)-51,103,(char)-61,(char)-55,(char)-1,75,(char)-54,
    38,(char)-3,60,109,(char)-85,55,(char)-61,(char)-57,(char)-120,68,44,108,84,(char)-64,28,(char)-92,101,40,7,24,82,44,(char)-80,(char)-22,(char)-80,119,47,114,43,91,55,(char)-67,(char)-67,(char)-120,
    (char)-10,(char)-82,(char)-11,(char)-38,(char)-83,(char)-62,47,(char)-89,(char)-97,(char)-69,(char)-85,(char)-17,3,(char)-17,73,99,68,111,14,6,(char)-80,86,(char)-44,(char)-66,125,(char)-75,0,(char)-104,
    106,42,(char)-45,(char)-108,(char)-46,104,38,0,37,(char)-128,37,(char)-124,6,41,118,(char)-5,43,(char)-29,4,(char)-126,72,(char)-46,(char)-128,53,(char)-116,(char)-69,92,(char)-53,68,117,102,51,117,
    (char)-64,(char)-28,(char)-128,(char)-124,35,86,40,100,(char)-107,121,(char)-122,(char)-69,7,(char)-58,92,(char)-66,96,(char)-121,(char)-93,88,(char)-51,119,68,(char)-122,8,(char)-40,(char)-127,87,117,
    112,23,(char)-119,(char)-24,(char)-124,53,0,(char)-69,(char)-24,95,115,(char)-97,119,(char)-59,108,(char)-10,(char)-105,(char)-43,1,42,64,89,(char)-12,(char)-128,70,(char)-43,(char)-24,(char)-51,(char)-26,
    (char)-45,(char)-53,11,(char)-12,(char)-120,57,5,114,(char)-64,(char)-74,(char)-70,(char)-29,(char)-119,(char)-64,2,(char)-79,(char)-22,(char)-8,10,(char)-91,42,(char)-68,44,(char)-4,(char)-65,127,3,
    57,39,(char)-69,(char)-44,(char)-110,2,(char)-79,(char)-80,(char)-74,(char)-74,(char)-38,94,75,62,70,20,72,(char)-94,(char)-111,105,(char)-5,123,(char)-43,42,(char)-7,25,37,(char)-37,35,10,(char)-56,
    24,(char)-49,(char)-80,(char)-70,(char)-26,111,123,(char)-41,8,48,0,(char)-16,(char)-125,4,117,34,(char)-104,108,105,104,(char)-16,1,4,0,(char)-32,(char)-15,16,0,17,(char)-28,2,50,(char)-32,(char)-53,
    99,(char)-10,(char)-104,(char)-69,27,34,78,(char)-45,96,47,68,0,0,(char)-85,(char)-51,33,(char)-36,90,43,48,1,(char)-16,69,76,14,0,(char)-75,(char)-38,35,86,(char)-81,(char)-78,(char)-8,0,(char)-9,103,
    (char)-6,50,113,6,84,(char)-29,0,127,(char)-30,25,120,(char)-125,(char)-56,(char)-87,101,(char)-87,8,(char)-56,(char)-77,(char)-50,48,2,60,105,(char)-102,(char)-117,48,44,(char)-100,5,(char)-40,122,
    0,0,28,(char)-31,19,0,(char)-64,2,(char)-122,37,71,(char)-120,80,(char)-120,(char)-127,27,(char)-62,56,(char)-112,38,67,52,(char)-89,115,13,4,22,26,64,91,(char)-47,94,116,24,29,71,(char)-89,(char)-47,
    69,(char)-12,42,(char)-6,26,77,(char)-95,25,116,27,(char)-35,103,(char)-99,(char)-103,(char)-79,85,124,(char)-20,110,(char)-76,10,(char)-19,64,(char)-121,(char)-48,49,116,10,(char)-99,67,(char)-41,(char)-48,
    (char)-121,(char)-24,59,(char)-12,51,(char)-6,23,(char)-35,(char)-69,(char)-37,100,(char)-98,(char)-37,(char)-44,122,126,81,9,(char)-103,(char)-78,(char)-61,(char)-32,127,101,18,22,1,17,9,25,5,10,(char)-83,
    93,(char)-81,(char)-89,45,2,0,64,43,34,(char)-107,66,(char)-122,77,41,(char)-125,10,(char)-121,78,(char)-82,26,121,40,(char)-7,(char)-12,10,24,112,(char)-103,(char)-15,(char)-40,(char)-107,(char)-86,
    85,(char)-90,78,(char)-123,122,(char)-107,26,9,(char)-87,85,75,(char)-105,69,44,(char)-103,72,18,9,22,(char)-71,84,77,(char)-86,40,(char)-92,33,101,(char)-46,(char)-54,(char)-95,(char)-111,(char)-51,
    (char)-88,(char)-112,73,49,11,62,43,1,(char)-101,18,14,(char)-27,26,16,48,96,(char)-128,15,(char)-8,65,8,(char)-90,(char)-57,(char)-93,7,0,(char)-128,78,0,0,(char)-97,1,(char)-128,13,120,61,2,62,127,
    2,108,30,(char)-64,74,65,(char)-100,0,(char)-33,(char)-21,24,57,8,25,122,(char)-103,(char)-77,(char)-15,38,12,(char)-48,83,(char)-31,96,87,29,88,(char)-121,88,(char)-64,70,113,14,(char)-27,0,(char)-71,
    (char)-46,(char)-83,43,7,(char)-41,45,116,30,(char)-47,(char)-63,(char)-56,115,(char)-75,31,14,127,58,(char)-124,(char)-124,(char)-32,(char)-78,(char)-122,12,34,(char)-127,48,(char)-116,(char)-84,50,
    (char)-25,10,(char)-92,(char)-53,(char)-14,72,(char)-67,97,80,(char)-127,66,83,72,(char)-34,74,(char)-64,(char)-64,(char)-93,(char)-122,32,105,25,51,(char)-80,120,(char)-112,73,60,102,84,(char)-61,(char)-128,
    (char)-88,(char)-18,(char)-52,(char)-24,(char)-54,(char)-10,89,54,85,29,(char)-57,64,0,16,68,60,92,121,(char)-121,(char)-54,(char)-82,126,101,55,116,62,(char)-76,(char)-40,24,56,10,(char)-115,(char)-31,
    (char)-87,67,(char)-88,83,11,26,93,(char)-71,34,(char)-115,(char)-105,(char)-53,100,22,(char)-112,81,118,(char)-25,124,(char)-40,(char)-114,(char)-11,122,1,(char)-110,(char)-5,(char)-43,(char)-41,14,
    10,35,86,(char)-111,(char)-52,24,92,19,36,94,(char)-18,59,(char)-93,69,50,119,(char)-42,47,59,(char)-24,93,43,68,28,72,(char)-126,78,(char)-82,88,(char)-23,(char)-103,(char)-58,(char)-25,(char)-10,(char)-80,
    (char)-33,(char)-35,91,(char)-125,(char)-101,24,52,79,(char)-128,67,6,87,108,(char)-29,75,41,(char)-67,(char)-118,(char)-62,106,(char)-54,71,(char)-69,62,(char)-62,23,(char)-41,(char)-97,112,38,15,64,
    89,6,(char)-90,(char)-60,18,50,34,(char)-74,(char)-72,113,(char)-118,74,118,65,(char)-68,(char)-44,(char)-33,11,42,59,104,16,44,40,(char)-18,(char)-93,(char)-21,105,(char)-102,(char)-101,(char)-86,81,
    116,37,65,57,(char)-55,106,(char)-73,115,(char)-98,89,88,(char)-51,(char)-102,(char)-76,(char)-94,(char)-108,(char)-3,(char)-125,(char)-42,82,(char)-81,(char)-31,(char)-79,(char)-44,27,81,55,(char)-30,
    61,(char)-114,(char)-84,58,(char)-85,(char)-120,122,(char)-120,16,(char)-121,(char)-34,(char)-11,3,25,(char)-70,47,(char)-112,(char)-71,115,120,61,66,47,111,(char)-98,(char)-112,26,25,25,25,101,15,103,
    85,87,105,(char)-73,79,70,(char)-30,(char)-69,28,(char)-99,(char)-47,106,29,62,47,(char)-30,34,5,104,108,11,42,122,50,(char)-121,(char)-83,74,(char)-65,(char)-110,74,20,(char)-1,(char)-74,(char)-16,
    98,(char)-22,(char)-60,(char)-34,124,39,7,(char)-63,(char)-20,94,7,(char)-62,(char)-38,98,52,(char)-35,(char)-20,(char)-86,(char)-27,(char)-72,27,(char)-85,(char)-33,(char)-69,37,37,65,32,(char)-84,
    (char)-25,108,86,58,31,(char)-60,106,88,113,(char)-71,39,(char)-112,24,(char)-55,(char)-2,(char)-80,43,(char)-12,55,(char)-127,(char)-9,115,86,(char)-27,(char)-3,(char)-66,(char)-6,(char)-90,50,73,84,
    10,11,(char)-29,19,(char)-45,(char)-116,(char)-4,(char)-100,76,(char)-112,33,113,42,56,(char)-72,(char)-54,11,(char)-92,115,22,(char)-42,(char)-87,22,12,(char)-100,47,(char)-16,57,(char)-110,(char)-76,
    (char)-92,(char)-114,(char)-67,(char)-79,35,(char)-65,(char)-101,(char)-24,62,125,(char)-90,69,30,113,1,87,(char)-80,(char)-113,61,124,(char)-55,(char)-88,42,74,(char)-71,95,37,(char)-111,(char)-107,
    4,(char)-52,20,(char)-91,43,(char)-61,56,20,(char)-57,(char)-118,(char)-32,53,58,66,(char)-59,(char)-8,92,106,53,(char)-10,(char)-85,(char)-63,54,54,(char)-112,37,29,(char)-115,35,(char)-21,11,110,70,
    30,(char)-22,(char)-51,(char)-30,(char)-63,37,12,72,(char)-101,(char)-100,(char)-109,(char)-101,127,73,(char)-94,(char)-40,103,53,5,(char)-85,76,(char)-49,(char)-28,99,49,(char)-22,36,(char)-75,65,(char)-106,
    (char)-77,(char)-122,(char)-57,40,44,(char)-101,92,(char)-90,(char)-106,62,102,86,(char)-46,17,62,(char)-52,85,109,17,10,41,28,61,(char)-17,(char)-114,(char)-109,25,105,11,(char)-83,81,118,86,84,(char)-96,
    (char)-44,(char)-84,(char)-117,(char)-94,(char)-36,(char)-92,28,62,119,(char)-29,68,(char)-55,70,(char)-126,46,28,(char)-8,125,58,82,89,28,41,0,122,95,46,(char)-84,(char)-34,108,39,81,115,34,(char)-3,
    99,107,(char)-21,93,8,74,(char)-82,23,(char)-7,(char)-16,41,(char)-76,52,(char)-112,(char)-38,(char)-11,77,(char)-66,36,125,52,15,(char)-106,90,(char)-47,(char)-50,95,5,73,45,(char)-76,(char)-42,53,
    118,(char)-60,126,(char)-22,79,112,(char)-53,30,(char)-20,74,(char)-102,66,121,(char)-65,32,(char)-80,(char)-123,(char)-108,123,46,79,44,(char)-72,63,(char)-82,23,38,(char)-10,42,44,(char)-39,(char)-68,
    (char)-92,96,(char)-16,(char)-124,(char)-32,120,(char)-118,(char)-88,(char)-51,(char)-47,51,65,(char)-40,124,(char)-16,(char)-63,58,(char)-41,26,57,35,70,(char)-116,89,(char)-105,20,19,105,1,(char)-102,
    75,73,70,73,(char)-79,106,(char)-127,25,(char)-127,54,(char)-86,118,(char)-87,(char)-108,95,101,(char)-118,29,93,(char)-120,(char)-102,25,28,52,(char)-15,123,(char)-70,(char)-56,15,(char)-88,(char)-20,
    25,(char)-125,(char)-50,72,(char)-7,(char)-116,(char)-40,(char)-116,20,87,56,(char)-40,(char)-12,42,9,109,(char)-31,90,(char)-12,(char)-125,(char)-125,117,(char)-33,50,(char)-76,(char)-40,90,6,67,(char)-41,
    39,(char)-2,(char)-72,(char)-35,(char)-30,(char)-86,54,106,(char)-109,(char)-107,50,(char)-104,104,(char)-78,(char)-120,(char)-27,(char)-50,27,23,108,65,(char)-55,(char)-26,54,53,(char)-64,(char)-71,
    73,116,92,28,(char)-108,45,(char)-122,(char)-25,5,(char)-41,119,31,(char)-91,(char)-50,(char)-9,(char)-4,(char)-91,(char)-17,(char)-62,(char)-45,(char)-96,28,47,(char)-82,90,85,(char)-109,(char)-125,
    81,98,11,(char)-123,30,123,25,(char)-20,60,28,(char)-74,15,39,(char)-111,35,82,(char)-40,17,(char)-12,81,26,54,(char)-116,(char)-87,(char)-53,5,(char)-72,7,(char)-111,(char)-104,(char)-25,105,(char)-104,
    (char)-6,(char)-121,(char)-91,(char)-82,(char)-61,121,11,63,(char)-17,121,(char)-57,(char)-104,(char)-52,(char)-52,(char)-52,29,(char)-47,48,35,80,100,(char)-85,(char)-30,86,(char)-77,96,(char)-99,(char)-84,
    34,48,(char)-17,(char)-105,53,17,(char)-74,(char)-16,(char)-7,(char)-36,(char)-11,101,(char)-86,23,(char)-100,(char)-75,(char)-38,(char)-38,90,115,(char)-117,15,(char)-11,100,54,(char)-10,(char)-105,
    (char)-53,85,(char)-18,(char)-67,111,7,51,(char)-1,(char)-42,(char)-65,113,(char)-62,15,(char)-8,(char)-1,(char)-21,(char)-51,26,21,(char)-6,(char)-65,126,(char)-17,(char)-76,(char)-13,23,(char)-42,
    14,(char)-11,(char)-49,55,123,(char)-72,72,101,(char)-69,118,(char)-88,(char)-73,(char)-72,(char)-20,94,5,79,(char)-52,41,(char)-78,(char)-84,(char)-114,76,(char)-8,63,(char)-77,(char)-74,(char)-77,
    (char)-42,(char)-112,(char)-96,84,(char)-92,9,(char)-110,(char)-13,(char)-83,(char)-110,8,83,(char)-81,82,(char)-100,(char)-103,17,(char)-55,89,0,60,(char)-1,82,107,(char)-70,80,(char)-111,(char)-30,
    36,(char)-124,108,23,(char)-95,48,(char)-89,(char)-13,121,116,122,(char)-123,36,(char)-71,(char)-74,36,(char)-29,(char)-39,24,(char)-74,109,(char)-31,(char)-66,52,(char)-98,(char)-70,(char)-96,108,(char)-65,
    (char)-85,83,(char)-94,(char)-1,58,101,100,97,59,(char)-37,(char)-101,(char)-29,(char)-49,(char)-82,86,(char)-54,(char)-95,(char)-23,23,(char)-118,(char)-121,(char)-36,(char)-18,44,(char)-113,127,(char)-67,
    (char)-87,(char)-75,94,77,17,(char)-43,(char)-124,(char)-84,50,(char)-26,(char)-37,18,83,108,43,5,57,(char)-2,2,115,122,(char)-91,34,(char)-39,41,36,(char)-110,93,109,(char)-58,(char)-87,2,62,(char)-56,
    69,8,(char)-109,(char)-99,13,(char)-8,50,(char)-87,(char)-66,(char)-122,86,(char)-47,13,93,122,(char)-31,(char)-80,(char)-31,19,6,(char)-27,25,124,(char)-123,(char)-6,108,83,(char)-94,70,54,90,(char)-74,
    (char)-12,(char)-61,(char)-5,6,(char)-13,(char)-105,(char)-102,(char)-21,(char)-24,16,0,63,(char)-107,(char)-65,113,(char)-37,(char)-41,(char)-14,(char)-7,120,(char)-33,(char)-71,(char)-8,54,(char)-6,
    54,81,(char)-17,(char)-125,125,(char)-30,24,105,(char)-1,75,71,(char)-22,110,(char)-39,73,(char)-96,(char)-91,(char)-57,120,(char)-57,(char)-92,39,(char)-74,(char)-69,(char)-127,28,127,81,45,(char)-73,
    (char)-94,54,(char)-2,21,(char)-77,41,126,30,49,78,21,(char)-15,65,(char)-13,76,(char)-26,(char)-8,87,(char)-10,86,(char)-91,(char)-52,94,81,27,66,(char)-13,(char)-35,(char)-68,16,83,(char)-19,(char)-118,
    (char)-20,77,4,19,(char)-49,(char)-51,15,(char)-95,55,15,120,(char)-40,(char)-1,(char)-5,27,93,103,(char)-23,109,(char)-56,(char)-68,(char)-83,(char)-23,(char)-84,(char)-121,59,16,6,5,(char)-2,5,(char)-86,
    120,110,(char)-68,(char)-124,19,(char)-65,(char)-17,(char)-71,122,(char)-15,(char)-54,(char)-27,102,70,(char)-118,(char)-72,111,110,103,(char)-104,(char)-32,(char)-117,(char)-46,12,69,(char)-30,(char)-49,
    (char)-94,(char)-93,(char)-83,(char)-103,(char)-126,28,69,116,(char)-62,(char)-27,(char)-52,(char)-88,103,29,77,(char)-30,53,(char)-122,66,(char)-38,99,114,(char)-20,116,116,(char)-71,(char)-72,(char)-86,
    50,75,(char)-61,(char)-34,(char)-60,94,(char)-34,(char)-57,(char)-123,37,(char)-2,101,(char)-92,90,(char)-91,(char)-77,(char)-21,44,85,124,(char)-23,(char)-98,(char)-116,(char)-116,(char)-61,(char)-97,
    (char)-120,6,(char)-10,106,(char)-79,(char)-92,(char)-68,47,(char)-103,(char)-116,114,17,59,83,87,51,60,36,(char)-64,99,(char)-85,(char)-71,(char)-74,(char)-126,(char)-16,127,(char)-37,(char)-125,(char)-87,
    77,(char)-61,71,92,(char)-23,(char)-55,(char)-59,100,(char)-123,(char)-100,(char)-15,(char)-115,68,16,24,(char)-61,125,(char)-6,76,36,(char)-51,43,121,51,63,56,(char)-21,49,119,119,126,(char)-95,76,
    (char)-55,(char)-66,(char)-58,86,(char)-30,74,(char)-32,90,100,(char)-72,(char)-106,(char)-41,(char)-106,(char)-40,(char)-30,(char)-22,99,73,(char)-25,69,(char)-103,20,1,(char)-44,88,(char)-47,(char)-52,
    (char)-128,(char)-110,(char)-73,54,84,127,43,(char)-3,(char)-2,(char)-72,78,126,96,118,(char)-117,124,(char)-53,(char)-125,3,(char)-14,123,73,(char)-49,(char)-76,(char)-10,(char)-51,(char)-21,(char)-9,
    (char)-28,101,(char)-115,90,(char)-98,(char)-62,11,49,(char)-11,(char)-4,(char)-97,31,57,(char)-66,(char)-124,(char)-1,(char)-99,(char)-100,21,101,30,(char)-78,(char)-41,74,(char)-21,11,43,28,113,47,
    (char)-45,71,6,47,63,(char)-89,(char)-52,(char)-34,(char)-95,14,(char)-91,(char)-7,126,(char)-91,126,(char)-4,(char)-26,75,(char)-91,(char)-34,1,94,(char)-45,86,(char)-47,52,85,(char)-6,(char)-50,(char)-99,
    33,100,(char)-3,88,112,73,35,(char)-4,(char)-54,(char)-6,(char)-75,(char)-22,107,107,(char)-43,87,(char)-22,(char)-110,75,99,93,60,(char)-76,(char)-9,(char)-117,(char)-110,119,(char)-12,85,(char)-45,
    (char)-42,25,21,68,(char)-7,83,(char)-19,(char)-59,(char)-63,(char)-57,(char)-121,(char)-76,(char)-100,50,(char)-27,(char)-29,(char)-76,(char)-96,76,14,119,(char)-79,(char)-12,5,39,25,(char)-112,(char)-46,
    46,(char)-85,84,102,(char)-70,(char)-122,(char)-40,(char)-82,18,125,91,51,41,(char)-26,70,(char)-65,(char)-125,74,(char)-22,11,(char)-43,110,51,(char)-44,51,40,111,(char)-17,(char)-105,16,(char)-66,
    72,(char)-115,(char)-24,46,(char)-63,(char)-58,(char)-65,(char)-103,87,20,73,37,(char)-76,89,71,(char)-99,(char)-93,(char)-114,115,(char)-21,(char)-120,74,85,18,(char)-21,3,10,81,(char)-64,103,80,23,
    45,(char)-70,90,(char)-117,86,103,51,(char)-43,28,(char)-40,44,(char)-51,75,(char)-27,100,115,(char)-40,121,(char)-102,73,(char)-11,85,(char)-77,(char)-105,22,39,(char)-28,45,(char)-124,(char)-39,(char)-24,
    51,(char)-30,(char)-63,(char)-117,(char)-99,76,(char)-39,(char)-20,4,(char)-119,27,108,(char)-59,114,(char)-98,(char)-127,39,(char)-73,23,(char)-21,85,(char)-40,(char)-69,31,(char)-53,(char)-104,59,
    (char)-53,(char)-62,(char)-1,29,49,(char)-8,88,(char)-102,9,(char)-71,(char)-48,0,49,(char)-2,(char)-26,65,57,61,73,54,119,(char)-54,(char)-11,98,34,(char)-78,88,46,82,57,73,(char)-57,124,119,36,87,
    82,38,41,39,(char)-77,6,(char)-90,121,(char)-19,37,(char)-70,(char)-34,94,23,77,(char)-86,117,11,(char)-104,(char)-65,(char)-104,70,(char)-26,100,(char)-26,(char)-77,52,95,88,19,(char)-95,102,(char)-100,
    110,12,124,(char)-101,(char)-103,(char)-112,69,77,(char)-64,51,9,(char)-117,(char)-108,(char)-19,108,(char)-37,76,(char)-108,125,81,53,(char)-89,15,79,(char)-17,29,83,(char)-102,127,(char)-79,29,29,
    (char)-6,117,94,66,37,28,14,99,30,82,(char)-8,103,11,77,(char)-119,68,(char)-108,(char)-93,(char)-22,13,(char)-6,13,110,(char)-96,(char)-60,(char)-57,(char)-55,(char)-55,19,(char)-7,63,(char)-84,(char)-93,
    39,104,40,24,55,(char)-60,40,(char)-22,(char)-101,(char)-69,90,90,(char)-102,(char)-69,(char)-22,55,(char)-84,87,22,(char)-83,41,(char)-29,(char)-107,(char)-83,41,82,(char)-124,24,(char)-5,13,19,6,(char)-40,
    (char)-16,67,(char)-113,(char)-80,(char)-25,15,(char)-73,(char)-116,(char)-67,123,82,56,57,69,(char)-127,(char)-28,37,(char)-6,(char)-89,(char)-28,(char)-68,38,25,125,(char)-109,(char)-124,117,(char)-71,
    10,(char)-39,(char)-99,(char)-97,110,(char)-33,(char)-74,105,(char)-25,72,97,(char)-92,108,110,51,59,31,(char)-126,(char)-13,35,(char)-76,72,51,(char)-95,(char)-4,(char)-8,(char)-92,(char)-51,19,124,
    (char)-92,(char)-128,27,(char)-66,34,100,41,124,71,(char)-19,83,(char)-42,96,(char)-76,120,(char)-65,72,66,(char)-115,(char)-12,(char)-116,(char)-29,(char)-116,(char)-20,(char)-126,(char)-84,117,45,
    (char)-32,3,103,124,(char)-128,(char)-13,112,(char)-17,52,(char)-123,(char)-88,(char)-81,122,23,125,105,124,(char)-26,(char)-70,(char)-17,(char)-102,8,84,116,102,(char)-58,(char)-72,82,(char)-12,17,
    9,(char)-33,(char)-3,33,48,(char)-87,32,24,22,14,115,94,54,(char)-13,24,121,(char)-43,(char)-17,112,(char)-65,(char)-42,(char)-83,44,(char)-105,(char)-107,(char)-57,117,21,(char)-37,(char)-9,115,(char)-122,
    (char)-124,(char)-80,(char)-29,(char)-22,58,(char)-39,(char)-17,95,53,62,79,126,(char)-125,(char)-109,51,95,(char)-8,(char)-1,(char)-15,(char)-75,106,(char)-39,(char)-111,62,99,51,110,111,(char)-100,
    (char)-90,(char)-64,(char)-56,(char)-96,(char)-66,(char)-102,(char)-3,(char)-118,(char)-70,102,(char)-6,(char)-120,92,(char)-30,(char)-104,(char)-98,(char)-43,16,(char)-2,71,(char)-26,(char)-20,87,76,
    (char)-39,(char)-39,107,(char)-44,(char)-12,(char)-39,(char)-23,(char)-57,(char)-82,68,(char)-81,108,47,(char)-118,(char)-110,(char)-84,44,(char)-59,(char)-20,(char)-54,(char)-79,(char)-66,(char)-11,
    (char)-2,(char)-83,(char)-53,(char)-5,59,95,(char)-26,10,(char)-123,41,(char)-67,68,93,39,(char)-90,103,(char)-65,20,(char)-27,112,83,91,109,89,108,43,59,(char)-53,54,(char)-100,(char)-58,(char)-109,
    22,(char)-108,(char)-51,35,116,85,95,(char)-66,72,2,(char)-70,(char)-50,(char)-48,(char)-109,111,(char)-41,76,(char)-95,(char)-86,(char)-44,100,23,(char)-1,(char)-78,111,81,112,(char)-89,(char)-36,60,
    90,(char)-82,71,(char)-91,122,(char)-29,(char)-88,(char)-29,42,115,(char)-30,113,(char)-93,(char)-74,(char)-101,48,(char)-95,10,(char)-109,(char)-90,(char)-37,113,(char)-23,(char)-47,(char)-14,87,77,
    31,(char)-111,48,(char)-21,(char)-8,(char)-121,121,69,(char)-98,84,(char)-102,(char)-19,124,93,91,(char)-124,(char)-115,127,49,35,(char)-66,125,(char)-59,33,52,31,(char)-37,46,105,110,(char)-83,67,43,
    (char)-124,43,(char)-42,16,(char)-15,15,(char)-123,(char)-33,(char)-74,118,121,(char)-10,124,(char)-78,(char)-8,(char)-89,55,68,111,80,52,(char)-27,18,(char)-63,(char)-84,125,119,83,124,(char)-75,(char)-40,
    (char)-102,87,66,(char)-127,36,84,(char)-17,(char)-27,(char)-99,27,(char)-99,(char)-88,(char)-3,102,(char)-42,81,107,23,(char)-3,12,20,(char)-118,(char)-102,9,(char)-87,(char)-48,32,(char)-108,(char)-34,
    95,(char)-120,116,42,(char)-46,(char)-113,99,(char)-59,85,(char)-11,(char)-108,26,110,(char)-6,42,(char)-68,93,(char)-30,32,117,13,77,29,61,117,126,75,(char)-76,(char)-16,115,(char)-54,117,102,(char)-41,
    (char)-51,(char)-71,66,122,(char)-126,(char)-26,(char)-51,93,93,108,123,57,28,(char)-1,38,(char)-116,73,(char)-1,16,99,123,91,63,(char)-91,(char)-123,55,(char)-119,82,(char)-109,10,(char)-91,(char)-101,
    36,(char)-101,10,(char)-106,(char)-102,(char)-7,62,27,(char)-10,(char)-29,(char)-29,(char)-35,(char)-18,(char)-116,(char)-7,(char)-23,(char)-31,43,(char)-12,(char)-101,98,(char)-78,42,(char)-23,19,(char)-81,
    (char)-106,(char)-101,5,(char)-86,33,(char)-20,(char)-20,(char)-33,65,(char)-34,(char)-30,(char)-75,(char)-52,55,(char)-35,(char)-46,104,16,(char)-29,61,25,(char)-109,(char)-2,(char)-64,91,(char)-1,
    (char)-114,(char)-38,75,(char)-30,(char)-81,9,(char)-11,127,(char)-75,8,105,(char)-104,53,16,15,(char)-119,62,(char)-1,(char)-108,30,(char)-14,(char)-24,(char)-89,(char)-44,83,90,(char)-32,(char)-114,
    (char)-45,(char)-68,115,(char)-68,96,(char)-14,(char)-30,83,52,113,(char)-114,(char)-96,(char)-97,77,(char)-110,99,5,(char)-79,47,32,27,(char)-12,43,5,33,47,43,(char)-89,11,(char)-15,62,(char)-35,(char)-127,
    (char)-19,(char)-25,118,30,127,122,(char)-4,121,(char)-82,60,54,62,(char)-22,(char)-35,106,(char)-42,(char)-68,(char)-47,(char)-96,73,(char)-106,(char)-111,122,(char)-90,(char)-61,(char)-35,(char)-47,
    (char)-72,(char)-38,70,(char)-42,100,(char)-48,(char)-24,(char)-68,106,(char)-42,(char)-8,(char)-75,13,79,(char)-45,75,(char)-75,16,34,48,46,(char)-87,93,(char)-113,(char)-22,(char)-41,(char)-101,(char)-105,
    56,126,113,125,97,106,(char)-96,(char)-26,(char)-103,(char)-105,32,(char)-45,18,(char)-29,60,(char)-57,(char)-105,(char)-12,(char)-55,120,63,(char)-68,(char)-63,(char)-84,33,(char)-100,57,71,(char)-22,
    (char)-97,71,24,31,31,106,72,50,(char)-71,(char)-33,(char)-66,91,(char)-73,(char)-100,(char)-58,(char)-98,71,6,21,(char)-21,(char)-97,112,107,(char)-91,100,43,102,46,(char)-11,(char)-5,1,(char)-55,(char)-127,
    (char)-97,(char)-30,22,62,118,48,7,(char)-115,(char)-86,115,38,23,75,(char)-64,50,126,(char)-55,30,(char)-13,110,(char)-98,98,(char)-10,11,(char)-37,(char)-81,(char)-59,(char)-56,98,(char)-32,(char)-125,
    (char)-114,(char)-28,(char)-115,17,65,17,27,(char)-109,(char)-101,(char)-13,110,102,(char)-73,1,63,(char)-109,(char)-73,(char)-23,(char)-125,50,22,127,27,(char)-20,30,114,12,(char)-59,(char)-113,(char)-98,
    (char)-69,80,29,(char)-107,(char)-39,(char)-79,123,54,32,96,118,119,(char)-55,(char)-72,(char)-86,23,(char)-98,123,4,96,116,(char)-71,92,(char)-3,(char)-128,(char)-101,17,(char)-27,84,110,78,(char)-57,
    (char)-93,(char)-41,(char)-79,12,64,0,(char)-32,5,(char)-82,(char)-1,(char)-111,(char)-76,100,121,65,48,104,44,76,5,7,26,62,(char)-101,11,104,(char)-117,26,(char)-95,23,111,(char)-88,(char)-111,48,(char)-39,
    99,(char)-22,52,(char)-68,93,36,123,65,(char)-111,53,(char)-11,49,59,113,114,23,109,54,71,(char)-115,96,(char)-114,118,(char)-124,94,45,12,(char)-111,82,95,115,27,(char)-121,29,(char)-128,(char)-95,
    26,(char)-117,(char)-97,(char)-62,68,115,(char)-116,(char)-59,(char)-80,24,14,52,54,(char)-86,109,(char)-42,(char)-102,(char)-6,(char)-21,113,(char)-24,64,(char)-38,(char)-86,6,(char)-128,8,(char)-108,
    54,(char)-57,(char)-117,(char)-122,(char)-57,59,16,(char)-37,(char)-111,6,(char)-86,9,40,(char)-34,52,39,(char)-70,19,(char)-62,(char)-114,27,0,24,91,(char)-71,(char)-106,(char)-52,63,28,(char)-111,
    51,(char)-94,76,0,0,72,(char)-2,45,60,35,(char)-97,(char)-75,(char)-3,26,(char)-128,95,(char)-96,34,33,(char)-92,16,(char)-55,23,38,1,(char)-68,92,38,(char)-93,4,2,(char)-31,(char)-60,(char)-120,(char)-15,
    (char)-27,13,0,(char)-69,95,(char)-32,(char)-92,27,(char)-120,66,43,78,26,(char)-126,(char)-1,(char)-61,82,51,105,(char)-7,(char)-84,81,(char)-13,3,10,96,127,66,(char)-95,0,(char)-32,108,16,(char)-100,
    57,(char)-40,(char)-93,(char)-4,58,(char)-82,100,(char)-113,(char)-55,(char)-29,105,(char)-49,22,110,(char)-121,68,4,114,51,(char)-63,14,62,60,(char)-63,(char)-63,44,(char)-109,81,2,(char)-32,(char)-40,
    (char)-125,47,(char)-81,(char)-47,(char)-97,123,34,37,(char)-41,(char)-98,109,39,118,41,(char)-107,17,(char)-106,12,(char)-103,121,67,46,64,(char)-111,(char)-34,(char)-118,(char)-61,(char)-98,(char)-59,
    51,(char)-79,44,(char)-84,(char)-50,(char)-124,74,0,40,8,(char)-41,(char)-52,(char)-79,(char)-113,(char)-62,96,127,59,(char)-59,(char)-29,115,96,105,67,95,(char)-7,124,124,25,(char)-45,0,(char)-32,(char)-101,
    77,(char)-13,34,1,(char)-64,119,(char)-45,(char)-1,(char)-9,63,121,(char)-28,97,(char)-38,(char)-126,(char)-66,5,(char)-64,23,112,0,0,(char)-64,0,16,(char)-10,(char)-28,2,(char)-36,42,98,(char)-60,96,
    37,48,(char)-88,52,102,(char)-32,86,44,0,(char)-48,38,112,47,75,72,57,73,(char)-16,37,(char)-77,(char)-93,(char)-107,10,21,(char)-49,15,(char)-92,34,(char)-108,74,66,104,64,17,(char)-64,(char)-127,(char)-125,
    (char)-110,(char)-85,18,(char)-105,(char)-124,(char)-124,31,(char)-28,(char)-22,37,(char)-107,36,73,20,2,(char)-127,(char)-46,(char)-96,(char)-127,(char)-117,(char)-123,63,(char)-48,4,36,123,15,26,31,
    69,66,84,(char)-69,4,(char)-89,61,36,23,99,71,40,102,101,(char)-121,(char)-5,121,(char)-18,1,(char)-57,(char)-115,70,21,42,85,(char)-62,(char)-37,(char)-55,(char)-123,74,(char)-63,67,33,23,66,45,(char)-109,
    43,(char)-15,(char)-25,(char)-78,43,(char)-85,82,59,(char)-84,115,(char)-38,43,5,98,(char)-97,(char)-104,(char)-94,(char)-23,65,(char)-128,94,(char)-20,(char)-74,0,(char)-4,0,112,(char)-64,(char)-50,
    96,80,(char)-110,15,(char)-76,50,(char)-83,(char)-112,47,0,(char)-40,61,(char)-96,80,12,(char)-30,(char)-4,24,(char)-118,67,16,124,18,(char)-54,(char)-128,82,(char)-101,66,17,100,(char)-21,15,101,66,
    20,123,(char)-88,23,100,(char)-32,65,16,20,2,(char)-76,27,(char)-84,5,(char)-10,43,87,80,121,(char)-128,(char)-117,91,7,15,(char)-125,(char)-123,91,(char)-66,1,(char)-24,110,67,(char)-97,22,(char)-38,
    100,91,(char)-82,(char)-95,(char)-96,65,(char)-110,(char)-48,(char)-94,4,(char)-123,45,70,105,15,(char)-48,91,111,(char)-100,(char)-57,(char)-96,69,(char)-101,33,125,97,19,55,51,(char)-94,(char)-37,
    17,63,(char)-42,102,41,(char)-110,(char)-81,(char)-48,(char)-123,(char)-63,34,(char)-125,69,83,97,(char)-39,(char)-20,(char)-116,10,90,28,21,(char)-29,119,102,123,58,46,72,49,(char)-71,(char)-33,(char)-40,
    (char)-70,(char)-79,(char)-51,31,124,(char)-82,(char)-124,11,37,(char)-116,(char)-35,(char)-98,(char)-117,(char)-63,77,(char)-112,(char)-49,21,(char)-118,117,(char)-21,(char)-58,14,(char)-106,56,(char)-64,
    66,75,(char)-101,(char)-76,(char)-64,(char)-80,(char)-126,102,(char)-117,104,28,42,122,(char)-82,103,80,37,0,12,(char)-10,78,(char)-4,47,24,(char)-122,3,3,100,(char)-108,(char)-44,12,28,(char)-46,112,
    (char)-28,42,82,89,12,120,(char)-58,14,40,43,48,(char)-107,(char)-90,27,(char)-90,101,59,(char)-82,(char)-25,11,(char)-94,36,43,(char)-86,(char)-90,27,(char)-90,101,59,(char)-82,(char)-25,7,97,20,39,
    105,(char)-106,23,101,85,55,109,(char)-41,15,(char)-29,52,47,(char)-21,(char)-74,31,(char)-25,117,63,(char)-17,(char)-9,19,(char)-109,(char)-112,(char)-110,(char)-111,83,(char)-88,(char)-90,68,82,81,
    (char)-45,(char)-48,(char)-46,(char)-87,65,(char)-47,51,48,(char)-94,(char)-13,77,(char)-106,101,74,98,102,97,(char)-91,(char)-75,(char)-51,123,(char)-88,(char)-73,(char)-125,(char)-69,65,67,99,(char)-47,
    70,(char)-21,(char)-60,(char)-35,(char)-78,(char)-20,55,(char)-29,(char)-122,29,113,65,68,65,(char)-25,(char)-71,32,(char)-62,34,(char)-24,(char)-116,(char)-8,(char)-80,(char)-92,96,25,48,(char)-19,
    (char)-77,76,(char)-44,(char)-15,(char)-92,(char)-65,(char)-83,33,(char)-110,108,38,(char)-11,(char)-108,(char)-51,14,52,8,(char)-110,(char)-87,(char)-26,(char)-22,(char)-36,116,(char)-57,85,67,(char)-50,
    10,109,(char)-46,59,(char)-37,31,(char)-90,12,101,116,91,(char)-73,(char)-118,103,126,61,(char)-103,(char)-50,38,(char)-93,8,(char)-47,44,83,(char)-55,47,21,(char)-93,90,(char)-18,23,(char)-39,(char)-80,
    92,73,118,101,85,55,12,(char)-21,14,112,(char)-2,75,87,71,(char)-2,(char)-49,(char)-116,(char)-51,(char)-65,(char)-25,(char)-65,36,95,126,(char)-39,(char)-36,55,(char)-80,(char)-71,(char)-93,(char)-27,
    15,69,(char)-83,(char)-20,21,(char)-6,110,(char)-11,108,107,114,15,(char)-11,116,55,13,13,106,94,(char)-63,(char)-35,(char)-113,(char)-122,3,0,0 };
    static constexpr const char* assets_ibmplexmono_v12_ibmplexmono_css =
        R"(/* cyrillic-ext */
@font-face {
  font-family: 'IBM Plex Mono';
  font-style: normal;
  font-weight: 400;
  font-display: swap;
  src: url(./-F63fjptAgt5VM-kVkqdyU8n1iIq131nj-otFQ.woff2) format('woff2');
  unicode-range: U+0460-052F, U+1C80-1C88, U+20B4, U+2DE0-2DFF, U+A640-A69F, U+FE2E-FE2F;
}
/* cyrillic */
@font-face {
  font-family: 'IBM Plex Mono';
  font-style: normal;
  font-weight: 400;
  font-display: swap;
  src: url(./-F63fjptAgt5VM-kVkqdyU8n1isq131nj-otFQ.woff2) format('woff2');
  unicode-range: U+0301, U+0400-045F, U+0490-0491, U+04B0-04B1, U+2116;
}
/* vietnamese */
@font-face {
  font-family: 'IBM Plex Mono';
  font-style: normal;
  font-weight: 400;
  font-display: swap;
  src: url(./-F63fjptAgt5VM-kVkqdyU8n1iAq131nj-otFQ.woff2) format('woff2');
  unicode-range: U+0102-0103, U+0110-0111, U+0128-0129, U+0168-0169, U+01A0-01A1, U+01AF-01B0, U+1EA0-1EF9, U+20AB;
}
/* latin-ext */
@font-face {
  font-family: 'IBM Plex Mono';
  font-style: normal;
  font-weight: 400;
  font-display: swap;
  src: url(./-F63fjptAgt5VM-kVkqdyU8n1iEq131nj-otFQ.woff2) format('woff2');
  unicode-range: U+0100-024F, U+0259, U+1E00-1EFF, U+2020, U+20A0-20AB, U+20AD-20CF, U+2113, U+2C60-2C7F, U+A720-A7FF;
}
/* latin */
@font-face {
  font-family: 'IBM Plex Mono';
  font-style: normal;
  font-weight: 400;
  font-display: swap;
  src: url(./-F63fjptAgt5VM-kVkqdyU8n1i8q131nj-o.woff2) format('woff2');
  unicode-range: U+0000-00FF, U+0131, U+0152-0153, U+02BB-02BC, U+02C6, U+02DA, U+02DC, U+2000-206F, U+2074, U+20AC, U+2122, U+2191, U+2193, U+2212, U+2215, U+FEFF, U+FFFD;
})";
    static constexpr const char assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1isq131njotFQ_woff2[] = {
        119,79,70,50,0,1,0,0,0,0,20,(char)-52,0,14,0,0,0,0,44,28,0,0,20,117,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,82,27,30,28,46,6,96,0,(char)-127,68,17,16,10,(char)-64,64,(char)-78,77,11,(char)-126,
    22,0,1,54,2,36,3,(char)-126,26,4,32,5,(char)-125,58,7,(char)-119,42,27,22,35,85,70,(char)-122,(char)-115,3,64,(char)-77,(char)-3,(char)-119,35,(char)-86,53,(char)-117,2,(char)-2,(char)-1,(char)-106,
    (char)-36,24,67,123,64,(char)-83,27,(char)-116,114,21,(char)-86,61,(char)-69,69,88,(char)-24,(char)-93,114,(char)-9,119,103,80,(char)-72,59,(char)-88,(char)-94,8,(char)-105,(char)-1,80,102,77,108,(char)-81,
    28,(char)-20,(char)-116,(char)-21,39,(char)-16,89,(char)-59,29,(char)-7,(char)-124,(char)-103,16,77,(char)-100,(char)-64,7,27,62,101,16,14,(char)-124,(char)-87,(char)-92,5,37,42,59,49,(char)-92,(char)-56,
    (char)-126,9,(char)-91,67,37,(char)-125,59,(char)-55,45,99,71,47,127,57,(char)-33,(char)-16,(char)-27,(char)-1,100,122,(char)-51,8,73,102,33,(char)-86,(char)-3,(char)-34,122,102,39,(char)-124,(char)-96,
    (char)-128,81,71,(char)-89,124,(char)-22,(char)-55,(char)-44,23,38,(char)-62,(char)-57,2,41,(char)-55,(char)-106,21,(char)-16,(char)-35,14,79,(char)-37,(char)-4,119,120,24,(char)-119,(char)-123,54,101,
    96,(char)-109,41,(char)-57,(char)-55,(char)-111,7,28,(char)-19,64,68,76,12,92,(char)-74,46,(char)-36,47,23,(char)-99,44,(char)-35,126,70,(char)-71,77,92,(char)-18,41,(char)-1,(char)-75,102,97,23,77,
    90,(char)-104,(char)-32,(char)-40,36,(char)-109,99,(char)-118,(char)-30,64,(char)-58,(char)-122,(char)-59,24,(char)-58,85,(char)-94,120,(char)-4,(char)-81,(char)-71,127,(char)-17,93,48,(char)-25,38,
    125,70,(char)-41,1,83,(char)-110,13,(char)-4,(char)-2,(char)-3,(char)-100,78,(char)-97,(char)-112,(char)-91,100,(char)-22,54,51,117,4,(char)-96,62,85,3,11,87,(char)-31,58,56,47,(char)-105,65,7,(char)-72,
    (char)-117,(char)-87,(char)-118,(char)-88,118,(char)-5,51,1,63,66,28,104,(char)-108,(char)-122,17,4,104,(char)-9,(char)-61,(char)-2,108,(char)-8,124,26,(char)-20,(char)-112,112,114,(char)-126,20,10,
    24,(char)-29,(char)-75,(char)-60,80,(char)-122,(char)-80,13,116,2,(char)-39,121,(char)-9,(char)-50,125,(char)-91,118,(char)-26,(char)-83,38,68,97,(char)-106,(char)-113,29,(char)-96,63,(char)-29,63,18,
    93,(char)-102,38,61,(char)-114,(char)-1,(char)-43,(char)-102,(char)-34,(char)-4,(char)-122,55,(char)-64,69,9,(char)-74,(char)-118,56,97,85,68,(char)-59,40,(char)-65,(char)-118,(char)-64,88,(char)-94,
    (char)-3,20,(char)-57,71,23,(char)-104,(char)-1,126,46,(char)-77,121,(char)-5,(char)-37,30,111,65,(char)-75,(char)-114,64,29,8,5,36,(char)-27,(char)-49,38,11,(char)-17,(char)-1,(char)-108,(char)-110,
    2,65,(char)-54,(char)-39,98,(char)-74,61,4,117,(char)-89,8,20,(char)-114,39,96,13,0,(char)-14,(char)-28,(char)-71,59,119,66,41,64,(char)-94,(char)-14,(char)-108,(char)-32,109,40,15,42,63,(char)-109,
    (char)-120,(char)-59,42,82,(char)-85,104,117,(char)-54,48,12,47,(char)-69,(char)-50,(char)-60,(char)-74,12,124,(char)-120,(char)-74,(char)-92,(char)-47,(char)-117,99,102,0,101,(char)-123,(char)-24,(char)-2,
    (char)-113,(char)-127,0,(char)-120,5,42,(char)-108,12,24,(char)-25,(char)-112,16,85,106,64,(char)-80,(char)-3,33,(char)-124,(char)-2,16,82,63,8,(char)-83,(char)-8,(char)-8,35,80,52,(char)-104,63,(char)-124,
    (char)-35,31,(char)-62,(char)-27,15,(char)-31,3,40,4,16,77,(char)-7,(char)-18,(char)-96,(char)-127,(char)-95,(char)-103,21,75,42,48,0,124,(char)-64,(char)-114,(char)-127,(char)-115,(char)-125,39,(char)-63,
    (char)-57,23,(char)-97,(char)-7,(char)-73,(char)-25,(char)-106,(char)-126,27,(char)-66,(char)-65,(char)-76,20,0,64,(char)-79,119,(char)-45,21,(char)-7,32,3,52,(char)-92,57,(char)-128,(char)-15,(char)-10,
    46,101,11,69,9,1,26,19,22,21,(char)-90,14,(char)-24,4,84,0,68,114,(char)-71,66,(char)-93,115,54,(char)-55,26,64,81,(char)-69,76,(char)-81,104,52,72,82,10,(char)-127,(char)-10,1,(char)-96,(char)-90,23,
    0,10,37,(char)-23,(char)-51,65,9,75,(char)-107,4,(char)-28,15,115,54,35,29,37,11,(char)-55,123,(char)-113,(char)-125,62,114,(char)-84,(char)-63,108,38,(char)-19,(char)-73,23,69,45,(char)-97,52,(char)-73,
    37,0,39,26,(char)-57,112,(char)-5,(char)-20,(char)-7,(char)-43,(char)-42,(char)-40,(char)-1,(char)-4,103,(char)-128,63,(char)-17,(char)-65,31,101,41,(char)-66,(char)-77,(char)-96,(char)-98,70,98,(char)-87,
    (char)-86,102,96,(char)-124,(char)-37,117,(char)-57,76,(char)-53,24,49,(char)-53,66,(char)-117,45,(char)-79,(char)-44,92,(char)-13,44,103,(char)-114,(char)-39,(char)-26,91,(char)-92,67,(char)-40,(char)-112,
    (char)-96,78,125,6,(char)-60,12,(char)-118,(char)-22,54,(char)-84,75,68,(char)-113,126,113,33,11,(char)-12,(char)-126,72,34,(char)-107,116,40,(char)-56,(char)-2,(char)-81,7,77,67,0,(char)-112,84,(char)-40,
    107,84,43,32,(char)-26,16,32,51,0,89,0,104,110,111,35,(char)-124,(char)-34,(char)-92,44,8,104,76,73,(char)-22,57,(char)-63,(char)-101,100,(char)-127,83,(char)-4,31,11,73,67,2,(char)-87,(char)-19,60,
    40,(char)-54,85,(char)-91,(char)-117,(char)-119,(char)-81,124,62,(char)-119,82,(char)-60,94,120,112,(char)-1,(char)-54,(char)-44,30,(char)-33,67,96,(char)-25,(char)-93,115,(char)-44,(char)-125,(char)-53,
    (char)-124,(char)-31,(char)-55,124,(char)-89,(char)-128,(char)-38,23,(char)-45,(char)-84,24,43,(char)-103,56,11,(char)-96,120,(char)-59,(char)-27,(char)-18,(char)-70,(char)-35,(char)-122,(char)-30,6,
    (char)-56,16,(char)-111,(char)-33,(char)-27,(char)-11,35,84,13,(char)-73,33,(char)-94,(char)-92,32,89,114,25,0,52,(char)-121,(char)-98,(char)-86,72,(char)-56,(char)-25,34,(char)-61,112,(char)-105,(char)-35,
    (char)-28,35,39,3,(char)-126,(char)-126,80,(char)-120,(char)-8,3,(char)-64,(char)-75,111,(char)-79,(char)-18,10,76,87,(char)-8,(char)-98,11,(char)-123,(char)-118,(char)-50,40,(char)-60,(char)-63,3,(char)-99,
    (char)-39,121,(char)-93,(char)-46,105,28,15,(char)-119,40,(char)-118,(char)-69,113,(char)-13,80,119,104,(char)-69,108,(char)-119,63,83,113,35,(char)-125,(char)-57,20,2,(char)-29,90,1,(char)-123,(char)-37,
    30,4,(char)-70,(char)-78,71,(char)-98,(char)-82,(char)-41,(char)-79,(char)-29,(char)-119,10,(char)-115,116,(char)-27,(char)-8,36,(char)-57,8,(char)-108,(char)-48,52,(char)-65,20,51,86,(char)-23,75,44,
    (char)-7,(char)-60,(char)-101,(char)-69,84,12,66,37,49,69,104,(char)-68,(char)-55,42,118,(char)-62,(char)-6,110,15,69,(char)-93,99,62,66,(char)-124,(char)-95,(char)-68,(char)-47,124,(char)-59,49,(char)-73,
    (char)-66,84,(char)-19,71,(char)-80,113,(char)-36,(char)-38,(char)-79,(char)-77,18,26,(char)-53,87,(char)-47,(char)-32,88,9,69,(char)-87,(char)-40,(char)-120,99,(char)-45,80,(char)-64,(char)-47,(char)-46,
    35,127,122,(char)-123,80,(char)-6,(char)-44,9,10,19,(char)-78,(char)-110,(char)-1,36,36,8,(char)-30,(char)-39,18,(char)-62,19,20,(char)-86,39,(char)-9,64,(char)-60,65,104,(char)-100,(char)-47,(char)-96,
    87,(char)-100,(char)-19,77,(char)-14,(char)-72,(char)-71,(char)-111,86,102,50,(char)-51,101,(char)-90,82,45,39,85,127,(char)-101,(char)-74,(char)-63,(char)-23,62,(char)-111,45,86,98,(char)-72,(char)-119,
    64,106,12,(char)-50,(char)-90,(char)-58,98,53,123,(char)-100,41,86,(char)-103,(char)-92,(char)-87,56,39,73,(char)-127,44,67,(char)-110,(char)-12,101,(char)-32,(char)-68,(char)-61,18,0,(char)-80,(char)-72,
    114,30,(char)-29,(char)-83,(char)-114,(char)-16,114,(char)-24,8,114,(char)-15,98,100,(char)-18,(char)-48,108,(char)-77,(char)-90,67,33,114,(char)-78,84,(char)-128,(char)-86,(char)-82,(char)-83,(char)-96,
    (char)-91,12,(char)-101,(char)-98,(char)-106,119,106,102,7,19,(char)-103,43,22,83,100,(char)-17,(char)-40,(char)-59,(char)-29,(char)-18,(char)-31,84,(char)-110,105,80,(char)-76,49,69,(char)-12,(char)-10,
    12,(char)-22,72,(char)-63,(char)-118,(char)-118,(char)-75,24,8,(char)-117,5,(char)-57,4,35,(char)-44,55,(char)-89,105,19,(char)-40,(char)-109,(char)-69,(char)-32,(char)-76,6,(char)-107,(char)-37,(char)-116,
    6,(char)-41,(char)-116,(char)-68,4,(char)-111,(char)-7,23,82,(char)-115,(char)-121,1,(char)-75,118,(char)-66,38,(char)-26,90,(char)-95,53,50,(char)-75,36,(char)-37,68,123,77,(char)-49,32,45,(char)-35,
    127,(char)-75,7,44,(char)-78,(char)-81,14,78,(char)-102,46,(char)-65,(char)-9,(char)-108,125,110,83,124,10,100,93,(char)-18,123,(char)-22,1,(char)-73,(char)-96,(char)-5,(char)-104,(char)-50,(char)-14,
    89,(char)-17,(char)-20,(char)-8,(char)-61,(char)-64,(char)-126,123,(char)-44,(char)-77,87,14,(char)-86,(char)-46,(char)-126,103,(char)-100,(char)-41,(char)-127,107,(char)-79,35,(char)-63,95,43,69,33,
    86,12,(char)-54,18,(char)-90,(char)-18,(char)-36,28,(char)-73,46,8,110,42,124,96,33,13,(char)-103,(char)-76,(char)-37,17,72,46,(char)-71,(char)-29,(char)-31,(char)-112,76,(char)-123,(char)-10,102,(char)-102,
    (char)-70,(char)-120,49,(char)-23,(char)-10,38,62,59,74,94,87,50,(char)-12,65,94,5,9,(char)-117,95,(char)-8,(char)-125,115,2,60,(char)-119,10,(char)-92,33,101,(char)-22,109,(char)-55,(char)-57,20,41,
    56,121,37,112,(char)-25,113,3,71,(char)-124,81,13,(char)-33,(char)-61,(char)-58,40,36,112,(char)-73,(char)-10,(char)-7,(char)-19,84,(char)-122,(char)-87,(char)-104,123,114,(char)-110,(char)-100,(char)-73,
    (char)-52,(char)-48,(char)-101,(char)-55,20,50,16,98,12,(char)-100,118,87,(char)-69,88,(char)-39,58,(char)-46,(char)-30,82,(char)-14,(char)-15,121,(char)-76,(char)-99,29,(char)-102,(char)-19,(char)-110,
    (char)-128,126,18,(char)-107,72,45,6,(char)-38,97,12,(char)-98,31,(char)-36,(char)-33,(char)-52,29,(char)-21,96,(char)-118,(char)-124,16,91,(char)-30,90,(char)-23,(char)-28,(char)-110,95,16,71,51,(char)-112,
    (char)-125,(char)-99,(char)-60,94,(char)-100,119,(char)-83,(char)-86,(char)-99,(char)-81,(char)-123,(char)-44,(char)-58,2,(char)-39,(char)-112,(char)-69,(char)-72,71,(char)-49,82,(char)-88,(char)-29,
    (char)-124,114,(char)-121,122,(char)-121,17,12,14,13,(char)-95,(char)-47,58,(char)-115,(char)-113,120,28,(char)-127,16,(char)-13,89,(char)-81,(char)-111,(char)-33,(char)-125,93,116,85,31,(char)-10,(char)-84,
    (char)-53,(char)-68,(char)-58,99,30,(char)-108,(char)-116,53,80,127,(char)-122,(char)-39,(char)-83,(char)-113,24,58,(char)-105,9,66,(char)-62,45,(char)-39,23,89,81,(char)-94,(char)-79,(char)-110,48,
    (char)-118,(char)-5,(char)-31,23,(char)-117,99,(char)-95,(char)-90,105,19,(char)-125,121,20,(char)-87,(char)-28,(char)-103,(char)-88,71,32,19,37,85,88,(char)-114,24,75,112,(char)-21,97,(char)-102,126,
    (char)-62,102,(char)-98,(char)-34,39,(char)-75,74,(char)-110,(char)-56,(char)-54,16,26,43,123,(char)-4,109,(char)-109,(char)-122,111,(char)-39,127,115,(char)-11,(char)-12,44,(char)-57,24,14,(char)-75,
    28,(char)-77,98,(char)-96,21,(char)-110,(char)-60,124,(char)-3,81,(char)-85,89,(char)-20,(char)-102,76,(char)-27,(char)-87,11,107,38,(char)-124,(char)-30,2,(char)-106,25,58,(char)-8,44,(char)-18,(char)-110,
    125,(char)-98,63,115,(char)-76,(char)-89,46,121,(char)-100,0,(char)-44,101,106,(char)-125,(char)-24,(char)-56,36,58,28,50,9,(char)-111,125,125,48,53,85,101,(char)-32,100,(char)-38,83,(char)-10,(char)-96,
    97,71,(char)-81,(char)-113,(char)-28,66,106,(char)-28,(char)-88,46,(char)-55,(char)-19,(char)-26,126,(char)-66,(char)-21,26,43,(char)-79,48,(char)-58,(char)-34,43,(char)-55,47,59,102,(char)-24,(char)-116,
    (char)-47,42,29,44,(char)-96,(char)-124,90,100,(char)-12,(char)-112,35,(char)-42,17,58,(char)-61,(char)-79,6,(char)-12,(char)-122,(char)-18,84,(char)-109,69,(char)-23,(char)-124,(char)-113,87,31,14,
    75,92,(char)-7,99,32,(char)-85,106,106,45,5,106,125,(char)-40,38,115,(char)-31,(char)-2,(char)-45,37,(char)-27,81,62,(char)-108,(char)-48,35,(char)-73,82,119,113,(char)-125,82,(char)-92,(char)-30,(char)-86,
    (char)-108,(char)-106,58,(char)-6,121,69,(char)-43,(char)-34,(char)-77,(char)-48,(char)-84,113,(char)-85,110,8,(char)-2,(char)-57,34,(char)-54,122,(char)-108,(char)-33,82,(char)-78,(char)-89,72,80,(char)-115,
    (char)-68,(char)-91,22,80,(char)-29,(char)-74,(char)-46,(char)-85,(char)-100,79,(char)-7,(char)-38,(char)-68,126,(char)-110,(char)-47,20,11,(char)-90,17,(char)-47,(char)-64,(char)-5,40,(char)-77,61,
    123,(char)-99,(char)-80,105,(char)-37,(char)-5,(char)-28,(char)-106,(char)-122,3,107,(char)-27,(char)-122,(char)-100,(char)-44,25,(char)-115,(char)-102,79,(char)-100,(char)-46,(char)-91,83,36,(char)-30,
    24,(char)-56,(char)-28,(char)-7,(char)-10,19,45,(char)-83,(char)-86,49,(char)-10,(char)-124,36,58,55,(char)-7,(char)-99,7,7,(char)-124,(char)-35,114,52,72,(char)-32,4,(char)-97,95,(char)-96,(char)-123,
    (char)-120,(char)-48,(char)-121,(char)-114,(char)-91,(char)-85,(char)-78,100,(char)-118,(char)-15,(char)-38,22,(char)-25,19,(char)-62,(char)-26,(char)-25,88,(char)-57,(char)-90,(char)-120,(char)-26,
    (char)-38,61,(char)-102,(char)-57,104,121,(char)-49,(char)-75,96,70,20,(char)-40,(char)-116,80,51,12,(char)-106,(char)-63,116,90,51,32,4,90,109,104,(char)-108,31,35,117,(char)-90,(char)-56,105,35,(char)-121,
    (char)-38,(char)-72,(char)-54,(char)-78,103,25,84,(char)-23,(char)-34,(char)-68,(char)-35,76,3,118,36,105,87,72,22,10,36,31,15,(char)-15,(char)-9,(char)-26,(char)-113,(char)-72,(char)-38,(char)-58,(char)-13,
    (char)-44,(char)-126,(char)-32,(char)-36,7,52,33,(char)-111,(char)-109,56,110,80,77,124,(char)-60,(char)-109,(char)-4,90,97,(char)-26,(char)-30,23,19,102,(char)-69,87,(char)-106,(char)-37,115,56,23,
    48,(char)-2,(char)-108,(char)-2,93,(char)-124,41,113,100,(char)-29,63,118,(char)-72,(char)-44,(char)-69,116,91,63,(char)-76,(char)-42,22,(char)-16,104,103,(char)-116,(char)-40,(char)-103,(char)-5,(char)-105,
    101,(char)-34,(char)-29,(char)-25,(char)-1,81,(char)-114,(char)-4,(char)-24,(char)-125,(char)-19,(char)-32,(char)-45,51,(char)-117,51,(char)-33,(char)-109,(char)-17,(char)-35,(char)-72,23,124,67,(char)-105,
    (char)-22,(char)-56,(char)-103,110,(char)-98,(char)-38,30,114,98,(char)-122,94,(char)-53,(char)-20,33,(char)-66,(char)-12,(char)-17,122,(char)-111,(char)-106,(char)-51,(char)-13,76,22,(char)-106,(char)-1,
    (char)-84,87,(char)-3,1,71,(char)-71,1,(char)-83,18,(char)-47,(char)-102,(char)-67,(char)-83,5,(char)-82,33,(char)-125,(char)-74,(char)-74,(char)-90,(char)-112,(char)-67,6,(char)-76,40,113,(char)-41,
    (char)-56,(char)-11,(char)-107,17,(char)-91,(char)-110,22,(char)-111,(char)-93,(char)-82,106,(char)-79,105,(char)-122,(char)-106,(char)-8,101,115,(char)-48,(char)-64,97,117,(char)-5,(char)-22,(char)-104,
    94,102,(char)-99,111,78,(char)-107,64,(char)-41,34,29,(char)-117,116,(char)-24,(char)-128,0,37,(char)-34,106,21,(char)-54,(char)-24,84,(char)-86,(char)-104,(char)-13,96,80,119,(char)-75,80,64,84,(char)-53,
    91,105,1,113,(char)-51,(char)-110,98,(char)-90,111,(char)-19,(char)-31,114,(char)-63,116,86,71,34,(char)-3,(char)-83,(char)-10,119,24,115,(char)-41,(char)-10,50,83,(char)-40,25,(char)-52,54,(char)-125,
    30,(char)-104,90,88,55,(char)-88,20,(char)-22,13,86,(char)-13,(char)-77,(char)-62,(char)-36,(char)-60,(char)-94,4,(char)-110,126,(char)-25,52,114,21,108,(char)-8,127,(char)-83,26,113,(char)-70,5,34,
    (char)-83,(char)-4,(char)-19,60,(char)-106,(char)-22,113,39,(char)-64,70,(char)-111,(char)-69,90,(char)-127,(char)-46,58,85,74,90,(char)-124,83,(char)-83,(char)-120,(char)-58,66,35,74,21,(char)-83,(char)-45,
    32,68,116,118,27,97,36,72,(char)-68,(char)-82,(char)-77,19,16,4,(char)-100,(char)-128,(char)-71,(char)-95,(char)-63,80,(char)-1,(char)-122,101,(char)-53,(char)-112,26,(char)-43,(char)-101,(char)-127,
    (char)-53,(char)-15,(char)-41,72,118,51,(char)-127,122,27,(char)-73,(char)-80,108,(char)-70,(char)-38,82,(char)-102,40,69,101,(char)-45,64,11,(char)-19,(char)-77,68,27,(char)-126,(char)-88,(char)-89,
    78,(char)-91,48,(char)-41,55,23,124,(char)-69,(char)-25,111,(char)-3,(char)-66,(char)-116,3,63,(char)-88,88,(char)-11,(char)-75,95,(char)-49,49,(char)-27,53,(char)-95,(char)-35,13,40,103,(char)-35,(char)-31,
    78,57,83,(char)-87,(char)-87,99,(char)-41,(char)-12,77,37,(char)-11,(char)-69,(char)-16,(char)-63,77,(char)-76,(char)-52,113,2,(char)-27,26,57,49,93,(char)-78,59,(char)-90,116,(char)-63,114,(char)-105,
    57,(char)-42,126,(char)-29,(char)-1,(char)-115,94,(char)-119,123,(char)-98,(char)-52,14,75,(char)-20,(char)-50,121,(char)-32,85,72,121,(char)-48,(char)-4,(char)-126,(char)-65,(char)-66,(char)-67,(char)-62,
    (char)-116,(char)-52,(char)-109,110,120,(char)-23,(char)-97,(char)-76,(char)-20,45,(char)-13,61,120,5,(char)-125,(char)-29,(char)-24,39,(char)-1,8,(char)-12,(char)-68,113,109,(char)-8,102,(char)-89,
    117,(char)-92,(char)-123,(char)-15,23,64,54,(char)-3,(char)-32,(char)-113,(char)-122,29,106,69,(char)-37,125,24,55,(char)-103,(char)-79,(char)-6,50,(char)-84,26,(char)-87,100,103,(char)-4,50,92,(char)-95,
    (char)-27,(char)-117,(char)-22,(char)-124,90,(char)-127,108,(char)-9,(char)-12,(char)-32,122,(char)-31,45,78,45,(char)-25,(char)-74,(char)-16,(char)-111,47,65,50,46,91,116,(char)-19,(char)-52,25,(char)-68,
    (char)-107,(char)-53,56,(char)-6,(char)-110,50,(char)-22,115,109,(char)-12,(char)-79,121,(char)-39,83,116,39,(char)-66,(char)-72,111,(char)-76,15,95,(char)-20,(char)-92,79,101,(char)-73,(char)-113,(char)-75,
    (char)-47,(char)-97,47,44,(char)-45,(char)-105,(char)-128,85,(char)-31,52,(char)-26,(char)-1,(char)-50,(char)-118,89,127,(char)-49,(char)-37,14,46,(char)-60,82,(char)-108,68,(char)-126,16,80,4,49,12,
    32,44,(char)-89,5,(char)-89,117,103,47,14,(char)-8,(char)-120,(char)-56,(char)-115,21,109,97,83,105,19,(char)-34,(char)-29,(char)-56,49,(char)-38,40,8,(char)-4,(char)-20,(char)-98,10,(char)-87,74,(char)-93,
    63,(char)-11,(char)-65,(char)-117,(char)-98,44,(char)-4,32,117,(char)-92,123,25,83,63,8,90,18,(char)-79,5,(char)-4,100,(char)-33,121,111,(char)-62,2,26,80,19,(char)-32,(char)-56,3,101,(char)-85,(char)-35,
    (char)-82,(char)-78,49,78,(char)-75,(char)-94,25,11,29,115,(char)-71,(char)-53,86,(char)-17,78,109,(char)-88,127,36,64,33,(char)-124,(char)-93,2,(char)-118,43,(char)-16,72,(char)-3,109,52,(char)-105,
    96,84,72,33,(char)-116,96,33,(char)-36,47,29,(char)-17,11,(char)-123,(char)-16,59,17,62,(char)-50,(char)-114,31,21,30,(char)-28,10,(char)-72,7,(char)-123,71,(char)-127,(char)-119,(char)-63,(char)-71,
    72,116,106,(char)-123,(char)-97,(char)-85,54,(char)-37,(char)-12,106,(char)-2,(char)-42,(char)-74,12,75,38,27,97,61,86,(char)-50,(char)-24,88,28,30,32,70,(char)-59,(char)-52,(char)-3,79,(char)-40,91,
    (char)-76,109,38,(char)-71,(char)-94,101,29,(char)-106,105,(char)-52,100,(char)-21,89,(char)-5,42,(char)-104,(char)-31,37,(char)-125,(char)-64,85,31,39,(char)-30,(char)-33,96,53,24,80,23,59,(char)-42,
    5,(char)-74,(char)-64,(char)-63,45,(char)-98,117,(char)-19,(char)-97,69,(char)-34,116,(char)-123,(char)-16,49,(char)-9,4,(char)-20,(char)-102,112,(char)-114,(char)-75,(char)-65,69,(char)-128,(char)-113,
    115,98,(char)-105,46,(char)-58,98,23,47,49,(char)-68,(char)-119,(char)-91,(char)-68,(char)-10,106,82,114,9,(char)-46,49,(char)-60,31,42,(char)-81,104,48,53,84,76,(char)-52,(char)-94,95,86,(char)-106,
    (char)-15,33,(char)-23,(char)-53,82,(char)-80,91,(char)-94,85,105,81,28,(char)-35,(char)-90,(char)-38,118,22,30,29,(char)-83,123,80,(char)-104,26,(char)-18,14,(char)-102,112,101,(char)-101,18,81,20,
    (char)-65,39,118,(char)-107,116,(char)-125,62,75,(char)-120,(char)-1,(char)-17,57,40,32,(char)-86,21,22,(char)-58,28,(char)-57,104,(char)-27,30,77,79,23,87,15,(char)-17,74,(char)-77,29,74,(char)-33,
    (char)-1,(char)-52,(char)-102,(char)-94,0,25,33,107,77,34,41,(char)-41,(char)-50,(char)-110,(char)-74,(char)-46,59,(char)-43,114,70,72,(char)-87,39,(char)-86,(char)-7,(char)-120,(char)-42,(char)-127,
    91,(char)-43,4,(char)-18,(char)-44,(char)-102,13,6,105,(char)-44,(char)-118,(char)-29,(char)-96,31,82,18,(char)-38,80,69,(char)-72,43,3,(char)-98,(char)-109,105,56,52,(char)-79,111,94,85,(char)-6,31,
    (char)-3,(char)-71,87,88,69,(char)-84,(char)-61,85,39,79,(char)-18,(char)-5,(char)-106,97,(char)-89,81,105,14,(char)-58,(char)-2,109,(char)-32,6,(char)-93,8,(char)-13,14,14,10,45,(char)-78,95,(char)-107,
    16,60,104,(char)-71,(char)-29,122,25,(char)-101,120,120,2,91,(char)-8,(char)-46,(char)-94,51,93,51,42,86,(char)-67,1,36,87,94,(char)-51,(char)-41,(char)-72,(char)-28,(char)-39,63,91,(char)-116,(char)-9,
    21,(char)-47,13,(char)-77,(char)-22,109,(char)-17,59,(char)-58,119,(char)-68,(char)-22,(char)-73,77,(char)-30,27,(char)-13,7,4,(char)-16,(char)-95,55,(char)-59,(char)-49,(char)-38,(char)-43,95,123,(char)-65,
    49,2,42,(char)-30,(char)-67,(char)-4,(char)-100,51,(char)-77,45,108,(char)-87,97,58,104,78,45,(char)-101,(char)-77,94,(char)-73,(char)-68,19,(char)-53,100,(char)-12,34,10,67,109,100,54,51,34,(char)-74,
    (char)-9,116,97,90,78,(char)-47,(char)-77,112,68,(char)-112,107,26,117,(char)-125,32,(char)-32,41,41,60,76,(char)-22,49,(char)-63,(char)-27,(char)-90,(char)-125,126,(char)-64,(char)-33,30,90,(char)-9,
    (char)-106,115,(char)-15,(char)-67,(char)-76,(char)-79,2,(char)-104,119,(char)-15,27,(char)-25,(char)-93,50,68,86,26,104,(char)-23,60,49,(char)-12,53,14,74,44,(char)-102,79,35,(char)-39,109,41,65,(char)-11,
    (char)-92,7,(char)-58,(char)-17,92,(char)-77,79,(char)-80,119,(char)-25,50,63,(char)-92,(char)-45,(char)-64,56,12,(char)-51,73,78,6,99,55,108,(char)-47,(char)-111,2,(char)-55,(char)-109,(char)-24,(char)-31,
    74,(char)-26,(char)-86,(char)-50,114,(char)-90,(char)-4,(char)-73,(char)-1,(char)-35,26,79,(char)-61,(char)-41,(char)-1,94,(char)-115,65,7,108,86,126,119,(char)-2,92,(char)-113,(char)-103,(char)-76,
    118,11,121,30,(char)-71,117,(char)-46,83,(char)-31,12,97,(char)-51,125,109,49,35,(char)-17,102,(char)-2,(char)-50,22,127,(char)-53,(char)-50,(char)-4,(char)-101,110,40,(char)-119,(char)-13,86,84,126,
    61,(char)-83,41,(char)-77,(char)-39,(char)-16,53,78,95,113,21,47,(char)-112,(char)-32,115,(char)-12,127,106,(char)-95,(char)-1,(char)-47,(char)-23,(char)-5,(char)-28,74,30,(char)-61,(char)-74,96,116,
    (char)-59,(char)-32,119,126,89,31,91,(char)-10,(char)-52,(char)-63,(char)-74,59,(char)-14,42,90,41,38,(char)-36,86,(char)-4,50,126,(char)-122,51,67,(char)-87,48,86,(char)-46,95,(char)-60,97,28,8,(char)-63,
    120,119,(char)-4,(char)-18,(char)-37,86,(char)-16,(char)-9,64,(char)-65,7,15,(char)-99,(char)-14,29,(char)-47,(char)-53,40,(char)-40,(char)-69,(char)-74,25,(char)-7,(char)-14,(char)-6,99,(char)-42,(char)-128,
    (char)-57,98,(char)-11,(char)-71,108,(char)-97,(char)-67,105,105,98,(char)-79,(char)-21,(char)-39,(char)-52,(char)-90,(char)-15,(char)-87,(char)-10,92,(char)-4,(char)-70,103,(char)-42,122,116,71,(char)-95,
    121,(char)-54,116,(char)-101,59,14,(char)-107,(char)-8,(char)-20,(char)-75,(char)-2,114,(char)-42,120,(char)-45,20,(char)-16,6,127,(char)-100,(char)-102,(char)-75,(char)-37,10,(char)-1,37,(char)-66,
    68,102,(char)-24,(char)-20,(char)-30,12,47,39,76,9,6,(char)-73,(char)-122,117,(char)-31,(char)-93,82,109,(char)-117,(char)-24,(char)-94,96,82,102,(char)-42,(char)-57,(char)-96,27,107,46,110,(char)-90,
    (char)-122,32,(char)-84,(char)-29,91,35,108,124,47,76,66,(char)-88,(char)-66,(char)-55,(char)-55,26,(char)-10,(char)-109,59,10,123,29,14,(char)-4,(char)-112,29,79,(char)-42,106,(char)-87,89,(char)-114,
    (char)-109,45,(char)-63,(char)-106,(char)-109,(char)-114,44,106,(char)-9,(char)-26,(char)-128,(char)-66,69,(char)-42,72,92,(char)-66,(char)-62,(char)-125,63,(char)-75,26,13,70,31,97,(char)-5,26,78,121,
    106,49,(char)-70,(char)-118,(char)-78,(char)-116,46,118,(char)-35,(char)-98,104,98,(char)-79,105,108,86,(char)-45,4,(char)-104,(char)-45,58,76,(char)-12,(char)-46,(char)-55,96,(char)-41,64,52,106,(char)-33,
    (char)-47,32,(char)-104,(char)-34,(char)-74,96,(char)-93,84,32,(char)-35,(char)-56,59,78,1,79,52,(char)-114,(char)-15,(char)-29,4,(char)-56,114,42,94,106,122,122,(char)-57,11,104,(char)-36,(char)-11,
    (char)-111,101,(char)-13,(char)-42,(char)-51,(char)-106,(char)-30,(char)-55,(char)-108,(char)-67,(char)-21,(char)-10,(char)-44,63,69,(char)-3,1,112,113,(char)-6,(char)-88,(char)-46,(char)-19,76,(char)-99,
    (char)-5,(char)-128,51,(char)-9,(char)-111,(char)-111,95,(char)-124,(char)-19,36,(char)-121,(char)-113,(char)-81,23,56,4,122,63,(char)-33,110,(char)-124,(char)-98,123,(char)-59,74,(char)-34,39,(char)-51,
    (char)-1,113,(char)-82,35,(char)-43,(char)-45,(char)-91,(char)-44,(char)-85,28,(char)-96,24,(char)-35,(char)-77,(char)-12,(char)-60,20,(char)-42,(char)-43,(char)-81,(char)-73,107,(char)-107,(char)-123,
    124,(char)-67,(char)-58,(char)-40,(char)-119,(char)-75,(char)-113,(char)-113,22,114,90,(char)-91,(char)-83,50,(char)-84,110,(char)-26,(char)-41,(char)-126,94,(char)-79,117,104,40,66,96,38,(char)-21,
    26,(char)-14,103,(char)-82,(char)-71,11,106,(char)-101,(char)-23,(char)-26,55,(char)-67,21,96,(char)-98,(char)-120,(char)-114,(char)-84,103,(char)-56,(char)-127,117,120,(char)-30,(char)-47,54,(char)-2,
    0,9,22,78,(char)-14,125,(char)-85,(char)-14,73,(char)-17,(char)-26,(char)-111,(char)-119,15,(char)-117,125,(char)-49,104,49,117,(char)-27,(char)-85,(char)-55,85,(char)-65,117,(char)-83,33,(char)-64,
    (char)-59,(char)-6,(char)-57,(char)-105,(char)-60,50,(char)-73,21,(char)-11,(char)-79,116,(char)-35,8,(char)-81,(char)-118,(char)-122,(char)-23,(char)-94,(char)-39,5,(char)-99,105,15,(char)-9,78,(char)-11,
    39,8,(char)-32,(char)-96,(char)-13,9,34,(char)-98,30,39,(char)-64,58,103,47,(char)-45,(char)-9,13,(char)-43,(char)-65,(char)-82,(char)-115,61,76,(char)-86,30,(char)-102,111,112,127,(char)-26,59,53,(char)-5,
    (char)-13,(char)-79,114,5,(char)-80,(char)-58,(char)-66,(char)-26,110,(char)-106,(char)-104,75,(char)-70,100,(char)-78,65,57,(char)-43,107,124,(char)-51,79,(char)-8,(char)-17,121,(char)-122,(char)-3,
    60,(char)-51,(char)-9,(char)-116,16,110,77,(char)-47,5,13,14,3,(char)-95,(char)-62,(char)-93,24,(char)-64,(char)-24,(char)-58,71,34,(char)-2,95,3,94,65,(char)-45,71,22,112,66,79,62,(char)-114,102,(char)-44,
    (char)-85,92,21,74,106,(char)-69,(char)-6,73,(char)-30,73,78,86,107,106,39,(char)-69,73,(char)-109,(char)-15,(char)-33,58,73,16,(char)-96,(char)-123,28,(char)-15,(char)-27,(char)-15,(char)-73,(char)-70,
    (char)-60,(char)-33,(char)-52,(char)-40,74,72,33,46,1,(char)-3,68,87,19,56,86,(char)-65,23,(char)-5,(char)-35,(char)-9,(char)-108,(char)-49,(char)-84,111,(char)-67,46,44,23,(char)-37,83,(char)-117,(char)-47,
    89,(char)-21,(char)-42,45,(char)-37,71,41,(char)-50,17,71,(char)-62,1,2,68,20,(char)-97,120,(char)-76,62,(char)-8,19,(char)-114,(char)-75,(char)-15,34,23,45,105,40,124,126,(char)-105,10,(char)-82,124,
    (char)-123,(char)-31,(char)-78,(char)-21,74,(char)-66,(char)-62,(char)-66,(char)-94,(char)-12,(char)-119,66,23,(char)-29,(char)-107,(char)-54,(char)-99,42,(char)-3,57,106,67,(char)-103,(char)-114,(char)-69,
    98,(char)-15,(char)-75,51,103,(char)-16,34,(char)-127,45,(char)-74,(char)-4,103,108,(char)-53,95,10,69,(char)-84,74,71,(char)-62,33,(char)-80,66,(char)-95,(char)-4,(char)-123,(char)-20,54,77,89,115,
    (char)-77,(char)-74,(char)-84,(char)-50,(char)-106,63,49,18,(char)-49,95,(char)-49,(char)-87,(char)-2,(char)-104,38,3,53,(char)-53,(char)-104,110,(char)-43,(char)-98,(char)-42,(char)-90,91,(char)-115,
    89,(char)-44,(char)-101,(char)-53,14,(char)-14,78,(char)-115,(char)-3,(char)-8,79,(char)-1,54,(char)-37,0,(char)-20,(char)-21,116,6,(char)-70,(char)-6,(char)-125,127,8,73,(char)-29,(char)-110,(char)-63,
    (char)-49,37,(char)-89,42,(char)-44,99,35,123,120,27,(char)-91,12,(char)-23,70,(char)-34,30,112,(char)-125,(char)-70,9,(char)-20,(char)-27,(char)-68,57,65,(char)-17,20,(char)-83,38,(char)-11,99,(char)-35,
    (char)-57,89,(char)-17,24,125,(char)-77,80,18,(char)-9,(char)-43,(char)-82,108,67,74,4,89,123,(char)-33,(char)-77,(char)-62,(char)-26,116,89,(char)-83,78,(char)-73,109,(char)-43,(char)-35,24,102,76,
    (char)-113,(char)-25,59,(char)-82,60,(char)-112,14,74,115,(char)-74,106,91,(char)-56,(char)-20,58,54,(char)-71,(char)-91,(char)-7,9,16,6,(char)-58,119,14,(char)-68,99,(char)-60,95,(char)-7,(char)-101,
    3,(char)-33,24,(char)-69,(char)-80,54,14,113,(char)-29,(char)-86,(char)-15,123,73,(char)-87,(char)-40,51,(char)-74,(char)-113,96,16,41,77,(char)-120,89,(char)-74,46,(char)-43,73,(char)-105,23,(char)-127,
    15,79,96,19,71,(char)-64,(char)-121,(char)-1,119,99,(char)-17,(char)-103,54,(char)-66,61,(char)-15,93,(char)-17,92,53,4,60,(char)-55,54,91,62,90,112,(char)-37,(char)-38,17,91,34,(char)-52,39,(char)-42,
    127,(char)-36,(char)-64,(char)-8,(char)-28,93,4,(char)-39,(char)-8,(char)-12,118,(char)-99,70,(char)-74,(char)-62,87,(char)-10,(char)-9,38,(char)-116,(char)-97,48,26,62,94,(char)-97,(char)-17,18,80,
    (char)-98,(char)-13,3,(char)-100,(char)-6,17,(char)-17,(char)-22,(char)-44,83,(char)-71,(char)-125,(char)-49,51,(char)-69,41,85,25,88,11,115,50,39,(char)-110,124,104,(char)-11,(char)-114,(char)-68,98,
    (char)-118,(char)-41,(char)-83,(char)-99,97,(char)-127,(char)-110,21,(char)-115,(char)-35,(char)-31,(char)-68,(char)-38,29,2,71,113,21,(char)-106,117,(char)-50,108,(char)-95,106,(char)-93,86,(char)-35,
    (char)-49,(char)-17,(char)-95,(char)-56,37,98,55,(char)-20,(char)-34,77,93,(char)-63,(char)-124,(char)-18,(char)-116,102,(char)-45,(char)-85,(char)-60,(char)-85,87,54,72,(char)-98,93,55,(char)-105,33,
    113,126,(char)-77,5,76,(char)-105,(char)-65,(char)-76,122,(char)-82,86,90,(char)-79,(char)-50,(char)-32,35,(char)-30,123,(char)-79,(char)-115,(char)-40,(char)-34,23,(char)-55,101,11,74,(char)-93,(char)-18,
    88,(char)-92,64,63,118,53,108,61,(char)-89,(char)-14,(char)-66,36,(char)-66,(char)-92,(char)-38,10,120,(char)-79,(char)-126,(char)-74,(char)-83,32,(char)-69,96,27,(char)-83,57,(char)-27,(char)-114,(char)-111,
    (char)-115,(char)-127,(char)-56,41,38,(char)-114,25,(char)-104,(char)-32,(char)-21,114,81,113,16,98,80,73,(char)-29,(char)-1,(char)-65,127,(char)-33,70,(char)-83,(char)-19,59,(char)-16,(char)-117,82,
    (char)-7,124,(char)-25,84,111,(char)-8,(char)-85,75,(char)-1,103,(char)-12,(char)-118,(char)-80,(char)-14,122,(char)-55,(char)-63,(char)-39,(char)-57,110,70,(char)-31,(char)-81,(char)-46,38,97,10,107,
    (char)-127,(char)-26,23,2,(char)-14,(char)-94,82,95,(char)-2,71,(char)-71,57,(char)-57,(char)-95,16,(char)-107,48,33,34,(char)-60,(char)-63,93,(char)-12,100,(char)-84,(char)-1,7,35,62,(char)-30,38,126,
    18,32,78,18,(char)-30,97,74,(char)-110,82,(char)-97,(char)-116,125,(char)-94,(char)-115,(char)-13,(char)-52,(char)-111,71,(char)-110,(char)-17,(char)-1,(char)-4,79,(char)-118,(char)-28,73,97,(char)-119,
    45,117,19,(char)-110,(char)-95,(char)-90,80,(char)-68,42,88,33,(char)-112,84,94,62,71,(char)-76,(char)-22,120,14,(char)-112,(char)-36,(char)-39,(char)-42,(char)-124,(char)-11,(char)-55,(char)-104,(char)-126,
    30,22,(char)-120,(char)-95,(char)-1,(char)-8,(char)-21,(char)-28,118,13,73,49,(char)-49,(char)-57,35,(char)-7,124,12,(char)-124,2,100,(char)-98,66,82,12,(char)-82,127,(char)-5,87,127,119,(char)-56,17,
    0,(char)-6,66,(char)-13,74,65,(char)-57,(char)-63,(char)-41,36,4,(char)-56,(char)-73,(char)-101,3,127,72,40,101,57,18,57,80,64,(char)-88,103,41,(char)-116,18,48,(char)-47,(char)-127,22,(char)-46,(char)-90,
    9,(char)-68,80,0,126,(char)-120,(char)-76,(char)-32,65,(char)-18,0,4,83,(char)-51,27,(char)-17,44,9,77,(char)-22,(char)-109,22,52,(char)-120,(char)-26,32,12,36,116,(char)-57,(char)-107,22,62,46,(char)-117,
    (char)-126,(char)-78,(char)-119,67,(char)-106,(char)-47,(char)-5,117,61,50,0,104,(char)-106,(char)-55,50,105,14,(char)-32,32,57,(char)-10,(char)-113,(char)-100,98,(char)-3,(char)-66,(char)-99,(char)-2,
    (char)-70,(char)-75,39,121,42,(char)-121,98,111,(char)-56,(char)-97,(char)-2,(char)-69,100,(char)-57,(char)-72,99,89,(char)-99,(char)-19,(char)-100,93,(char)-16,2,56,86,24,52,110,(char)-67,(char)-39,
    29,(char)-89,41,52,(char)-82,79,(char)-102,127,105,72,6,(char)-79,(char)-57,101,(char)-36,75,(char)-110,(char)-17,(char)-100,0,(char)-35,100,20,(char)-19,(char)-38,(char)-23,93,(char)-10,61,(char)-16,
    (char)-98,106,(char)-70,123,0,92,94,28,67,1,(char)-72,(char)-2,(char)-9,115,(char)-3,127,33,(char)-21,(char)-98,121,0,81,(char)-112,0,(char)-128,0,76,(char)-2,(char)-65,0,116,(char)-91,(char)-126,6,
    (char)-75,5,0,(char)-15,42,(char)-122,(char)-45,21,(char)-24,68,14,(char)-64,(char)-67,15,65,(char)-89,(char)-33,(char)-116,(char)-128,(char)-112,78,73,(char)-120,102,63,4,(char)-27,(char)-121,63,30,
    70,8,9,(char)-65,38,37,74,53,96,(char)-24,(char)-41,74,111,(char)-64,56,84,58,(char)-107,(char)-108,66,66,6,(char)-108,50,8,105,21,22,66,64,9,53,82,34,(char)-32,68,68,50,16,25,70,16,65,(char)-48,(char)-72,
    120,(char)-83,34,27,63,18,108,(char)-8,107,36,47,(char)-22,(char)-18,18,78,126,(char)-88,(char)-110,47,96,(char)-87,(char)-121,(char)-32,20,56,(char)-104,30,(char)-18,(char)-19,(char)-28,56,106,(char)-115,
    27,(char)-24,56,113,110,119,18,34,66,26,(char)-70,(char)-32,(char)-3,(char)-38,32,(char)-61,97,21,(char)-8,(char)-103,(char)-114,(char)-1,38,50,104,(char)-108,(char)-85,41,56,(char)-95,25,(char)-90,
    123,(char)-21,(char)-51,60,(char)-16,(char)-119,(char)-43,(char)-19,45,67,0,66,(char)-72,36,(char)-45,39,121,(char)-124,(char)-109,15,(char)-86,(char)-128,(char)-114,72,26,(char)-53,(char)-116,(char)-92,
    (char)-69,(char)-41,(char)-55,(char)-94,0,(char)-84,(char)-7,(char)-73,70,64,(char)-108,(char)-6,56,(char)-126,68,54,(char)-81,70,36,33,(char)-79,61,2,(char)-90,(char)-34,72,4,25,42,127,68,50,53,4,(char)-104,
    109,21,(char)-128,109,(char)-77,(char)-34,33,35,100,90,12,51,69,(char)-116,(char)-22,(char)-109,(char)-61,89,(char)-18,(char)-33,108,38,(char)-58,56,14,(char)-17,96,79,(char)-51,(char)-107,(char)-73,
    65,(char)-103,97,90,89,(char)-32,98,(char)-94,(char)-26,(char)-79,24,54,100,88,19,(char)-121,(char)-88,30,(char)-77,(char)-59,(char)-124,(char)-115,114,(char)-117,26,(char)-27,34,90,39,116,60,(char)-51,
    (char)-72,(char)-12,9,(char)-12,(char)-102,64,(char)-81,35,(char)-94,(char)-53,52,9,(char)-14,58,51,(char)-28,(char)-25,(char)-10,28,25,(char)-33,115,(char)-94,110,100,(char)-115,(char)-104,(char)-81,
    51,(char)-21,(char)-79,(char)-123,36,39,112,114,(char)-59,(char)-24,(char)-76,18,38,(char)-114,6,54,(char)-21,10,(char)-83,(char)-104,(char)-40,48,81,(char)-15,(char)-103,35,24,(char)-27,16,(char)-59,
    57,103,78,(char)-105,(char)-26,21,(char)-46,17,(char)-1,40,0,(char)-92,(char)-104,15,8,(char)-3,(char)-66,(char)-118,40,73,32,80,6,102,22,14,78,110,1,(char)-19,(char)-86,84,(char)-85,(char)-63,(char)-42,
    (char)-96,81,19,14,30,62,17,49,121,37,(char)-119,(char)-79,(char)-109,(char)-64,106,74,55,76,(char)-53,118,92,(char)-49,7,(char)-126,(char)-64,16,40,12,(char)-114,64,(char)-94,(char)-48,24,44,14,79,
    32,(char)-110,(char)-56,20,42,(char)-115,(char)-50,96,(char)-78,(char)-40,28,46,(char)-113,47,16,(char)-118,(char)-60,18,(char)-87,76,(char)-82,80,(char)-86,(char)-44,26,(char)-83,78,(char)-33,0,(char)-59,
    112,(char)-126,66,(char)-91,(char)-47,25,76,22,(char)-101,(char)-61,(char)-27,(char)-15,5,66,(char)-111,88,34,(char)-107,(char)-55,21,74,(char)-107,90,(char)-93,37,117,122,(char)-125,(char)-47,100,(char)-74,
    88,109,(char)-15,(char)-40,64,103,119,56,93,(char)-18,(char)-94,(char)-3,112,121,(char)-46,12,(char)-72,(char)-75,125,59,(char)-108,28,(char)-57,(char)-69,59,(char)-73,19,57,(char)-119,1,(char)-108,
    80,(char)-58,(char)-123,52,45,101,107,(char)-57,(char)-11,(char)-14,(char)-53,33,(char)-108,113,33,77,75,(char)-39,(char)-38,113,(char)-67,(char)-4,18,8,101,92,72,(char)-45,82,(char)-74,118,92,47,(char)-65,
    20,66,25,23,(char)-46,(char)-76,(char)-108,(char)-83,29,(char)-41,(char)-53,95,97,(char)-85,(char)-92,2,(char)-62,93,71,(char)-39,121,(char)-89,105,(char)-123,47,77,107,43,(char)-92,15,50,5,0,0,48,73,
    19,0,(char)-128,75,(char)-61,(char)-18,(char)-1,14,(char)-82,(char)-21,(char)-33,96,(char)-14,(char)-25,(char)-3,(char)-109,(char)-125,127,(char)-112,90,(char)-41,(char)-1,76,(char)-10,(char)-77,(char)-77,
    (char)-9,112,127,(char)-48,125,(char)-1,75,37,(char)-57,(char)-57,65,(char)-46,91,99,0,0 };
    static constexpr const char assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1i8q131njo_woff2[] = {
        119,79,70,50,0,1,0,0,0,0,35,(char)-32,0,14,0,0,0,0,82,36,0,0,35,(char)-122,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,58,27,30,28,(char)-124,80,6,96,0,(char)-124,44,17,16,10,(char)-127,(char)-125,
    32,(char)-25,12,11,(char)-124,12,0,1,54,2,36,3,(char)-124,16,4,32,5,(char)-125,58,7,(char)-121,121,27,12,65,(char)-77,(char)-94,118,(char)-76,90,2,(char)-118,(char)-94,92,(char)-113,62,41,(char)-94,
    122,115,120,(char)-10,127,74,110,(char)-116,33,53,(char)-96,(char)-41,67,(char)-52,(char)-122,89,(char)-38,101,(char)-34,116,(char)-70,(char)-88,(char)-125,105,28,(char)-91,(char)-68,(char)-66,16,2,
    (char)-111,105,(char)-77,(char)-37,(char)-35,(char)-20,(char)-26,96,(char)-104,(char)-98,51,(char)-44,56,(char)-8,(char)-86,97,(char)-47,(char)-33,(char)-60,16,84,93,45,(char)-86,61,(char)-33,(char)-74,
    104,57,90,23,84,92,(char)-114,(char)-48,(char)-40,39,(char)-71,4,(char)-43,(char)-38,(char)-65,61,115,119,31,89,1,11,15,24,66,(char)-59,(char)-20,(char)-56,(char)-94,74,69,3,(char)-107,127,(char)-62,
    68,(char)-111,50,97,25,27,(char)-97,(char)-6,59,68,115,(char)-42,(char)-124,36,16,(char)-44,34,78,(char)-128,(char)-124,(char)-104,0,(char)-119,111,76,8,(char)-101,0,65,44,104,17,45,84,(char)-4,74,(char)-27,
    74,(char)-11,(char)-18,43,70,75,(char)-17,(char)-88,81,42,6,85,(char)-89,(char)-82,(char)-25,109,(char)-81,(char)-3,(char)-85,(char)-97,(char)-44,(char)-1,84,107,(char)-1,75,(char)-21,59,120,79,(char)-102,
    (char)-83,(char)-96,(char)-90,(char)-94,12,(char)-87,(char)-54,109,10,114,88,(char)-5,(char)-110,111,(char)-103,49,(char)-125,(char)-121,62,36,111,43,21,13,41,84,(char)-28,30,(char)-38,(char)-100,(char)-24,
    (char)-7,(char)-72,(char)-47,(char)-66,38,(char)-96,(char)-128,2,(char)-116,(char)-93,38,(char)-15,40,(char)-67,57,(char)-124,3,(char)-96,19,(char)-5,(char)-60,104,(char)-109,7,(char)-49,(char)-107,
    106,42,101,(char)-112,(char)-79,(char)-69,6,90,(char)-56,(char)-111,60,77,94,3,(char)-77,126,(char)-23,(char)-49,15,110,(char)-31,(char)-100,38,(char)-91,(char)-121,11,(char)-108,6,(char)-52,78,(char)-20,
    (char)-1,116,(char)-22,93,(char)-70,(char)-3,95,111,33,64,(char)-37,65,62,(char)-56,(char)-77,74,10,(char)-112,(char)-58,110,(char)-11,(char)-80,(char)-91,107,(char)-112,(char)-92,16,82,3,2,100,9,28,
    126,127,121,121,11,(char)-115,13,74,80,9,91,83,14,103,(char)-17,(char)-125,58,63,(char)-4,(char)-97,77,45,(char)-57,59,118,38,(char)-120,74,119,68,29,(char)-43,65,46,(char)-86,3,(char)-20,(char)-23,
    107,102,55,(char)-38,(char)-43,104,(char)-75,2,(char)-29,(char)-38,27,43,108,56,(char)-40,(char)-56,(char)-16,(char)-42,102,(char)-63,(char)-127,20,66,(char)-88,20,86,114,(char)-92,(char)-77,15,8,43,
    (char)-126,(char)-110,(char)-96,(char)-24,0,(char)-117,(char)-22,(char)-6,18,(char)-16,(char)-11,119,69,91,30,124,(char)-1,(char)-36,55,(char)-82,105,91,1,35,(char)-69,(char)-17,76,(char)-53,32,126,
    (char)-81,(char)-99,59,30,71,97,24,(char)-90,1,103,50,(char)-102,117,19,(char)-84,(char)-91,2,79,88,81,(char)-4,59,(char)-65,(char)-74,(char)-66,7,83,(char)-3,39,18,27,48,80,66,(char)-76,(char)-111,
    61,(char)-100,(char)-42,76,(char)-99,(char)-9,114,0,(char)-95,42,(char)-71,(char)-68,101,(char)-104,10,(char)-108,87,0,122,(char)-84,(char)-9,(char)-97,(char)-126,(char)-96,(char)-124,65,(char)-120,
    32,(char)-123,48,79,53,70,88,(char)-46,24,97,117,99,(char)-124,109,(char)-115,17,(char)-114,32,(char)-60,81,64,23,(char)-4,(char)-19,65,(char)-20,(char)-66,54,24,96,(char)-96,0,(char)-54,87,66,(char)-56,
    49,14,(char)-53,32,(char)-9,45,122,(char)-57,(char)-75,53,0,(char)-6,(char)-89,(char)-68,(char)-93,9,(char)-60,97,(char)-83,(char)-35,10,(char)-128,(char)-122,65,(char)-49,(char)-118,(char)-128,18,36,
    1,(char)-34,86,(char)-96,(char)-32,107,(char)-35,(char)-36,8,(char)-96,85,(char)-59,(char)-1,(char)-8,(char)-96,11,(char)-96,(char)-68,(char)-104,111,103,99,27,(char)-112,(char)-99,2,16,16,(char)-15,
    (char)-96,100,0,36,(char)-114,4,24,31,122,(char)-104,58,(char)-50,90,36,53,(char)-48,(char)-9,3,103,70,(char)-112,(char)-47,0,(char)-22,123,25,(char)-9,8,116,(char)-59,(char)-113,106,(char)-71,(char)-91,
    (char)-119,17,32,66,(char)-96,55,(char)-112,(char)-86,(char)-87,33,(char)-14,103,(char)-95,80,97,27,90,(char)-120,63,125,(char)-39,31,(char)-112,41,81,(char)-20,71,126,(char)-98,54,(char)-2,55,(char)-90,
    (char)-128,(char)-56,15,(char)-55,(char)-98,37,56,24,(char)-120,18,68,120,77,47,(char)-1,(char)-118,(char)-63,126,(char)-102,49,(char)-71,31,(char)-10,99,63,(char)-14,87,(char)-56,88,(char)-2,5,59,(char)-2,
    57,(char)-53,70,(char)-79,31,(char)-7,31,(char)-115,40,43,44,54,(char)-41,116,(char)-29,(char)-75,(char)-87,87,(char)-63,(char)-57,(char)-51,66,43,(char)-117,0,19,(char)-43,91,42,77,(char)-29,85,(char)-10,
    (char)-15,47,(char)-101,(char)-86,(char)-4,(char)-82,41,(char)-3,(char)-100,120,(char)-4,11,77,(char)-18,89,38,(char)-20,(char)-23,(char)-109,101,(char)-47,98,63,(char)-10,99,63,(char)-10,99,63,(char)-10,
    99,63,(char)-14,(char)-109,82,(char)-112,50,9,(char)-110,(char)-24,114,10,42,(char)-88,60,(char)-75,22,66,3,(char)-7,(char)-30,51,24,(char)-32,16,(char)-116,26,23,(char)-112,39,14,(char)-56,(char)-18,
    (char)-90,(char)-77,(char)-55,74,40,(char)-3,4,88,2,(char)-36,(char)-75,124,84,15,(char)-36,118,50,66,13,(char)-104,(char)-35,(char)-112,97,(char)-123,46,(char)-29,32,98,(char)-8,1,(char)-60,27,(char)-128,
    (char)-84,(char)-13,19,96,26,18,(char)-112,0,(char)-108,70,52,67,0,104,(char)-96,9,105,(char)-35,60,(char)-33,40,68,64,47,(char)-82,76,106,62,101,(char)-58,(char)-101,(char)-84,(char)-33,87,4,95,(char)-82,
    14,(char)-26,(char)-23,(char)-34,(char)-21,(char)-125,119,(char)-56,88,50,(char)-111,76,37,51,(char)-56,89,100,(char)-120,(char)-36,65,(char)-34,74,(char)-95,82,22,81,(char)-106,80,99,(char)-102,(char)-61,
    (char)-19,(char)-17,69,(char)-58,(char)-109,69,(char)-93,80,(char)-71,9,(char)-6,(char)-4,16,(char)-97,91,7,104,(char)-113,(char)-39,(char)-28,36,50,(char)-66,(char)-19,(char)-103,(char)-81,88,45,(char)-17,
    3,116,0,(char)-27,33,80,(char)-6,1,40,21,(char)-64,(char)-50,69,(char)-120,44,41,33,(char)-128,63,79,2,67,8,53,(char)-7,(char)-13,(char)-7,(char)-40,54,(char)-22,(char)-4,(char)-7,77,32,0,44,3,78,0,
    46,1,110,105,(char)-46,1,(char)-60,98,(char)-50,(char)-25,33,38,81,(char)-13,95,65,(char)-91,(char)-112,83,(char)-69,111,(char)-44,(char)-48,114,40,81,(char)-86,(char)-51,88,54,16,51,63,43,(char)-69,
    (char)-119,38,(char)-104,(char)-92,(char)-104,78,(char)-103,42,(char)-27,58,(char)-104,124,82,(char)-95,86,(char)-91,(char)-50,80,124,(char)-90,(char)-103,97,(char)-90,58,(char)-11,62,104,(char)-48,
    44,(char)-33,84,122,(char)-115,(char)-34,107,(char)-43,(char)-19,(char)-68,11,62,58,107,(char)-116,(char)-55,(char)-2,117,78,(char)-117,2,47,60,(char)-11,(char)-109,34,91,108,(char)-76,(char)-55,(char)-96,
    (char)-51,(char)-74,(char)-38,102,(char)-56,30,59,(char)-19,(char)-78,(char)-37,(char)-80,(char)-67,(char)-10,(char)-39,111,(char)-121,17,(char)-121,29,112,(char)-48,81,(char)-121,(char)-116,115,(char)-60,
    105,39,(char)-100,116,(char)-54,25,(char)-57,(char)-11,(char)-103,(char)-93,(char)-57,44,(char)-33,(char)-102,109,(char)-82,121,122,45,(char)-74,(char)-64,66,(char)-117,44,(char)-11,(char)-99,(char)-17,
    (char)-3,(char)-49,124,(char)-53,(char)-84,(char)-78,(char)-36,10,107,(char)-84,52,(char)-34,106,63,88,(char)-81,(char)-33,6,63,90,103,(char)-64,49,107,(char)-71,(char)-28,(char)-128,(char)-71,(char)-27,
    (char)-14,64,64,(char)-62,8,17,5,93,(char)-66,26,3,68,3,(char)-112,93,64,12,4,12,(char)-99,0,(char)-116,31,0,(char)-44,(char)-57,1,(char)-7,18,64,2,79,45,0,98,(char)-87,32,(char)-17,(char)-64,95,(char)-30,
    (char)-37,118,114,(char)-79,70,(char)-56,63,24,10,101,97,(char)-4,(char)-43,62,(char)-100,(char)-122,83,(char)-84,(char)-5,(char)-10,(char)-125,100,(char)-27,7,6,(char)-105,(char)-20,(char)-44,117,(char)-13,
    127,(char)-3,(char)-125,(char)-125,107,(char)-55,(char)-50,(char)-105,(char)-126,2,(char)-78,(char)-84,112,38,104,3,(char)-75,(char)-8,(char)-5,(char)-122,(char)-6,(char)-63,8,4,70,90,33,22,(char)-6,
    16,45,125,(char)-28,(char)-22,87,74,1,(char)-35,65,38,29,56,57,97,25,(char)-56,101,72,(char)-67,(char)-114,(char)-32,(char)-61,56,68,(char)-22,105,(char)-7,82,52,65,12,78,(char)-99,54,(char)-117,2,(char)-56,
    72,(char)-27,(char)-91,104,(char)-110,11,(char)-127,82,67,119,(char)-107,(char)-58,25,9,(char)-83,68,42,(char)-60,94,(char)-118,(char)-118,(char)-87,85,(char)-127,54,105,(char)-117,66,16,42,(char)-56,
    40,(char)-125,23,(char)-101,101,15,(char)-103,(char)-27,(char)-112,(char)-11,(char)-28,19,(char)-116,70,(char)-120,51,(char)-98,36,(char)-119,(char)-95,80,(char)-41,(char)-85,(char)-82,(char)-45,(char)-12,
    (char)-13,113,39,8,(char)-82,122,(char)-13,10,(char)-95,(char)-88,(char)-38,82,(char)-44,98,(char)-118,31,(char)-14,(char)-93,104,33,86,36,109,(char)-119,(char)-71,16,98,40,(char)-119,(char)-8,(char)-58,
    (char)-48,78,(char)-105,(char)-37,69,(char)-110,20,55,100,(char)-97,25,(char)-88,(char)-109,101,(char)-21,(char)-35,89,(char)-113,125,112,(char)-26,(char)-89,104,(char)-87,122,82,106,115,103,(char)-51,
    70,0,110,(char)-54,(char)-52,97,3,89,(char)-53,(char)-115,(char)-65,(char)-126,35,(char)-51,(char)-38,86,(char)-84,(char)-50,(char)-92,(char)-120,(char)-119,(char)-99,(char)-42,39,86,78,108,(char)-4,
    (char)-41,76,84,44,69,(char)-53,27,(char)-26,1,68,68,103,24,9,113,(char)-113,91,7,34,43,6,(char)-20,(char)-69,59,76,(char)-112,(char)-116,(char)-78,(char)-102,(char)-30,61,(char)-102,106,(char)-82,40,
    105,88,(char)-3,(char)-90,(char)-31,(char)-1,(char)-115,100,(char)-36,(char)-16,3,65,(char)-103,48,53,28,(char)-38,(char)-71,(char)-80,61,79,26,(char)-56,(char)-99,13,53,(char)-94,(char)-49,(char)-124,
    (char)-84,95,56,(char)-87,113,101,(char)-61,(char)-27,(char)-58,(char)-55,62,(char)-29,97,15,110,112,(char)-80,64,76,(char)-7,50,104,64,102,59,73,(char)-33,(char)-76,28,(char)-95,72,114,10,(char)-125,
    103,(char)-44,(char)-18,102,42,(char)-95,48,1,(char)-16,(char)-45,(char)-112,28,(char)-79,(char)-51,22,(char)-97,(char)-24,63,38,(char)-18,(char)-16,86,(char)-90,65,(char)-103,(char)-9,84,(char)-127,
    48,103,(char)-46,(char)-35,(char)-71,33,(char)-2,76,59,(char)-123,26,(char)-108,(char)-29,54,(char)-42,(char)-35,(char)-72,(char)-48,(char)-109,(char)-8,(char)-79,48,58,(char)-102,(char)-11,113,17,(char)-92,
    123,19,(char)-21,(char)-44,(char)-71,115,60,(char)-39,75,(char)-120,(char)-56,4,(char)-39,108,13,(char)-8,80,(char)-78,67,(char)-54,(char)-63,(char)-74,116,(char)-96,2,113,110,73,66,56,31,116,(char)-117,
    (char)-34,(char)-37,18,75,34,108,(char)-111,(char)-51,123,106,31,(char)-105,61,(char)-83,(char)-2,49,45,92,(char)-13,86,102,(char)-84,62,7,(char)-17,57,103,(char)-26,(char)-44,(char)-113,6,0,(char)-72,
    20,76,18,(char)-119,84,(char)-106,24,38,80,7,2,101,15,(char)-126,(char)-71,27,(char)-28,94,41,(char)-68,101,99,50,(char)-67,112,100,(char)-109,5,(char)-57,93,(char)-33,120,(char)-47,8,1,(char)-13,(char)-32,
    71,96,(char)-124,51,(char)-16,(char)-78,80,14,47,(char)-46,105,126,83,(char)-54,(char)-82,(char)-17,6,(char)-115,(char)-81,98,(char)-112,(char)-74,(char)-67,(char)-26,(char)-91,(char)-83,10,31,45,(char)-88,
    (char)-37,76,98,(char)-5,56,(char)-79,(char)-71,(char)-14,0,98,(char)-62,(char)-58,(char)-90,97,4,11,35,(char)-70,61,21,113,(char)-68,55,(char)-52,(char)-86,(char)-22,94,(char)-18,94,70,(char)-117,(char)-94,
    (char)-95,111,4,113,61,(char)-68,57,33,(char)-128,105,(char)-36,68,45,116,(char)-56,63,85,(char)-86,(char)-51,123,(char)-118,(char)-21,(char)-60,(char)-10,58,(char)-8,(char)-26,(char)-48,12,16,51,38,
    26,(char)-110,(char)-51,37,(char)-26,(char)-71,(char)-60,114,45,44,(char)-29,(char)-124,(char)-51,85,(char)-73,(char)-128,26,21,86,86,120,83,97,(char)-22,(char)-118,2,66,(char)-48,(char)-84,(char)-2,
    58,49,(char)-67,14,(char)-48,2,(char)-85,15,6,106,(char)-25,(char)-44,(char)-53,(char)-23,32,(char)-7,106,73,44,115,(char)-15,44,84,92,(char)-106,(char)-125,(char)-102,72,46,30,67,(char)-97,(char)-33,
    (char)-63,105,115,22,(char)-92,73,27,(char)-121,(char)-42,114,43,(char)-117,(char)-46,(char)-16,(char)-31,(char)-60,(char)-115,(char)-11,83,(char)-69,(char)-69,50,(char)-128,(char)-92,(char)-76,(char)-66,
    (char)-73,(char)-28,(char)-63,123,(char)-99,(char)-5,(char)-9,88,(char)-110,1,(char)-76,121,71,(char)-52,(char)-68,56,(char)-71,90,(char)-66,(char)-52,20,13,93,7,78,76,61,121,64,90,61,(char)-108,6,(char)-99,
    (char)-29,65,25,(char)-115,(char)-25,(char)-101,(char)-87,126,56,5,(char)-58,(char)-35,(char)-113,114,89,36,34,81,(char)-10,(char)-111,51,27,(char)-111,57,(char)-85,118,(char)-127,(char)-84,(char)-119,
    0,(char)-72,10,80,(char)-117,35,(char)-17,78,81,(char)-43,32,113,16,65,1,82,(char)-105,49,58,26,47,50,11,66,83,(char)-36,(char)-101,(char)-77,85,(char)-127,(char)-45,71,(char)-120,(char)-59,49,59,114,
    118,2,(char)-99,104,89,27,92,104,(char)-39,31,(char)-81,(char)-66,(char)-93,(char)-97,25,38,(char)-72,(char)-112,(char)-79,32,(char)-43,29,116,(char)-65,56,72,117,77,(char)-49,57,117,(char)-94,51,(char)-98,
    (char)-67,40,46,127,(char)-80,(char)-83,74,37,45,(char)-5,(char)-8,98,(char)-123,(char)-51,(char)-76,(char)-29,(char)-7,(char)-123,(char)-107,86,(char)-66,85,(char)-47,70,(char)-56,(char)-96,99,89,54,
    3,92,73,49,(char)-27,107,88,67,92,(char)-32,29,12,(char)-8,(char)-4,(char)-92,(char)-42,30,57,(char)-103,96,(char)-45,(char)-112,55,(char)-53,15,114,(char)-71,(char)-80,(char)-115,(char)-11,66,(char)-125,
    (char)-90,(char)-42,(char)-108,(char)-116,(char)-54,(char)-37,79,(char)-74,25,59,(char)-19,(char)-75,99,50,(char)-99,(char)-15,66,9,(char)-107,(char)-51,(char)-103,(char)-106,79,(char)-85,112,82,(char)-30,
    85,99,(char)-89,(char)-27,(char)-9,90,55,40,86,12,(char)-8,13,(char)-64,(char)-6,(char)-78,45,112,(char)-4,(char)-100,(char)-54,76,(char)-35,(char)-100,111,105,105,37,48,87,(char)-10,(char)-64,(char)-20,
    95,7,18,61,61,96,(char)-119,122,61,(char)-113,(char)-121,39,(char)-107,70,(char)-37,(char)-71,64,92,112,71,121,117,(char)-38,(char)-19,89,32,(char)-85,(char)-34,(char)-94,119,21,95,110,(char)-110,(char)-11,
    (char)-59,(char)-81,(char)-79,(char)-79,116,44,66,(char)-123,(char)-62,(char)-47,62,50,(char)-16,30,(char)-68,(char)-65,(char)-5,58,(char)-47,(char)-8,(char)-28,(char)-81,(char)-125,(char)-56,(char)-58,
    (char)-34,9,(char)-28,96,(char)-106,113,(char)-81,106,(char)-87,126,122,(char)-16,(char)-54,(char)-101,97,(char)-111,(char)-36,16,(char)-124,42,(char)-120,(char)-11,(char)-81,(char)-60,60,(char)-87,
    (char)-16,(char)-124,(char)-67,59,(char)-64,(char)-71,119,(char)-95,(char)-79,(char)-40,(char)-34,(char)-28,25,(char)-54,(char)-16,108,60,32,(char)-33,104,(char)-19,45,99,17,117,22,121,(char)-45,29,
    (char)-46,(char)-115,(char)-11,(char)-81,91,(char)-35,104,(char)-115,(char)-123,(char)-123,48,(char)-50,(char)-15,30,(char)-10,65,(char)-126,(char)-21,109,(char)-17,(char)-27,(char)-54,(char)-95,(char)-64,
    (char)-3,(char)-44,(char)-45,35,(char)-60,(char)-10,80,17,14,(char)-117,2,(char)-125,(char)-30,(char)-70,(char)-47,(char)-60,(char)-106,18,103,(char)-6,(char)-100,48,51,(char)-16,58,100,(char)-111,108,
    (char)-22,68,43,65,78,(char)-74,(char)-85,(char)-88,(char)-105,107,(char)-106,(char)-43,(char)-79,(char)-28,75,106,27,(char)-83,6,105,(char)-123,(char)-51,24,(char)-113,(char)-63,(char)-107,28,44,82,
    (char)-72,(char)-127,21,58,75,(char)-45,47,(char)-28,26,55,(char)-115,88,(char)-48,114,43,(char)-29,82,(char)-118,4,78,(char)-80,78,(char)-38,112,34,(char)-65,(char)-110,94,119,67,77,41,85,112,43,(char)-86,
    (char)-72,(char)-92,(char)-108,(char)-118,(char)-4,(char)-89,(char)-75,25,(char)-128,3,(char)-8,(char)-96,(char)-10,67,(char)-38,95,(char)-22,127,89,46,(char)-19,43,70,(char)-4,39,(char)-91,(char)-114,
    39,(char)-111,36,104,83,74,31,(char)-19,(char)-13,66,(char)-16,100,(char)-63,46,(char)-14,(char)-74,22,85,(char)-95,35,18,11,(char)-112,(char)-55,(char)-116,59,(char)-16,70,(char)-7,(char)-68,23,(char)-52,
    81,(char)-6,57,(char)-89,(char)-100,(char)-121,74,36,79,111,96,(char)-50,101,(char)-82,(char)-91,(char)-117,124,91,(char)-97,19,(char)-34,(char)-67,(char)-20,31,(char)-50,(char)-96,17,5,85,36,(char)-35,
    102,72,98,(char)-101,39,23,59,(char)-126,8,(char)-19,37,72,(char)-108,62,117,(char)-81,118,(char)-17,(char)-76,(char)-29,116,(char)-67,24,9,(char)-108,(char)-66,32,59,(char)-84,107,(char)-47,55,(char)-60,
    22,73,8,(char)-76,(char)-121,114,(char)-97,(char)-91,30,(char)-103,(char)-121,66,125,(char)-73,(char)-97,85,58,98,(char)-64,(char)-20,40,(char)-86,(char)-7,37,60,(char)-33,(char)-2,(char)-61,11,(char)-46,
    68,97,(char)-84,38,(char)-22,(char)-94,71,36,(char)-97,(char)-98,100,(char)-81,12,(char)-27,(char)-106,(char)-48,(char)-10,107,7,(char)-86,(char)-13,(char)-100,43,29,105,(char)-127,(char)-43,(char)-23,
    (char)-64,12,(char)-3,55,(char)-126,(char)-46,115,103,82,76,27,58,91,23,28,(char)-4,(char)-44,118,(char)-12,(char)-118,31,(char)-74,60,105,(char)-42,(char)-39,76,115,(char)-59,(char)-32,85,(char)-2,
    (char)-127,15,(char)-19,(char)-1,108,118,(char)-96,(char)-104,(char)-19,75,14,26,87,(char)-45,14,(char)-79,43,62,(char)-96,95,(char)-50,(char)-7,13,(char)-6,(char)-99,(char)-55,57,(char)-52,(char)-125,
    71,52,109,69,(char)-110,107,(char)-110,120,1,22,5,(char)-56,50,(char)-89,87,(char)-82,(char)-94,(char)-118,(char)-6,(char)-75,60,(char)-62,69,(char)-89,123,20,(char)-49,(char)-125,(char)-106,(char)-68,
    (char)-11,113,69,(char)-39,(char)-80,61,24,43,(char)-125,(char)-52,(char)-1,(char)-13,(char)-93,(char)-63,(char)-40,117,120,59,(char)-78,66,4,71,(char)-86,(char)-2,64,53,(char)-48,(char)-27,(char)-71,
    102,56,119,(char)-91,22,(char)-23,(char)-63,36,45,120,9,(char)-124,(char)-106,61,(char)-32,(char)-19,50,(char)-48,18,(char)-95,(char)-128,92,90,(char)-30,(char)-46,29,122,11,(char)-120,123,(char)-93,
    47,(char)-17,81,84,92,(char)-51,(char)-75,127,(char)-125,98,39,81,(char)-118,(char)-88,(char)-112,(char)-122,99,56,(char)-110,(char)-99,(char)-88,61,49,59,(char)-28,(char)-50,(char)-39,(char)-4,(char)-47,
    29,(char)-90,54,(char)-117,116,94,90,78,58,(char)-116,(char)-105,64,26,(char)-27,81,41,36,(char)-89,(char)-108,(char)-104,(char)-62,120,(char)-39,(char)-50,1,(char)-67,(char)-98,(char)-106,11,(char)-24,
    25,40,(char)-88,28,(char)-70,26,(char)-86,(char)-125,10,78,(char)-90,29,89,(char)-80,(char)-111,65,52,92,75,70,90,(char)-128,(char)-108,32,61,(char)-3,120,(char)-39,30,(char)-59,(char)-125,(char)-96,
    (char)-127,7,(char)-34,60,(char)-96,77,(char)-44,7,(char)-115,91,(char)-39,89,(char)-88,(char)-111,(char)-94,(char)-106,(char)-53,106,(char)-58,(char)-85,(char)-62,66,67,(char)-125,12,(char)-75,33,4,
    18,124,88,(char)-72,(char)-87,81,84,(char)-111,83,(char)-70,(char)-86,(char)-19,51,116,(char)-38,72,(char)-64,(char)-95,61,119,64,(char)-69,(char)-30,(char)-37,6,117,(char)-22,77,(char)-85,(char)-54,
    52,73,(char)-91,124,(char)-71,(char)-93,(char)-54,(char)-122,(char)-41,(char)-55,14,91,(char)-101,(char)-104,(char)-14,121,(char)-64,(char)-60,54,(char)-64,124,(char)-5,6,(char)-104,9,50,27,(char)-107,
    (char)-122,(char)-4,53,22,84,(char)-44,(char)-41,(char)-11,38,(char)-57,25,40,(char)-17,86,25,(char)-38,(char)-29,(char)-31,(char)-61,85,(char)-121,110,72,112,(char)-93,(char)-92,63,63,(char)-19,72,
    76,(char)-46,(char)-2,67,57,(char)-89,14,96,(char)-66,(char)-35,(char)-40,59,10,(char)-23,(char)-45,(char)-88,90,(char)-127,4,(char)-11,(char)-115,81,(char)-103,(char)-64,(char)-46,(char)-80,(char)-29,
    (char)-6,102,(char)-37,52,(char)-37,11,(char)-71,(char)-22,(char)-80,82,66,(char)-84,(char)-18,(char)-7,109,85,(char)-10,(char)-55,46,31,(char)-25,115,94,45,(char)-61,(char)-77,54,(char)-67,(char)-105,
    (char)-74,(char)-118,(char)-62,(char)-66,32,(char)-115,(char)-28,(char)-38,(char)-104,34,(char)-77,(char)-71,57,(char)-113,(char)-30,94,98,127,(char)-108,82,52,30,70,65,78,(char)-46,(char)-25,70,44,
    0,84,(char)-72,(char)-56,(char)-106,14,(char)-108,(char)-76,(char)-91,35,(char)-91,25,(char)-9,3,(char)-39,6,(char)-89,(char)-103,12,35,(char)-103,79,(char)-81,67,(char)-44,(char)-100,45,(char)-107,
    (char)-123,(char)-54,63,(char)-93,(char)-77,101,114,(char)-16,111,115,72,(char)-41,(char)-62,(char)-52,(char)-62,(char)-73,(char)-45,126,(char)-1,81,46,28,50,51,(char)-77,(char)-36,69,(char)-23,(char)-88,
    (char)-85,65,(char)-14,(char)-57,32,(char)-95,72,(char)-93,58,99,103,(char)-71,(char)-120,(char)-97,63,76,102,(char)-79,5,72,(char)-58,0,105,(char)-31,76,(char)-119,56,(char)-24,92,57,(char)-4,35,(char)-100,
    (char)-9,(char)-94,(char)-18,(char)-32,55,1,123,(char)-61,71,(char)-100,12,58,(char)-93,97,(char)-36,(char)-74,(char)-50,58,(char)-18,41,1,(char)-41,100,(char)-128,81,125,32,17,51,(char)-37,98,(char)-120,
    (char)-105,(char)-16,55,(char)-101,(char)-79,(char)-96,125,(char)-78,86,58,99,23,5,111,86,19,(char)-93,(char)-109,121,37,(char)-48,(char)-21,(char)-14,(char)-105,42,(char)-76,(char)-67,25,57,10,(char)-119,
    (char)-126,(char)-13,(char)-83,88,(char)-53,87,(char)-96,(char)-76,102,(char)-89,42,20,(char)-93,106,(char)-86,(char)-81,80,33,(char)-94,(char)-2,(char)-97,101,(char)-7,60,(char)-81,109,24,(char)-72,
    41,(char)-77,(char)-103,17,35,127,92,(char)-109,103,(char)-24,94,55,(char)-109,(char)-102,42,70,5,(char)-10,(char)-76,34,(char)-61,(char)-102,90,28,66,42,(char)-55,(char)-94,38,(char)-32,82,121,70,51,
    (char)-116,(char)-48,101,53,(char)-36,43,(char)-11,8,92,20,(char)-91,13,(char)-101,36,26,74,(char)-25,(char)-76,124,58,(char)-44,50,(char)-83,102,(char)-77,(char)-127,54,(char)-122,36,(char)-29,(char)-107,
    (char)-54,(char)-80,79,(char)-70,39,(char)-19,122,(char)-36,(char)-1,(char)-101,125,(char)-114,17,72,54,86,50,(char)-44,(char)-83,(char)-61,45,13,21,(char)-101,(char)-76,35,(char)-85,(char)-83,(char)-99,
    (char)-83,(char)-84,(char)-15,(char)-13,(char)-87,17,(char)-40,37,33,(char)-98,(char)-29,(char)-88,22,(char)-90,(char)-54,88,(char)-67,(char)-76,(char)-86,97,(char)-86,12,41,43,(char)-44,60,(char)-88,
    104,(char)-54,17,(char)-67,(char)-72,124,115,115,54,82,(char)-77,(char)-99,(char)-74,(char)-17,60,(char)-126,46,(char)-82,(char)-35,(char)-45,(char)-95,(char)-116,(char)-48,9,9,(char)-113,(char)-91,
    62,(char)-104,(char)-1,27,7,(char)-51,81,109,(char)-35,84,90,24,(char)-128,53,67,17,(char)-88,4,(char)-34,6,(char)-51,29,11,100,(char)-89,(char)-26,(char)-66,(char)-11,(char)-97,52,3,44,(char)-18,(char)-98,
    63,59,(char)-70,(char)-58,(char)-23,98,(char)-97,112,117,106,(char)-23,126,74,69,(char)-95,(char)-77,73,(char)-125,(char)-86,(char)-59,53,(char)-120,82,33,42,104,(char)-115,26,86,(char)-37,99,73,(char)-43,
    46,(char)-40,89,(char)-25,(char)-100,52,44,56,105,94,(char)-27,59,14,112,(char)-82,43,93,57,21,(char)-121,(char)-125,57,(char)-2,37,84,(char)-124,(char)-113,14,85,86,(char)-83,(char)-55,(char)-10,42,
    59,82,(char)-37,(char)-106,(char)-122,14,106,59,92,(char)-83,(char)-47,(char)-46,(char)-15,(char)-37,14,(char)-59,71,(char)-82,101,(char)-65,(char)-56,113,(char)-28,14,(char)-80,(char)-27,122,43,(char)-53,
    (char)-111,50,(char)-105,103,(char)-69,(char)-120,111,52,97,93,(char)-82,(char)-52,102,90,(char)-121,(char)-26,108,72,16,(char)-57,(char)-114,102,107,(char)-3,35,(char)-85,58,(char)-23,34,(char)-90,
    (char)-103,75,86,(char)-35,(char)-8,(char)-19,(char)-59,45,(char)-113,16,51,(char)-75,(char)-125,(char)-110,(char)-1,14,59,43,(char)-46,120,(char)-86,96,(char)-23,82,(char)-21,(char)-48,(char)-52,29,
    47,122,110,15,(char)-109,106,52,(char)-51,75,76,111,(char)-57,(char)-77,(char)-67,125,118,6,44,(char)-93,100,(char)-37,17,(char)-116,(char)-123,(char)-95,(char)-109,24,57,59,43,(char)-93,98,103,3,(char)-39,
    59,(char)-118,(char)-3,(char)-91,(char)-26,(char)-5,(char)-35,(char)-43,(char)-39,(char)-46,(char)-9,(char)-77,(char)-5,(char)-66,(char)-21,3,127,114,(char)-89,(char)-24,(char)-48,97,22,(char)-127,42,
    (char)-89,52,(char)-41,108,(char)-86,(char)-75,119,54,9,51,63,101,(char)-117,52,105,(char)-126,(char)-68,37,(char)-15,(char)-8,55,(char)-87,(char)-123,(char)-11,(char)-123,110,(char)-68,(char)-55,64,
    (char)-105,(char)-112,56,(char)-7,(char)-38,56,79,(char)-109,73,(char)-109,(char)-54,(char)-120,79,(char)-101,13,74,(char)-98,(char)-69,68,40,126,105,99,105,125,(char)-17,(char)-44,(char)-87,12,43,60,
    119,(char)-126,(char)-126,93,(char)-41,(char)-77,12,54,(char)-40,(char)-112,127,115,(char)-32,(char)-2,(char)-59,79,38,118,4,107,(char)-56,(char)-6,23,40,66,101,94,70,(char)-74,(char)-98,88,(char)-87,
    80,(char)-112,42,(char)-77,13,(char)-98,20,(char)-87,(char)-75,88,3,(char)-65,(char)-67,(char)-83,102,(char)-15,104,(char)-43,5,(char)-23,(char)-44,124,106,122,(char)-63,88,(char)-70,72,(char)-57,(char)-51,
    (char)-20,(char)-87,44,(char)-45,(char)-127,(char)-115,(char)-71,(char)-79,(char)-82,69,(char)-28,122,(char)-31,(char)-102,(char)-111,(char)-107,(char)-104,29,(char)-105,(char)-63,75,9,(char)-120,66,
    101,(char)-7,41,74,3,(char)-91,66,(char)-95,(char)-92,86,42,12,(char)-34,20,(char)-79,8,78,(char)-55,(char)-42,(char)-110,10,(char)-91,(char)-116,(char)-55,73,(char)-44,(char)-126,57,63,(char)-46,69,
    86,110,(char)-26,64,101,(char)-67,54,(char)-25,46,(char)-91,107,78,45,53,40,45,(char)-108,106,52,(char)-23,(char)-127,91,(char)-73,109,(char)-30,126,104,127,(char)-67,72,(char)-94,(char)-55,30,(char)-124,
    46,(char)-81,(char)-70,(char)-52,(char)-26,(char)-21,(char)-53,126,47,(char)-32,101,57,(char)-90,89,(char)-90,79,106,(char)-48,40,(char)-103,(char)-121,(char)-58,(char)-121,(char)-7,90,(char)-79,105,
    (char)-58,113,(char)-88,119,(char)-94,28,105,42,78,(char)-29,48,39,(char)-77,(char)-77,118,7,19,(char)-9,81,75,13,29,(char)-76,125,4,77,122,42,94,(char)-93,(char)-76,36,(char)-77,(char)-61,91,119,(char)-20,
    88,86,114,61,60,(char)-15,81,74,(char)-101,(char)-65,(char)-64,98,(char)-47,(char)-110,(char)-30,(char)-108,(char)-69,68,124,(char)-86,75,(char)-27,48,(char)-40,(char)-103,7,(char)-35,(char)-14,89,28,
    2,(char)-89,71,46,31,(char)-21,(char)-44,46,80,46,(char)-74,(char)-5,(char)-34,(char)-42,(char)-106,(char)-2,(char)-107,124,(char)-94,(char)-68,(char)-70,(char)-60,(char)-22,82,24,21,(char)-112,60,(char)-23,
    (char)-66,(char)-44,(char)-109,92,(char)-19,2,(char)-42,78,(char)-38,(char)-63,(char)-124,(char)-88,(char)-124,(char)-125,52,80,41,20,127,100,(char)-30,8,20,114,116,27,(char)-76,31,84,(char)-51,(char)-8,
    2,33,(char)-95,92,47,(char)-69,(char)-13,(char)-34,(char)-119,(char)-95,41,35,(char)-35,85,80,(char)-5,(char)-36,117,29,85,0,111,89,93,113,(char)-13,(char)-36,(char)-21,(char)-52,(char)-54,(char)-77,
    (char)-95,(char)-7,(char)-82,3,46,(char)-32,(char)-44,(char)-19,46,(char)-38,13,(char)-95,(char)-6,(char)-8,(char)-98,(char)-18,7,18,(char)-108,69,(char)-114,58,52,(char)-52,98,31,(char)-44,33,(char)-81,
    41,56,42,102,(char)-98,(char)-69,(char)-38,82,(char)-96,93,79,39,119,51,39,(char)-68,26,(char)-47,(char)-56,(char)-77,87,6,84,(char)-96,92,48,107,81,0,43,84,(char)-30,77,(char)-111,27,72,21,74,5,(char)-87,
    (char)-46,(char)-62,97,(char)-109,(char)-96,41,(char)-107,10,101,74,(char)-128,65,12,(char)-23,114,(char)-100,(char)-80,5,(char)-114,118,(char)-35,107,(char)-71,(char)-96,4,(char)-23,(char)-126,75,115,
    65,120,(char)-41,(char)-11,0,61,9,84,(char)-18,(char)-115,96,(char)-41,(char)-59,118,6,(char)-68,108,108,(char)-70,(char)-91,19,(char)-105,66,(char)-65,76,71,(char)-1,(char)-109,15,(char)-23,77,(char)-82,
    114,61,(char)-20,(char)-44,60,62,11,13,1,126,104,78,(char)-121,(char)-92,(char)-73,(char)-60,(char)-112,(char)-105,(char)-82,(char)-108,(char)-37,(char)-104,(char)-100,(char)-72,(char)-33,(char)-41,
    126,(char)-118,113,(char)-123,(char)-10,(char)-65,82,(char)-46,(char)-104,(char)-87,(char)-65,(char)-114,(char)-75,(char)-58,(char)-80,13,(char)-43,44,3,111,(char)-18,(char)-113,21,(char)-39,84,(char)-123,
    58,61,(char)-83,85,11,19,80,123,(char)-41,(char)-61,88,(char)-19,99,115,(char)-127,62,91,(char)-94,17,89,(char)-74,66,83,(char)-79,16,118,(char)-106,125,(char)-17,31,(char)-48,31,19,121,(char)-122,(char)-32,
    (char)-121,92,(char)-127,68,(char)-123,(char)-79,(char)-43,46,(char)-108,11,124,(char)-34,(char)-113,(char)-52,49,(char)-33,118,(char)-2,(char)-118,(char)-78,(char)-26,25,78,(char)-111,0,1,105,(char)-52,
    58,20,(char)-100,119,40,63,(char)-49,(char)-62,(char)-6,(char)-44,(char)-21,29,(char)-56,(char)-38,(char)-100,33,76,(char)-18,80,45,80,28,65,(char)-113,(char)-4,91,102,111,80,120,80,(char)-39,30,91,
    67,(char)-47,(char)-63,127,(char)-114,5,(char)-106,121,(char)-69,(char)-77,114,80,(char)-78,(char)-100,(char)-36,110,112,13,(char)-23,(char)-6,(char)-38,49,(char)-45,117,99,5,(char)-63,6,117,103,(char)-10,
    94,(char)-2,(char)-20,(char)-10,(char)-34,(char)-74,(char)-99,64,(char)-123,66,(char)-64,99,(char)-73,(char)-75,105,(char)-95,(char)-27,(char)-92,(char)-49,(char)-125,(char)-64,(char)-45,(char)-84,(char)-45,
    (char)-17,97,(char)-54,(char)-69,57,(char)-36,(char)-4,84,7,(char)-105,5,(char)-105,(char)-69,46,(char)-127,(char)-68,(char)-31,87,(char)-66,(char)-86,114,(char)-73,74,110,(char)-4,15,57,104,(char)-75,
    (char)-103,(char)-103,56,115,10,68,76,11,125,107,(char)-54,(char)-46,8,37,(char)-23,98,(char)-115,40,107,(char)-51,(char)-65,11,52,(char)-30,(char)-61,(char)-68,84,(char)-34,17,(char)-15,(char)-62,(char)-97,
    (char)-127,54,116,(char)-22,(char)-60,(char)-31,(char)-95,33,(char)-114,105,42,79,(char)-97,(char)-116,75,(char)-72,96,36,(char)-9,116,71,28,35,(char)-25,(char)-70,38,(char)-43,(char)-75,(char)-43,(char)-87,
    72,(char)-71,(char)-28,99,17,(char)-35,61,70,(char)-14,(char)-59,120,92,(char)-51,8,74,(char)-122,79,76,(char)-99,52,52,56,72,17,(char)-90,(char)-98,56,108,(char)-126,(char)-31,(char)-10,(char)-6,(char)-122,
    122,122,(char)-7,(char)-62,99,(char)-46,(char)-128,111,100,(char)-101,(char)-39,(char)-9,(char)-121,(char)-61,(char)-20,120,(char)-25,51,3,(char)-33,(char)-56,126,(char)-77,(char)-1,15,(char)-125,(char)-39,
    (char)-96,0,7,(char)-118,(char)-48,51,58,109,124,122,91,(char)-45,(char)-66,(char)-83,(char)-69,(char)-57,13,79,(char)-31,27,(char)-109,89,9,91,(char)-116,(char)-28,(char)-98,(char)-38,(char)-120,(char)-85,
    36,(char)-40,49,(char)-87,(char)-82,65,28,(char)-23,117,(char)-82,103,65,(char)-42,28,(char)-115,105,(char)-119,(char)-18,105,(char)-127,3,(char)-20,(char)-110,71,41,(char)-32,17,88,20,37,82,(char)-89,
    1,(char)-80,110,(char)-101,104,(char)-101,110,(char)-5,83,15,30,15,(char)-93,51,8,(char)-58,114,43,(char)-106,(char)-19,(char)-86,113,71,90,(char)-100,81,16,(char)-22,(char)-4,90,66,(char)-90,82,(char)-83,
    31,(char)-4,(char)-57,12,(char)-62,60,(char)-107,(char)-99,79,(char)-32,17,(char)-72,1,89,(char)-40,126,(char)-23,99,(char)-43,(char)-69,(char)-108,(char)-65,(char)-105,84,(char)-128,(char)-18,17,(char)-72,
    (char)-43,106,6,(char)-89,(char)-117,31,98,22,87,127,(char)-115,(char)-104,(char)-121,37,(char)-10,(char)-111,(char)-122,(char)-15,(char)-62,64,(char)-75,(char)-65,(char)-38,17,59,88,(char)-17,8,114,
    (char)-38,70,(char)-70,94,(char)-103,(char)-126,(char)-52,115,20,(char)-64,10,85,23,(char)-14,(char)-78,11,113,51,(char)-67,30,92,(char)-113,(char)-126,(char)-61,(char)-90,70,83,122,60,(char)-34,(char)-108,
    (char)-128,3,(char)-87,76,(char)-52,(char)-123,(char)-123,81,(char)-80,(char)-72,77,20,(char)-27,41,92,(char)-56,92,123,60,(char)-94,54,(char)-15,7,96,4,(char)-66,(char)-111,(char)-1,(char)-54,(char)-85,
    (char)-22,74,75,(char)-7,(char)-64,(char)-27,(char)-101,(char)-45,90,55,(char)-118,55,(char)-16,69,(char)-4,13,(char)-30,(char)-115,(char)-32,(char)-9,(char)-2,78,61,(char)-38,(char)-34,20,(char)-125,
    77,(char)-62,(char)-9,37,78,118,(char)-60,123,73,(char)-82,46,31,(char)-87,11,(char)-49,91,122,120,85,(char)-62,54,120,(char)-79,111,79,94,85,110,(char)-48,14,118,(char)-55,(char)-90,(char)-12,(char)-26,
    1,67,(char)-20,(char)-1,83,14,(char)-46,67,75,(char)-70,(char)-96,(char)-86,99,(char)-45,125,124,(char)-107,(char)-51,(char)-87,87,9,(char)-105,25,67,(char)-19,97,105,16,109,49,(char)-98,82,54,(char)-87,
    124,12,(char)-36,38,(char)-91,(char)-82,63,(char)-107,(char)-61,(char)-43,24,(char)-83,(char)-39,114,(char)-18,92,115,(char)-104,37,44,77,79,91,71,(char)-96,(char)-106,79,110,4,(char)-98,(char)-106,
    86,(char)-72,(char)-11,55,51,(char)-61,12,(char)-6,(char)-121,(char)-5,(char)-123,104,(char)-25,(char)-78,70,62,(char)-65,113,25,(char)-38,41,(char)-20,(char)-121,23,56,34,(char)-38,57,118,47,(char)-61,
    104,(char)-12,50,56,(char)-10,(char)-120,118,(char)-57,2,(char)-16,(char)-41,(char)-24,112,65,69,(char)-111,37,8,109,9,(char)-86,40,(char)-14,(char)-114,14,3,(char)-97,100,(char)-40,(char)-118,95,16,
    113,13,83,(char)-72,(char)-75,(char)-92,112,43,38,(char)-30,26,126,(char)-127,89,50,60,14,(char)-18,(char)-61,(char)-64,125,64,85,(char)-22,(char)-98,91,(char)-72,20,85,(char)-78,52,111,110,(char)-47,
    (char)-13,(char)-54,91,(char)-98,82,87,(char)-113,119,30,(char)-54,51,47,(char)-73,(char)-89,(char)-24,54,12,2,98,(char)-34,12,(char)-108,85,(char)-18,(char)-127,(char)-41,(char)-96,(char)-68,107,42,
    (char)-9,(char)-108,(char)-68,25,(char)-120,1,105,(char)-97,(char)-84,115,83,(char)-58,(char)-101,111,89,(char)-65,77,89,(char)-83,(char)-120,38,(char)-37,(char)-83,(char)-47,(char)-90,104,(char)-113,
    117,(char)-69,(char)-7,68,0,36,64,37,(char)-45,95,(char)-48,(char)-74,(char)-60,37,(char)-60,109,(char)-95,61,(char)-2,(char)-31,1,(char)-113,(char)-7,(char)-84,48,104,(char)-88,31,(char)-75,127,(char)-21,
    (char)-116,(char)-42,(char)-3,102,(char)-112,124,(char)-51,(char)-5,(char)-21,85,(char)-74,(char)-90,76,18,(char)-90,(char)-46,64,26,53,(char)-92,70,91,112,107,(char)-104,(char)-59,54,(char)-100,80,
    (char)-28,34,84,(char)-31,(char)-51,25,105,9,(char)-6,5,(char)-13,(char)-40,(char)-57,55,77,0,(char)-31,(char)-117,(char)-52,38,(char)-75,48,72,102,(char)-112,102,(char)-54,32,94,(char)-34,(char)-97,
    39,(char)-13,(char)-71,(char)-38,88,22,(char)-37,(char)-98,(char)-80,(char)-118,(char)-84,97,82,(char)-11,70,(char)-36,94,(char)-44,41,(char)-33,(char)-118,121,(char)-87,108,(char)-86,(char)-27,69,62,
    65,(char)-62,49,(char)-60,(char)-106,(char)-49,(char)-7,(char)-16,(char)-25,(char)-115,(char)-85,(char)-75,4,(char)-118,(char)-59,(char)-95,83,97,(char)-108,54,(char)-99,61,(char)-41,4,(char)-58,(char)-49,
    (char)-55,(char)-73,(char)-80,127,30,(char)-1,(char)-78,86,95,59,(char)-89,(char)-24,82,(char)-8,22,20,120,(char)-41,34,(char)-117,13,9,90,(char)-4,73,(char)-58,(char)-79,(char)-31,(char)-69,72,14,(char)-95,
    (char)-112,4,63,(char)-56,(char)-93,(char)-56,50,(char)-96,88,(char)-51,(char)-72,15,127,(char)-34,56,(char)-106,(char)-1,47,(char)-39,100,(char)-77,(char)-21,49,122,(char)-69,(char)-35,44,40,(char)-81,
    53,4,39,77,(char)-103,8,50,66,(char)-91,121,116,57,68,(char)-82,84,58,56,(char)-77,77,45,5,(char)-34,(char)-51,13,(char)-77,(char)-13,(char)-19,58,(char)-104,93,(char)-10,82,51,55,71,(char)-90,(char)-31,
    88,9,93,68,39,95,72,(char)-127,47,122,41,98,(char)-125,(char)-55,97,55,98,(char)-116,(char)-38,110,58,60,36,96,48,(char)-86,85,48,(char)-35,122,(char)-2,(char)-102,37,(char)-76,25,(char)-80,66,21,(char)-123,
    (char)-60,83,68,51,(char)-53,100,(char)-49,125,(char)-125,105,83,(char)-61,114,87,103,73,125,61,59,92,114,87,(char)-7,54,(char)-52,27,123,(char)-82,(char)-119,69,52,(char)-33,(char)-12,17,(char)-27,
    6,75,(char)-82,70,(char)-98,32,119,107,(char)-84,86,55,(char)-124,(char)-49,(char)-50,(char)-51,30,(char)-56,(char)-112,41,(char)-119,(char)-112,(char)-104,50,(char)-75,90,19,0,108,11,36,14,59,56,(char)-77,
    (char)-51,45,(char)-7,12,(char)-43,94,(char)-31,(char)-38,(char)-127,(char)-115,80,67,116,8,(char)-124,100,(char)-8,(char)-63,(char)-97,(char)-64,(char)-4,4,(char)-2,(char)-40,(char)-52,(char)-40,111,
    (char)-6,(char)-99,80,(char)-114,(char)-127,(char)-128,74,(char)-42,(char)-89,97,(char)-9,(char)-35,0,(char)-18,93,116,43,(char)-101,113,41,76,(char)-109,(char)-104,66,(char)-87,88,125,(char)-86,66,
    67,25,(char)-107,(char)-91,(char)-78,13,44,76,(char)-44,36,(char)-16,67,106,65,(char)-109,(char)-87,0,93,(char)-115,(char)-84,36,(char)-11,(char)-22,(char)-84,(char)-87,5,93,(char)-53,122,(char)-58,
    45,40,(char)-11,86,(char)-93,(char)-81,123,(char)-108,28,3,75,67,(char)-29,14,94,125,(char)-112,98,98,69,74,(char)-116,42,112,(char)-86,121,(char)-126,(char)-83,90,(char)-96,(char)-84,(char)-58,46,(char)-81,
    (char)-19,32,(char)-82,(char)-54,46,(char)-51,79,(char)-53,(char)-106,(char)-61,41,98,77,(char)-110,79,(char)-44,97,(char)-52,68,47,(char)-65,(char)-105,(char)-95,48,86,55,55,27,107,20,9,(char)-95,(char)-19,
    31,109,(char)-24,36,(char)-113,(char)-95,(char)-36,85,96,(char)-101,(char)-67,116,90,86,(char)-92,(char)-97,105,(char)-79,48,(char)-27,19,28,35,(char)-12,1,(char)-58,114,(char)-58,0,(char)-99,(char)-71,
    81,(char)-89,(char)-48,109,4,43,(char)-48,48,21,6,(char)-39,33,(char)-106,39,22,16,106,(char)-55,(char)-73,(char)-128,(char)-20,(char)-115,31,(char)-80,121,(char)-45,17,(char)-40,24,44,34,(char)-3,(char)-31,
    13,(char)-22,(char)-111,(char)-76,(char)-125,9,(char)-124,(char)-98,(char)-103,(char)-126,(char)-111,93,(char)-125,84,21,(char)-128,89,(char)-45,(char)-38,(char)-30,116,0,61,26,111,(char)-117,7,(char)-30,
    (char)-25,58,81,(char)-36,52,16,(char)-5,(char)-36,45,120,103,118,73,47,13,(char)-70,66,(char)-54,(char)-20,37,(char)-85,(char)-92,10,78,(char)-73,60,(char)-40,108,(char)-105,30,124,21,115,28,(char)-102,
    44,(char)-90,115,(char)-127,37,75,(char)-35,(char)-119,(char)-5,53,35,(char)-27,87,(char)-58,19,(char)-7,(char)-42,(char)-13,43,(char)-124,121,(char)-116,103,(char)-61,(char)-70,8,122,22,(char)-105,
    98,(char)-56,(char)-127,(char)-120,41,85,(char)-50,(char)-43,30,100,(char)-71,22,(char)-118,(char)-112,105,(char)-91,89,114,(char)-93,(char)-26,(char)-102,5,(char)-73,48,107,(char)-116,37,25,15,(char)-47,
    76,(char)-60,(char)-21,91,56,(char)-31,120,113,(char)-115,(char)-121,(char)-10,24,(char)-58,118,81,98,38,(char)-9,(char)-56,(char)-38,(char)-83,69,107,115,(char)-74,(char)-37,(char)-68,89,(char)-43,
    (char)-108,(char)-79,106,(char)-81,58,(char)-62,10,(char)-89,(char)-52,(char)-19,57,107,(char)-84,(char)-59,121,(char)-88,37,(char)-126,(char)-71,17,(char)-50,86,(char)-68,38,(char)-89,(char)-49,14,
    34,(char)-91,47,103,(char)-19,111,20,(char)-18,(char)-11,(char)-55,(char)-100,(char)-23,73,(char)-54,(char)-21,(char)-127,(char)-41,(char)-107,96,(char)-20,(char)-57,119,(char)-116,(char)-54,57,(char)-52,
    53,52,21,110,(char)-32,(char)-85,44,34,47,70,49,(char)-12,84,(char)-56,(char)-95,116,64,(char)-13,(char)-93,(char)-56,(char)-120,(char)-70,(char)-120,(char)-100,43,(char)-5,111,35,78,69,91,(char)-61,
    (char)-36,(char)-82,60,98,79,(char)-97,(char)-16,14,(char)-3,(char)-120,(char)-52,(char)-20,103,(char)-96,24,(char)-3,76,(char)-14,35,(char)-12,(char)-69,9,(char)-23,(char)-10,35,74,(char)-112,(char)-12,
    (char)-111,(char)-74,(char)-37,27,(char)-1,(char)-21,(char)-95,(char)-81,3,(char)-61,21,(char)-52,(char)-119,63,49,(char)-35,(char)-60,(char)-55,96,(char)-49,(char)-84,(char)-56,(char)-65,(char)-61,
    (char)-90,(char)-75,44,(char)-45,(char)-106,(char)-14,(char)-125,14,108,(char)-84,(char)-44,(char)-70,(char)-13,61,(char)-80,(char)-84,125,(char)-106,(char)-44,(char)-120,(char)-23,(char)-57,120,2,(char)-34,
    49,(char)-6,124,(char)-84,110,82,(char)-60,73,88,21,58,(char)-33,15,(char)-64,(char)-52,(char)-103,110,(char)-104,14,48,11,107,12,53,122,108,(char)-4,(char)-106,8,126,14,(char)-96,(char)-97,51,(char)-101,
    (char)-29,(char)-39,(char)-100,(char)-4,100,(char)-10,47,(char)-19,(char)-73,24,(char)-103,(char)-1,(char)-91,51,26,(char)-40,111,(char)-35,9,102,(char)-105,(char)-112,(char)-105,105,(char)-122,(char)-47,
    53,116,93,90,(char)-16,(char)-34,(char)-104,(char)-9,89,75,(char)-82,60,65,6,(char)-66,(char)-112,(char)-48,94,(char)-111,64,(char)-8,123,(char)-82,(char)-117,114,(char)-97,(char)-22,(char)-26,(char)-13,
    89,(char)-68,(char)-1,(char)-118,(char)-111,(char)-5,(char)-69,(char)-45,20,(char)-23,(char)-117,52,1,86,(char)-76,(char)-86,(char)-127,(char)-19,(char)-14,112,(char)-58,(char)-39,109,(char)-100,(char)-15,
    26,62,76,(char)-123,2,28,54,123,74,96,(char)-92,(char)-97,(char)-123,(char)-34,20,(char)-91,(char)-106,86,32,(char)-108,11,35,(char)-84,112,10,43,116,60,(char)-124,(char)-46,114,(char)-77,(char)-85,
    44,(char)-57,(char)-99,83,106,(char)-30,93,64,(char)-38,6,112,(char)-8,(char)-44,(char)-11,(char)-23,(char)-103,(char)-52,(char)-59,(char)-23,68,(char)-59,29,98,(char)-128,(char)-102,13,88,5,12,45,(char)-114,
    (char)-124,(char)-45,(char)-90,(char)-90,(char)-78,72,(char)-38,36,(char)-58,(char)-91,118,36,30,72,(char)-116,(char)-60,(char)-69,(char)-54,39,4,(char)-16,(char)-99,(char)-60,56,(char)-94,(char)-109,
    31,64,88,63,84,32,109,(char)-40,(char)-125,127,(char)-115,77,(char)-75,(char)-79,(char)-83,108,(char)-122,(char)-98,32,(char)-57,38,5,37,97,(char)-23,82,80,(char)-75,(char)-2,55,47,(char)-25,9,(char)-57,
    (char)-5,(char)-37,(char)-44,68,(char)-115,122,38,50,24,(char)-107,(char)-119,(char)-68,41,(char)-64,(char)-115,114,(char)-84,(char)-108,41,40,(char)-32,(char)-90,(char)-89,116,(char)-85,(char)-97,114,
    32,9,(char)-11,99,21,87,50,(char)-109,45,113,(char)-16,77,102,(char)-68,(char)-61,78,123,(char)-7,(char)-29,(char)-94,(char)-122,(char)-39,(char)-79,7,(char)-16,106,(char)-109,70,(char)-79,(char)-103,
    33,27,(char)-6,(char)-114,(char)-71,47,49,(char)-124,(char)-81,72,(char)-37,73,76,7,(char)-84,34,91,110,(char)-87,7,(char)-122,75,(char)-83,86,(char)-82,5,123,74,90,68,(char)-20,91,(char)-84,108,(char)-42,
    45,(char)-10,99,(char)-33,(char)-128,(char)-43,76,(char)-65,(char)-5,106,13,(char)-99,(char)-66,(char)-26,(char)-43,93,(char)-6,38,(char)-73,(char)-118,(char)-45,(char)-56,102,55,114,(char)-74,(char)-47,
    (char)-64,54,(char)-29,(char)-100,9,(char)-101,78,107,(char)-30,52,(char)-89,85,119,44,119,(char)-92,(char)-39,(char)-119,(char)-102,91,108,115,38,110,1,75,110,127,(char)-93,93,(char)-104,48,115,(char)-122,
    118,65,(char)-54,109,(char)-13,(char)-8,97,(char)-77,32,(char)-124,(char)-92,28,116,(char)-32,87,(char)-30,65,15,10,(char)-90,71,87,39,(char)-8,(char)-34,63,88,(char)-6,(char)-110,(char)-44,(char)-74,
    106,(char)-107,49,(char)-103,32,(char)-71,116,(char)-10,(char)-79,94,41,67,(char)-118,3,(char)-76,(char)-30,(char)-22,89,(char)-24,(char)-20,85,(char)-37,73,7,20,17,(char)-48,3,40,(char)-83,(char)-67,
    (char)-12,(char)-116,(char)-5,(char)-116,13,(char)-72,98,(char)-105,110,51,89,95,(char)-41,15,65,(char)-16,93,(char)-15,(char)-104,(char)-29,(char)-66,(char)-70,(char)-41,(char)-113,(char)-114,75,(char)-99,
    (char)-15,11,109,(char)-44,(char)-18,(char)-70,127,(char)-122,(char)-34,8,99,92,(char)-125,(char)-97,114,43,(char)-50,98,(char)-62,96,15,(char)-128,43,94,(char)-112,(char)-104,(char)-97,(char)-45,1,
    (char)-84,(char)-33,(char)-69,(char)-126,(char)-110,(char)-84,97,(char)-35,(char)-104,12,124,(char)-58,24,(char)-35,(char)-29,(char)-87,1,39,(char)-19,(char)-57,102,65,23,(char)-47,32,(char)-95,(char)-75,
    (char)-82,56,(char)-52,(char)-33,(char)-37,(char)-46,82,(char)-43,91,20,86,(char)-89,(char)-84,81,(char)-93,114,(char)-44,(char)-86,(char)-73,(char)-57,(char)-77,(char)-4,11,55,12,(char)-94,123,(char)-100,
    30,(char)-115,14,85,(char)-77,(char)-39,(char)-104,45,117,(char)-5,(char)-26,(char)-6,116,(char)-8,108,(char)-61,(char)-75,(char)-111,(char)-98,52,43,(char)-54,83,(char)-78,(char)-55,89,28,(char)-30,
    122,(char)-95,(char)-1,(char)-91,(char)-24,23,(char)-3,(char)-117,21,78,58,19,(char)-7,105,68,(char)-9,(char)-84,10,111,(char)-64,(char)-2,22,89,(char)-21,22,(char)-46,117,(char)-36,(char)-16,84,92,
    (char)-71,(char)-34,(char)-79,46,78,(char)-57,(char)-28,(char)-112,(char)-20,103,(char)-68,68,(char)-119,(char)-85,86,25,(char)-16,114,(char)-65,37,35,83,107,(char)-45,(char)-40,(char)-45,99,52,67,(char)-25,
    (char)-12,123,105,(char)-79,(char)-92,103,(char)-35,(char)-105,(char)-18,(char)-96,(char)-89,(char)-70,64,66,(char)-120,27,(char)-43,(char)-88,(char)-27,31,(char)-109,59,(char)-7,16,(char)-70,82,(char)-90,
    (char)-89,47,38,82,61,13,101,37,(char)-127,(char)-43,(char)-54,106,8,(char)-27,(char)-105,(char)-71,95,(char)-10,67,(char)-84,(char)-72,(char)-9,55,(char)-46,(char)-12,(char)-76,(char)-11,36,90,126,
    67,93,85,80,(char)-75,114,124,55,127,(char)-106,54,(char)-4,(char)-27,(char)-98,47,89,(char)-109,36,103,(char)-12,(char)-95,95,126,(char)-42,105,(char)-128,5,26,30,25,(char)-42,(char)-9,30,(char)-21,
    44,(char)-34,(char)-41,(char)-119,(char)-38,(char)-41,89,120,(char)-84,(char)-77,87,(char)-17,122,(char)-55,30,7,(char)-13,66,51,(char)-51,86,(char)-117,(char)-61,(char)-25,(char)-56,83,(char)-119,117,
    27,24,(char)-116,45,(char)-41,(char)-44,(char)-19,63,(char)-40,17,68,(char)-10,109,52,50,75,77,77,117,56,(char)-57,118,74,2,(char)-110,(char)-115,(char)-68,2,110,(char)-20,(char)-21,(char)-38,72,(char)-41,
    (char)-118,(char)-79,91,43,83,72,66,115,(char)-74,30,121,79,43,9,79,(char)-30,77,(char)-36,29,15,(char)-117,(char)-92,103,56,(char)-111,(char)-23,(char)-1,(char)-14,(char)-6,57,124,(char)-56,68,61,74,
    53,(char)-51,51,7,(char)-28,(char)-111,(char)-45,(char)-20,(char)-57,(char)-87,(char)-94,80,107,65,44,(char)-92,(char)-90,(char)-60,17,(char)-54,(char)-55,(char)-15,(char)-92,(char)-25,(char)-29,18,
    (char)-46,127,31,(char)-52,(char)-97,123,37,33,118,126,84,(char)-60,119,(char)-33,(char)-17,10,(char)-41,21,(char)-122,(char)-5,123,(char)-101,(char)-83,(char)-11,(char)-69,0,(char)-50,(char)-97,(char)-16,
    (char)-54,95,103,(char)-39,51,(char)-118,(char)-79,45,(char)-102,121,19,(char)-26,(char)-116,(char)-111,(char)-27,(char)-47,(char)-28,58,82,73,118,54,63,97,50,(char)-99,(char)-99,(char)-58,85,56,(char)-91,
    93,(char)-4,94,109,(char)-23,(char)-46,10,125,81,(char)-95,42,(char)-40,(char)-56,(char)-122,39,(char)-121,(char)-104,84,(char)-59,(char)-22,(char)-18,125,(char)-70,28,(char)-85,(char)-61,(char)-32,
    (char)-56,(char)-79,(char)-22,94,83,(char)-127,58,(char)-35,(char)-124,(char)-74,(char)-32,79,80,110,91,(char)-78,79,50,(char)-107,(char)-52,(char)-101,97,76,62,(char)-83,(char)-76,29,(char)-13,(char)-96,
    (char)-10,(char)-92,5,(char)-72,47,(char)-1,127,(char)-65,54,(char)-63,(char)-109,(char)-33,(char)-119,26,(char)-43,(char)-94,(char)-81,17,12,113,(char)-45,50,12,(char)-34,122,111,(char)-76,(char)-89,
    (char)-67,12,19,62,80,77,28,40,35,(char)-125,99,52,81,(char)-117,4,76,13,(char)-85,22,(char)-103,10,(char)-25,(char)-90,125,(char)-49,48,(char)-60,84,40,21,49,(char)-107,86,(char)-4,(char)-66,(char)-17,
    30,(char)-85,57,68,100,(char)-82,46,(char)-99,105,(char)-75,(char)-116,(char)-19,(char)-108,124,74,54,(char)-15,(char)-14,57,49,(char)-121,77,(char)-113,93,43,(char)-86,(char)-25,9,(char)-47,(char)-96,
    (char)-70,(char)-55,(char)-58,126,(char)-94,(char)-26,(char)-24,(char)-73,(char)-84,(char)-52,115,110,125,82,29,(char)-40,81,90,42,(char)-94,59,117,(char)-30,50,(char)-100,92,11,105,125,94,(char)-107,
    49,89,87,36,(char)-85,(char)-114,(char)-126,(char)-122,(char)-98,71,28,(char)-3,87,(char)-56,53,(char)-73,111,83,(char)-92,103,(char)-105,(char)-123,47,42,124,105,(char)-40,(char)-95,(char)-33,(char)-8,
    118,(char)-107,126,(char)-43,(char)-105,(char)-115,(char)-6,79,(char)-60,73,(char)-43,(char)-51,61,45,64,59,(char)-47,(char)-14,(char)-38,120,23,(char)-48,33,90,(char)-20,(char)-107,(char)-87,37,(char)-76,
    (char)-66,97,(char)-70,(char)-6,(char)-120,(char)-102,61,(char)-21,44,(char)-72,50,(char)-96,(char)-54,(char)-23,82,(char)-69,(char)-104,7,(char)-19,(char)-75,(char)-50,14,(char)-18,(char)-69,67,(char)-2,
    (char)-89,(char)-67,(char)-22,(char)-34,32,(char)-64,10,45,108,(char)-26,56,119,101,87,(char)-83,78,(char)-102,(char)-110,99,98,39,(char)-5,(char)-90,56,(char)-123,(char)-45,110,(char)-18,113,(char)-87,
    (char)-66,(char)-18,(char)-113,125,41,70,(char)-102,77,(char)-110,(char)-72,34,127,(char)-94,34,120,(char)-23,1,101,(char)-22,(char)-124,(char)-95,(char)-63,65,78,(char)-98,26,89,20,(char)-61,71,(char)-120,
    23,(char)-21,(char)-15,116,54,120,(char)-120,17,61,(char)-64,55,2,47,(char)-38,92,94,85,95,84,(char)-60,85,47,(char)-33,(char)-68,(char)-14,(char)-59,102,113,63,79,(char)-60,(char)-21,23,111,6,(char)-27,
    103,46,(char)-70,20,(char)-13,(char)-70,(char)-110,46,(char)-92,66,83,(char)-24,95,14,24,53,(char)-93,34,120,(char)-108,104,(char)-35,(char)-123,10,(char)-52,93,5,(char)-122,76,(char)-126,(char)-58,
    (char)-32,58,(char)-55,(char)-73,(char)-109,(char)-44,(char)-112,(char)-86,66,0,(char)-10,(char)-104,(char)-94,50,(char)-89,(char)-43,(char)-98,(char)-119,(char)-64,5,98,35,(char)-56,52,74,87,110,(char)-114,
    19,(char)-13,(char)-120,(char)-88,(char)-44,(char)-56,121,25,34,(char)-11,(char)-38,50,(char)-53,37,(char)-112,76,97,(char)-40,18,81,(char)-55,76,34,(char)-95,(char)-21,(char)-58,58,(char)-52,9,65,(char)-90,
    (char)-119,(char)-49,(char)-90,(char)-33,(char)-3,(char)-128,(char)-116,(char)-57,(char)-91,(char)-15,109,89,90,(char)-23,(char)-19,(char)-12,(char)-89,55,39,(char)-48,24,(char)-51,(char)-124,64,(char)-87,
    (char)-99,45,4,78,(char)-109,(char)-23,(char)-112,(char)-88,(char)-1,(char)-99,(char)-25,(char)-77,(char)-116,39,51,101,75,81,(char)-45,(char)-114,(char)-12,0,(char)-106,95,(char)-81,(char)-12,(char)-27,
    (char)-70,9,72,(char)-6,(char)-91,(char)-106,59,(char)-73,36,(char)-60,79,121,(char)-54,82,(char)-80,(char)-98,(char)-90,40,(char)-89,(char)-89,(char)-105,68,31,10,9,14,57,20,(char)-3,71,13,14,(char)-82,
    42,(char)-7,57,63,99,(char)-62,24,41,68,(char)-56,21,(char)-68,1,59,55,(char)-76,20,(char)-8,(char)-72,(char)-24,(char)-40,(char)-51,88,(char)-20,(char)-53,(char)-27,(char)-6,117,(char)-57,(char)-59,
    (char)-103,(char)-77,(char)-45,(char)-104,78,(char)-101,45,107,(char)-73,(char)-32,3,(char)-96,(char)-5,69,43,(char)-2,126,(char)-78,(char)-105,77,38,(char)-124,36,6,37,74,40,52,1,(char)-115,(char)-4,
    75,116,80,(char)-36,109,44,119,(char)-10,(char)-94,(char)-28,(char)-46,12,(char)-55,127,105,(char)-108,(char)-98,(char)-109,(char)-88,119,(char)-5,104,108,72,12,12,(char)-101,(char)-5,(char)-77,123,
    (char)-81,(char)-69,31,115,121,92,(char)-75,(char)-94,101,51,(char)-78,117,19,72,(char)-9,11,(char)-58,(char)-35,59,118,42,(char)-100,64,(char)-94,(char)-32,(char)-112,88,5,62,113,71,(char)-73,49,(char)-20,
    (char)-32,21,97,31,(char)-90,3,(char)-3,(char)-61,103,50,(char)-25,(char)-90,64,(char)-88,(char)-45,(char)-128,12,(char)-65,92,103,(char)-108,(char)-45,(char)-101,69,(char)-61,(char)-59,121,(char)-17,
    83,(char)-13,86,(char)-26,(char)-69,(char)-6,23,(char)-48,(char)-40,3,60,33,(char)-92,25,34,40,113,(char)-95,56,37,33,123,(char)-56,64,(char)-35,(char)-52,105,106,105,(char)-57,123,(char)-55,(char)-92,
    36,53,96,(char)-7,(char)-107,106,9,(char)-109,(char)-38,(char)-11,17,(char)-75,95,76,96,92,(char)-51,(char)-52,(char)-68,(char)-54,32,(char)-120,(char)-9,(char)-93,62,118,81,(char)-103,(char)-91,(char)-110,
    (char)-118,51,(char)-124,95,(char)-29,(char)-125,(char)-30,127,37,(char)-32,(char)-12,(char)-8,(char)-91,36,108,98,112,34,(char)-106,(char)-76,54,112,(char)-85,(char)-2,119,(char)-69,109,(char)-25,62,
    (char)-108,(char)-20,(char)-67,(char)-113,39,(char)-32,7,105,(char)-109,(char)-109,(char)-123,(char)-66,(char)-76,109,(char)-31,(char)-73,119,102,(char)-57,67,(char)-73,67,(char)-43,(char)-31,(char)-24,
    (char)-114,(char)-125,110,29,(char)-37,(char)-119,(char)-48,(char)-93,(char)-27,(char)-74,(char)-14,(char)-91,(char)-3,(char)-14,(char)-31,(char)-115,(char)-57,(char)-69,(char)-120,(char)-116,46,123,
    5,(char)-67,66,(char)-105,(char)-111,23,121,(char)-68,111,70,71,65,(char)-47,(char)-15,(char)-111,(char)-9,(char)-45,81,(char)-119,(char)-51,(char)-7,56,49,36,60,122,(char)-11,(char)-53,(char)-96,(char)-21,
    87,104,(char)-115,27,5,107,(char)-113,38,65,73,96,(char)-76,(char)-114,(char)-76,60,46,34,110,57,41,11,93,(char)-39,(char)-118,4,4,38,41,(char)-68,(char)-55,68,5,35,(char)-65,(char)-68,97,45,36,5,(char)-59,
    (char)-1,52,(char)-51,31,(char)-19,(char)-16,(char)-30,54,59,60,96,(char)-87,(char)-110,(char)-6,94,39,35,(char)-29,117,95,(char)-110,91,(char)-94,5,(char)-9,18,(char)-101,(char)-128,125,(char)-119,
    (char)-53,(char)-66,0,32,(char)-10,47,(char)-116,2,119,(char)-80,17,124,97,40,2,15,(char)-42,0,(char)-23,(char)-24,(char)-110,(char)-74,84,76,(char)-103,32,(char)-46,(char)-75,(char)-32,45,36,72,(char)-52,
    (char)-115,56,(char)-40,(char)-103,15,(char)-5,43,(char)-18,56,(char)-85,(char)-1,(char)-65,(char)-67,(char)-121,(char)-18,90,(char)-84,(char)-2,(char)-60,63,31,80,55,(char)-4,(char)-19,17,16,8,32,48,
    45,24,(char)-64,60,2,65,87,(char)-60,(char)-68,75,(char)-75,13,28,27,(char)-101,(char)-111,(char)-111,(char)-93,(char)-32,(char)-26,90,(char)-32,(char)-116,92,(char)-109,(char)-107,(char)-93,(char)-100,
    27,(char)-68,(char)-98,(char)-67,(char)-126,(char)-63,34,0,110,(char)-34,62,(char)-79,(char)-12,(char)-126,(char)-103,79,(char)-87,32,(char)-17,(char)-21,68,(char)-64,26,(char)-27,(char)-107,(char)-17,
    81,34,62,(char)-66,(char)-31,103,126,(char)-32,39,126,(char)-47,59,97,(char)-30,99,0,63,(char)-13,(char)-125,62,105,73,124,36,(char)-13,51,63,(char)-24,(char)-109,(char)-94,71,(char)-17,(char)-112,(char)-118,
    (char)-119,(char)-113,(char)-128,(char)-97,(char)-7,(char)-127,(char)-97,(char)-12,78,73,(char)-15,(char)-15,63,126,(char)-26,7,125,18,62,62,2,125,104,19,19,1,16,16,(char)-32,107,34,16,64,14,(char)-63,
    127,13,(char)-76,93,(char)-44,47,(char)-128,6,2,2,124,29,35,16,98,4,0,(char)-35,(char)-128,55,(char)-55,100,(char)-100,89,(char)-46,26,124,102,108,(char)-58,12,102,48,(char)-125,25,20,80,64,1,5,103,
    9,(char)-9,37,21,(char)-116,126,(char)-19,1,0,105,(char)-85,(char)-6,95,16,(char)-72,11,(char)-2,(char)-89,119,(char)-17,(char)-7,(char)-53,117,(char)-76,123,(char)-33,95,(char)-87,(char)-93,(char)-35,
    7,(char)-2,(char)-78,(char)-65,(char)-30,(char)-81,(char)-42,(char)-47,(char)-18,67,127,(char)-83,(char)-114,(char)-6,71,(char)-35,(char)-57,(char)-2,(char)-118,(char)-65,(char)-26,111,(char)-44,81,
    93,(char)-12,(char)-60,18,84,17,21,47,90,14,71,1,(char)-96,(char)-6,(char)-9,(char)-25,23,(char)-43,83,(char)-84,5,60,30,(char)-17,(char)-31,(char)-32,67,(char)-127,(char)-15,53,124,60,(char)-101,(char)-113,
    (char)-58,42,(char)-36,(char)-59,105,111,122,(char)-122,(char)-37,(char)-42,101,91,(char)-14,69,(char)-36,11,(char)-6,(char)-56,53,(char)-43,105,44,(char)-76,0,(char)-58,(char)-52,102,33,91,18,64,(char)-46,
    1,69,(char)-98,90,59,(char)-17,(char)-54,88,104,(char)-3,(char)-2,27,(char)-26,(char)-58,88,(char)-3,49,26,(char)-33,(char)-119,14,(char)-59,83,(char)-64,(char)-88,0,(char)-27,(char)-122,(char)-72,(char)-86,
    30,(char)-56,89,64,75,(char)-128,114,67,112,21,(char)-125,(char)-30,127,(char)-64,(char)-99,93,(char)-55,9,(char)-69,(char)-106,19,100,(char)-30,4,(char)-117,48,(char)-113,112,69,(char)-75,125,(char)-96,
    57,64,(char)-128,114,99,(char)-20,42,(char)-15,(char)-56,74,(char)-125,25,(char)-7,73,64,46,111,15,(char)-16,64,(char)-72,(char)-55,(char)-64,87,26,79,(char)-105,112,(char)-44,116,71,(char)-8,(char)-92,
    (char)-114,22,79,(char)-72,(char)-91,60,34,4,43,53,112,75,121,4,(char)-26,(char)-30,(char)-100,(char)-8,25,(char)-39,(char)-68,43,89,32,60,16,110,50,1,33,23,(char)-62,(char)-16,(char)-124,91,(char)-54,
    35,(char)-54,(char)-118,6,114,31,80,76,(char)-93,(char)-67,26,(char)-23,124,(char)-66,(char)-80,(char)-93,64,7,120,(char)-124,61,58,103,(char)-127,18,41,124,17,(char)-44,46,(char)-125,(char)-29,60,(char)-11,
    8,64,(char)-46,(char)-20,4,66,85,105,(char)-92,(char)-4,(char)-57,4,87,127,3,(char)-8,(char)-10,(char)-21,41,1,(char)-32,(char)-57,(char)-1,77,(char)-117,127,74,22,(char)-16,(char)-68,46,5,(char)-24,
    20,0,32,(char)-128,58,118,94,1,(char)-112,(char)-89,26,96,0,(char)-92,(char)-25,27,120,87,121,10,50,48,75,(char)-23,(char)-34,108,43,5,(char)-109,(char)-45,49,(char)-45,(char)-87,80,(char)-81,88,29,
    23,(char)-119,(char)-118,(char)-15,(char)-58,114,(char)-10,71,55,85,17,53,34,5,49,15,(char)-110,124,6,(char)-91,124,124,100,92,96,22,30,(char)-35,32,46,50,(char)-66,(char)-8,(char)-115,(char)-62,(char)-106,
    (char)-52,(char)-123,(char)-114,6,43,26,(char)-113,(char)-116,(char)-104,6,15,(char)-107,(char)-103,(char)-102,(char)-122,(char)-108,90,50,(char)-91,(char)-107,86,(char)-110,3,86,61,(char)-109,(char)-31,
    (char)-103,(char)-117,46,22,104,(char)-101,19,30,115,107,114,81,(char)-109,(char)-51,(char)-50,(char)-80,(char)-48,121,(char)-26,25,31,(char)-69,75,(char)-22,61,85,(char)-86,78,(char)-99,(char)-4,123,
    31,35,(char)-90,(char)-32,(char)-24,(char)-108,(char)-125,13,120,40,(char)-54,92,(char)-13,76,(char)-61,18,(char)-123,77,68,(char)-23,(char)-80,(char)-4,84,(char)-14,(char)-96,(char)-16,51,59,41,(char)-63,
    12,(char)-27,(char)-91,5,18,104,(char)-111,26,2,22,54,29,26,(char)-86,87,42,102,37,46,(char)-27,(char)-113,80,(char)-83,84,17,33,113,(char)-21,(char)-92,(char)-93,(char)-30,24,36,96,31,(char)-122,(char)-101,
    (char)-69,(char)-56,36,46,(char)-92,(char)-108,(char)-94,(char)-20,(char)-94,(char)-86,71,(char)-85,(char)-80,(char)-62,58,75,45,(char)-79,(char)-47,102,(char)-125,6,108,(char)-80,(char)-45,96,60,(char)-30,
    45,(char)-3,112,49,20,62,(char)-46,(char)-111,(char)-118,17,(char)-53,55,(char)-6,(char)-4,(char)-49,87,(char)-16,(char)-38,(char)-73,(char)-10,(char)-5,(char)-53,97,55,69,69,(char)-70,(char)-73,64,
    0,(char)-47,(char)-16,8,84,39,16,2,64,23,(char)-112,16,31,33,53,(char)-75,67,117,89,(char)-86,(char)-18,4,(char)-100,(char)-15,69,63,14,1,(char)-21,(char)-55,113,1,34,92,59,14,73,102,(char)-59,113,40,
    76,45,12,125,53,120,28,23,(char)-120,65,36,69,(char)-120,44,(char)-32,(char)-106,(char)-114,127,71,(char)-75,(char)-56,(char)-62,29,(char)-66,93,(char)-91,54,117,87,(char)-87,67,52,(char)-26,104,127,
    118,27,112,52,107,83,(char)-125,(char)-53,(char)-55,(char)-64,(char)-58,76,(char)-53,(char)-50,(char)-91,65,(char)-107,110,118,(char)-51,(char)-102,52,99,115,(char)-85,82,(char)-93,83,(char)-125,114,
    109,(char)-68,(char)-86,(char)-76,109,76,(char)-26,(char)-14,(char)-109,9,112,(char)-16,55,83,(char)-56,(char)-70,66,(char)-42,(char)-37,70,(char)-26,(char)-71,74,(char)-74,62,58,(char)-78,101,(char)-6,
    (char)-74,(char)-126,(char)-107,37,(char)-23,40,(char)-83,(char)-59,(char)-72,115,101,53,(char)-41,(char)-128,28,31,(char)-30,(char)-11,(char)-105,34,111,66,25,82,(char)-101,(char)-111,56,(char)-57,
    (char)-38,53,26,52,32,(char)-77,89,(char)-69,43,85,45,(char)-88,74,26,(char)-85,53,63,(char)-50,114,(char)-80,(char)-2,74,7,57,(char)-80,8,(char)-88,(char)-94,73,126,6,35,33,65,(char)-12,12,(char)-116,
    76,(char)-52,44,(char)-84,108,(char)-20,28,(char)-100,92,114,(char)-72,(char)-27,(char)-126,121,120,(char)-27,(char)-55,87,(char)-64,(char)-89,80,(char)-111,98,116,41,24,82,(char)-91,73,(char)-57,(char)-60,
    (char)-110,(char)-127,(char)-115,(char)-125,(char)-117,(char)-121,79,64,72,68,76,66,74,118,(char)-17,(char)-21,(char)-65,95,(char)-79,(char)-30,(char)-60,75,(char)-112,40,73,50,44,28,(char)-68,96,52,
    81,33,(char)-19,(char)-76,(char)-117,86,(char)-76,45,8,34,17,(char)-123,64,(char)-37,107,(char)-97,(char)-35,(char)-10,56,(char)-23,(char)-108,(char)-61,(char)-114,(char)-40,100,(char)-77,126,40,(char)-21,
    81,(char)-123,10,(char)-93,(char)-92,86,(char)-94,76,12,(char)-107,(char)-125,48,74,(char)-3,(char)-86,(char)-57,108,(char)-33,(char)-102,107,(char)-98,(char)-43,(char)-42,(char)-104,33,92,32,(char)-115,
    114,17,(char)-2,(char)-76,(char)-60,27,(char)-5,(char)-111,(char)-111,80,(char)-100,48,(char)-53,6,(char)-33,(char)-124,50,83,(char)-112,(char)-1,(char)-87,51,70,(char)-125,122,77,26,(char)-11,105,(char)-10,
    (char)-69,22,109,(char)-38,(char)-75,(char)-70,(char)-83,(char)-61,88,93,58,117,27,103,(char)-126,(char)-119,(char)-58,91,103,(char)-110,29,(char)-26,120,109,(char)-78,(char)-87,(char)-90,(char)-103,
    (char)-30,(char)-91,17,(char)-25,(char)-100,81,97,80,(char)-91,109,22,(char)-14,(char)-121,(char)-10,(char)-101,42,103,(char)-99,119,(char)-63,69,(char)-105,(char)-116,(char)-70,(char)-84,(char)-38,
    21,87,(char)-43,(char)-72,110,(char)-111,33,(char)-37,(char)-67,114,(char)-61,77,(char)-75,110,(char)-7,(char)-61,82,111,(char)-3,(char)-33,95,5,70,123,116,68,103,116,69,119,(char)-12,68,111,(char)-12,
    69,127,12,(char)-120,22,35,86,(char)-100,120,9,18,37,73,(char)-122,(char)-51,(char)-69,112,(char)-16,121,(char)-17,67,4,68,36,100,20,84,52,116,41,24,82,(char)-91,73,(char)-57,(char)-52,93,(char)-9,(char)-52,
    (char)-41,75,(char)-25,(char)-114,(char)-121,(char)-79,100,(char)-28,81,108,28,92,60,124,2,(char)-62,(char)-81,(char)-77,(char)-21,120,60,30,(char)-28,(char)-85,26,8,(char)-11,(char)-74,(char)-104,(char)-108,
    64,(char)-88,(char)-56,85,(char)-34,(char)-47,(char)-49,22,(char)-16,(char)-92,(char)-94,(char)-32,79,21,34,119,(char)-93,15,(char)-58,52,119,85,(char)-111,(char)-48,(char)-15,122,44,(char)-28,33,0,
    84,88,(char)-125,(char)-97,14,32,(char)-108,(char)-119,30,(char)-53,105,(char)-42,(char)-18,110,(char)-6,44,4,(char)-34,(char)-90,(char)-97,54,68,(char)-19,102,77,110,(char)-30,119,79,(char)-18,(char)-118,
    39,(char)-96,5,1,5,15,(char)-22,82,80,(char)-117,(char)-126,(char)-126,(char)-128,90,2,(char)-88,75,65,65,65,45,122,46,26,95,44,116,(char)-80,8,(char)-117,(char)-79,4,75,(char)-79,12,103,98,13,(char)-26,
    97,45,(char)-42,97,8,(char)-21,(char)-85,(char)-13,107,100,(char)-14,(char)-41,89,19,104,(char)-75,(char)-16,(char)-42,(char)-123,116,(char)-72,(char)-63,95,(char)-11,(char)-49,3,(char)-13,(char)-122,
    (char)-5,93,(char)-11,(char)-79,(char)-31,(char)-76,(char)-107,39,(char)-92,(char)-68,58,59,(char)-86,12,(char)-33,(char)-65,80,56,52,52,(char)-14,(char)-109,7,(char)-5,(char)-3,90,42,117,(char)-83,
    (char)-63,(char)-49,(char)-109,(char)-96,(char)-106,112,(char)-29,(char)-84,60,0,0,0 };


    static constexpr std::array files =
    {
        File { "cmaj-generic-patch-view.js", std::string_view (cmajgenericpatchview_js, 25749) },
        File { "assets/cmajor-logo.svg", std::string_view (assets_cmajorlogo_svg, 2981) },
        File { "assets/sound-stacks-logo.svg", std::string_view (assets_soundstackslogo_svg, 6659) },
        File { "assets/ibmplexmono/v12/-F63fjptAgt5VM-kVkqdyU8n1iAq131nj-otFQ.woff2", std::string_view (assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1iAq131njotFQ_woff2, 3504) },
        File { "assets/ibmplexmono/v12/-F63fjptAgt5VM-kVkqdyU8n1iEq131nj-otFQ.woff2", std::string_view (assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1iEq131njotFQ_woff2, 8036) },
        File { "assets/ibmplexmono/v12/-F63fjptAgt5VM-kVkqdyU8n1iIq131nj-otFQ.woff2", std::string_view (assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1iIq131njotFQ_woff2, 4192) },
        File { "assets/ibmplexmono/v12/ibmplexmono.css", std::string_view (assets_ibmplexmono_v12_ibmplexmono_css, 1580) },
        File { "assets/ibmplexmono/v12/-F63fjptAgt5VM-kVkqdyU8n1isq131nj-otFQ.woff2", std::string_view (assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1isq131njotFQ_woff2, 5324) },
        File { "assets/ibmplexmono/v12/-F63fjptAgt5VM-kVkqdyU8n1i8q131nj-o.woff2", std::string_view (assets_ibmplexmono_v12_F63fjptAgt5VMkVkqdyU8n1i8q131njo_woff2, 9184) }
    };

};

} // namespace cmaj
