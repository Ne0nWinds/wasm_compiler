import compiler from "./compiler.js"

const editorElement = document.getElementById("editor");
const editor = ace.edit(editorElement);
editor.resize();

const runElement = document.getElementById("run");
const disassembleElement = document.getElementById("disassemble");

async function runTestCases(instance) {
	const test_cases = [
		'5', 5,
		'2+2', 4,
		'22+28', 50,
		'50 - 50 + 25', 25,
		'       50              - 50          + 25     \n', 25,
		'10 / 2', 5,
		'2 * 8 + 1', 17,
		'2 * 8 / 2', 8,
		'10 / 5 + 32 - 1', 33,
	];

	const { length } = test_cases;

	let i = 0;
	while (i < length) {
		const actualValue = await compile(test_cases[i]);
		const expectedValue = test_cases[i + 1];

		if (expectedValue != actualValue) {
			console.log("=== Test Case Failed ===");
			console.log(`${test_cases[i]}\nExpected Value: ${expectedValue}\nActual Value: ${actualValue}`);
			break;
		}
		i += 2;
	}
	if (i == length)
		console.log("=== All Test Cases Passed ===");
}

async function compile(val) {
	const text_encoder = new TextEncoder('utf-8');

	compiler.bump_reset_js();
	const compile_text_ptr = compiler.bump_alloc_js(val.length + 1);
	const view = new Uint8Array(compiler.memory.buffer, compile_text_ptr, val.length);
	text_encoder.encodeInto(val, view);

	const length = compiler.compile(compile_text_ptr, view.byteLength) | 0;
	const binary = new Uint8Array(compiler.memory.buffer, compiler.get_wasm_binary(), length);
	// console.log(binary);
	const { instance } = await WebAssembly.instantiate(binary);

	const value = instance.exports.main();
	return value;
}

runElement.onclick = async () => {
	const value = await compile(editor.getValue());
	console.log(value);
}
console.clear();
runTestCases();
