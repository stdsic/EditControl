#include "resource.h"
#define CLASS_NAME L"MakeApiEdit"

LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnKeyUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnImeChar(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam);

LRESULT OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnSetFocus(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnKillFocus(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnHScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    // BOOL bDebugConsole = AllocConsole();

    WNDCLASS wc = {
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0,0,
        hInst,
        NULL, LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        NULL,
        CLASS_NAME,
    };

    RegisterClass(&wc);

    HWND hWnd = CreateWindow(
        CLASS_NAME,
        CLASS_NAME,
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
        NULL,
        (HMENU)NULL,
        hInst,
        NULL
    );

    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // if (bDebugConsole) { FreeConsole(); }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
    switch (iMessage) {
    case WM_CREATE:
        return OnCreate(hWnd, wParam, lParam);

    case WM_SIZE:
        return OnSize(hWnd, wParam, lParam);

    case WM_TIMER:
        return OnTimer(hWnd, wParam, lParam);

    case WM_KEYDOWN:
        return OnKeyDown(hWnd, wParam, lParam);

    case WM_KEYUP:
        return OnKeyUp(hWnd, wParam, lParam);

    case WM_PAINT:
        return OnPaint(hWnd, wParam, lParam);

    case WM_CHAR:
        return OnChar(hWnd, wParam, lParam);

    case WM_IME_CHAR:
        return OnImeChar(hWnd, wParam, lParam);

    case WM_IME_COMPOSITION:
        return OnImeComposition(hWnd, wParam, lParam);

    case WM_IME_STARTCOMPOSITION:
        return 0;

    case WM_LBUTTONDOWN:
        return OnLButtonDown(hWnd, wParam, lParam);

    case WM_MOUSEMOVE:
        return OnMouseMove(hWnd, wParam, lParam);

    case WM_LBUTTONUP:
        return OnLButtonUp(hWnd, wParam, lParam);

    case WM_SETFOCUS:
        return OnSetFocus(hWnd, wParam, lParam);

    case WM_KILLFOCUS:
        return OnKillFocus(hWnd, wParam, lParam);

    case WM_MOUSEWHEEL:
        return OnMouseWheel(hWnd, wParam, lParam);

    case WM_SETCURSOR:
        return OnSetCursor(hWnd, wParam, lParam);

    case WM_CONTEXTMENU:
        return OnContextMenu(hWnd, wParam, lParam);

    case WM_HSCROLL:
        return OnHScroll(hWnd, wParam, lParam);

    case WM_VSCROLL:
        return OnVScroll(hWnd, wParam, lParam);

    case WM_COMMAND:
        return OnCommand(hWnd, wParam, lParam);

    case WM_DESTROY:
        return OnDestroy(hWnd, wParam, lParam);
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}