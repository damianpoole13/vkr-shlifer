#include "pch.h"
#include "CppUnitTest.h"
#include "../include/wui/window/window.hpp"
#include "../include/wui/window/i_window.hpp"
#include "../include/wui/control/text.hpp"
#include "../include/wui/common/rect.hpp"
#include "../include/wui/common/alignment.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GUITest
{
	TEST_CLASS(GUITest)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			std::shared_ptr<wui::window> window = std::make_shared<wui::window>();
			const auto width = window->position().width(), height = window->position().height();
			const int32_t top = 40, element_height = 40, space = 30;

			std::shared_ptr<wui::text> Text = std::make_shared<wui::text>(
				"Test",
				wui::hori_alignment::center, wui::vert_alignment::center,
				"h1_text");
			wui::rect pos = { space, top, width - space, top + element_height };
			Text->set_position(pos);

			Assert::AreEqual(pos, Text->position());
		}
	};
}
