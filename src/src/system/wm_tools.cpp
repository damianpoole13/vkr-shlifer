
#include <wui/system/wm_tools.hpp>

#pragma warning(suppress: 4091)
#include <Shlobj.h>
#include <atlbase.h>

namespace wui
{

void hide_taskbar_icon(system_context &ctx)
{
	CComPtr<ITaskbarList> pBuilder;
	HRESULT hr = pBuilder.CoCreateInstance(CLSID_TaskbarList);
	if (SUCCEEDED(hr))
	{
		pBuilder->HrInit();
		pBuilder->DeleteTab(ctx.hwnd);
	}
}

void show_taskbar_icon(system_context &ctx)
{
	CComPtr<ITaskbarList> pBuilder;
	HRESULT hr = pBuilder.CoCreateInstance(CLSID_TaskbarList);
	if (SUCCEEDED(hr))
	{
		pBuilder->HrInit();
		pBuilder->AddTab(ctx.hwnd);
	}
}

rect get_screen_size(system_context &context)
{
	MONITORINFO mi = { sizeof(mi) };
	if (GetMonitorInfo(MonitorFromWindow(context.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
	{
		auto width = mi.rcMonitor.right - mi.rcMonitor.left;
		auto height = mi.rcMonitor.bottom - mi.rcMonitor.top;

		return { 0, 0, width, height };
	}

	return { 0 };
}

}
