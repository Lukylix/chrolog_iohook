# chrolog_iohook

chrolog_iohook is a library that provides an interface for hooking into mouse and keyboard events in Node.js. It allows you to set callbacks for mouse and keyboard events and provides a logging functionality.

This library is based on the [keylogs](https://github.com/kernc/logkeys) program with modifications to handle mouse events.

## Installation

To install chrolog_iohook, use npm:

```
npm install chrolog_iohook
```

## Usage

```javascript
const { ChrologIOhook } = require("chrolog_iohook");

// Create an instance of ChrologIOhook
const instance = new ChrologIOhook();

// Set keyboard callback
instance.setKeyboardCallback((key) => {
	console.log("Keyboard callback:", key);
});

instance.setMouseCallback((event, value) => {
	console.log("Mouse callback:", event, value);
});

//Start the logger
instance.log();

console.log("Logging keys...");
```

## API

### `ChrologIOhook`

The `ChrologIOhook` class represents the main interface for setting mouse and keyboard callbacks and starting/stopping logging.

#### `ChrologIOhook.setMouseCallback(callback: function)`

Sets the callback function for mouse events.

- `callback` (function): A function that will be called whenever a mouse event occurs. The callback function should accept a single parameter, which will be the event object containing information about the mouse event.

#### `ChrologIOhook.setKeyboardCallback(callback: function)`

Sets the callback function for keyboard events.

- `callback` (function): A function that will be called whenever a keyboard event occurs. The callback function should accept a single parameter, which will be the event object containing information about the keyboard event.

#### `ChrologIOhook.log(): Promise`

Starts logging mouse and keyboard events. Returns a promise that resolves when logging is started successfully or rejects with an error if logging fails.

## Development

To build the chrolog_iohook library from source, you need to have the following dependencies installed:

- N-API (Node.js addon API)
- C++ compiler

To build the library, run the following commands:

```
npm install
npm run build
```

The built library will be located in the `build/Release` directory.

## License

This library is licensed under the GNU License. See the [LICENSE](LICENSE) file for more information.
