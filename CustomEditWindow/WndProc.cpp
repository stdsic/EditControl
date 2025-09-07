#include "resource.h"
#define CARET_WIDTH 2

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define GET_X_LPARAM(lParam) (int)(short)LOWORD(lParam)
#define GET_Y_LPARAM(lParam) (int)(short)HIWORD(lParam)

WCHAR* buf;
BOOL bComp;
size_t bufLength;
size_t docLength;

int off;
int FontHeight;
HWND hWndMain;

CONST WCHAR* Sample = L"쾅";
BYTE AsciiCharWidth[128];
BYTE HangulCharWidth;

// 캐럿 생성
int PrevX;
void SetCaret(BOOL bUpdatePrevX = TRUE);
void PrecomputeCharWidths();
int GetCharWidth(WCHAR* src, int length);

// 삽입, 삭제
BOOL Insert(int idx, WCHAR* str);
BOOL Delete(int idx, int cnt);

// 좌우 이동
BOOL IsCRLF(int idx);
int GetPrevOffset(int idx);
int GetNextOffset(int idx);

// 엔터 입력
int LineRatio, LineHeight;
// OnChar에서 Enter키가 입력되면 CRLF 문자를 조립하도록 만들었다.
// 이제 엔터를 적용하여 다음 줄부터 캐럿과 문자가 출력되어야 한다.
// OnPaint에서 직접 그려야 하는데 이때 두 가지 방법이 있다.

// 하나는 GetLineRange 따위의 함수를 이용하여 한 행의 범위를 구하는 것이고
// 다른 하나는 LineIndices[0x10000] 따위의 배열을 만들어 줄의 시작 위치를 기록하여 관리하는 것이다.

// 둘 다 장단점이 있으나, 최적화를 위해선 두 번째 방법을 결국 선택하게 되어 있다.
// 다만, 실습의 목적은 알고리즘 연구와 떠오르는대로, 생각나는대로 에디터의 기능을 구현해보는 것이므로
// 첫 번째 방법을 사용해보기로 한다.
// 차후 최적화에서 줄 정보를 유지/관리한다.

// 다음은 행 번호를 넘기면 줄의 시작과 끝 지점을 조사하는 함수이다.
// GetLine이 주요 함수이며 FindLineEnd는 유틸리티이다.
// int FindLineEnd(WCHAR*& p);
void GetLine(int line, int& start, int& end);


// 상하 이동
// 위 GetLine 함수를 이용해서 문자를 출력하는 것까지 완료했다.
// 다만, 캐럿이 다음 줄 입력을 인식하지 못한다.
// 또한, 커서 이동키로 캐럿을 이동하는 동작도 아직 구현하지 않았다.

// 위 기능들을 구현하기 위해선 캐럿의 위치를 화면상의 픽셀 좌표값으로 계산할 수 있어야 한다.
// 또, 실제로 문자를 중간에 삽입하는 등의 동작을 하려면 오프셋 값이 필요하다.
// off의 값이 몇 번째 행의 몇 번째 열에 위치하는지 알아야 이러한 기능을 구현할 수 있으므로 일단 이와 관련된 함수부터 작성해보자

BOOL bLineEnd = FALSE;
void GetRowAndColumn(int idx, int& row, int& column);
int GetOffset(int row, int column);

// 캐럿 이동
// 앞에서 오프셋을 이용해 행과 열을 구하는 함수를 작성해봤다.
// 이로부터 다시 화면 좌표값을 계산할 수 있어야 하는데, 별로 어렵지 않다
// 간단히 줄간을 이용해 높이를 계산하고 폭을 계산하기만 하면 된다.
void GetCoordinate(int idx, int& x, int& y);
// GetCoordinate에서 오류가 발생하여 이를 추적하기 위해 디버깅 메세지 함수를 추가한다.
void TraceFormat(LPCWSTR format, ...);

// 상하 이동(방향키)
// 범위 확인이 필요하므로 전체 행(줄)의 수를 계산하는 함수를 추가한다.
// OnKeyDown 메세지에 VK_UP과 VK_DOWN case 문을 추가했다.
// int GetLineCount();


// 자동 개행
// 에디트에서 가장 어려운 기능인 자동 개행을 구현해보자.
// 시작하기 전에 명확한 규칙을 만들고 최대한 일반화하여 함수를 만들어놔야 나중에 말썽이 없다.
// 가장 먼저 고려해야할 것은 "어떤 언어를 사용하는가"이다.
// 영어와 한글, 히라가나, 가타카나, 간체, 번체 등 국가별 자연어뿐만 아니라
// 컴퓨터 언어, 숫자, 이모지, 픽토그램, 기호 등의 문자 따위도 고려해야 한다.
// 뭔가 굉장히 복잡할 것 같지만 다행히 위 프로젝트에서는 몇 가지 상황만 고려하면 된다.

// 예를 들어, 한글의 경우 "안녕하세요", "반갑습니다", "나는", "바보다" 등 단어 단위로 끊어 개행하는 것이 일반적이며, 영어도 마찬가지이다.
// 숫자나 이모지, 픽토그램, 기호 등은 볼 것도 없이 한 문자 단위로 개행하면 되고, 나머지 일본어와 중국어는 미안하지만 알바아니다.
// 단어는 커녕 구문도 잘 모르는데 찾아서 구현해봐야 정확성도 낮고 사용하지 않아 의미도 없다.

