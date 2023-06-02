const Chrolog = require('./chrolog-iohook.js')

// Create an instance of ChrologIOhook
const instance = new Chrolog()

// Set keyboard callback
instance.setKeyboardCallback((key) => {
 console.log('Keyboard callback:', key)
})

instance.setMouseCallback((event, value) => {
 console.log('Mouse callback:', event, value)
})

//Start the logger
instance.log()

console.log('Logging keys...')
