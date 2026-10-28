# Halloween Robot Swarm

A Go process that serves an HTML page and runs an embedded NATS server.

Requires Go 1.26 or newer. With automatic toolchain selection enabled, Go
can download the toolchain specified in `server/go.mod`. Start the server:

```sh
cd server
go run .
```

Open http://localhost:8095 in your browser. NATS clients can connect to
`nats://127.0.0.1:4223`; no separate NATS process is needed. Both listeners
bind to all interfaces, allowing robots on the LAN to connect. NATS uses
core messaging without authentication or JetStream persistence.

Press Ctrl+C to gracefully shut down both servers. The HTTP server starts
after NATS is ready; startup fails if either port is unavailable.

With the NATS CLI installed, try these in two terminals:

```sh
nats --server nats://127.0.0.1:4223 sub robots.commands
nats --server nats://127.0.0.1:4223 pub robots.commands 'hello swarm'
```

Edit `server/index.html` to change the page, then restart the server. The HTML
is embedded in the executable at build time.

## Sending commands from the web page

The page has an LED strip color picker, mode selector, and timing controls.
Choose settings, then click **Apply to all robots**. Modes are:

| Mode | Behavior |
| --- | --- |
| Rainbow | Moving rainbow across the strip; ignores the selected color |
| Pulse sine wave | Selected color smoothly fades out and back in over the pulse period |
| Pulse square wave | Selected color alternates on and off for half the pulse period each |
| Chase | Five LEDs in the selected color travel along the strip and wrap around |
| Chase bounce | Five LEDs in the selected color travel back and forth, reversing at each end |
| Solid | All strip LEDs stay lit in the selected color |
| Off | All strip LEDs turn off |

**Frame interval** sets the time between animation updates in milliseconds
(1–60000, default 10). Rainbow advances its hue and both chase modes move one
LED per interval, so larger values slow these effects. **Pulse period** sets
the full sine or square pulse cycle in milliseconds (2–60000, default 2000), independent
of the frame interval. Keep the frame interval shorter than the pulse period
to show the pulse animation; actual update speed is limited by the hardware.

The strip starts in rainbow mode. Settings remain in memory until another
command or a restart; they are not saved across restarts or replayed to robots
that were disconnected when the command was sent. The web controls show the
settings to send, not live robot state.

The **Custom command** section also accepts a NATS subject (default
`robots.commands`) and a JSON message. To inspect outgoing commands, start a
subscriber before sending one:

```sh
nats --server nats://127.0.0.1:4223 sub robots.commands
```

Open http://localhost:8095, expand **Custom command**, and click **Send command**
to publish the example:

```json
{
  "target": "all",
  "command": "strip.set",
  "params": {
    "mode": "solid",
    "color": "#ff9f43",
    "frame_interval_ms": 10,
    "pulse_period_ms": 2000
  }
}
```

The browser sends an HTTP request to `POST /api/commands`. The Go server
validates the request and publishes through its connection to the embedded
NATS server. This keeps NATS connection details in the server and gives us a
place to add command validation and access control as the project grows.

The API accepts `application/json` with `subject` and `message` fields:

```json
{
  "subject": "robots.commands",
  "message": {
    "target": "all",
    "command": "strip.set",
    "params": {
      "mode": "solid",
      "color": "#ff9f43",
      "frame_interval_ms": 10,
      "pulse_period_ms": 2000
    }
  }
}
```

Requests are limited to 64 KiB. Subjects must have nonempty dot-separated
tokens without whitespace, control characters, or wildcards. Messages must
be valid, non-null JSON. The server publishes the message value itself,
without the HTTP wrapper. Success confirms publishing to NATS; it does not
confirm receipt or execution by a robot.