// 단어를 기준으로 개행할 때는 대부분 공백이나 구두점 따위가 구분자로 활용된다.
// 예를 들어, "Let's play together!", "우리 같이 놀자!", "Let's Keep in touch.", "연락하고 지내자." 등의 문장에서는 공백, 느낌표, 마침표 등을 구분자로 활용하면 자동 개행을 쉽게 구현할 수 있을 것이다.
// 에디터에 따라 탭, 콜론, 세미콜론, 괄호, 중괄호, 대괄호, 느낌표, 물결표, 우물 정(井)자, 별표, 빼기 더하기, 등호 등의 기호를 자동 개행 구분자로 활용하기도 하는데 이는 나중에 필요하면 추가하기로 한다.

// 위와 같은 평문과 달리 아래의 예시처럼 언어를 혼합해서 사용하는 경우도 있다.
// "이 프로젝트는 deadline이 빠듯해서, 미리 준비해야 해."
// "당신을 정말 {(x^2 + y^2 - 1)^3 - x^2 * y^3}해요."
// "This is a hard-to-solve, complex problem."
// "어이가 혼또니 ありません."

// 이런 문장에서도 자동 개행 기능을 지원하려면 구분자를 무엇으로 정해야 할까?
// 일단, 문제가 되는 상황을 직접 확인해보자.

// 1번 문장
// ".... 이 프로젝트는 deadline
// 이 빠듯해서, 미리 준비해야해."

// 2번 문장
// "당신을 정말 {(x^2 + y^2 - 1)
// ^3 - x^2 * y^3}해요."

// 3번 문장
// "........... This is a hard
// -to-solve, complex problem."

// 4번 문장
// "어이가 혼또니 あり
// ません."

// 사용자가 직접 엔터를 눌러 개행한 것이 아니라 에디터 크기에 맞춰 글자를 자동 개행한 상황이라 가정한 것이다.
// 줄바꿈 시 하이픈(-)을 고려해야 하는 영어나, 띄어쓰기가 거의 없는 일본어 등은 특히 문제가 심각해지며, 좀더 일반화된 규칙이 필요하다.

// 여기서 주목해야할 것은 문자셋이다.
// 유니코드 문자 체계를 공부한 사람이라면 어떻게 문자가 조립되고 각 국의 언어가 어떤 범위를 갖는지 대충은 들어봤을 것이다.

// 영어와 숫자는 ASCII로 범주를 나눌 수 있고 한글과 일본어, 중국어는 어처피 유니코드이므로 KJC 따위의 범주로 나누어볼 수 있다.
// 이렇게 범주를 나누어 구분자를 정하게 되면 위 예시 문장의 문제를 대부분 해결할 수 있다.
// 가령, 2번, 3번, 4번 문장은 다음과 같이 이쁘게 정렬될 것이다.

// 2번 문장
// "당신을 정말 
// {(x^2 + y^2 - 1) ^3 - x^2 * y^3}해요."

// 3번 문장
// "........... This is a 
// hard-to-solve, complex
// problem."

// 4번 문장
// "어이가 혼또니 
// ありません."

// 어째서 이렇게 되는가 하면,
// ASCII 문자셋에선 공백 문자, 영문자와 숫자, 구두점(쉼표, 따옴표, 마침표, 물음표, 느낌표 등)을 서로 구분하도록 하고
// 유니코드 문자셋에선 한일중 문자, 그외 문자(픽토그램 등)로 범주를 나누면 위와 같은 결과가 된다.
// 여기서 공백 문자, 영문자와 숫자, 구두점은 모두 ASCII에 속하는데, 왜 서로 구분하여 관리하는지 궁금할 수 있다.
// 이유는 좀만 생각해보면 알 수 있는데, 공백 문자(White Char)를 따로 나누는 이유는 선택적 개행 지점(개행 후보)과 강제 개행(제어 코드)을 구분할 수 있어야 하기 때문이다.
// 또, 영문자와 숫자의 경우 한 범주로 묶었는데 이는 영어 아이디를 떠올리면 쉽게 이해할 수 있다.
// "stdsic123"과 같은 아이디를 적어뒀다고 가정해보자.

// 숫자와 영어를 서로 다른 범주로 보고 자동 개행 조건을 추가해둔다면
// "stdsic123"에서 "123"만 분리하여 자동 개행을 수행할 것이다.
// 즉, 아무런 의미도 없는 "123"만 덩그러니 개행되어 무슨 의미를 갖는지 알아보기 힘들 것이다.
// 이런 이유로 영어와 숫자는 그냥 한 덩어리로 처리해버리는게 좋다.

// 마지막으로 구두점은 단어와 단어 사이, 또는 문장 끝에 붙는게 일반적이다.
// 만약, "안녕하세요!"와 같은 문장에서 "!"만 떼어내어 자동 개행 해버리면 어떻게 될까?
// 물어볼 것도 없이 아주 꼴 뵈기 싫을 것이다.
// 애초에 구두점은 단어 자체가 아니라 부속 기호이기 때문에 앞 단어와 붙여써야 의미를 가진다.

