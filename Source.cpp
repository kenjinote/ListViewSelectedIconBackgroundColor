#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "gdiplus")

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include "resource.h"

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)

using namespace Gdiplus;

TCHAR szClassName[] = TEXT("Window");
WNDPROC defListViewWndProc;

BOOL GetScaling(HWND hWnd, UINT* pnX, UINT* pnY)
{
	BOOL bSetScaling = FALSE;
	const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		HMODULE hShcore = LoadLibrary(TEXT("SHCORE"));
		if (hShcore)
		{
			typedef HRESULT __stdcall GetDpiForMonitor(HMONITOR, int, UINT*, UINT*);
			GetDpiForMonitor* fnGetDpiForMonitor = reinterpret_cast<GetDpiForMonitor*>(GetProcAddress(hShcore, "GetDpiForMonitor"));
			if (fnGetDpiForMonitor)
			{
				UINT uDpiX, uDpiY;
				if (SUCCEEDED(fnGetDpiForMonitor(hMonitor, 0, &uDpiX, &uDpiY)) && uDpiX > 0 && uDpiY > 0)
				{
					*pnX = uDpiX;
					*pnY = uDpiY;
					bSetScaling = TRUE;
				}
			}
			FreeLibrary(hShcore);
		}
	}
	if (!bSetScaling)
	{
		HDC hdc = GetDC(NULL);
		if (hdc)
		{
			*pnX = GetDeviceCaps(hdc, LOGPIXELSX);
			*pnY = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL, hdc);
			bSetScaling = TRUE;
		}
	}
	if (!bSetScaling)
	{
		*pnX = DEFAULT_DPI;
		*pnY = DEFAULT_DPI;
		bSetScaling = TRUE;
	}
	return bSetScaling;
}

Bitmap* LoadImageFromResource(const wchar_t* resid, const wchar_t* restype)
{
	Bitmap* pBmp = nullptr;
	HRSRC hrsrc = FindResourceW(GetModuleHandle(0), resid, restype);
	if (hrsrc)
	{
		DWORD dwResourceSize = SizeofResource(GetModuleHandle(0), hrsrc);
		if (dwResourceSize > 0)
		{
			HGLOBAL hGlobalResource = LoadResource(GetModuleHandle(0), hrsrc);
			if (hGlobalResource)
			{
				void* imagebytes = LockResource(hGlobalResource);
				HGLOBAL hGlobal = GlobalAlloc(GHND, dwResourceSize);
				if (hGlobal)
				{
					void* pBuffer = GlobalLock(hGlobal);
					if (pBuffer)
					{
						memcpy(pBuffer, imagebytes, dwResourceSize);
						IStream* pStream = nullptr;
						HRESULT hr = CreateStreamOnHGlobal(hGlobal, FALSE, &pStream);
						if (SUCCEEDED(hr))
						{
							pBmp = new Bitmap(pStream);
						}
						if (pStream)
						{
							pStream->Release();
							pStream = nullptr;
						}
					}
					GlobalFree(hGlobal);
					hGlobal = nullptr;
				}
			}
		}
	}
	return pBmp;
}

HBITMAP GetHBitmapFromBitmap(Bitmap* pBitmap, int nWidth, int nHeight)
{
	Bitmap* img = new Bitmap(nWidth, nHeight);
	{
		Graphics g(img);
		g.DrawImage(pBitmap, RectF((REAL)0, (REAL)0, (REAL)nWidth, (REAL)nHeight), (REAL)0, (REAL)0, (REAL)pBitmap->GetWidth(), (REAL)pBitmap->GetHeight(), UnitPixel);
	}
	HBITMAP hBitmap = NULL;
	img->GetHBITMAP(Color(), &hBitmap);
	delete img;
	return hBitmap;
}