The firmware subscribes to `robots.commands` and handles `strip.set` commands
with `target: "all"`. Both `params.mode` and `params.color` are required. Mode
values are `rainbow`, `pulse_sine`, `pulse_square`, `chase`, `chase_bounce`,
`solid`, and `off`; colors use `#RRGGBB` hexadecimal notation. Optional integer fields
`params.frame_interval_ms` and `params.pulse_period_ms` use the ranges above;
omitting them applies their defaults, including on subsequent commands.
Invalid strip settings are ignored without changing any current settings.
The Go server forwards JSON without interpreting the robot command schema.
The existing `echo` subject still handles matrix letter commands.

Configure `WIFI_SSID`, `WIFI_PSK`, and the NATS server's LAN address in
`firmware/src/main.cpp` before uploading firmware. The NATS port is `4223`.
Strip animations update without blocking NATS command processing.

## Matrix animations

The **Matrix animations** form controls **Both eyes** or the **Whole mouth**
on every connected robot. Left and right are from the perspective of looking
at the face:

| Matrix | I2C address | Face part |
| --- | --- | --- |
| 1 | `0x30` | Left eye |
| 2 | `0x31` | Right eye |
| 3 | `0x32` | Left half of the mouth |
| 4 | `0x33` | Right half of the mouth |

Each eye is 13×9. The mouth is one continuous 26×9 image split across the two
adjacent panels. Group commands select an animation and restart both members
on a shared timeline. Eyes and mouth can run different animations, such as
blinking eyes with a talking mouth. Individual panel controls remain
available; each mouth panel always displays its corresponding half of the
wide image. The form shows settings to send, not live robot state.

| Animation | Behavior |
| --- | --- |
| `blink` | Blue and purple eye: open 2400 ms, half closed 100 ms, closed 180 ms, half closed 100 ms |
| `look` | Eye looks left 900 ms, center 400 ms, right 900 ms, center 400 ms |
| `talk` | Orange and purple mouth: smile 300 ms, open 450 ms, smile 200 ms, open 250 ms |
| `smile` | Static orange smile with white teeth |
| `off` | Black frame, held until changed |

Send `matrix.set` to `robots.commands` to animate the whole mouth:

```json
{
  "target": "all",
  "command": "matrix.set",
  "params": { "matrix": "mouth", "animation": "talk" }
}
```

To blink both eyes, use `"params": { "matrix": "eyes", "animation": "blink" }`.
`params.matrix` accepts `"eyes"`, `"mouth"`, `"all"`, or an integer from 1 to 4.
`target: "all"` broadcasts to robots; `params.matrix` selects the face part
within each robot.

Eye targets accept `blink`, `look`, and `off`. Mouth targets accept `talk`,
`smile`, and `off`. The `"all"` selector accepts `off` to darken the whole
face. The website filters animation choices for the selected face part.
Unknown animation names, invalid selectors, and animations incompatible with
any selected panel are ignored without changing any selections. Commands
switch the built-in animations; they do not upload frame pixels or alter
frame timing.

Selections last until changed or the robot restarts. The existing red X and
green circle remain as startup/connectivity indicators. After a Wi-Fi
reconnection, selected animations resume. The legacy `echo` letter command
stops animations on all matrices and holds the letter until another command.

### Adding frames and animations

Edit `firmware/src/frames.cpp`. Eye frames contain 117 bytes (13×9) and full
mouth frames contain 234 bytes (26×9), in row-major order. The renderer reads
mouth columns 0–12 for matrix 3 and 13–25 for matrix 4, without allocating a
wide framebuffer. Pixels use RGB332 (3 red bits,
3 green bits, 2 blue bits), giving 256 possible colors. `rgb332(r, g, b)` in
`frames.h` converts 8-bit channel values to this format. The `K`, `P`, `W`,
`B`, `C`, and `O` symbols in the starter frames are RGB332 values for
readability; they do not restrict frames to a palette.

Store pixel arrays, `ColorFrame` sequences, animation names, and the animation
registry in `PROGMEM` so they stay in flash. A frame entry pairs a pixel-array
pointer with its hold time in milliseconds:

```cpp
const ColorFrame customFrames[] PROGMEM = {
  {eyeCenter, 1800},
  {eyeClosed, 200},
  {nullptr, 100} // nullptr is a fully black frame; no pixel array needed.
};
const char customName[] PROGMEM = "custom";
// Add this entry to animations[]:
// {customName, customFrames, sizeof(customFrames) / sizeof(ColorFrame), MATRIX_WIDTH}
```