// 이제, 문자셋을 나눠보자.
// 여기서 말하는 문자셋은 실제 ASCII, EUC-KR, Unicode 같은 문자셋이 아니라
// 이 프로그램 내에서만 사용할 "사용자 정의 단위"이다.
enum CustomCharset {
    CC_WHITE,           // 공백 문자(space, '\t', '\r', '\n', ...)
    CC_ALNUM,
    CC_PUNCT,
    CC_KJC,
    CC_OTHER
};

BOOL IsWhiteChar(WCHAR ch);
BOOL IsAlnumChar(WCHAR ch);
BOOL IsPunctChar(WCHAR ch);
BOOL IsKJCChar(WCHAR ch);
enum CustomCharset GetCustomCharset(WCHAR ch);

// 이제 자동 개행 기능을 구현해보자.
// 자동 개행은 개행 후보 지점을 찾는 것이 핵심이다.
// GetLine이나 FindLineEnd에서 이를 수행하면 될 것 같은데 일반화하여 별개의 함수로 만들고자 한다.
// 우선, 창이 가지는 화면 폭에 반응하여 개행이 이뤄져야 하는데
// 이때 폭 제한으로 인해 다음 줄로 넘겨야 하는 위치가 단어의 중간이라면,
// 왼쪽으로 이동해서 지정해둔 구분자(규칙)를 활용해 끊는 지점을 변경해야 한다.
// 또, 단어의 뒤에 붙는 구두점 따위가 혼자 잘려나가 다음 줄로 이동한 경우에는
// 앞에 있는 단어 전체를 끌어다가 다음 줄로 함께 내려야 한다.
// 반드시 그러한 것은 아니지만 이렇게 해야 줄 정보 처리가 가장 간단해진다.
// 일단, 이러한 규칙들만 고려해서 자동 개행 함수를 만들어보자.

struct WrapOptions {
    BOOL wordWrap;
    BOOL trimEndSpaces;
    BOOL KeepPunctWithWord;
    BOOL kjcCharWrap;
} g_Option = {
  TRUE,
  FALSE,
  TRUE,
  FALSE,
};

enum WBPType { WBP_WORD, WBP_PUNCT };
RECT g_crt;
int FindWrapPoint(int start, int end);
int WordBreakProc(int pos, int start, WBPType type);

// FindWrapPoint 함수는 줄 정보(start, end)를 전달 받는다.
// 곧, GetLine 함수 내부에서 활용되며 FindLineEnd와 같은 서브 함수이다.
// FindWrapPoint가 반환한 자동 개행 지점을 GetLine의 출력 인수 start와 end에 대입하기만 하면 OnPaint 코드에 의해 자동 개행 기능이 멋들어지게 동작할 것이다.

// 이제 GetLine 함수를 수정해보자.
// FindWrapPoint는 포인터 대신 인덱스를 이용해 개행할 지점을 찾는다.
// 문자셋이라는 개념을 추가하여 포인터가 하던 동작을 대체한 것인데
// 포인터를 증가시키며 문자를 탐색하고, ptr - buf문으로부터 얻은 오프셋 값을 활용하던 기존 로직과 논리는 동일하다.


// 다만, 지금 구조를 보면 GetLine -> FindLineEnd -> FindWrapPoint 순으로 함수가 호출되어야 하는데,
// FindWrapPoint로 인해 GetLine을 호출하는 비용이 더 늘어났다.
// GetCoordinate, GetRowAndColumn 등의 함수에서 GetLine을 항상 호출하는데,
// 안그래도 느린 상황에 자동 개행 로직이 추가되면서 속도가 더 느려졌다.

// 속도 향상을 위해 최적화가 필요한 시점이며,
// 앞에서 언급했듯이 줄 정보 구조체를 만들어 유지/관리하면 속도를 대폭 향상시킬 수 있다.

// 이제 줄 정보를 관리할 구조체를 만들어보자.
// GetLine이 담당하던 줄 정보 조사 동작을 미리 수행하여 유지/관리할 것이므로
// 당연히 줄의 시작 지점과 줄의 끝 지점 정보를 담아야 할 것이다.
struct LineInfo {
    int start, end;
};
LineInfo* lineInfo = NULL;
int lineInfoSize = 0x400;
int lineCount = 0;

// 당장 떠오르는 멤버 변수는 start와 end 뿐이므로 이렇게 두고 줄 정보를 조사하는 함수나 추가해보자.
void RebuildLineInfo();

// 줄 정보를 만들었으므로 GetLine 함수의 호출부를 찾아 하나씩 바꿔주면 된다.
// GetLine 함수 역시 줄 정보를 사용할 수 있으며, 코드가 훨씬 짧아진다.
// 또, FindLineEnd가 할 일이 없어졌는데 이제 필요 없으므로 과감하게 지워버리도록 하자.

