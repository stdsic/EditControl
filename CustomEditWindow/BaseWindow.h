#include <windows.h>

template <class DERIVED_TYPE>
class BaseWindow {
protected:
	HWND hWnd = NULL;
	virtual LPCWSTR ClassName() const = 0;
	virtual LRESULT OnMessage(UINT iMessage, WPARAM wParam, LPARAM lParam) = 0;

public:
	static LRESULT CALLBACK WndProc(HWND _hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
		DERIVED_TYPE* ptr = NULL;
		if (iMessage == WM_NCCREATE) {
			CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
			ptr = (DERIVED_TYPE*)pCS->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ptr);
			ptr->hWnd = _hWnd;
		}
		else {
			ptr = (DERIVED_TYPE*)GetWindowLongPtr(_hWnd, GWLP_USERDATA);
		}

		if (ptr) {
			return ptr->OnMessage(iMessage, wParam, lParam);
		}
		else {
			return DefWindowProc(_hWnd, iMessage, wParam, lParam);
		}
	}

public:
	HWND Window() const { return hWnd; }

public:
	BOOL Create(
		LPCWSTR lpszWindowName,
		DWORD dwStyle = WS_OVERLAPPEDWINDOW,
		DWORD dwExStyle = 0,
		LONG x = CW_USEDEFAULT,
		LONG y = CW_USEDEFAULT,
		LONG Width = CW_USEDEFAULT,
		LONG Height = CW_USEDEFAULT,
		HWND hParent = HWND_DESKTOP,
		HMENU hMenu = NULL
	) {
		HINSTANCE hInst = GetModuleHandle(NULL);
		WNDCLASSEX wcex = { 0, };

		if (!GetClassInfoEx(hInst, ClassName(), &wcex)){
			wcex.cbSize = sizeof(wcex);
			wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
			wcex.lpfnWndProc = DERIVED_TYPE::WndProc;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = sizeof(LONG_PTR);
			wcex.hInstance = hInst;
			wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
			wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground = NULL;
			wcex.lpszMenuName = NULL;
			wcex.lpszClassName = ClassName();
			wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

			RegisterClassEx(&wcex);
		}

		hWnd = CreateWindowEx(
			dwExStyle,
			ClassName(),
			lpszWindowName,
			dwStyle,
			x, y, Width, Height,
			hParent,
			hMenu,
			hInst,
			this
		);
	
		return ((hWnd) ? TRUE : FALSE);
	}
};