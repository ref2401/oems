#include "oems/dx11.h"

#include <cassert>


namespace {

using namespace oems;

constexpr char* c_window_class_name = "oems_window_class";

LRESULT CALLBACK window_proc(HWND p_hwnd, UINT message, WPARAM w_param, LPARAM l_param);


void init_dx11_stuff(HWND p_hwnd, ID3D11Device*& p_out_device, 
	ID3D11DeviceContext*& p_out_ctx, ID3D11Debug*& p_out_debug)
{
	assert(p_hwnd);

	constexpr D3D_FEATURE_LEVEL expected_feature_level = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL actual_feature_level;

	// create device & context ---
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
		D3D11_CREATE_DEVICE_DEBUG, &expected_feature_level, 1, D3D11_SDK_VERSION, 
		&p_out_device,
		&actual_feature_level,
		&p_out_ctx);

	assert(hr == S_OK);
	assert(actual_feature_level == expected_feature_level);

	// init debug interface ---
#ifdef OEMS_DEBUG
	hr = p_out_device->QueryInterface<ID3D11Debug>(&p_out_debug);
	assert(hr == S_OK);
#endif
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
	return DefWindowProc(p_hwnd, message, w_param, l_param);
}

} // namespace


namespace oems {

// ----- dx11_rhi -----

dx11_rhi::dx11_rhi()
{
	p_hwnd_ = make_window({ 60, 60 }, { 128, 128 });
	init_dx11_stuff(p_hwnd_, p_device_.ptr, p_ctx_.ptr, p_debug_.ptr);
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

} // namespace oems