// 이번엔 캐럿의 이동을 보자.
// 방향키를 이용해 캐럿을 상,하로 이동시켜보면 아주 조잡하게 움직인다는 것을 알 수 있다.
// 일반적인 에디터 프로그램은 이전 위치를 기억하여 캐럿이 이전 줄이나 다음 줄로 이동할 오프셋값을 이용하여 같은 위치로 이동시킨다.
// SetCaret은 GetCoordinate 함수를 호출하여 화면상의 x좌표를 구해오는데 이를 기억해두었다가
// 캐럿 이동에 활용해보자.
int GetDocsXPosOnLine(int row, int dest);

LRESULT OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}

LRESULT OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}

LRESULT OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}

LRESULT OnSetFocus(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    SetCaret(FALSE);
    return 0;
}

LRESULT OnKillFocus(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DestroyCaret();
    return 0;
}
LRESULT OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}
LRESULT OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}
LRESULT OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}
LRESULT OnHScroll(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}
LRESULT OnVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}
LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}
LRESULT OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}
LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    if (wParam != SIZE_MINIMIZED) {
        if (GetFocus() == hWnd) {
            SetCaret();
        }
        GetClientRect(hWnd, &g_crt);
        RebuildLineInfo();
    }
    return 0;

}
LRESULT OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    int row = 0, column = 0;
    int start = 0, end = 0, toff = 0;
    
    
    switch (wParam) {
    case VK_UP:
        GetRowAndColumn(off, row, column);
        if (row > 0) {
            row--;
            off = GetDocsXPosOnLine(row, PrevX);
            SetCaret(FALSE);
        }
        break;

    case VK_DOWN:
        GetRowAndColumn(off, row, column);
        if (row < lineCount - 1) {
            row++;
            off = GetDocsXPosOnLine(row, PrevX);
            SetCaret(FALSE);
        }
        break;


    // VK_LEFT와 VK_RIGHT 코드를 수정했다. 코드가 구조적으로 작성되어 자세히 살펴보지 않으면 흐름을 파악하기 어렵다.
    // 기존 코드는 GetNextOffset이나 GetPrevOffset으로 오프셋 값을 조정한 후 SetCaret을 호출하여 캐럿의 위치를 이동시키는 단순한 구조였다.
    // SetCaret은 내부적으로 GetCoordinate를 호출하는데 이 함수가 화면상의 좌표를 계산해서 반환하면 SetCaret은 캐럿을 옮기는 동작만 한다.
    // 여기서 중요한건 SetCaret 함수가 언제나 현재 위치, 즉 전역 변수 off를 참조한다는 것인데
    // 오프셋 값을 계산하고 캐럿의 위치를 옮기는 이러한 구조에선 반드시 현재 오프셋 값을 기준으로 분기문을 만들어야 한다.
    
    // 이게 무슨말인가 하면, 다음과 같은 구조로는 코드를 작성할 수 없다는 것이다.
    // toff = GetNextOffset(off);
    // GetRowAndColumn(toff, row, column);
    // GetLine(row, start, end);
    // if(toff == end && buf[end] != '\r'){
    //      ...
    // }
    // else{ 
    //      ...
    //      off = toff;
    // }
    // SetCaret();
    // 대뜸 결론부터 말하니 왜 위와 같은 구조로는 코드를 작성할 수 없다는 것인지 이해하기 어려울텐데 함께 정리해보자.
    
    // GetRowAndColumn 함수는 이분 탐색 알고리즘을 사용한다.
    // 이분 탐색은 그 특성상 중간값(row)을 가지는데 이 값이 항상 큰 것에서 작은 것으로, 작은 것에서 큰 것으로 순차적으로 변한다.
    // 예를 들어, 문제가 생기는 idx가 첫 번째 줄에 있고 그 값이 20이라고 해보자.
    // 즉, 자동 개행된 지점이 lineInfo[0].end = 20인 것이다.
    
    // 앞서 말했듯, 자동 개행된 경우 start와 end는 서로 동일한 값을 가진다.
    // lineInfo[0].end와 lineInfo[1].start가 같다는 말이다.
    
    // 여기서 행의 값, 즉 row는 (left + right) / 2 연산식에 의해 결정된다.
    // (left + right)의 값이 짝수건 홀수건 항상 1 다음에 0의 순서가 온다.
    // 즉, if(start == idx)의 분기가 항상 먼저 실행된다.
    // 다른 경우도 마찬가지로 이런 순서를 따른다.
    
    // 이는 VK_LEFT나 VK_RIGHT를 어떤 식으로 작성하건 바꿀 수 없다.
    // 이 구조가 싫으면 GetRowAndColumn의 로직을 바꿔야 하는데 다른 방법도 마땅치가 않다.
    
    // GetRowAndColumn 함수의 로직을 유지하고 캐럿을 줄 끝에 위치시키려면 if(start == idx) 분기에서 처리해야 한다.
    // 어떤 분기에서 어떤 처리를 하느냐는 프로그램마다 다르겠지만,
    // 자동 개행 기능을 지원하는 에디터는 모두 GetRowAndColumn에 있는 세 가지 분기문을 필요로 할 것이다.
    
    // 자동 개행 기능은 훌륭한 서비스이지만, 메모리상의 오프셋과 화면상의 오프셋을 절대 1:1 매칭시킬 수 없다는 단점이 있다.
    // 이를 매핑할 로직을 추가하여야 하는데 이때 반드시 줄의 끝과 처음에 대한 분기가 필요하다.
    
    // GetRowAndColumn도 이러한 이유로 위와 같은 로직을 갖게 되었다.
    // 현재 위 함수에서 줄 끝의 열 번호를 가져오는 분기문은 다음과 같다.
    // if(start == idx){
    //      if(row > 0 && bLineEnd) { row--; }
    // }
    // ...
    // column = idx - lineInfo[row].start;
    // 처음 목표로 했던 줄 끝에 캐럿이 위치하도록 만들기 위해 row를 감소시켰다.
    // 이후 SetCaret을 호출하여 캐럿을 이동시키고 화면에 출력한다.
    // 
    case VK_LEFT:
        if (off > 0) {
            GetRowAndColumn(off, row, column);
            GetLine(row, start, end);

            if (off == start) {
                if (buf[GetPrevOffset(off)] == '\r') {
                    off = GetPrevOffset(off);
                    bLineEnd = FALSE;
                }else{
                    bLineEnd = TRUE;
                }
            }
            else {
                off = GetPrevOffset(off);
                bLineEnd = FALSE;
            }
            SetCaret();
        }
        break;

    case VK_RIGHT:
        if (off < wcslen(buf)) {
            toff = GetNextOffset(off);
            GetRowAndColumn(toff, row, column);
            GetLine(row, start, end);
            if(toff == start && buf[start] != '\r'){
                bLineEnd = TRUE;
            }
            else{ 
                bLineEnd = FALSE;
                off = toff;
            }
            SetCaret();
            /*
            GetRowAndColumn(off, row, column);
            GetLine(row, start, end);

            if (off == end) {
                if (buf[end] == '\r') {
                    off = GetNextOffset(off);
                }
                bLineEnd = FALSE;
            }
            else {
                off = GetNextOffset(off);
                if (off == end && buf[off] != '\r') {
                    bLineEnd = TRUE;
                }
                else {
                    bLineEnd = FALSE;
                }
            }
            SetCaret();
            */
        }
        break;

    case VK_DELETE:
        if (IsCRLF(off)) {
            Delete(off, 2);
        }
        else {
            Delete(off, 1);
        }
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case VK_BACK:
        if (off == 0) { break; }
        off = GetPrevOffset(off);
        if (IsCRLF(off)) {
            Delete(off, 2);
        }
        else {
            Delete(off, 1);
        }
        InvalidateRect(hWnd, NULL, TRUE);
        SetCaret();
        break;

    case VK_HOME:
        GetRowAndColumn(off, row, column);
        off = GetOffset(row, 0);
        SetCaret();
        break;

    case VK_END:
        GetRowAndColumn(off, row, column);
        off = GetOffset(row, 2147483647);
        SetCaret();
        break;
    }

    WCHAR title[512];
    wsprintf(title, L"off = %d", off);
    SetWindowText(hWnd, title);

    return 0;
}
LRESULT OnKeyUp(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}

LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    int start, end;
    for (int i = 0; i < lineCount; i++) {
        start = lineInfo[i].start;
        end = lineInfo[i].end;
        TextOut(hdc, 0, i * LineHeight, buf + start, end - start);
    }
    EndPaint(hWnd, &ps);
    return 0;
}

LRESULT OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    WCHAR Abuf[0x10];
    char ch = wParam;

    if ((ch < ' ' /* 제어코드(~32) */ && ch != '\r' && ch != '\t') || ch == 127 /* Ctrl + BS */) { return 0; }
    if (ch == '\r') {
        Abuf[0] = '\r';
        Abuf[1] = '\n';
        Abuf[2] = 0;
    }
    else {
        Abuf[0] = ch;
        Abuf[1] = 0;
    }

    for (int i = 0; i < LOWORD(lParam); i++) {
        // 문자 입력 코드
        Insert(off, Abuf);
        off += wcslen(Abuf);
    }

    bComp = FALSE;
    InvalidateRect(hWnd, NULL, TRUE);
    SetCaret();
    return 0;
}

LRESULT OnImeChar(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    WCHAR Wbuf[0x10];

    Wbuf[0] = wParam;
    Wbuf[1] = 0;

    if (bComp) {
        off -= 2;
        Delete(off, 1);
    }
    Insert(off, Wbuf);
    off += wcslen(Wbuf);
    bComp = FALSE;
    InvalidateRect(hWnd, NULL, TRUE);
    SetCaret();
    return 0;
}

LRESULT OnImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    int Length = 0;
    HIMC hImc = NULL;
    WCHAR* Cbuf = NULL;

    if (lParam & GCS_COMPSTR) {
        hImc = ImmGetContext(hWnd);
        Length = ImmGetCompositionString(hImc, GCS_COMPSTR, NULL, 0);
        Cbuf = (WCHAR*)malloc(sizeof(WCHAR) * (Length + 1));
        memset(Cbuf, 0, sizeof(WCHAR) * (Length + 1));
        ImmGetCompositionString(hImc, GCS_COMPSTR, Cbuf, Length);
        Cbuf[Length] = 0;

        if (bComp) {
            off -= 2;
            Delete(off, 1);
        }

        if (Length == 0) {
            bComp = FALSE;
        }
        else {
            bComp = TRUE;
        }

        Insert(off, Cbuf);
        off += Length;
        ImmReleaseContext(hWnd, hImc);
        free(Cbuf);
        InvalidateRect(hWnd, NULL, TRUE);
        SetCaret();
    }

    return DefWindowProc(hWnd, WM_IME_COMPOSITION, wParam, lParam);
}

