import compiler from "./compiler.js"

const editorElement = document.getElementById("editor");
const editor = ace.edit(editorElement);
editor.resize();

const runElement = document.getElementById("run");
const disassembleElement = document.getElementById("disassemble");

async function runTestCases(instance) {
	const test_cases = [
		'{ return 5; }', 5,
		'{ return 2+2; }', 4,
		'{ return 22+28; }', 50,
		'{ return 50 - 50 + 25; }', 25,
		'{ return        50              - 50          + 25  ;   \n }', 25,
		'{ return 10 / 2; }', 5,
		'{ return 2 * 8 + 1; }', 17,
		'{ return 2 * 8 / 2; }', 8,
		'{ return 10 / 5 + 32 - 1; }', 33,
		'{ return 1 + 2 * 8; }', 17,
		'{ return 2 + 5 * 3 - 1; }', 16,
		'{ return 6 / 2 * 10 + 45 - 20 * 2; }', 35,
		'{ return 2 + 8 - 1 + 5 * 2 + 4 / 2 * 8; }', 35,
		'{ return 1 + (2 + 5) * 3 + 1; }', 23,
		'{ return 2 + (24 / ( 4 + 4 )) * 3; }', 11,
		'{ return -2; }', -2,
		'{ return -2 + 42; }', 40,
		'{ return 2 + -(24 / ( 4 + 4 )) * 3; }', -7,
		'{ return +10; }', 10,
		'{ return 255; }', 255,
		'{ return 1024 * 1024; }', 1048576,
		'{ return 2 > 5; }', 0,
		'{ return 2 < 5; }', 1,
		'{ return 5 >= 5; }', 1,
		'{ return 2 <= 5; }', 1,
		'{ return 25 * 4 < 25 * 4 + 8; }', 1,
		'{ 5; return 10; }', 10,
		'{ 25 * 4 < 25 * 4 + 8; 10; return 2 + (24 / ( 4 + 4 )) * 3; }', 11,
		'{ a = 50; return a; }', 50,
		'{ a = 25 * 4 + 2; a; return a; }', 102,
		'{ a = 5; b = 10; a = a + b + 1; return a; }', 16,
		'{ a = 5; b = 10; a = a + b + 1; return -a; }', -16,
		'{ a = b = 3; return a + b; }', 6,
		'{ a = b = 3 * 2; return -a + b * 2; }', 6,
		'{ abc = 5; return abc; }', 5,
		'{ a1 = 2; a2 = 4; return a1; }', 2,
		'{ qwerty = 2900; uiop = 500; return qwerty + -uiop; }', 2400,
		'{ return 25; 50; }', 25,
		'{ 2 + 2; { 8; } return 19; }', 19,
		'{ if (1) return 29; return 5; }', 29,
		'{ x = 100; y = 50;\n if (x > y) {\n y = x + y * 2;\n return y;\n }\nreturn 0;\n }', 200,
		'{ if (1) return 5; else return 10; }', 5,
		'{ if (0) return 5; else return 10; }', 10,
		'{ x = 100; if (x > 55) { x = x * 3; return x; } else return 0; }', 300,
		'{ x = 100; if (x <= 55) { x = x * 3; return x; } else { x = x / 2; return x + 1; } }', 51,
	];

	const { length } = test_cases;

	let i = 0;
	while (i < length) {
		console.log("Running Test Case " + i);
		console.log(test_cases[i]);
		const actualValue = await compile(test_cases[i]);
		const expectedValue = test_cases[i + 1];

		if (actualValue == null) {
			console.clear();
			console.log("=== Test Case Failed ===");
			console.log(`${test_cases[i]}\nCompilation Failure`);
			break;
		} else if (expectedValue != actualValue) {
			console.clear();
			console.log("=== Test Case Failed ===");
			console.log(`${test_cases[i]}\nExpected Value: ${expectedValue}\nActual Value: ${actualValue}`);
			break;
		}
		i += 2;
	}
	if (i == length) {
		console.clear();
		console.log("=== All Test Cases Passed ===");
	}
}

let binary;

async function compile(val) {
	const text_encoder = new TextEncoder('utf-8');

	compiler.bump_reset_js();
	const compile_text_ptr = compiler.bump_alloc_js(val.length + 1);
	const view = new Uint8Array(compiler.memory.buffer, compile_text_ptr, val.length);
	text_encoder.encodeInto(val, view);

	const length = compiler.compile(compile_text_ptr, view.byteLength) | 0;
	if (!length) {
		return null;
	}
	binary = new Uint8Array(compiler.memory.buffer, compiler.get_wasm_binary(), length);
	const { instance } = await WebAssembly.instantiate(binary);

	const value = instance.exports.main();
	return value;
}

runElement.onclick = async () => {
	const editorValue = editor.getValue();
	const value = await compile(editorValue);
	console.log(value);
}
runTestCases();

let binaryExplorerEventListener = null;
disassembleElement.onclick = async () => {
	if (binary && !binaryExplorerEventListener) {
		const dst = "https://wasdk.github.io/wasmcodeexplorer/?api=postmessage";
		const windowName = "BinaryExplorer";
		window.open(dst, "Binary Explorer", "popup");
		const binaryCopy = new Uint8Array(binary);
		binaryExplorerEventListener = (e) => {
			if (e.data.type == "wasmexplorer-ready") {
				window.removeEventListener("message", binaryExplorerEventListener, false);
				binaryExplorerEventListener = null;
				e.source.postMessage({
					type: "wasmexplorer-load",
					data: binaryCopy
				}, "*", [binaryCopy.buffer]);
			}
		};
		window.addEventListener("message", binaryExplorerEventListener, false);
	}
}
