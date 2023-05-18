# chrolog_iohook

chrolog_iohook is a library that provides an interface for hooking into mouse and keyboard events in Node.js. It allows you to set callbacks for mouse and keyboard events and provides a logging functionality.

This library is based on the [keylogs](https://github.com/kernc/logkeys) program with modifications to handle mouse events.

## Installation

To install chrolog_iohook, use npm:

```
npm install chrolog_iohook
```

**Note:**

- This library is only compatible with Linux operating systems.
- Root privileges are required to use this library.

## Usage

```javascript
const { ChrologIOhook } = require('chrolog_iohook')
const os = require('os')

if (os.platform !== 'linux') return console.log('The program will not work')
// Create an instance of ChrologIOhook
const instance = new ChrologIOhook()

// Set keyboard callback
instance.setKeyboardCallback((key) => {
 console.log('Keyboard callback:', key)
})

instance.setMouseCallback((event, value) => {
 console.log('Mouse callback:', event, value)
})

// Start the logger
instance.log()

console.log('Logging keys...')
```

## Security and Interprocess Communication

To enhance security it is recommended to execute the sensitive portion of the code in a separate process with elevated privileges. This approach helps isolate the privileged operations and restricts potential security risks.

Be cautious of potential permission issues when granting read and write access to everyone, as it could allow unauthorized individuals to potentially read your keyboard inputs without requiring root privileges.

If you discover an alternative method, kindly inform me as IPC (Inter-Process Communication) does not function with sudo-prompt.

Here's an example of how you can refactor the code to run the privileged section in a separate process:

main.js

```js
import sudo from 'sudo-prompt'
import fs from 'fs'
import { exec } from 'child_process'

exec('which node', (error, stdout, stderr) => {
 if (error) {
  console.log(`error: ${error.message}`)
  return
 }
 if (stderr) {
  console.log(`stderr: ${stderr}`)
  return
 }

 const nodePath = stdout.trim()
 const scriptPath = './hookLinuxInputs.js'
 const tempFilePath = './ipc_temp_file.txt'

 const command = `${nodePath} ${scriptPath} -- --tempFile ${tempFilePath}`
 sudo.exec(command, options, (error, stdout) => {
  if (error) {
   console.log('Error executing child process:', error)
  } else {
   console.log('Child process output:', stdout)
  }
 })

 fs.writeFileSync(tempFilePath, '')
 let fileContent = ''

 fs.watchFile(tempFilePath, { persistent: true, interval: 50 }, (curr, prev) => {
  fs.readFile(tempFilePath, 'utf8', (error, data) => {
   if (error) {
    console.log('Error reading file:', error)
    return
   }

   fileContent += data
   const messages = fileContent.split('\n')

   messages.forEach((message) => {
    const trimmedMessage = message.trim()

    console.log('Message:', trimmedMessage)
    // Parse your message here
    fileContent = ''
   })
  })
 })
})
```

hookLinuxInputs.js

```js
import { exec } from 'child_process'
import ChrologIOhook from 'chrolog-iohook'
import fs from 'fs'

const tempFilePath =
 process.argv[
  process.argv.findIndex((arg, index) => arg === '--tempFile' && process.argv[index + 1]) + 1
 ]

fs.writeFileSync(tempFilePath, '')

exec(`chmod +rw ${tempFilePath}`, (error) => {
 if (error) {
  console.error(error)
  process.exit(1)
 }
})

// Function to send message to the parent process
function send(message) {
 fs.writeFileSync(tempFilePath, message)
}
console.log('Creating instance...')

const instance = new ChrologIOhook()

console.log('Hooking inputs...')

// Set keyboard callback
instance.setKeyboardCallback(() => {
 send('keyboard_event')
})

console.log('Hooking mouse...')

instance.setMouseCallback(() => {
 send('mouse_event')
})

console.log('Starting logger...')

// Start the logger
instance.log()

console.log('Inputs hooked')

// Cleanup the temp file on exit
process.on('exit', () => {
 if (fs.existsSync(tempFilePath)) {
  fs.unlinkSync(tempFilePath)
 }
})
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