LRESULT OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    hWndMain = hWnd;
    bufLength = 0x1000;
    docLength = 0;
    bComp = FALSE;
    PrevX = 0;

    buf = (WCHAR*)malloc(sizeof(WCHAR) * bufLength);
    memset(buf, 0, sizeof(WCHAR) * bufLength);
    wcscpy_s(buf, bufLength, L"아 디버깅 하는거 힘듭니다 별 문제는 없는거 같습니다.\r\n동해물과 백두산이 마르고 닳도록 하느님이 보우하사 우리나라 만세. 무궁화 삼천리 화려강산 대한사람 대한으로 길이 보전하세.\r\nabcdefghijklmnopqrstuvwxyz\r\nabcdefghijklmnopqrstuvwxyz\r\n이돼지새끼야 그만처먹고 자라.");
    // wcscpy_s(buf, bufLength, L"동해물과 백두산이 마르고 닳도록 하느님이 보우하사\r\nabcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz\r\n우리나라 만세 무궁화 삼천리 화려 강산 대한 사람 대한으로 길이 보전하세");
    docLength = wcslen(buf);
    off = 0;

    {
        TEXTMETRIC tm;
        HDC hdc = GetDC(hWnd);
        GetTextMetrics(hdc, &tm);
        FontHeight = tm.tmHeight;
        ReleaseDC(hWnd, hdc);
    }

    PrecomputeCharWidths();

    LineRatio = 120;
    LineHeight = (int)(FontHeight * LineRatio / 100);

    lineInfoSize = 0x1000;
    lineInfo = (LineInfo*)malloc(sizeof(LineInfo) * lineInfoSize);
    memset(lineInfo, 0, sizeof(lineInfo) * lineInfoSize);
    
    return 0;
}

LRESULT OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    if (buf) { free(buf); }
    if (lineInfo) { free(lineInfo); }
    PostQuitMessage(0);
    return 0;
}

void SetCaret(BOOL bUpdatePrevX /*= TRUE*/){
    SIZE sz;
    HDC hdc;
    int toff, x, y;
    int caretWidth;

    hdc = GetDC(hWndMain);
    toff = bComp ? off - 2 : off;
    caretWidth = bComp ? GetCharWidth(buf + toff, 1) : CARET_WIDTH;

    CreateCaret(hWndMain, NULL, caretWidth, FontHeight);
    ShowCaret(hWndMain);

    GetCoordinate(toff, x, y);
    SetCaretPos(x, y);
    ReleaseDC(hWndMain, hdc);

    if (bUpdatePrevX != FALSE) { 
        PrevX = x;
    }
}

void PrecomputeCharWidths() {
    SIZE sz;
    HDC hdc = GetDC(hWndMain);

    WCHAR ch[2] = { 0, };
    for (int i = 1; i < 128; i++) {
        ch[0] = (WCHAR)i;
        GetTextExtentPoint32(hdc, ch, 1, &sz);
        AsciiCharWidth[i] = sz.cx;
    }
    AsciiCharWidth[0] = AsciiCharWidth[32];

    GetTextExtentPoint32(hdc, Sample, 1, &sz);
    HangulCharWidth = sz.cx;

    ReleaseDC(hWndMain, hdc);
}

int GetCharWidth(WCHAR* src, int length) {
    int width = 0;
    for (int i = 0; i < length; i++) {
        if ((int)(*(src + i)) < 128) {
            width += AsciiCharWidth[*(src + i)];
        }
        else {
            width += HangulCharWidth;
        }
    }
    return max(width, 1);
}

BOOL Insert(int idx, WCHAR* str) {
    int length = wcslen(str);
    if (length == 0) { return FALSE; }

    int Needed = docLength + length + 1;
    if (Needed > bufLength) {
        bufLength = Needed + 0x400;
        buf = (WCHAR*)realloc(buf, sizeof(WCHAR) * bufLength);
        if (buf == NULL) { return FALSE; }
    }

    int move = docLength + idx + 1;
    memmove(buf + idx + length, buf + idx, move * sizeof(WCHAR));
    memcpy(buf + idx, str, length * sizeof(WCHAR));
    docLength += length;
    bLineEnd = FALSE;

    RebuildLineInfo();
    return TRUE;
}

BOOL Delete(int idx, int cnt) {
    if (docLength < idx + cnt) { return FALSE; }

    int move = docLength - idx - cnt + 1;
    memmove(buf + idx, buf + idx + cnt, move * sizeof(WCHAR));
    docLength -= cnt;

    RebuildLineInfo();
    return TRUE;
}

BOOL IsCRLF(int idx) {
    if (buf[idx] == '\r' && buf[idx + 1] == '\n') {
        return TRUE;
    }
    return FALSE;
}

