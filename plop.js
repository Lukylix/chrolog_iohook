import sudo from "sudo-prompt";
import fs from "fs";
import { exec } from "child_process";

exec("which node", (error, stdout, stderr) => {
	if (error) {
		console.log(`error: ${error.message}`);
		return;
	}
	if (stderr) {
		console.log(`stderr: ${stderr}`);
		return;
	}

	const nodePath = stdout.trim();
	const scriptPath = "./hookLinuxInputs.js";
	const tempFilePath = "./ipc_temp_file.txt";

	const command = `${nodePath} ${scriptPath} -- --tempFile ${tempFilePath}`;
	sudo.exec(command, options, (error, stdout) => {
		if (error) {
			console.log("Error executing child process:", error);
		} else {
			console.log("Child process output:", stdout);
		}
	});

	fs.writeFileSync(tempFilePath, "");
	let fileContent = "";

	fs.watchFile(tempFilePath, { persistent: true, interval: 50 }, (curr, prev) => {
		fs.readFile(tempFilePath, "utf8", (error, data) => {
			if (error) {
				console.log("Error reading file:", error);
				return;
			}

			fileContent += data;
			const messages = fileContent.split("\n");

			messages.forEach((message) => {
				const trimmedMessage = message.trim();

				console.log("Message:", trimmedMessage);
				// Parse your message here
				fileContent = "";
			});
		});
	});
});
