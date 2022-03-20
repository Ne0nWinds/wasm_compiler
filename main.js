import compiler from "./compiler.js"

const editorElement = document.getElementById("editor");
const editor = ace.edit(editorElement);
editor.resize();

const runElement = document.getElementById("run");
const disassembleElement = document.getElementById("disassemble");

const text_encoder = new TextEncoder('utf-8');
runElement.onclick = async () => {
	const val = editor.getValue();

	const compile_text_ptr = compiler.bump_alloc_js(val.length);
	const view = new Uint8Array(compiler.memory.buffer, compile_text_ptr, val.length);
	text_encoder.encodeInto(val, view);

	const length = compiler.compile(compile_text_ptr, view.byteLength) | 0;
	const binary = new Uint8Array(compiler.memory.buffer, compiler.get_wasm_binary(), length);
	const { instance } = await WebAssembly.instantiate(binary);
	console.log(binary);
}