LRESULT CALLBACK ListViewProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int nSelectItemIndex = -1;
	switch (msg)
	{
	case WM_MOUSEMOVE:
		{
			LV_HITTESTINFO lvhti = {};
			lvhti.pt.x = LOWORD(lParam);
			lvhti.pt.y = HIWORD(lParam);
			ListView_HitTest(hWnd, &lvhti);
			if (lvhti.flags & LVHT_ONITEM)
			{
				if (nSelectItemIndex != lvhti.iItem)
				{
					LVITEM lvi = {};
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					lvi.iItem = lvhti.iItem;
					lvi.state = LVIS_SELECTED;
					ListView_SetItem(hWnd, &lvi);
					nSelectItemIndex = lvhti.iItem;

					TRACKMOUSEEVENT tme;
					tme.cbSize = sizeof(tme);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hWnd;
					_TrackMouseEvent(&tme);
				}
			}
			else
			{
				int iItem = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
				if (iItem != -1)
				{
					LVITEM lvi = {};
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					SendMessage(hWnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvi);
				}
				nSelectItemIndex = -1;
			}
		}
		break;
	case WM_MOUSELEAVE:
		{
			int iItem = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
			if (iItem != -1)
			{
				LVITEM lvi = {};
				lvi.mask = LVIF_STATE;
				lvi.stateMask = LVIS_SELECTED;
				SendMessage(hWnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvi);
			}
			nSelectItemIndex = -1;
		}
		break;
	case WM_DESTROY:
		nSelectItemIndex = -1;
		break;
	default:
		break;
	}
	return CallWindowProc(defListViewWndProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hList;
	static HIMAGELIST hImgList;
	static HFONT hFont;
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	switch (msg)
	{
	case WM_CREATE:
		InitCommonControls();
		hList = CreateWindow(WC_LISTVIEW, 0, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)(lParam))->hInstance, NULL);
		defListViewWndProc = (WNDPROC)SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListViewProc1);
		ListView_SetExtendedListViewStyleEx(hList, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
		ListView_SetIconSpacing(hList, 201, 90);
		{
			hImgList = ImageList_Create(201, 90, ILC_COLOR32, 1, 0);
			Bitmap* pBitmap = LoadImageFromResource(MAKEINTRESOURCE(IDB_PNG1), L"PNG");
			if (pBitmap)
			{
				HBITMAP hBitmap = GetHBitmapFromBitmap(pBitmap, 201, 90);
				if (hBitmap)
				{
					ImageList_Add(hImgList, hBitmap, 0);
					DeleteObject(hBitmap);
				}
				delete pBitmap;
			}
		}
		{
			ListView_SetImageList(hList, hImgList, LVSIL_NORMAL);
			LV_ITEM lvi = {};
			lvi.mask = LVIF_IMAGE;
			lvi.iSubItem = 0;
			for (int i = 0; i < 100; i++)
			{
				lvi.iItem = i;
				lvi.iImage = 0;
				ListView_InsertItem(hList, &lvi);
			}
		}		
		SendMessage(hList, WM_APP, 0, 0);
		break;
	case WM_NOTIFY:
		{
			if (((LPNMHDR)lParam)->hwndFrom == hList)
			{
				if (((LPNMLISTVIEW)lParam)->hdr.code == NM_CUSTOMDRAW)
				{
					NMLVCUSTOMDRAW* lvcd = (NMLVCUSTOMDRAW*)lParam;
					NMCUSTOMDRAW& nmcd = lvcd->nmcd;
					switch (nmcd.dwDrawStage)
					{
					case CDDS_PREPAINT:
						return CDRF_NOTIFYITEMDRAW;
					case CDDS_ITEMPREPAINT:
						{
							RECT rect;
							ListView_GetItemRect(hList, lvcd->nmcd.dwItemSpec, &rect, LVIR_ICON);
							if (ListView_GetItemState(hList, lvcd->nmcd.dwItemSpec, LVIS_SELECTED) == (LVIS_SELECTED))
							{
								HBRUSH hBrush = CreateSolidBrush(RGB(153, 201, 239));
								FillRect(lvcd->nmcd.hdc, &rect, hBrush);
								DeleteObject(hBrush);
							}
							ImageList_Draw(hImgList, 0, lvcd->nmcd.hdc, rect.left, rect.top, ILD_TRANSPARENT);
						}
						return CDRF_SKIPDEFAULT;
					}
					return CDRF_DODEFAULT;
				}
			}
		}
		break;
	case WM_SIZE:
		MoveWindow(hList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		ListView_Arrange(hList, LVA_ALIGNLEFT);
		break;
	case WM_NCCREATE:
		{
			const HMODULE hModUser32 = GetModuleHandle(TEXT("user32.dll"));
			if (hModUser32)
			{
				typedef BOOL(WINAPI*fnTypeEnableNCScaling)(HWND);
				const fnTypeEnableNCScaling fnEnableNCScaling = (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
				if (fnEnableNCScaling)
				{
					fnEnableNCScaling(hWnd);
				}
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_APP:
		GetScaling(hWnd, &uDpiX, &uDpiY);
		DeleteObject(hFont);
		hFont = CreateFontW(-POINT2PIXEL(10), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, L"MS Shell Dlg");
		SendMessage(hList, WM_SETFONT, (WPARAM)hFont, 0);
		break;
	case WM_DPICHANGED:
		SendMessage(hList, WM_APP, 0, 0);
		break;
	case WM_DESTROY:
		ImageList_Destroy(hImgList);
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
{
	ULONG_PTR gdiToken;
	GdiplusStartupInput gdiSI;
	GdiplusStartup(&gdiToken, &gdiSI, NULL);
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("Window"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}
