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
        {WM_WINDOWPOSCHANGED, &CustomEditWindow::OnWindowPosChanged},
        {WM_GETDLGCODE, &CustomEditWindow::OnGetDlgCode},
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
    LRESULT OnGetDlgCode(WPARAM wParam, LPARAM lParam);
    LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);

private:
    LPCWSTR ClassName() const { return L"250819_CustomEditWindow_Project"; }

public:
    CustomEditWindow() : buf(NULL), Sample(L"쾅"), lineInfo(NULL), hBitmap(NULL)
    {
        memset(AsciiCharWidth, 0, sizeof(AsciiCharWidth));
        g_Option = {
            TRUE,
            FALSE,
            TRUE,
            FALSE,
        };
    }
    ~CustomEditWindow() 
    {
    }

    LRESULT OnMessage(UINT iMessage, WPARAM wParam, LPARAM lParam);

private:
    WCHAR* buf;
    BOOL bComp;
    size_t bufLength;
    size_t docLength;

    int off;
    int FontHeight;

    CONST WCHAR* Sample;
    BYTE AsciiCharWidth[128];
    BYTE HangulCharWidth;

    // 캐럿 생성
    int PrevX;
    
    // 엔터 입력
    int LineRatio, LineHeight;

    // 상하 이동
    BOOL bLineEnd, bLineFirst;

    // 자동 개행(문자셋)
    enum CustomCharset {
        CC_WHITE,           // 공백 문자(space, '\t', '\r', '\n', ...)
        CC_ALNUM,
        CC_PUNCT,
        CC_KJC,
        CC_OTHER
    };

    // 자동 개행(옵션)
    struct WrapOptions {
        BOOL wordWrap;
        BOOL trimEndSpaces;
        BOOL KeepPunctWithWord;
        BOOL kjcCharWrap;
    };
    WrapOptions g_Option;
    enum WBPType { WBP_WORD, WBP_PUNCT };
    RECT g_crt;

    // 줄 정보 관리 구조체
    struct LineInfo {
        int start, end;
    };
    LineInfo* lineInfo;

    int lineInfoSize;
    int lineCount;

    // 스크롤바
    int xPos, yPos, xMax, yMax, FontWidth;

    // 탭 문자
    int TabWidth, TabSize;

    // IME 활용 현재 입력 문자 확인
    BOOL bAlphaNum;

    // 영역 선택(드래그)
    int SelectStart, SelectEnd;
    BOOL bCapture;
    COLORREF SelectFgColor, SelectBgColor;
    int HideType;

    // 비트맵 생성
    // Bitmap* hBitmap = NULL;
    HBITMAP hBitmap;
    RECT g_wrt;

    // 정적 변수 HScroll, MouseWheel
    int Sum;

    // DialogBox에서의 키 입력 지원
    BOOL bWantTab;

private:
    // 캐럿 생성
    void SetCaret(BOOL bUpdatePrevX = TRUE, BOOL bScrollToCaret = TRUE);
    void PrecomputeCharWidths();
    int GetCharWidth(WCHAR* src, int length);

    // 삽입, 삭제
    BOOL Insert(int idx, WCHAR* str);
    BOOL Delete(int idx, int cnt);

    // 좌우 이동
    BOOL IsCRLF(int idx);
    int GetPrevOffset(int idx);
    int GetNextOffset(int idx);

    // 주 정보 조사
    void GetLine(int line, int& start, int& end);

    // 상하 이동
    void GetRowAndColumn(int idx, int& row, int& column);
    int GetOffset(int row, int column);

    // 캐럿 이동
    void GetCoordinate(int idx, int& x, int& y);

    // 디버깅 함수
    void TraceFormat(LPCWSTR format, ...);

    // 자동 개행(문자셋)
    BOOL IsWhiteChar(WCHAR ch);
    BOOL IsAlnumChar(WCHAR ch);
    BOOL IsPunctChar(WCHAR ch);
    BOOL IsKJCChar(WCHAR ch);
    enum CustomCharset GetCustomCharset(WCHAR ch);

    // 자동 개행(옵션)
    int FindWrapPoint(int start, int end);
    int WordBreakProc(int pos, int start, WBPType type);

    // 캐럿 수평 이동
    int GetDocsXPosOnLine(int row, int dest);

    // 스크롤바
    void UpdateScrollInfo();

    // 탭 문자 그리기
    int DrawLine(HDC hdc, int line);
    void DrawSegment(HDC hdc, int& x, int y, int idx, int length, BOOL ignore, COLORREF fg, COLORREF bg);

    // 캐럿 이동(마우스)
    int GetOffsetFromPoint(int x, int y);
    int GetOffsetFromPoint(POINT Mouse);

    // 선택 및 이동 기능
    BOOL IsDelims(int idx);
    int GetPrevWord(int idx);
    int GetNextWord(int idx);

    // 선택 영역 지우기/확장
    void ClearSelection();
    void ExpandSelection(int start, int end);

    // 선택 영역의 문자열 삭제
    BOOL DeleteSelection();

    // 그리기 최적화
    void Invalidate(int idx1, int idx2 = -1);

    // 문단 찾기
    int FindParagraphStart(int idx);

    // 더블 클릭(단어 선택)
    void SelectWord(int idx, int& start, int& end);

    // 줄 정보 갱신
    void RebuildLineInfo(int idx = -1, int length = -1);
};