int GetPrevOffset(int idx) {
    if (idx <= 0) { return 0; }
    if (IsCRLF(idx - 2)) { return idx - 2; }
    return idx - 1;
}

int GetNextOffset(int idx) {
    if (docLength < idx) { return docLength; }
    if (IsCRLF(idx)) { return idx + 2; }
    return idx + 1;
}

void GetLine(int line, int& start, int& end) {
    WCHAR* ptr = buf;
    int lineStart = 0, lineEnd = 0, prevEnd = 0;

    if (line > 0) {
        prevEnd = lineInfo[line - 1].end;
        if (ptr[prevEnd] == 0) {
            start = -1;
            end = -1;
            return;
        }

        if (ptr[prevEnd] == '\r') {
            lineStart = prevEnd + 2;
        }
        else {
            lineStart = prevEnd;
        }

        lineEnd = lineStart;
    }

    while (1) {
        WCHAR ch = ptr[lineEnd];
        if (ch == '\r' || ch == 0) { break; }
        lineEnd++;
    }

    start = lineStart;
    end = FindWrapPoint(lineStart, lineEnd);
}

// 자동 개행된 경우 줄의 끝과 다음 줄의 처음이 같은 오프셋을 가진다는게 핵심 문제이다.
// VK_RIGHT, VK_LEFT의 코드를 텍스트로 도식화 해보면 다음과 같다.
// GetNext(Prev)Offset() -> SetCaret() -> GetCoordinate() -> GetRowAndColumn() -> GetCoordinate() -> SetCaret()
// GetRowAndColumn에 의해 메모리상의 오프셋이 정해지는데 이때 문제가 발생한다.
// 화면과 메모리상의 좌표가 1:1로 대응되지 않는 것인데, 이유는 자동 개행으로 인해 첫 번째 줄을 제외한 start와 end의 값이 서로 같다는 것이다.
// 즉, 오프셋 값이 동일하다.

// 현재 구조를 보면, SetCaret 함수는 여러 메세지에서 호출된다.
// 즉, 호출원이 여러 곳이며 이러한 공통 함수를 수정할 때는 해당 함수를 호출하는 모든 호출원의 흐름을 고려해야 한다.
// 지금 문제는 VK_LEFT와 VK_RIGHT로 캐럿을 이동할 때 발생하므로 이 메세지의 코드를 수정하여 캐럿의 이동을 조정해보자.
void GetRowAndColumn(int idx, int& row, int& column) {
    WCHAR* ptr = buf;
    row = 0;

    int left = 0, right = lineCount - 1;

    while (left <= right) {
        row = (left + right) / 2;

        int start = lineInfo[row].start;
        int end = lineInfo[row].end;

        if (start < idx && idx < end) {
            break;
        }

        if (idx == start) {
            if (row > 0 && bLineEnd) { row--; }
            break;
        }

        if (idx == end) {
            if (ptr[end] == 0 || ptr[end] == '\r') {
                break;
            }
        }

        if (start > idx) {
            right = row - 1;
        }
        else {
            left = row + 1;
        }
    }

    column = idx - lineInfo[row].start;
}

int GetOffset(int row, int column) {
    int start = lineInfo[row].start;
    int end = lineInfo[row].end;
    column = min(column, end - start);
    return column + start;
}

void GetCoordinate(int idx, int& x, int& y) {
    int row, column;
    GetRowAndColumn(idx, row, column);

    y = row * LineHeight;
    x = 0;

    int start = lineInfo[row].start;
    int end = lineInfo[row].end;

    WCHAR* ptr = buf + start;
    while (ptr != buf + idx) {
        if (*ptr == '\t') {
            // Tabsize;
        }
        else {
            x += GetCharWidth(ptr, 1);
            ptr += 1;
        }
    }
}

BOOL IsWhiteChar(WCHAR ch) {
    return ch == L' ' ||
        ch == L'\r' ||
        ch == L'\n';
}

BOOL IsAlnumChar(WCHAR ch) {
    return (ch >= L'0' && ch <= L'9') ||
        (ch >= L'A' && ch <= L'Z') ||
        (ch >= L'a' && ch <= L'z');
}

BOOL IsPunctChar(WCHAR ch) {
    return (ch >= 0x21 && ch <= 0x2F) ||    // ! " # $ % & ' ( ) * + , - . /
        (ch >= 0x3A && ch <= 0x40) ||        // : ; < = > ? @ 
        (ch >= 0x5B && ch <= 0x60) ||       // [ \ ] ^ _ \ `
        (ch >= 0x7B && ch <= 0x7E);         // { | } ~
}

BOOL IsKJCChar(WCHAR ch) {
    return (ch >= 0xAC00 && ch <= 0xD7AF) ||    /* 한글 */
        (ch >= 0x3040 && ch <= 0x30FF) ||       /* 일본어 */
        (ch >= 0x4E00 && ch <= 0x9FFF);         /* 중국어  */
}

