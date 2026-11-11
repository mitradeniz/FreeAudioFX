# Using the plugins in your DAW

Every Open Audio Plugin is a standard **VST3** plugin on macOS, Windows and
Linux, an **Audio Unit (AU)** on macOS, and the amps, the Pedalboard and the
Looper are also **standalone apps**. So they work in practically every DAW that loads
VST3 or AU plugins. In your DAW's plugin list, look for the manufacturer
**Open Audio Plugins**, or search for **OAP**.

> [!NOTE]
> All plugins pass [pluginval](https://github.com/Tracktion/pluginval) at its
> strictest level, which exercises them the way hosts do. Hands-on testing in
> each DAW below is still in progress: if something misbehaves in yours,
> please [open an issue](https://github.com/mitradeniz/FreeAudioFX/issues).

## Formats and folders

| | macOS | Windows | Linux |
| --- | --- | --- | --- |
| **VST3** | ✓ | ✓ | ✓ |
| **Audio Unit** | ✓ | | |
| **Standalone** (amps, Pedalboard, Looper) | ✓ `.app` | ✓ `.exe` | ✓ |
| **VST3 folder** | `~/Library/Audio/Plug-Ins/VST3` *(copied automatically)* | `C:\Program Files\Common Files\VST3` | `~/.vst3` |
| **AU folder** | `~/Library/Audio/Plug-Ins/Components` *(copied automatically)* | | |

On Windows and Linux, copy the `.vst3` folders there yourself after building
(see [BUILDING.md](BUILDING.md)).

## DAW by DAW

| DAW | Platforms | Use | Make it find the plugins |
| --- | --- | --- | --- |
| **Ableton Live** 11 / 12 (Lite, Intro, Standard, Suite) | macOS, Windows | VST3 or AU | **Settings → Plug-Ins**: turn on **Use VST3 Plug-In System Folders** (and **Use Audio Units v2** on a Mac), then **Rescan**. Find them under **Plug-Ins** in the browser. |
| **Logic Pro** | macOS | AU | Scanned at launch. If one is missing: **Logic Pro → Settings → Plug-in Manager**, select it, **Reset & Rescan Selection**. Insert from an Audio FX slot → **Audio Units → Open Audio Plugins**. |
| **GarageBand** | macOS | AU | **GarageBand → Settings → Audio/MIDI**: tick **Enable Audio Units**. In a track's **Smart Controls → Plug-ins**, click an empty slot → **Audio Units → Open Audio Plugins**. |
| **MainStage** | macOS | AU | Like Logic Pro. Great for playing the amps live. |
| **REAPER** | macOS, Windows, Linux | VST3 (or AU) | **Options → Preferences → Plug-ins → VST**: check the VST3 folder is in the path list, then **Re-scan**. Add with the track's **FX** button. |
| **FL Studio** | macOS, Windows | VST3 | **Options → Manage plugins → Find installed plugins**. Add to a Mixer insert slot. |
| **Cubase / Nuendo** | macOS, Windows | VST3 | Scanned at launch; check **Studio → VST Plug-in Manager**. Add from an audio track's **Inserts**. |
| **Studio One** | macOS, Windows | VST3 or AU | Scanned at launch (**Settings → Locations → VST Plug-ins** lists the folders). Drag from **Browser → Effects**. |
| **Bitwig Studio** | macOS, Windows, Linux | VST3 | The system VST3 folder is scanned automatically (**Settings → Locations**). Find them under **Plug-ins** in the browser. |
| **Cakewalk / Sonar** | Windows | VST3 | **Preferences → File → VST Settings → Scan**. |
| **Ardour / Mixbus** | macOS, Windows, Linux | VST3 (or AU) | **Window → Plugin Manager**, or **Preferences → Plugins → Scan for Plugins**. |
| **Audacity** 3.2 or newer | macOS, Windows, Linux | VST3 (or AU) | **Effect → Plugin Manager**: enable the new plugins. Use them as realtime effects on a track. |
| **Pro Tools** | | ✗ | Needs the AAX format, which isn't built yet. |
| **iPad / iPhone** (Logic, GarageBand…) | | ✗ | Needs AUv3, which isn't built yet. |

Menu names change between DAW versions; if a menu above looks different in
yours, look for "plug-in manager", "rescan" or "VST3 folder" in its settings.

## Setting up a guitar track (any DAW)

1. **Audio settings:** choose your audio interface for input and output, and
   set the buffer size to 64-128 samples for a tight feel.
2. **Create a mono audio track** whose input is the interface input your
   guitar is plugged into (for example *Input 1*, not *1/2*).
3. **Turn on input monitoring** (Ableton: *Monitor In*; Logic and GarageBand:
   the monitoring button; REAPER: arm the track with monitoring on; Cubase:
   the speaker button).
4. **Insert the plugins** as track effects, in signal order: pedals first,
   then an amp; or one of the effect amps on its own; or the Pedalboard into
   an amp. The Looper goes last, after the amp.
5. **Turn off direct monitoring** on your interface if you hear a dry guitar
   underneath the amp.
6. Set the interface's input gain so hard strums peak green or yellow, never
   red.

> [!TIP]
> The amps' and pedals' latency is tiny (6-45 samples) and always reported to
> your DAW, so recorded parts stay in time. The delay you feel while playing
> is mostly your DAW's buffer size.

## Looping with a foot controller

The [Looper](plugins/Looper.md) is made for the mouse, but its switches are
also plugin parameters (**Main Switch**, **Stop Switch**, **Undo Switch** and
**Clear Switch**), so any DAW can map them to a MIDI foot controller or
automate them:

| DAW | Map a footswitch to *Main Switch* |
| --- | --- |
| **Ableton Live** | Unfold the Looper device (the triangle in its title bar) so its parameters show. Press **Cmd+M** (Mac) / **Ctrl+M** (Windows), click **Main Switch**, press the footswitch, press Cmd+M / Ctrl+M again. |
| **Logic Pro / MainStage** | **Logic Pro → Control Surfaces → Learn Assignment** (Logic), or map a screen control to the parameter in Edit mode (MainStage). |
| **REAPER** | In the FX window: **Param → FX parameter list → Learn → Main Switch**, then press the footswitch. |
| **Bitwig Studio** | Right-click the parameter in the device panel → **Learn Controller Assignment**. |
| **Cubase / Studio One / FL Studio** | Use the DAW's MIDI learn or remote-control mapping for plugin parameters (Quick Controls, Control Link, *Link to controller*). |

Then set the Looper's **Switches** control to match your controller:
**Momentary** if the parameter should be on only while your foot is down
(click, double-click and hold then work like with the mouse), or **Latching**
if your controller flips it with every press. With **Auto Loop** on, a
single press of the footswitch starts a whole auto loop: count-in,
recording and layers. Details in the
[Looper manual](plugins/Looper.md#foot-controllers-and-automation).

## Standalone apps

All the amps, the Pedalboard and the Looper also come as standalone apps,
handy for practising without a DAW:

| Plugin | Standalone app, after building (macOS shown) |
| --- | --- |
| Drive Amp, Clean Amp | `build/plugins/DriveAmp/DriveAmp_artefacts/Release/Standalone/OAP Drive Amp.app` (and `CleanAmp/…`) |
| Effect amps | `build/plugins/effectamps/DreamAmp_artefacts/Release/Standalone/OAP Dream Amp.app` (and all the others) |
| Pedalboard | `build/plugins/Pedalboard/Pedalboard_artefacts/Release/Standalone/OAP Pedalboard.app` |
| Looper | `build/plugins/Looper/Looper_artefacts/Release/Standalone/OAP Looper.app` |

On Windows the same folders contain `.exe` files, and on Linux plain
executables. Open
**Options → Audio/MIDI Settings**, pick your interface, unmute the input, and
**use headphones** to avoid feedback.
