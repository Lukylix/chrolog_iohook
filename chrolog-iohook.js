const { ChrologIOhook } = require("./build/Release/chrolog_iohook.node");

class Chrolog {
	constructor() {
		this.instance = new ChrologIOhook();
	}

	setKeyboardCallback(callback) {
		this.instance.setKeyboardCallback(callback);
	}

	setMouseCallback(callback) {
		this.instance.setMouseCallback(callback);
	}

	log() {
		this.instance.log();
	}
}

module.exports = Chrolog;
