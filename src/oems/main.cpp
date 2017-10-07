#include <iostream>

#include "oems/oems_sorter.h"


void main()
{
	bool keep_console_shown = false;

	try {
		oems::dx11_rhi		dx11;
		oems::oems_sorter	sorter(dx11.p_device(), dx11.p_ctx(), dx11.p_debug());
	}
	catch (const std::exception& e) {
		keep_console_shown = true;

		const std::string msg = oems::make_exception_message(e);
		std::cout << "----- Exception -----" << std::endl << msg << std::endl;
	}

	if (keep_console_shown)
		std::cin.get();
}
