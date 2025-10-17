#include "BaseWindow.h"

class CustomEditWindow : public BaseWindow<CustomEditWindow> {
	static const int _nMsg = 0x400;
	
	typedef struct tag_MSGMAP {
		UINT iMessage;
		LRESULT(CustomEditWindow::* lpfnWndProc)(WPARAM, LPARAM);
	}MSGMAP;

    MSGMAP MainMsg[_nMsg] = {
        {WM_PAINT, &CustomEditWindow::OnPaint},
        {WM_TIMER, &CustomEditWindow::OnTimer},
        {WM_SIZE, &CustomEditWindow::OnSize},
        {WM_KEYDOWN, &CustomEditWindow::OnKeyDown},
        {WM_CHAR, &CustomEditWindow::OnChar},
        {WM_IME_CHAR, &CustomEditWindow::OnImeChar},
        {WM_IME_COMPOSITION, &CustomEditWindow::OnImeComposition},
        {WM_IME_STARTCOMPOSITION, &CustomEditWindow::OnImeStartComposition},
        {WM_LBUTTONDBLCLK, &CustomEditWindow::OnLButtonDblClk},
        {WM_LBUTTONDOWN, &CustomEditWindow::OnLButtonDown},
        {WM_MOUSEMOVE, &CustomEditWindow::OnMouseMove},
        {WM_LBUTTONUP, &CustomEditWindow::OnLButtonUp},
        {WM_SETFOCUS, &CustomEditWindow::OnSetFocus},
        {WM_KILLFOCUS, &CustomEditWindow::OnKillFocus},
        {WM_MOUSEWHEEL, &CustomEditWindow::OnMouseWheel},
        {WM_CONTEXTMENU, &CustomEditWindow::OnContextMenu},
        {WM_HSCROLL, &CustomEditWindow::OnHScroll},
        {WM_VSCROLL, &CustomEditWindow::OnVScroll},
        {WM_COMMAND, &CustomEditWindow::OnCommand},
        {WM_WINDOWPOSCHANGED, &CustomEditWindow::OnWindowPosChanged },
        {WM_CREATE, &CustomEditWindow::OnCreate},
        {WM_DESTROY, &CustomEditWindow::OnDestroy},
    };

private:
    LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
    LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
    LRESULT OnSize(WPARAM wParam, LPARAM lParam);
    LRESULT OnKeyDown(WPARAM wParam, LPARAM lParam);
    LRESULT OnChar(WPARAM wParam, LPARAM lParam);
    LRESULT OnImeChar(WPARAM wParam, LPARAM lParam);
    LRESULT OnImeComposition(WPARAM wParam, LPARAM lParam);
    LRESULT OnImeStartComposition(WPARAM wParam, LPARAM lParam);
    LRESULT OnLButtonDblClk(WPARAM wParam, LPARAM lParam);
    LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);
    LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam);
    LRESULT OnLButtonUp(WPARAM wParam, LPARAM lParam);
    LRESULT OnSetFocus(WPARAM wParam, LPARAM lParam);
    LRESULT OnKillFocus(WPARAM wParam, LPARAM lParam);
    LRESULT OnMouseWheel(WPARAM wParam, LPARAM lParam);
    LRESULT OnContextMenu(WPARAM wParam, LPARAM lParam);
    LRESULT OnHScroll(WPARAM wParam, LPARAM lParam);
    LRESULT OnVScroll(WPARAM wParam, LPARAM lParam);
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnWindowPosChanged(WPARAM wParam, LPARAM lParam);
    LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);

private:
    LPCWSTR ClassName() const { return L"250819_CustomEditWindow_Project"; }

public:
    CustomEditWindow();
    ~CustomEditWindow();

    LRESULT OnMessage(UINT iMessage, WPARAM wParam, LPARAM lParam);
};