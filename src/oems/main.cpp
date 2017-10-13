#include <algorithm>
#include <iostream>
#include <random>
#include <vector>
#include "oems/oems_sorter.h"


std::vector<float> generate_test_data(size_t count)
{	
	std::random_device rd;
	std::mt19937_64 g(rd());
	const std::uniform_real_distribution<float> distr(-100.0f, 100.0f);

	std::vector<float> v(count);
	std::generate(v.begin(), v.end(), [&distr, &g] { return distr(g); });

	return v;
}

void main()
{
	bool keep_console_shown = false;

	try {
		// init ---
		oems::dx11_rhi		dx11;
		oems::oems_sorter	sorter(dx11.p_device(), dx11.p_ctx(), dx11.p_debug());

		// gen data & sort ---
		std::vector<float> data = generate_test_data(1024);
		//sorter.sort(data);

		//while (true) {
		//	bool exit = false;
		//	MSG msg;
		//	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		//		if (msg.message == WM_QUIT) {
		//			exit = true;
		//			break;
		//		};

		//		TranslateMessage(&msg);
		//		DispatchMessageA(&msg);
		//	}
		//	if (exit) break;
			sorter.sort(data);
			dx11.p_swap_chain()->Present(1, 0);
		//}
	}
	catch (const std::exception& e) {
		keep_console_shown = true;

		const std::string msg = oems::make_exception_message(e);
		std::cout << "----- Exception -----" << std::endl << msg << std::endl;
	}

	if (keep_console_shown)
		std::cin.get();
}
