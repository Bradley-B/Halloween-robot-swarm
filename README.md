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
accept local connections. NATS uses core messaging without authentication
or JetStream persistence.

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

The page contains a form with a NATS subject (default `robots.commands`) and
a JSON message. Start a subscriber before sending a command:

```sh
nats --server nats://127.0.0.1:4223 sub robots.commands
```

Open http://localhost:8095 and click **Send command** to publish the example:

```json
{
  "target": "all",
  "command": "robot.scene",
  "params": { "scene": "spooky" }
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
    "command": "robot.scene",
    "params": { "scene": "spooky" }
  }
}
```

Requests are limited to 64 KiB. Subjects must have nonempty dot-separated
tokens without whitespace, control characters, or wildcards. Messages must
be valid, non-null JSON. The server publishes the message value itself,
without the HTTP wrapper. Success confirms publishing to NATS; it does not
confirm receipt or execution by a robot.

The example command format is a starting point for firmware development.
The server does not interpret LED commands or enforce a robot command schema
yet. NATS currently listens on loopback; microcontrollers on another device
will need a LAN-accessible listener configured before connecting.

## Planned robot controls

Microcontrollers in multiple robot Halloween costumes will connect to NATS
and listen for commands issued through the web server. Each costume has LED
matrices for its eyes and mouth, plus an LED strip.

The controls below are brainstorming proposals; firmware support is not
implemented yet. The web form can publish JSON messages now, but the final
command schema and routing for individual robots and groups still need to
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
