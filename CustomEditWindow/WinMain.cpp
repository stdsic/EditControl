#include "pch.h"
#include "CustomEditWindow.h"
#define CLASS_NAME L"MyEditClassTest"
#define IDC_EDIT1 0x4001

//LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnKeyUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnImeChar(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam);
//
//LRESULT OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnSetFocus(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnKillFocus(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnHScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam);
//
//LRESULT OnWindowPosChanged(HWND hWnd, WPARAM wParam, LPARAM lParam);
//LRESULT OnLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    WNDCLASS wc = {
        CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        WndProc,
        0,0,
        hInst,
        NULL, LoadCursor(NULL, IDC_ARROW),
        // (HBRUSH)(COLOR_WINDOW + 1),
        NULL,
        NULL,
        CLASS_NAME,
    };

    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        CLASS_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        (HMENU)NULL,
        hInst,
        NULL
    );

    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

CustomEditWindow g_EditWindow;
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
    RECT crt;

    switch (iMessage) {
    case WM_CREATE:
        GetClientRect(hWnd, &crt);
        g_EditWindow.Create(L"MyEditWindow", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL, 0, crt.left, crt.top, crt.right, crt.bottom, hWnd, (HMENU)(INT_PTR)IDC_EDIT1);
        return 0;
        
    case WM_SETFOCUS:
        SetFocus(g_EditWindow.Window());
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}


//LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
//    switch (iMessage) {
//    case WM_CREATE:
//        return OnCreate(hWnd, wParam, lParam);
//
//    case WM_SIZE:
//        return OnSize(hWnd, wParam, lParam);
//
//    case WM_WINDOWPOSCHANGED:
//        return OnWindowPosChanged(hWnd, wParam, lParam);
//
//    case WM_TIMER:
//        return OnTimer(hWnd, wParam, lParam);
//
//    case WM_KEYDOWN:
//        return OnKeyDown(hWnd, wParam, lParam);
//
//    case WM_KEYUP:
//        return OnKeyUp(hWnd, wParam, lParam);
//
//    case WM_PAINT:
//        return OnPaint(hWnd, wParam, lParam);
//
//    case WM_CHAR:
//        return OnChar(hWnd, wParam, lParam);
//
//    case WM_IME_CHAR:
//        return OnImeChar(hWnd, wParam, lParam);
//
//    case WM_IME_COMPOSITION:
//        return OnImeComposition(hWnd, wParam, lParam);
//
//    case WM_IME_STARTCOMPOSITION:
//        return 0;
//
//    case WM_LBUTTONDBLCLK:
//        return OnLButtonDblClk(hWnd, wParam, lParam);
//
//    case WM_LBUTTONDOWN:
//        return OnLButtonDown(hWnd, wParam, lParam);
//
//    case WM_MOUSEMOVE:
//        return OnMouseMove(hWnd, wParam, lParam);
//
//    case WM_LBUTTONUP:
//        return OnLButtonUp(hWnd, wParam, lParam);
//
//    case WM_SETFOCUS:
//        return OnSetFocus(hWnd, wParam, lParam);
//
//    case WM_KILLFOCUS:
//        return OnKillFocus(hWnd, wParam, lParam);
//
//    case WM_MOUSEWHEEL:
//        return OnMouseWheel(hWnd, wParam, lParam);
//
//    case WM_SETCURSOR:
//        return OnSetCursor(hWnd, wParam, lParam);
//
//    case WM_CONTEXTMENU:
//        return OnContextMenu(hWnd, wParam, lParam);
//
//    case WM_HSCROLL:
//        return OnHScroll(hWnd, wParam, lParam);
//
//    case WM_VSCROLL:
//        return OnVScroll(hWnd, wParam, lParam);
//
//    case WM_COMMAND:
//        return OnCommand(hWnd, wParam, lParam);
//
//    case WM_DESTROY:
//        return OnDestroy(hWnd, wParam, lParam);
//    }
//
//    return DefWindowProc(hWnd, iMessage, wParam, lParam);
//}