Use 1–65535 ms per frame and 1–255 entries per animation. The last entry loops
back to the first. An animation with one entry holds indefinitely. The final
registry field is the source width: `MATRIX_WIDTH` for eyes, `MOUTH_WIDTH` for
mouths, or 0 for the universal black animation. All frames in an animation
use that width. Add its name to the website's animation options, set its
`data-part` to `eyes` or `mouth`, and add a description to `animationHints`
in `server/index.html` as well.

Pixel arrays can be shared between animations and matrices. The starter set
has seven unique color frames (1053 bytes of pixel data in flash), plus small
flash descriptors. Playback adds only a flash pointer, timestamp, frame index,
and flag per matrix (about 48 bytes total on ESP8266). Rendering reads pixels
directly from flash into the driver's existing buffers, without allocating
additional framebuffers or copying animation sequences into RAM. Only changed
frames are sent over I2C. Playback uses `millis()` without delays. Group
members share a start timestamp and retain their scheduled frame boundaries,
so sequential I2C writes do not accumulate timing drift. If playback stalls,
it skips expired frames and sends only the current one, keeping grouped
panels on the same logical frame.

## Planned robot controls

Microcontrollers in multiple robot Halloween costumes will connect to NATS
and listen for commands issued through the web server. Each costume has LED
matrices for its eyes and mouth, plus an LED strip.

The controls below are brainstorming proposals beyond the implemented
`strip.set` and `matrix.set` controls. Routing for individual robots and groups
still needs to be defined.

### Targeting

Every command should support targeting one robot, a named group, or all robots.

### Commands

Include whole-robot scenes for quick costume changes, plus individual controls
for the eyes, mouth, and strip.

| Command | What it controls | Example options |
| --- | --- | --- |
| `robot.scene` | Coordinated eyes, mouth, and strip | Idle, happy, angry, spooky, malfunction, sleeping |
| `robot.brightness` | Overall brightness | 0–100% |
| `robot.blackout` | Turn all LEDs off | Useful between performances |
| `eyes.expression` | Eye shapes | Normal, happy, angry, surprised, squinting |
| `eyes.look` | Direction the eyes face | Left, right, up, down, center |
| `eyes.blink` | One-time eye animation | Blink both, wink left, wink right |
| `mouth.expression` | Mouth shape | Smile, frown, neutral, open, jagged |
| `mouth.animate` | Repeating mouth animation | Talking, laughing, chattering |
| `strip.color` | Solid strip color | Orange, purple, green, custom color |
| `strip.effect` | Animated strip pattern | Pulse, chase, rainbow, heartbeat, flicker |
| `robot.reset` | Return to the default scene | A quick recovery action |

### Scene ideas

- **Idle:** occasional blinking, neutral mouth, slow orange pulse.
- **Happy:** curved eyes, smiling mouth, rainbow chase.
- **Angry:** narrowed red eyes, jagged mouth, red heartbeat.
- **Spooky:** green eyes, chattering mouth, purple flicker.
- **Malfunction:** shifting expressions, scrambled mouth, erratic strip animation.
- **Sleeping:** closed eyes, neutral mouth, faint blue breathing effect.

### Animation behavior

Use a shared set of controls where applicable: color, brightness, speed, and
duration. One-time actions such as a wink should finish and return to the
current scene. Persistent changes such as an angry expression should remain
until changed.

### Coordinated routines

- **Synchronized blink:** all targeted robots blink together.
- **Wave:** a strip chase travels through the robots in order.
- **Roll call:** each robot lights up briefly.
- **Follow the leader:** robots copy a scene with a short delay.
- **Group scare:** all targeted robots switch to spooky mode together.

### Suggested first version

Start with scene, brightness, blackout, eye expression, blink, mouth expression,
strip color, and strip effect. Add the remaining controls and coordinated
routines later.
