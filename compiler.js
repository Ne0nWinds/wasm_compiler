"use strict";

let memory = new WebAssembly.Memory({ 'initial': 32 });
const imports = {
	memory: memory,
};

const { instance } = await WebAssembly.instantiateStreaming(
	fetch("./build/binary.wasm"),
	{
		"env": imports
	}
);

console.log(instance);
export default instance.exports;
