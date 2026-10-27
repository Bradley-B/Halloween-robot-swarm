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

The page has an LED strip color picker and a mode selector. Choose a color
and mode, then click **Apply to all robots**. Modes are:

| Mode | Behavior |
| --- | --- |
| Rainbow | Moving rainbow across the strip; ignores the selected color |
| Pulse sine wave | Selected color smoothly fades out and back in over two seconds |
| Pulse square wave | Selected color alternates one second on, one second off |
| Solid | All strip LEDs stay lit in the selected color |
| Off | All strip LEDs turn off |

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
  "params": { "mode": "solid", "color": "#ff9f43" }
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
    "params": { "mode": "solid", "color": "#ff9f43" }
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
values are `rainbow`, `pulse_sine`, `pulse_square`, `solid`, and `off`; colors
use `#RRGGBB` hexadecimal notation. Invalid strip settings are ignored without
changing the current state. The Go server forwards JSON without interpreting
the robot command schema. The existing `echo` subject still handles matrix
letter commands.

Configure `WIFI_SSID`, `WIFI_PSK`, and the NATS server's LAN address in
`firmware/src/main.cpp` before uploading firmware. The NATS port is `4223`.
Strip animations update without blocking NATS command processing.

## Planned robot controls

Microcontrollers in multiple robot Halloween costumes will connect to NATS
and listen for commands issued through the web server. Each costume has LED
matrices for its eyes and mouth, plus an LED strip.

The controls below are brainstorming proposals beyond the implemented
`strip.set` control. Routing for individual robots and groups still needs to
be defined.

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
