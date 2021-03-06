#include "oems/dx11.h"

#include <cassert>
#include <sstream>
#include <comdef.h>


namespace {

using namespace oems;

constexpr char* c_window_class_name = "oems_window_class";

LRESULT CALLBACK window_proc(HWND p_hwnd, UINT message, WPARAM w_param, LPARAM l_param);


com_ptr<ID3DBlob> compile_shader(const std::string& source_code, const char* p_source_filename)
{
	com_ptr<ID3DBlob> p_bytecode;
	com_ptr<ID3DBlob> p_error_blob;

	HRESULT hr = D3DCompile(
		source_code.c_str(),
		source_code.size(),
		p_source_filename,
		nullptr,							// defines
		D3D_COMPILE_STANDARD_FILE_INCLUDE,	// includes
		"cs_main",
		"cs_5_0",
		0,									// compile flags
		0,									// effect compilation flags
		&p_bytecode.ptr,
		&p_error_blob.ptr
	);

	if (hr != S_OK) {
		std::string error(static_cast<char*>(p_error_blob->GetBufferPointer()), p_error_blob->GetBufferSize());
		throw std::runtime_error(error);
	}

	return p_bytecode;
}

HWND make_window(uint2 position, uint2 client_area_size)
{
	assert(position > 0);
	assert(client_area_size > 0);

	HINSTANCE p_hinstance = GetModuleHandle(nullptr);

	// register the window's class ---
	WNDCLASSEX wnd_class = {};
	wnd_class.cbSize		= sizeof(wnd_class);
	wnd_class.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wnd_class.lpfnWndProc	= window_proc;
	wnd_class.cbClsExtra	= 0;
	wnd_class.cbWndExtra	= 0;
	wnd_class.hInstance		= p_hinstance;
	wnd_class.hIcon			= nullptr;
	wnd_class.hCursor		= LoadCursor(nullptr, IDI_APPLICATION);
	wnd_class.hbrBackground = nullptr;
	wnd_class.lpszMenuName	= nullptr;
	wnd_class.lpszClassName	= c_window_class_name;

	ATOM reg_res = RegisterClassEx(&wnd_class);
	assert(reg_res != 0);

	// create the window ---
	RECT rect;
	rect.left	= position.x;
	rect.top	= position.y;
	rect.right	= position.x + client_area_size.x;
	rect.bottom	= position.y + client_area_size.y;
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, false, WS_EX_APPWINDOW);

	HWND p_hwnd = CreateWindowEx(WS_EX_APPWINDOW, c_window_class_name, "oems",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		nullptr, nullptr, p_hinstance, nullptr);
	assert(p_hwnd);

	return p_hwnd;
}

LRESULT CALLBACK window_proc(HWND p_hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
	switch (message) {
		default:
			return DefWindowProc(p_hwnd, message, w_param, l_param);

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}
}

} // namespace


namespace oems {

// ----- dx11_rhi -----

dx11_rhi::dx11_rhi()
{
	p_hwnd_ = make_window({ 100, 100 }, { 256, 256 });

	constexpr D3D_FEATURE_LEVEL expected_feature_level = D3D_FEATURE_LEVEL_11_1;
	D3D_FEATURE_LEVEL actual_feature_level;

	// create device & context ---
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT,
		&expected_feature_level, 1, D3D11_SDK_VERSION,
		&p_device_.ptr,
		&actual_feature_level,
		&p_ctx_.ptr);

	THROW_IF_DX_ERROR(hr);
	ENFORCE(actual_feature_level == expected_feature_level,
		"Failed to create a device with feature level: D3D_FEATURE_LEVEL_11_0.");

	// init debug interface ---
#ifdef OEMS_DEBUG
	hr = p_device_->QueryInterface<ID3D11Debug>(&p_debug_.ptr);
	THROW_IF_DX_ERROR(hr);
#endif
}

dx11_rhi::~dx11_rhi() noexcept
{
#ifdef OEMS_DEBUG
	p_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif

	if (IsWindow(p_hwnd_))
		DestroyWindow(p_hwnd_);
	p_hwnd_ = nullptr;

	UnregisterClass(c_window_class_name, GetModuleHandle(nullptr));
}

// ----- hlsl_compute ----

hlsl_compute::hlsl_compute(ID3D11Device* p_device, const char* p_source_filename)
{
	assert(p_device);
	assert(p_source_filename);

	try {
		const std::string source_code = read_text(p_source_filename);
		ENFORCE(source_code.length() > 0, "Compute shader source file is empty. ", p_source_filename);

		p_bytecode = compile_shader(source_code, p_source_filename);

		const HRESULT hr = p_device->CreateComputeShader(
			p_bytecode->GetBufferPointer(), p_bytecode->GetBufferSize(),
			nullptr, &p_shader.ptr);

		ENFORCE(hr == S_OK, std::to_string(hr));
	}
	catch (...) {
		const std::string exc_msg = EXCEPTION_MSG("Compute shader creation error.");
		std::throw_with_nested(std::runtime_error(exc_msg));
	}
}

// ----- funcs -----

com_ptr<ID3D11Buffer> make_buffer(ID3D11Device* p_device, const D3D11_SUBRESOURCE_DATA* p_data,
	UINT byte_count, D3D11_USAGE usage, UINT bing_flags, UINT cpu_access_flags)
{
	assert(p_device);
	assert(byte_count > 0);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth		= byte_count;
	desc.Usage			= usage;
	desc.BindFlags		= bing_flags;
	desc.CPUAccessFlags = cpu_access_flags;

	com_ptr<ID3D11Buffer> buffer;
	const HRESULT hr = p_device->CreateBuffer(&desc, p_data, &buffer.ptr);
	THROW_IF_DX_ERROR(hr);

	return buffer;
}

com_ptr<ID3D11Buffer> make_structured_buffer(ID3D11Device* p_device, 
	UINT item_count, UINT item_byte_count, 
	D3D11_USAGE usage, UINT bing_flags, UINT cpu_access_flags)
{
	assert(p_device);
	assert(item_count > 0);
	assert(item_byte_count > 0);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth				= item_count * item_byte_count;
	desc.Usage					= usage;
	desc.BindFlags				= bing_flags;
	desc.CPUAccessFlags			= cpu_access_flags;
	desc.MiscFlags				= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride	= item_byte_count;

	com_ptr<ID3D11Buffer> buffer;
	HRESULT hr = p_device->CreateBuffer(&desc, nullptr, &buffer.ptr);
	THROW_IF_DX_ERROR(hr);

	return buffer;
}

void throw_if_dx_error(HRESULT hr, ID3D11Device* p_device, 
	const char* p_filename, uint64_t line_number)
{
	if (hr == S_OK) return;

	const _com_error ce(hr);
	std::stringstream stream;

	stream << p_filename << '(' << line_number << "): HRESULT 0x" << std::hex << hr << std::endl 
		<< '\t' << ce.ErrorMessage() << std::endl;

	if (p_device && (hr == DXGI_ERROR_DEVICE_REMOVED)) {
		_com_error reason(p_device->GetDeviceRemovedReason());
		stream << '\t' << reason.ErrorMessage() << std::endl;
	}

	throw std::runtime_error(stream.str());
}

} // namespace oems