enum CustomCharset GetCustomCharset(WCHAR ch) {
    if (IsWhiteChar(ch)) { return (enum CustomCharset)CC_WHITE; }
    if (IsAlnumChar(ch)) { return (enum CustomCharset)CC_ALNUM; }
    if (IsPunctChar(ch)) { return (enum CustomCharset)CC_PUNCT; }
    if (IsKJCChar(ch)) { return (enum CustomCharset)CC_KJC; }
    return (enum CustomCharset)CC_OTHER;
}

int FindWrapPoint(int start, int end) {
    if (start >= end) { return start; }

    WCHAR* ptr = buf;
    int left = start + 1, right = end, fit = left;
    int maxWidth = g_crt.right - g_crt.left - CARET_WIDTH;

    while (left <= right) {
        int mid = (left + right) / 2;
        int width = GetCharWidth(ptr + start, mid - start);
        if (width <= maxWidth) { fit = mid; left = mid + 1; }
        else { right = mid - 1; }
    }

    // 줄끝 후보
    // 화면 폭을 넘어서지 않는 문자열의 최대 길이
    int pos = fit;

    // 폭이 너무 좁아서 아무 글자도 못 넣는 경우
    // GetCharWidth가 반환하는 너비가 두 글자 이상이고 maxWidth를 초과할 때 right는 left보다 작아지면서 fit = start + 1과 같아진다.
    // 최종적으로 pos == start + 1이 참이면 강제 개행 지점으로 보고 리턴한다.
    // 즉, 한 글자만 현재 줄에 남기고 나머지를 다음 줄로 넘긴다.
    if (pos == start + 1) { return pos; }

    // 단어 단위 정렬
    if (g_Option.wordWrap) {
        if (pos < end && !IsWhiteChar(ptr[pos - 1]) && !IsWhiteChar(ptr[pos])) {
            pos = WordBreakProc(pos, start, WBP_WORD);
            if (pos <= start) { pos = fit; }
        }

        if (pos < end && g_Option.KeepPunctWithWord) {
            WCHAR Next = buf[pos];
            if (IsPunctChar(Next)) {
                int pos2 = WordBreakProc(pos, start, WBP_PUNCT);
                if (pos2 < pos && pos2 > start) { pos = pos2; }
            }
        }
    }
    // 문자 단위 정렬
    else { 
        if (!g_Option.kjcCharWrap) {
            // 하이픈 뒤에서 끊기 허용
            if (pos > start && ptr[pos - 1] == L'-') {
                // ok
            }
            else if (pos < end && !IsWhiteChar(ptr[pos])) {
                if (pos > start) {
                    CustomCharset Prev = GetCustomCharset(pos - 1);
                    int prevPos = pos - 1;
                    while (prevPos > start && GetCustomCharset(prevPos - 1) == Prev) { prevPos--; }
                    pos = prevPos;
                }
            }
        }
    }

    if (pos > end) { pos = end; }

    // 줄 뒤쪽 공백 제거
    if (g_Option.trimEndSpaces) {
        int t = pos;
        while (t > start && IsWhiteChar(ptr[t - 1])) t--;
        pos = (t > start) ? t : pos;
    }

    return pos;
}

int WordBreakProc(int pos, int start, WBPType type) {
    WCHAR* ptr = buf;

    int back = pos - 1;
    CustomCharset Current = GetCustomCharset(ptr[pos]);
    CustomCharset Previous = GetCustomCharset(ptr[back]);

    switch (type) {
    case WBP_WORD:
        // FALLBACK: 문자셋이 서로 일치하지 않는 지점까지 좌측으로 이동
        while (back > start && GetCustomCharset(ptr[back - 1]) == Previous) { back--; }
        break;

    case WBP_PUNCT:
        // 구두점일 때만 단어 단위로 정렬
        if (Previous != CC_PUNCT && (Previous == CC_KJC || Previous == CC_ALNUM)) {
            CustomCharset WordType = Previous;
            while (back > start && (GetCustomCharset(ptr[back - 1]) == WordType)) { back--; }
        }
        break;
    }

    return back;
}

void RebuildLineInfo() {
    int curLine = 0, pos = 0, wrapEnd = 0;
    int start = 0, end = 0;
    WCHAR* ptr = buf;

    while (1) {
        GetLine(curLine, start, end);

        lineInfo[curLine].start = start;
        lineInfo[curLine].end = end;

        if (start == -1) { lineCount = curLine; break; }
        curLine++;
    }
}

int GetDocsXPosOnLine(int row, int dest) {
    int start = lineInfo[row].start;
    int end = lineInfo[row].end;
    
    int len = end - start;
    WCHAR* ptr = buf + start;

    int Width = 0;
    if (dest == 0) { 
        return start;
    }
    else {
        while (ptr - buf != end) {
            Width += GetCharWidth(ptr, 1);
            ptr += 1;
            if (Width >= dest) { break; }
        }
    }

    int ret = ptr - buf;
    if (ret == end && buf[ret] != '\r' && buf[ret] != 0) {
        bLineEnd = TRUE;
    }
    else {
        bLineEnd = FALSE;
    }

    return ret;
}

void TraceFormat(LPCWSTR format, ...)
{
    WCHAR buffer[512];
    va_list args;
    va_start(args, format);
    vswprintf(buffer, 512, format, args);
    va_end(args);
    OutputDebugString(buffer);
}