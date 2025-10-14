#include "pch.h"
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

BOOL bLineEnd = FALSE, bLineFirst = FALSE;
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
// void RebuildLineInfo();              // 최적화로 인해 변경됨

// 줄 정보를 만들었으므로 GetLine 함수의 호출부를 찾아 하나씩 바꿔주면 된다.
// GetLine 함수 역시 줄 정보를 사용할 수 있으며, 코드가 훨씬 짧아진다.
// 또, FindLineEnd가 할 일이 없어졌는데 이제 필요 없으므로 과감하게 지워버리도록 하자.

// 이번엔 캐럿의 이동을 보자.
// 방향키를 이용해 캐럿을 상,하로 이동시켜보면 아주 조잡하게 움직인다는 것을 알 수 있다.
// 일반적인 에디터 프로그램은 이전 위치를 기억하여 캐럿이 이전 줄이나 다음 줄로 이동할 오프셋값을 이용하여 같은 위치로 이동시킨다.
// SetCaret은 GetCoordinate 함수를 호출하여 화면상의 x좌표를 구해오는데 이를 기억해두었다가
// 캐럿 이동에 활용해보자.
int GetDocsXPosOnLine(int row, int dest);

// 자동 개행을 완성했다.
// 이제 스크롤 바를 추가하여 화면에 보이지 않는 글자를 보이도록 만들어보자.
int xPos, yPos, xMax, yMax, FontWidth;
void UpdateScrollInfo();

// 탭 문자 지원
// 탭은 텍스트 편집기에서 필수적이며 문자 코드를 갖고 있다.
// 이 문자 코드는 알다시피 \t인데 일반적으로 스페이스바 4개의 폭을 가지는 빈 공간으로 표현한다.
// 탭은 서식이 없는 문서에서 어질러진 자료를 정리할 때 유용하다.
// 예로, 자료를 도표 형태로 정리하거나 단어 또는 문장을 구분할 때를 들 수 있다.
// 탭은 일정한 크기뿐만 아니라 일정한 위치를 기준으로 반복된다.
int TabWidth, TabSize;

// 탭을 화면에 그리기 위해 서브 함수를 만들어보자.
// WM_PAINT 메세지를 복잡하게 만드는 것 보단 필요한 동작을 함수로 만들어 두는 것이 좋다.
// 현재 코드를 보면 한 줄에 대한 정보를 가져오고 문자열을 출력하는 구조인데
// 탭을 인식하고 그리는 동작까지 포괄하여 하나의 함수로 만들어보자.
// 먼저, 각 줄을 그리는 함수이다.
int DrawLine(HDC hdc, int line);
// 각 줄의 정보를 가지고 문자열을 화면에 출력할 때 탭 문자를 인식하도록 만들어보자.
void DrawSegment(HDC hdc, int& x, int y, int idx, int length, BOOL ignore, COLORREF fg, COLORREF bg);
// 정렬에서 탭을 인식해야 하므로 우선, IsWhiteChar 함수를 수정해야 한다.
// 이후 GetCharWidth에서 탭을 인식하도록 만들어야 하는데 FindWrapPoint에서 이분 탐색을 활용하기 때문에 이 함수에 직접 분기를 추가했다.
// 외에 화면 좌표를 사용하는 함수에선 직접 현재 포인터가 가리키는 지점이 탭 문자('\t')인지 확인하고 좌표값을 계산하도록 만들었다.
// 어려운 코드는 없지만 GetCharWidth에 분기를 추가한 이유는 알고있어야 한다. 다시 말하지만, 현재 이분 탐색을 활용하고 있기 때문에 이런 처리가 필요하다.

// TODO: 문장 끝에 탭이 반복될 경우 다음 줄로 넘어가면서 캐럿이 함께 이동된다. 캐럿을 처리할 때 분기를 추가해야 한다.

// 하나씩 해결해보자. 문제의 본질은 다음과 같다.
// 자동 개행을 수행하여 캐럿이 다음 줄로 이동했지만, 문자는 이전 줄에 삽입된다.
// 즉, 삽입 위치와 캐럿 위치가 일치하지 않는다는 것이다.
// 문자 정렬의 경우 아무런 문제가 없지만 단어 단위 정렬을 지원하므로 방향키 뿐만 아니라 삽입/삭제간에도 bLineEnd를 활성화 해야한다.

// 우선, 삭제 코드부터 보자.
// del키를 눌러 삭제하는 코드는 그대로 두면 되고 VK_BACK만 수정하면 된다.
// 단어 단위 정렬이 활성화 된 상태에서는 Backspace키를 눌렀을 때에도 bLineEnd를 활성/비활성 하는 코드가 추가되어야 캐럿이 제대로 움직인다.
// 문자 단위 정렬에서는 이럴 필요가 없으므로 wordWrap을 기준으로 분기한다.
// 
// 이때 예외 처리해야될 것이 무엇이 있는지 살펴보자.
// 1. wordWrap이 TRUE인가
// 2. 현재 캐럿이 위치한 지점의 열 번호가 0인가. 즉, 가장 앞에 위치해 있는가
// 3. 지우려고 하는 지점의 문자가 강제 개행 문자인가

// 이를 토대로 의사 코드를 작성해보면 다음과 같다.
// GetRowAndColumn
// tempOffset = GetPrevOffset
// if (WordWrap == TRUE) {
//      if(IsCRLF(tempOffset)){
//          bLineEnd = FALSE;
//          Delete(toff, 2);
//      }else{
//          bLineEnd = TRUE;
//          Delete(toff, 1);
//      }
// }
// 이 논리 그대로 사용하면 충분할 것 같다. VK_BACK 코드를 위 의사코드를 참고하여 수정한다.

// 이번엔 삽입 코드를 보자.
// 삭제는 별로 어려울게 없다. 진짜 문제는 문자를 삽입할 때인데, 결론만 말하면 bAlphaNum 변수를 추가하기로 했다.
// bAlphaNum 변수는 입력하려는 문자가 알파벳인지 아닌지를 판단하는데 이 변수가 필요한 이유는 간단하다.
// 한글과 같은 조립 문자의 경우 IME가 저수준에서 캐럿 위치를 일부 보정해주는데 이를 활용할 생각이기 때문이다.

// "아 디버깅 하는거 힘듭니다 별 문제는 없는거 같습니다." 문장에서 "별 "다음에 캐럿을 위치시켜보자.
// 그리고 삽입 함수 Insert 내부에서 bLineEnd를 다음과 같은 분기로 조정한다고 가정해보자.

// if(g_Option.wordWrap){
//      int row, column, start, end;
//      GetRowAndColumn(idx, row, column);
//      start = lineInfo[row].start;
//      end = lineInfo[row].end;
// 
//      if(column > 0 && idx == end){
//          bLineEnd = TRUE;
//      }else{
//          bLineEnd = FALSE;
//      }
// }else{
//      bLineEnd = FALSE;
// }

// 방향키로 "별 " 다음에 캐럿을 위치시키면 이 자리의 오프셋 값은 end와 같다.
// 이 상태에서 bLineEnd를 TRUE로 만들고 캐럿이 다음 줄로 이동되지 않도록 만들면 논리적으로 괜찮아 보인다.
// 그러나 실상은 그렇지 않은데, 이렇게 되면 IME의 캐럿 위치 보정을 강제로 막아 조립이 완료된 후에도 캐럿이 줄 끝에 남아있는 현상이 발생한다.
// "별 " 다음에 문자를 하나 조립해보자. 그러면 어떤 현상이 발생하는지 금세 알 수 있다.

// 앞서 말했듯, IME가 조립 문자에 한해 캐럿의 위치를 보정해준다.
// 시스템이 개입하여 캐럿 위치를 의도적으로 바꿔주기 때문에 시각적으로 가장 자연스럽다.
// ImmSetCompositionWindow 같은 API를 통해 조합 위치를 조정하는데 이걸 직접 건드릴 필요도 없고 건드릴 수도 없다.

// 때문에 이를 활용해야 하는데 한글은 자연스럽게 처리되므로 넘어가면 되고 알파벳이나 숫자같은 ASCII 문자에 대해서는 분기를 만들어 처리해야 한다.
// 이를 위해 bAlphaNum 전역 변수를 추가하고, WM_IME_COMPOSITION 메세지에 현재 한/영 전환 상태를 확인하는 코드를 추가했다.
// 또, WM_IME_CHAR와 WM_CHAR 메세지를 처리하는 함수 상단에 bAlphaNum을 결정하는 코드도 추가했다. 

// 이렇게 하면 위 상황에서 조립 문자와 단일 문자에 대한 처리가 모두 끝난 것이다.
// 확인해보면, 캐럿의 위치가 정상적으로 이동하는 것을 알 수 있다.

BOOL bAlphaNum = TRUE;

// 이어서 그 다음 문제를 보자. 똑같이 삽입에서 문제가 발생하는데 이번엔 줄 앞에서의 삽입이 문제가 된다.
// 단어 단위 정렬로 인해 자동 개행이 발생한 직후 다음 줄의 첫 번째 열 앞에 캐럿이 위치한 상황이라 가정한다.
// 즉, "아 디버깅 하는거 힘듭니다 별 문제는 없는거 같습니다." 문장에서 "문제는"의 앞에 캐럿이 위치한 상황이다.
// 여기서 알파벳이나 숫자같은 단일 문자를 입력해보자.

// 캐럿은 여전히 "문제는"의 앞에 위치해 있는데 글자는 "별 " 다음에 삽입된다.
// 한글은 조립 문자라 IME의 지원을 받아 자동으로 이쁘게 처리되지만 단일 문자는 그렇지 않다.
// 그래서 직접 캐럿의 위치를 조정해야 한다.

// 진짜 단순하게 캐럿만 옮기면 끝이다.
// 이 문제를 해결할 방법은 굉장히 다양하겠지만 여기서는 가장 단순한 방법을 사용하기로 한다.

// 우선, 상황을 분석하여 의사코드를 만들어보자.
// Column은 0이면서 row가 0보다 커야 한다.
// 또, 이전 줄이 단어 단위 정렬로인한 자동 개행이 이뤄진 줄인지 판별해야 하고
// 지금 입력하려는 문자가 단일 문자(영어 등)인지 판단해야 한다.

// 복잡해보이는데 이미 다 만들어둔 함수를 이쁘게 쓰기만 하면 된다.
// 이를 코드로 만들어보면 다음과 같다.
// GetRowAndColumn(off, row, column);
// if(column == 0 && row > 0){
//      toff = GetPrevOffset(off);
//      if(IsCRLF(toff) == FALSE){
//          if(bAlphaNum){
//              ...
//          }
//      }
// }

// 이미 Insert에 분기를 하나 만들어뒀는데 여기에 else if문만 하나 더 추가하면 끝이다. 위 코드를 참고하여 Insert 함수를 수정해보자.

// 여기까지 하면 삽입/삭제 동작에서 캐럿의 위치가 자연스러워진다.

// 사실 최근에는 캐럿 위치를 위 프로젝트처럼 억지로 끼워 맞추지 않고
// 그냥 그대로 둔 상태에서 새로 조립하거나 삽입하는 문자가 있을 때
// 해당 문자에 밑줄을 그어 작성하고 있는 문자임을 분명히 보여주는 방법을 많이 이용한다.

// 즉, 캐럿이 줄 앞에 있고 이전 줄의 끝에 공간이 조금 남아 문자를 끼워 넣을 수 있는 상황일 때
// 억지로 캐럿을 이전 줄로 옮기지 않고 그대로 둔 다음에 특별히 시각적인 처리(밑줄)를 하는 방법을 택한다.
// 이렇게 하면 현재 자동 개행 로직으로 인해 발생하는 캐럿 위치의 부조화를 최소한의 비용으로 아주 이쁘게 처리할 수 있다.

// 이는 현재 구조 특성에 따른 분명한 한계이다.
// 현재 구조를 보면 전역 변수를 하나 두고 이를 여러 함수에서 참조하고 있는데
// 이를 흔히 "공통 결합도를 가진다"라고 한다.
// 곧, 모듈 독립성이 낮은 구조라 유지보수가 어려운건 물론이고 테스트하기도 복잡하다는 뜻이다.

// 지금 만들고 있는 프로젝트의 코드를 보면 알겠지만 사실 굉장히 초보 수준이다.
// 에디트 동작을 하나씩 살펴보고 어떻게 만들면 될지 논리적으로 생각하는 것에 목적을 두고 있다.
// 즉, 완벽한 에디트 프로그램을 만드는게 목적이 아니기 때문에 최대한 여러 상황을 살펴보고 이를 해결하는 방법을 연구해볼 것이다.

// 참고로, 위와 같은 문제는 문자 단위 정렬에서도 발생한다.
// wordWrap을 기준으로 굳이 분기한 이유는 앞서 말했듯 여러 상황을 확인하고 연구해보기 위해서이다.

// TODO: 탭 문자가 반복될 때 상하이동시 캐럿의 위치가 이상하다. 즉, 수평 좌표 처리가 이상해진다. 이 역시 수정이 필요하다. 
// 이 문제는 실행 순서가 잘못되어서 그렇다. ptr += 1 코드 뒤에 if(Width >= dest){...} 분기문이 위치하도록 자리를 바꿔보자.
// 아주 간단하게 해결된다.

// 이번엔 마우스로 캐럿 위치를 옮길 수 있도록 만들어보자.
// 왼쪽 버튼을 눌렀을 때 반응하도록 만들면 되므로 LBUTTONDOWN 메세지에서 처리한다.
// 일단 유틸리티 함수부터 만들어보자.

int GetOffsetFromPoint(int x, int y);
int GetOffsetFromPoint(POINT Mouse);

// 소스 코드 상단에 보면 #define GET_X_LPARAM과 #define GET_Y_LPARAM 매크로가 있다.
// 반드시 이 둘을 사용하여야 다중 모니터 환경에서도 제대로 동작한다.
// 마우스가 캡처된 상황이라면 클라이언트 영역을 벗어난 상태에서도 좌표값이 전달되는데
// 일반적으로 LBUTTONDOWN 메세지보단 LBUTTONUP이나 MOUSEMOVE 메세지에서 음수 좌표값이 발생한다.
// 이를 감안하여 min, max 함수도 활용하고 있는데 전체적인 로직을 보면 GetDocsXPosOnLine 함수와 비슷하다는 걸 알 수 있다.

// 다음은 키보드와 마우스로 문자열을 일정 범위만큼 선택하는 기능을 추가해보자.
// 일정 범위를 선택하는 것이므로 SelcectStart, SelectEnd 따위의 변수를 만들어 관리하면 충분할 것이다.
// 또, 마우스의 왼쪽 버튼을 누른 상태에서 드래그하여 일정 범위를 선택하는 동작이 필요하므로 마우스를 캡처해야 한다.
// 선택된 영역을 표현하려면 직접 그려야하므로 백그라운드와 포그라운드를 칠할 브러시도 만들어야 한다.
// 뿐만 아니라 메인 윈도우가 키보드 입력을 받을 수 없는 상태일 때의 상황도 생각해봐야 한다.
// 키보드 입력을 받을 수 있는 상태가 되면 SetFocus 메세지가 보내지는데 이때 선택된 영역이 있다면 눈에 띄도록 색을 변경하고
// 키보드 입력을 받을 수 없는 상태일 때에는 괜히 시선이 분산되지 않게 어두운 계열로 덧칠하는 것이 좋다.
// 물론 프로그램마다 다르지만 이 프로젝트에서는 그렇게 진행한다.
// WM_CREATE에서 변수를 초기화하고 WM_SETFOCUS 메세지에 선택 영역의 백그라운드와 포그라운드 색상을 변경하는 코드를 추가하자.

int SelectStart, SelectEnd;
BOOL bCapture;
COLORREF SelectFgColor, SelectBgColor;
int HideType;

// 이번엔 선택된 영역을 그려보자.
// WM_PAINT에서 그리기를 담당하고 있지만 실제 문자열을 출력하는 것은 DrawLine 함수가 담당한다.
// DrawLine에서 각 줄에 대해 선택 영역이 있는지 확인하고 백그라운드와 포그라운드 색상을 만들어 DrawSegment에 전달하면
// DrawSegment가 해당 영역을 사각 영역으로 그린 후 전달받은 색상으로 칠하기만 하면 된다.
// 결국 DrawLine이 탭 문자 + 선택 영역을 기준으로 조각을 나누게 된 것이다.
// 이를 적용하여 DrawLine 함수와 DrawSegment 함수를 고쳐보자.
// 여기까지 작성한 후에 SelectStart와 SelectEnd를 상수 값으로 설정하고 확인해보면 선택 영역이 이쁘게 그려지는 것을 볼 수 있다.

// 이제 마우스로 선택할 수 있게 수정해보자.
// WM_LBUTTONDOWN과 WM_MOUSEMOVE, WM_LBUTTONUP 메세지를 모두 사용한다.

// 그 다음은 스크롤 지원이다.
// 문자열이 여러 줄 있을 때 스크롤 되어 보이지 않는 범위까지 선택해야 한다고 가정해보자.
// 이때 드래그중 화면이 스크롤되지 않으면 보이지 않는 범위를 선택할 수 없다.
// 일반적인 에디트에서는 모두 지원하는 기본적인 동작이므로 이 기능도 추가해야 하는데,
// 문제는 WM_MOUSEMOVE 메세지가 발생하는 시점이 "마우스가 이동할 때"라는 것이다.
// 결론부터 말하면 WM_MOUSEMOVE만 이용해서는 화면 높이보다 긴 문서를 스크롤 할 수 없다.
// 때문에 마우스가 작업 영역을 벗어나면 타이머를 설치하여 주기적으로 스크롤 되게 만들어야 한다.
// 메모장도 이러한 동작을 지원하는데 긴 문장을 붙여넣고 범위를 선택하여 작업 영역을 벗어나게 드래그해보자.
// 마우스가 작업 영역을 벗어난 후 마우스 이동을 멈추면 마우스가 향한 방향으로 알아서 스크롤 되는 것을 볼 수 있다.

// 위 내용 대부분은 WM_MOUSEMOVE에서 처리 가능하므로 이 메세지에 처리 로직을 추가해보자.
// 줄 정보를 갱신하는 함수가 스크롤 정보를 갱신하는 함수보다 늦게 호출되어 스크롤바가 제대로 범위를 갖추지 못한 상황이었다.
// xMax 값도 갱신이 제대로 되지 않은 상태였고, 이로 인해 우측 스크롤 기능이 제대로 동작하지 않는 상태였다.
// RebuildLineInfo 함수를 호출하는 지점에 UpdateScrollInfo 함수도 붙어있으므로 둘의 순서만 바꿔주면 된다.

// 다음은 선택 영역이 있고, 포커스를 잃었을 때의 처리를 해보자.
// 하이라이트 색상이 아니라 회색 바탕에 검은색 전경으로 변경하기만 하면 된다.
// KillFocus와 SetFocus에서 각각 다시 그리도록 요청하되 KillFocus에서는 색상값을 변경한다.
// DrawLine도 약간의 수정이 필요하다. 선택 영역(bInSelect)이 있고, 메인 윈도우가 포커스를 받은 상태(GetFocus == hWndMain)일 때 또는 HIdeType이 0이 아닐 때 배경과 전경 색상을 하이라이트 값으로 바꾼다.
// 기존 분기 조건에 한 가지 조건이 더 추가된 것 뿐이다.
// 이를 코드로 작성하고 실행해보면 포커스를 잃은 상태일 때 색상이 변하는 것을 볼 수 있다.

// 이번엔 특수키와 방향키를 조합하여 선택 및 이동 기능을 추가해보자.
// 일반적으로 사용되는 Ctrl, Shift 조합을 떠올리면 된다.
// 우선, 필요한 함수부터 만들어보자.

BOOL IsDelims(int idx);
int GetPrevWord(int idx);
int GetNextWord(int idx);

// 단어 단위 이동을 수행하는 함수와 구분자 판별 함수를 만들었다.
// 구분자를 이용해 단어 단위로 이동하면 되는데 함수의 내부 로직을 보면 다소 이해하기 어렵다.
// 상황은 두 가지로 나뉘는데 FindWrapPoint의 논리 로직을 일부 사용했다고 보면된다.
// 문자셋을 나누지 않고 구분자만 이용하는데 문자의 중간에 있을 때나 앞에 있을 때를 기준으로 작성하였다.
// 함수 이름이 워낙 설명적이기 때문에 위 두 가지 경우를 떠올리면서 시뮬레이션 해보면 금방 어떤 로직인지 이해할 수 있다.

// 그 다음은 좌우 이동에 이 함수를 적용해보자.
// 영역을 선택하는 동작을 포함하여야 하므로 도우미 함수도 몇 개 추가되어야 한다.
void ClearSelection();
void ExpandSelection(int start, int end);

// ClearSelection은 선택 영역이 있으면 시작과 끝을 0으로 만들고 화면을 다시 그린다.
// ExpandSelection은 영역을 확장하는 동작을 하는데 코드도 딱히 어려울 것이 없다.
// 현재 구조와 이미 작성한 함수를 고려하여 선택과 단어 단위 이동을 떠올린 그대로 구현한다.
// Shift와 Ctrl은 서로 다른 기능을 갖기 때문에 따로 처리해야 하는데 순서를 어떻게 해야할지 고민해봐야 한다.
// 사실 답이 정해져 있다고 보면 되는데 Shift + Ctrl 조합키를 이용한다고 가정해보자.
// Ctrl 동작을 먼저 수행하지 않고 Shift동작을 먼저 수행하게 되면 불필요한 코드가 반복된다.
// Ctrl로 단어 단위 이동을 수행하여 오프셋값을 수정한 이후에 Shift 키 처리를 하면 자연스럽다.
// 그 외에는 딱히 어려울 것이 없으므로 변경된 코드를 보고 분석해보자.
// WM_KEYDOWN 메세지의 처리 로직이 대부분 변경되었으므로 반드시 한 번 살펴봐야 한다.

// 이제 선택 영역을 삭제하는 동작을 추가해보자.
// 선택은 완벽하게 구현해두었으므로 삭제하는 로직만 수정하면 된다.
// BS, DEL, 문자 입력, 세 가지 경우를 고려하면 되는데 이 동작을 돕는 도우미 함수를 만들어보자.
BOOL DeleteSelection();

// 실행해보면 DEL, 문자 입력에 대해서는 잘 동작하지만 BS 키에 대한 동작은 다소 이상하다.
// if(off == 0) 분기문으로 인해 제대로 지워지지 않는데 이 분기문을 if(DeleteSelection() == FALSE) 분기문 내부로 옮기면 제대로 동작한다.

// 이번엔 복사, 붙여넣기 등의 동작을 만들어보자.
// 이러한 동작들은 메뉴로 지원할 수도 있으나 현재 만들고 있는 윈도우는 최종적으로 컨트롤이 되어야 한다.
// 따라서, 메뉴를 가질 수가 없으며 컨텍스트 메뉴를 이용해야 한다.
#define IDM_CUT         2001
#define IDM_COPY        2002
#define IDM_PASTE       2003
#define IDM_SELECTALL   2004

// WM_CONTEXTMENU 메세지를 보자.
// 이 메세지는 시스템이 발생시키는 메세지인데 윈도우 내부에서 마우스 우클릭이 발생했을 때 전달된다.
// 우클릭이 발생한 지점의 좌표 값이 lParam으로 전달되며, 이때 좌표 값은 화면(모니터) 영역을 기준으로 한다.
// lParam으로 -1이 전달되는 경우가 있는데 이는 좌표 값이 아닌 특수값이며 Shift + F10이 입력되었을 때 발생한다.
// 딱히 어려운 코드는 없으며 실행 중에 팝업 메뉴를 만드는 동작만 한다.
// 팝업 메뉴는 만들었으므로 WM_COMMAND에서 해당 항목을 눌렀을 때의 처리를 하면 된다.
// 이 코드 역시 딱히 어려운게 없으며 HGLOBAL이나 Clipboard에 대한 내용은 여기서 다루지 않기로 한다.
// 여기까지 작성하고 프로그램을 실행해보면 복사 및 붙여넣기 등의 기능이 잘 동작하는 것을 볼 수 있다.

// 잠깐 테스트를 하는동안 버그를 발견했다.
// off == 0인 상태에서 엔터 키를 입력해보면 문자가 제대로 그려지지 않는다.
// 이는 아주 간단히 고칠 수 있는데 DrawLine 함수의 선두에 있는 if(start == 0 && end == 0) 분기문에 한 가지 조건만 추가하면 된다.
// 분기문을 if(start == 0 && end == 0 && line > 0)로 변경하면 문자가 제대로 그려질 것이다.

// 마지막으로 단축키를 지정해보자.
// Ctrl + C는 WM_CHAR 메세지에서 받을 수 있다.
// 이 조합키는 액셀러레이터 대신 쓸 수 있으므로 기억해두는 것이 좋다.
// WM_CHAR 메세지 처리 로직을 보면 선두에 이러한 조합키들의 코드를 분기 처리하고 있다.
// 그 다음은 Shift와 Ctrl입력 상태에서 KeyDown 메세지가 발생할 때의 로직을 추가해보자.
// 복사 및 붙여넣기 등의 동작을 Shift + Del, Shift + Insert, Ctrl + Insert 등의 조합으로 정의했다.
// Ctrl + Backspace가 가장 유용하게 쓰이는데 아직 추가하진 않았다.
// 이 코드는 간단하게 구현할 수 있으나 이는 다음에 추가하기로 하자.
// 여기까지 작성하고 실행해보면 적당히 쓰기 좋은 에디트 윈도우가 된 것을 알 수 있다.

// 이제 최적화 작업이 남았는데 그리는 동작부터 시작해보자.
// 단순히 for문을 돌면서 줄에 있는 문자열을 모두 그리고 있다.
// 사용자가 보고있는 화면을 벗어나는 범위는 사실상 그릴 필요가 없기 때문에 크기를 적당히 잘라서 출력하도록 만드는 것이 좋다.
// 또, 반투명 상태를 지원할 생각이므로 글자를 또렷하게 표현할 수 있어야 하는데, 기본 GDI만으로는 이런 동작이 불가능하다.
// DirectX나 GDI+ 같은 고급 그래픽 라이브러리를 활용해야 한다.
// 현재 구조를 유지하기 위해서는 HDC 타입과 호환 가능해야 하므로 GDI+를 사용하기로 한다.
// 미리 컴파일된 헤더(pch.h)를 만들고 프로젝트 설정을 변경했다.
// MSVC의 IntelliSense의 안정화를 위해 pch를 사용하는 것이 좋은데,
// pch가 없을 경우 버전이 꼬이는 현상이 발생하면서 타입이 꼬이거나 오래된 구문에 대한 오류가 발생하기도 한다.
// 여러모로 정신 건강에 이롭기 때문에 MSVC 사용할 생각이라면 pch를 설정하는 것이 좋다.

// 일단 설정은 마쳤고, 그리기 로직을 먼저 수정한 다음에 GDI+를 이용하기로 한다.
// 사용자가 보고 있는 작업 영역을 제외한 지점을 잘라내는 코드를 추가해보자.
// 간단한 산술로 작업 영역에 포함되는 줄 번호를 가져온다.
// 화면 상단의 시작 줄과 하단의 끝 줄 번호를 가져와 제어 변수에 대입했다.
// 이렇게하면 알아서 계산된 영역의 줄만 그리게 된다.

// 다음은 사이키 마냥 깜빡거리는 화면 갱신 동작을 완화해보자.
// 더블 버퍼링이라는 기법이 주로 사용되는데 줄 단위 더블 버퍼링을 적용할 예정이다.
// GDI+를 이용할 생각이므로 구문이 섞여 다소 지저분해 보일 수 있다.
// 일단 임시 코드를 따로 작성하고 완성되면 본 프로젝트에 적용하기로 한다.

// GDI+ 초기화 및 해제
ULONG_PTR g_Token;
void  InitializeGDIplus();
void ShutdownGDIPlus();

// 비트맵 생성
// Bitmap* hBitmap = NULL;
HBITMAP hBitmap = NULL;
RECT g_wrt;

// 그리기 최적화
void Invalidate(int idx1, int idx2 = -1);

// 그리기 최적화 함수 자체는 그리 어렵지 않다.
// 다만, 편집에 의해 무효화되어야 할 영역을 구분짓고 이를 설정하는 것은 꽤 난이도가 있다.
// 자동 개행 모드에 따라 무효영역을 찾는 규칙이 달라진다면 관리하기 어려울 것이다.
// 때문에 일반화 가능한 규칙을 찾아야 한다. 
// 가장 단순하게는 규칙을 무시하고 현재 줄과 위 아래 줄을 다시 그리는 방법이 있다.
// 조금 더 생각해보면, 현재 편집중인 위치(오프셋)가 어느 문단에 속하는지 확인하고
// 앞 뒤로 강제 개행 문자를 찾아 강제 개행이 포함되지 않은 열린 범위(현재 문단 전체)를 전부 다시 그려버리는 것이다.
// 별 도움이 안될거 같아도 현재 테스트 환경처럼 작업 영역이 굉장히 작은 상태가 아니라면 이 방법도 충분히 최적화에 도움이 된다.

// 외에도 시작과 끝 오프셋을 관리하는 해시를 만들어두고 해시 비교로 변경된 줄 정보를 빠르게 얻어오는 방법도 쓸 수 있을 것이다.
// 물론 필요한 메모리 용량은 커지고 해시를 관리하는 비용도 추가되겠지만 속도와 용량은 반비례하기 때문에 확실히 빠르긴할 것이다.

// 말하다보니 꽤 괜찮은 방법같은데 좀 더 구체적으로 구상해보자.
// 현재 캐럿이 포함된 문단의 시작과 끝을 구하고 문단이 가지는 줄 번호 범위를 구한 다음,
// 각 줄의 시작과 끝 오프셋을 경량 해시로 생성하여 문자 삽입 이후 오프셋을 비교하여
// 변경된 줄을 파악하고 해당 줄만 다시 그린다면 꽤 멋진 최적화 기법이 될 것 같다.

// 이렇게 하면 줄 전체에 대한 해시를 관리할 필요가 없어 비용이 줄어들고
// 해시를 생성하는 시점이 삽입 동작이 발생하기 직전이므로 정보 불일치 등의 위험 또한 없을 것이다.
// 근데 또 생각해보면 굳이 그래야 되나 싶기도 하고, 시간을 두고 생각해보기로 하자.

// 이러나 저러나 일단 문단을 활용하게 될 것임은 분명하다.
// 우선, 문단을 관리하는 함수와 구조체(또는 변수)를 만들어보자.

// 문단의 시작과 끝, 즉 경계를 구하는 함수가 필요할 것이다.
// 기본적으로 경계를 판단하려면 구분자가 필요한데, 지금은 볼 것도 없이 개행 문자를 기준으로 하면 될 것 같다.
struct Paragraph {
    int start;
    int end;
};

Paragraph FindParagraphRange(int idx);

// 정말 간단하게 문단의 범위를 찾는 함수를 만들었다.
// 좋은 방법이야 많겠지만, 우선 가장 간단하고 확실한 최적화 방법을 사용하기로 한다.
// 자동 개행으로 인해 발생할 수 있는 최소/최대 범위가 문단 영역이므로 해당 영역을 통째로 다시 그리는 것이다.

// 그런데, 막상 작성하고 보니 마음에 들지 않는다. 몇 가지 문제가 있는데 이를 살펴보자.
// 우선, 다시 그리려는 문단 영역이 작업영역의 크기보다 넓은 경우 그릴 필요가 없는 영역까지 포함하게 된다.
// 즉, GetCoordinate 호출시 불필요한 계산으로 속도 저하가 생길 것 같다.
// 다행히 InvalidateRect가 클리핑 영역을 알아서 판단하기 때문에 그리는 동작 자체는 빠르게 수행될 것이다.
// 또 다른 문제는 Paragraph 구조체 변수를 하나 만들어야 한다는 것인데 이게 생각보다 번거롭다.

// 위 두 가지 문제를 해결할 좋은 방법이 있다.
// 바로, 현재 문단부터 작업 영역 하단까지의 범위를 전부 다시 그리는 것이다.
// 이 동작은 Invalidate 함수를 호출할 때 첫 번째 인수만 전달하면 되는데,
// 이렇게 하면 문단 범위를 조사할 때 FindParagraphRange 함수의 while 반복문을 하나 줄일 수 있고,
// Invalidate 함수 내부적으로 호출하는 GetCoordinate 함수의 호출 횟수도 한 번으로 줄일 수 있다.
// 이 구조를 사용하게 되면 FindParagraphRange 함수의 리턴값도 구조체가 아닌 단순 int 값이 된다.
// 즉, Paragraph 구조체 변수를 굳이 만들 필요가 없다.

// 아무래도 두 번째 방법이 좋을 것 같다.
// 함수 이름과 리턴 타입을 수정하여 다시 작성해보자.
int FindParagraphStart(int idx);
// 이제 함수를 적용해보자.
// 삽입/삭제 동작을 하는 메세지에서 Invalidate를 호출할 때 FindParagraphStart 함수를 사용하면 된다.
// 기본적으로 삽입, 삭제, 붙여넣기, 잘라내기 등 편집 동작에서 이를 호출하면 되는데
// 예외적으로 삭제 동작에 의해 화면이 스크롤 되는 경우, 즉 스크롤 범위가 페이지 높이보다 작을 때 전체 영역을 다시 그리도록 했다.

// 다음은 선택 영역을 그릴 때 범위를 최적화 해보자.
// 정해진 범위를 그리기만 하면 되므로 이전보단 쉽다.
// 기존 코드에서 InvalidateRect를 Invalidate로 바꾸고 정규화만 하면 된다.

// 이제 완성까지 얼마 안남았다.
// 클릭과 관련된 기능을 몇 가지 더 추가하고 마우스 휠까지 지원하면 기능적인건 모두 완성된다.
// 물론 메모리 관리도 필요한데 일단 기능을 모두 추가한 다음에 수정하는 것으로 하자.

// 이번엔 더블 클릭 기능을 추가해보자.
// 대부분의 편집기에서 더블클릭으로 단어를 선택할 수 있다.
// 이 기능을 추가할건데 더블클릭 메세지를 받을 수 있어야 하므로 스타일을 추가한다.
// 마우스 클릭에 의해 캐럿이 이동하는데 이를 이용해 단어 단위 선택 함수를 만들어보자.
void SelectWord(int idx, int &start, int &end);

// SelectWord 라는 함수를 만들었는데 딱히 어렵지 않다.
// LButtonDblClk 함수도 추가했는데 마찬가지로 매우 간단하다.

// 다음은 Shift 클릭을 추가해보자.
// 선택 영역을 확장할 때 화면을 벗어나는 지점까지 범위를 넓히려면 반드시 필요하다.
// 이것도 아주 간단한 코드 몇 줄이면 간단하게 지원할 수 있다.

// 이제 마지막으로 마우스 휠과 메모리 관리만 남았다.
// 가능하다면 아주 큰 파일을 편집할 때의 최적화 방법도 추가할 예정이다.
// 지금은 우선 몇 가지 예외 분기를 추가해보자.

// 선택 중 문자 삽입에 의해 문자열이 지워지는 현상을 막기 위해 간단한 코드를 추가했다.
// 특히 조립 문자에 대해서는 특별한 처리가 필요한데 WM_LBUTTONUP에 주석과 함께 코드를 작성해두었다.
// 선택 영역이 존재하는 상태에서 문자를 조립하면 어쨋거나 IME는 입력된 문자를 열심히 조립한다.
// 이 상태에서 버튼을 놓고 방향키를 입력하면 조립된 문자가 선택 영역을 그대로 덮어쓴다.

// 자주 있는 일은 아니지만 어쨋거나 불편하고 버그라고 오해할 수 있다.
// 때문에 아예 이런 동작이 발생하지 않도록 막아두는 것이 좋다.
// IME에 알림을 하나 보내서 조립된 문자를 취소해버리면 해결할 수 있다.

// 메모리 관리 코드를 수정해보자.
// 사실 거의 다 만들어둔 상태라 코드를 추가할 곳이 RebuildLineInfo 함수밖에 없다.
// 루프 안에 예외 분기를 만들고 재할당 코드를 추가한다.
// 이러면 사실상 메모리 관리는 끝났다고 볼 수 있다.

// 마지막으로 휠 기능을 추가해보자.
// 코드 자체는 어렵지 않으므로 따로 설명하지 않기로 한다.
// 다만, WHEEL_DELTA와 wParam으로 전달되는 값에 대해서는 설명이 필요하다.
// 기본적으로 WHEEL_DELTA는 120이라는 매크로 상수값이다.
// wParam으로 전달되는 값은 HIWORD에 휠이 회전한 값, 통상 +-120이 전달된다.
// LOWORD는 조합키라고 보면 되는데 휠 메세지가 발생했을 때
// Ctrl, Shift 등의 키가 같이 눌린 상태였는지를 나타내는 값이 전달된다.
// 이제 코드를 작성해보자. 간단한 산술식이면 쉽게 만들 수 있다.

// 기본적인 코드는 다 작성했는데 최적화가 아쉽다.
// 약 100만 줄의 문서를 편집하고 있다고 해보자.
// 사실상 가로, 세로 줄 탐색에서는 이분 탐색을 사용하고 있기 때문에 캐럿이 위치를 찾는 것 자체는 크게 문제가 되지 않는다.
// 다만, 정렬에는 문제가 있을 수 있다.

// 단일 버퍼를 사용하는 현재 구조에서 1만번째 줄에 문자를 삽입/삭제하는 등의 편집을 한다고 해보자.
// 당장 큰 파일을 불러와서 초기화할 때는 크게 문제가 없지만, 편집으로 인해 문서의 줄 정보가 바뀔 때는 영향을 받는다.

// 그리기 동작에 최적화가 필요했던 것처럼 줄 정보를 갱신하는 동작도 최적화가 필요하다.
// 우선, 줄 정보를 갱신하는 시점과 현재 구조를 분석해보자.

// 줄 정보가 갱신되는 시점은 삽입/삭제가 발생할 때이며,
// 사용자에 의해 문서가 편집되면 삽입/삭제 함수 내부적으로 RebuildLineInfo 함수를 호출하여
// 문서 처음부터 끝까지 루프를 돌아 줄 정보를 갱신한다.

// RebuildLineInfo 함수가 가진 while 루프를 보면, 매회 GetLine 함수를 호출한다는 것을 알 수 있다.
// GetLine 함수는 내부적으로 이미 만들어져 있는 줄 정보 배열(lineInfo)을 참조하여
// 해당 줄에 대한 정보를 조사하고 FindWrapPoint 함수를 호출하여 자동 개행 후보 지점을 찾는다.
// 즉, 서로가 서로를 참조하되 적절한 예외 분기로 인해 충돌없이 제대로 동작하고 있다.
// 우선, 분석은 여기까지 하고 다음으로 넘어가보자.

// 앞서 보았듯, 줄 정보를 갱신할 때 그 범위가 문서 전체이므로 속도가 상당히 느리다.
// 갱신할 범위를 적당히 지정할 수 있다면 속도 문제를 해결할 수 있을 것이다.

// 우선 시그니처를 변경해보자.
void RebuildLineInfo(int idx = -1, int length = -1);

// 화면 크기가 변하는 경우에는 줄 정보 전체를 갱신해야 한다.
// 즉, 기존 동작을 그대로 수행하면 되므로 선택적 인수를 추가하여 수정할 부분을 최소화한다.

// 범위를 받도록 원형을 변경했으니 FindParaStart 함수로 문단의 시작 오프셋을 찾으면 최적화에 필요한 동작을 할 수 있다.
// 이제 어떻게 최적화할 것인가가 문제이다.
// 삽입/삭제의 영향을 받는 줄을 예측할 수 있다면 해당 줄의 정보만 갱신하면 된다.
// 다만, 이를 수학적으로 풀어내려면 여러 요소들이 더 필요하다.

// 생각해보자.
// 삭제 동작으로 인해 자동 개행이 해제되어 이전 줄로 이동을 하거나
// 삽입 동작으로 인해 자동 개행되어 다음 줄로 이동하거나
// 삭제 동작으로 인해 강제 개행 코드를 지워 이전 문단과 병합되거나
// 삽입 동작으로 인해 강제 개행 되어 문단이 나뉘거나
// 이렇게 삽입/삭제 동작으로부터 발생할 수 있는 상황이 대략 4가지 정도 존재한다.

// 이때, 모든 경우에서 현재 줄을 포함한 이후의 모든 줄은 반드시 갱신되어야 한다.
// 앞으로 앞,뒤를 편의상 전방과 후방이라 부르기로 한다.

// 전방의 줄 정보를 갱신하는 동작은 생략 가능하다.
// 신경써야할 것은 후방의 줄 정보를 갱신하는 동작 뿐이다.

// 이후의 줄 정보를 모두 갱신하는 것은 기본 동작으로 두고, 좀 더 최적화할 방법을 찾아야 한다.
// 나는 크게 세 가지가 떠올랐는데 가장 먼저 떠오른 것이 지연 갱신이다(Lazy Update).
// 또, 문단을 관리할 구조체를 두고 캐싱하거나 변경된 범위를 추적하는 방법도 떠올릴 수 있다.

// RebuildLineInfo 함수의 동작을 나누어보자.
// 앞에서 말했듯, 크게 두 가지인데 하나는 문서 전체를 갱신하는 것이고 다른 하나는 현재 문단(또는 현재 줄) 이후부터 갱신하는 것이다.
// 간단한 분기를 작성하고 이전 문단으로부터 현재 문단의 시작 오프셋을 찾아 줄 정보를 갱신한다.
// GetRowAndColumn 함수는 행열 값을 구할 때 lineInfo를 참조하는데 RebuildLineInfo 함수가 lineInfo를 갱신하는 함수이다.
// 즉, 현재 줄 정보를 만들고 있는 와중에 lineInfo를 참조하는 함수를 호출한 것이다.
// 메모리를 잘못 건드릴 가능성이 있으므로 FindParagraphStart 함수로 찾은 문단의 시작 오프셋(paraStart)에
// CLRF 길이(2)만큼 빼서 이전 줄의 오프셋을 GetRowAndColumn 인수로 전달한다.
// 즉, 이전 줄의 row 값을 구해온 다음 여기에 1만큼 더하여 현재 문단의 줄 번호를 구해오는 것이다. 

// 이제 진짜 마지막 최적화를 해보자.
// 이정도만 해도 이미 충분히 쓸만한데 이것보다 더 빠르게 반응하도록 만들 수 있다.
// 여러 줄을 삽입/삭제하여 줄 정보가 대량 변경될 때는 현재 코드를 그대로 쓰면 되고
// 타이핑으로 인해 문자 단위로 삽입/삭제되는 경우에는 전형적인 몇 가지 패턴만 존재하므로 조금 더 개선할 여지가 있다.

// 삽입/삭제가 발생하면 어쨋거나 현재 줄을 포함한 뒷 줄은 모두 변화가 생길 수밖에 없다.
// 재정렬(갱신) 해야하는 것은 동일하지만 그 범위를 줄이거나 간단한 산술로 반복문을 생략(또는 간소화)할 수 있어 속도를 향상시킬 수 있다.

// RebuildLineInfo 함수는 알다시피 줄 정보(각 줄의 시작과 끝 오프셋)를 조사하여 줄 정보 배열을 갱신하는 함수이다.
// 기존에는 조사 범위가 [문서의 시작, 문서의 끝]이었으나 지금은 [현재 문단, 문서의 끝]으로 변경되었다.
// 여기서 중요한건, 항상 문단의 앞에서부터 줄 정보를 탐색한다는 것이다.

// 1. ■■■ ■■■■ ■■ ■■ ■■■ ■■■■■ ■■ CLRF
// 2. ■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■ WRAP
// 3. ■■■■ ■■ ■■ ■■ ■■■ ■■ ■■ WRAP
// 4. ■■■■■■■■ ■■ ■■■■ ■■■ ■■■■ CRLF
// 5. ■■ ■■ ■■ ■■■■ CRLF

// 위 예시를 보고 가장 간단한 패턴부터 살펴보자.
// 삽입/삭제가 발생하였으나 줄 개수가 변하지 않은 경우가 가장 간단하다.
// 이때는 단순히 삽입/삭제된 글자 수만큼 현재 줄과 이후의 줄에 반영하면 된다.

// 두 번째 패턴은 삽입/삭제로 인해 줄 개수가 줄어들거나 늘어났을 때이다.
// 이때 줄 개수라는 것은 전체 줄의 수를 말한다.
// 이걸 강조하는 이유는 다음과 같은 상황에서 별도의 최적화 알고리즘이 필요하다고 오판할 수 있기 때문이다.
//                                                                                            ↓문자 삽입("■■■ ")
// "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■ "   WRAP      ->      "■■ ■■■■ ■■■ ■■ ■■■■■ ■■ ■■■■ "      WRAP (0, 30    -> 0, 29)
// "■■■■ ■■ ■■ ■■ ■■■ ■■ ■■ "               WRAP      ->      "■■■■ ■■■■ ■■ ■■ ■■ ■■■ ■■ ■■ "       WRAP (30, 54   -> 29, 58)
// "■■■■■■■■■ ■■ ■■■■ ■■■ ■■■■"        CRLF      ->      "■■■■■■■■ ■■ ■■■■ ■■■ ■■■■"           CRLF (54, 80   -> 58, 84)

// 위 예시에서 보다시피 문자가 삽입된 줄에서 자동 개행이 발생하여 다음 줄로 내려간 상황이다.
// 다음 줄로 내려간 문자열은 기존에 있던 두 번째 줄의 문자열과 병합되어 한 줄에 적당히 들어찼다.
// 즉, 현재까지는 줄 개수 자체에 변화가 없다.

// 위와 같이 현재 줄에 대한 정보가 변경되었음에도 이전 줄이나 다음 줄에 병합되어 줄 개수 자체에 변화가 없을 때에는
// 사실상 가장 간단한 패턴인 1번 패턴과 동일하게 처리하면 된다.

// 여기서 또 하나 중요한 정보를 알 수 있는데, 줄 개수에 변화가 생겼는지 구분하려면 문단의 마지막 줄만 보면 된다는 것이다.
// 즉, 여러 줄을 갖고 난리 피우지 않고 한 줄로 이뤄진 문단을 샘플로 하여 문제를 나누어 연구하면 되는 것이다.

// 이제 다음 예시를 보자.                                                                     ↓문자 삽입("■■■ ")
// "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"      CRLF    ->      "■■ ■■■■ ■■■ ■■ ■■■■■ ■■ ■■■■ "    WRAP     (0, 29  -> 0, 29)
//                                                                     ->      "■■■■"                                               CRLF     (NULL   -> 29, 33)
// "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"      CRLF    ->      "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"   CRLF     (29, 58 -> 33, 62) 

// 앞에서와 같이 CRLF의 오프셋은 포함하지 않았다.
// 위 예시를 보고 알 수 있는 것은 이전 줄 정보를 재활용할 수 있다는 것이다.
// 새로 추가된 줄에 대해서는 재정렬(갱신)이 필요하지만 이후의 줄은 단순히 삽입된 문자의 길이만큼 오프셋을 조정하면 된다.

// 마찬가지로 삭제도 이전 줄 정보를 재활용할 수 있다.
//                                                                                             ↓문자 삭제("■■■ ")
// "■■ ■■■■ ■■■ ■■ ■■■■■ ■■ ■■■■ "       WRAP    ->      "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"   CRLF     (0, 29  -> 0, 29)
// "■■■■"                                                  CRLF    ->                                                                        (29, 33 -> NULL)
// "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"      CRLF    ->      "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"   CRLF     (33, 62 -> 29, 58) 
// 삭제된 줄은 갱신하고 이전 줄 정보로부터 삭제된 줄의 다음 줄 정보를 가져와 재사용한다.
// 삽입이나 삭제나 둘다 문자 길이만큼 오프셋을 조정하면 완벽하게 처리된다.



LRESULT OnLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    int toff = GetOffsetFromPoint(x + xPos, y + yPos);

    SelectWord(toff, SelectStart, SelectEnd);
    if (SelectStart != SelectEnd) {
        off = SelectEnd;
        SetCaret();
        Invalidate(-1);
    }

    return 0;
}

LRESULT OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    int toff;
    int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

    BOOL bShift, bCtrl;
    bShift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
    bCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);

    if (bShift) {
        toff = GetOffsetFromPoint(x + xPos, y + yPos);
        ExpandSelection(off, toff);
        off = toff;
    }
    else {
        ClearSelection();
        off = GetOffsetFromPoint(x + xPos, y + yPos);
        SelectStart = SelectEnd = off;
    }

    SetCapture(hWnd);
    bCapture = TRUE;
    SetCaret();
    return 0;
}

LRESULT OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    if (!bCapture) { return 0; }
    int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

    int toff = off;
    off = SelectEnd = GetOffsetFromPoint(x + xPos, y + yPos);
    SetCaret();
    Invalidate(min(toff, off), max(toff, off));

    int row, column, start, end;
    BOOL bInstallTimer = FALSE;

    if (y > g_crt.bottom) {
        SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
        bInstallTimer = TRUE;
    }

    if (y < 0) {
        SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
        bInstallTimer = TRUE;
    }

    if (!g_Option.wordWrap && !g_Option.kjcCharWrap) {
        GetRowAndColumn(SelectEnd, row, column);
        start = lineInfo[row].start;
        end = lineInfo[row].end;

        if (x > g_crt.right && SelectEnd != end) {
            SendMessage(hWnd, WM_HSCROLL, SB_LINERIGHT, 0);
            bInstallTimer = TRUE;
        }
        if (x < 0 && SelectEnd != start) {
            SendMessage(hWnd, WM_HSCROLL, SB_LINELEFT, 0);
            bInstallTimer = TRUE;
        }
    }

    if (bInstallTimer) {
        SetTimer(hWnd, 1, 100, NULL);
    }
    else {
        KillTimer(hWnd, 1);
    }

    return 0;
}

LRESULT OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    bCapture = FALSE;
    ReleaseCapture();
    KillTimer(hWnd, 1);

    // 드래그 중 입력한 조립 문자를 취소한다.
    HIMC hImc = ImmGetContext(hWnd);
    ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmReleaseContext(hWnd, hImc);

    return 0;
}

LRESULT OnSetFocus(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    SetCaret(FALSE, FALSE);

    SelectFgColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
    SelectBgColor = GetSysColor(COLOR_HIGHLIGHT);

    if (HideType != 2 && SelectStart != SelectEnd) {
        InvalidateRect(hWnd, NULL, TRUE);
    }

    return 0;
}

LRESULT OnKillFocus(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DestroyCaret();

    if (HideType != 2) {
        SelectFgColor = RGB(0, 0, 0);
        SelectBgColor = RGB(192, 192, 192);
    }

    if (HideType != 2 && SelectStart != SelectEnd) {
        InvalidateRect(hWnd, NULL, TRUE);
    }

    return 0;
}

LRESULT OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    static int Sum = 0;
    int Lines = 0, nScroll = 0, WheelUnit = 0;
    SHORT WheelDelta;

    // HIWORD(wParam) : 휠 회전값(통상 +-120)
    WheelDelta = (SHORT)HIWORD(wParam);
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &Lines, 0);

    // WHEEL_DELTA(120)
    WheelUnit = WHEEL_DELTA / Lines;
    Sum += WheelDelta;
    nScroll = Sum / WheelUnit;
    Sum %= WheelUnit;

    int Steps = abs(nScroll);
    for (int i = 0; i < Steps; i++) {
        if (nScroll > 0) {
            SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
        }
        else {
            SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
        }
    }

    return 0;
}

LRESULT OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam) {

    return 0;
}

LRESULT OnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    HMENU hPopupMenu;
    POINT Mouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    hPopupMenu = CreatePopupMenu();
    AppendMenu(hPopupMenu, MF_STRING, IDM_CUT, L"잘라내기(&T)");
    AppendMenu(hPopupMenu, MF_STRING, IDM_COPY, L"복사(&C)");
    AppendMenu(hPopupMenu, MF_STRING, IDM_PASTE, L"붙여넣기(&P)");
    AppendMenu(hPopupMenu, MF_STRING, IDM_SELECTALL, L"모두선택(&A)");

    if (IsClipboardFormatAvailable(CF_TEXT) == FALSE) {
        EnableMenuItem(hPopupMenu, IDM_PASTE, MF_BYCOMMAND | MF_GRAYED);
    }

    if (SelectStart == SelectEnd) {
        EnableMenuItem(hPopupMenu, IDM_CUT, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hPopupMenu, IDM_COPY, MF_BYCOMMAND | MF_GRAYED);
    }

    // Shift + F10
    if ((lParam == (LPARAM)-1)) {
        GetCaretPos(&Mouse);
        ClientToScreen(hWnd, &Mouse);
    }

    TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, Mouse.x, Mouse.y, 0, hWnd, NULL);
    DestroyMenu(hPopupMenu);

    return 0;
}
LRESULT OnHScroll(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    SCROLLINFO si;
    int increase = 0;

    switch (LOWORD(wParam)) {
        case SB_LINELEFT:
            increase = -FontHeight;
            break;

        case SB_LINERIGHT:
            increase = FontHeight;
            break;

        case SB_PAGELEFT:
            increase = -(g_crt.right - g_crt.left);
            break;

        case SB_PAGERIGHT:
            increase = g_crt.right - g_crt.left;
            break;

        case SB_THUMBTRACK:
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo(hWnd, SB_HORZ, &si);
            increase = si.nTrackPos - xPos;
            break;
    }

    increase = max(-xPos, min(increase, xMax - xPos));
    xPos += increase;
    ScrollWindow(hWnd, -increase, 0, NULL, NULL);
    SetScrollPos(hWnd, SB_HORZ, xPos, TRUE);

    return 0;
}

LRESULT OnVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    int increase;
    int per;
    SCROLLINFO si;

    per = (g_crt.bottom / LineHeight) * LineHeight;
    increase = 0;

    switch (LOWORD(wParam)) {
        case SB_LINEUP:
            increase = -LineHeight;
            break;

        case SB_LINEDOWN:
            increase = LineHeight;
            break;

        case SB_PAGEUP:
            increase = -per;
            break;

        case SB_PAGEDOWN:
            increase = per;
            break;

            // SB_THUMBTRACK 메세지는 현재 위치를 임의의 위치로 옮겼을 때 발생하며 따라서 줄의 경계 따위는 완전히 무시한다.
        case SB_THUMBTRACK:
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo(hWnd, SB_VERT, &si);
            increase = si.nTrackPos - yPos;
            break;

        default:
            break;
    }

    // 0보다 작지 않게 고정
    increase = max(-yPos, min(increase, yMax - yPos - per));
    // 스크롤바 이동시 줄의 중간까지만 걸쳐 보이는 현상이 없도록 줄간의 배수로 강제 내림 연산
    increase = (increase / LineHeight) * LineHeight;
    yPos += increase;
    ScrollWindow(hWnd, 0, -increase, NULL, NULL);
    SetScrollPos(hWnd, SB_VERT, yPos, TRUE);

    return 0;
}
LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    HGLOBAL hMem;
    WCHAR* ptr;
    int SelectFirst, SelectSecond;

    switch (LOWORD(wParam)) {
    case IDM_CUT:
        if (bCapture) { break; }
        if (SelectStart != SelectEnd && bCapture == FALSE) {
            SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_COPY, 0), 0);
            DeleteSelection();
            Invalidate(FindParagraphStart(off));
            SetCaret();
        }
        break;

    case IDM_COPY:
        if (SelectStart != SelectEnd) {
            SelectFirst = min(SelectStart, SelectEnd);
            SelectSecond = max(SelectStart, SelectEnd);
            hMem = GlobalAlloc(GHND, sizeof(WCHAR) * (SelectSecond - SelectFirst + 1));
            ptr = (WCHAR*)GlobalLock(hMem);
            memcpy(ptr, buf + SelectFirst, sizeof(WCHAR) * (SelectSecond - SelectFirst));
            GlobalUnlock(hMem);
            if (OpenClipboard(hWnd)) {
                EmptyClipboard();
                SetClipboardData(CF_TEXT, hMem);
                CloseClipboard();
            }
        }
        break;

    case IDM_PASTE:
        if (bCapture) { break; }
        if (IsClipboardFormatAvailable(CF_TEXT)) {
            DeleteSelection();
            OpenClipboard(hWnd);
            hMem = GetClipboardData(CF_TEXT);
            ptr = (WCHAR*)GlobalLock(hMem);
            Insert(off, ptr);
            GlobalUnlock(hMem);
            CloseClipboard();
            Invalidate(FindParagraphStart(off));
            off += wcslen(ptr);
            SetCaret();
        }
        break;

    case IDM_SELECTALL:
        SelectStart = 0;
        SelectEnd = wcslen(buf);
        off = SelectEnd;
        InvalidateRect(hWnd, NULL, TRUE);
        SetCaret();
        break;
    }

    return 0;
}

LRESULT OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    int row = 0, column = 0;
    int start = 0, end = 0, toff = 0;
    int oldRow = 0;

    BOOL bShift, bCtrl;
    bShift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
    bCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);

    switch (wParam) {
        case VK_UP:
            if (bCtrl && bShift) { break; }
            GetRowAndColumn(off, row, column);
            if (row > 0) {
                if (bCtrl) {
                    SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
                    if (row != (g_crt.bottom + yPos) / LineHeight) { break; }
                }
                toff = off;
                row--;
                off = GetDocsXPosOnLine(row, PrevX);
                if (bShift) {
                    ExpandSelection(toff, off);
                }
                else {
                    if (SelectStart != SelectEnd) {
                        off = min(SelectStart, SelectEnd);
                        ClearSelection();
                        SetCaret();         // PrevX 갱신
                        SendMessage(hWnd, WM_KEYDOWN, VK_UP, 0);
                    }
                }
                SetCaret(FALSE);
            }

            if (!bShift) {
                ClearSelection();
            }
            break;

        case VK_DOWN:
            if (bCtrl && bShift) { break; }

            GetRowAndColumn(off, row, column);
            if (bCtrl) {
                SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
                if (row != yPos / LineHeight - 1) { break; }
            }

            if (row < lineCount - 1) {
                toff = off;
                row++;
                off = GetDocsXPosOnLine(row, PrevX);
                if (bShift) {
                    ExpandSelection(toff, off);
                }
                else {
                    if (SelectStart != SelectEnd) {
                        off = max(SelectStart, SelectEnd);
                        ClearSelection();
                        SetCaret();         // PrevX 갱신
                        SendMessage(hWnd, WM_KEYDOWN, VK_DOWN, 0);
                    }
                }
                SetCaret(FALSE);
            }

            if (!bShift) {
                ClearSelection();
            }
            break;


            // VK_LEFT와 VK_RIGHT 코드를 수정했다. 코드가 구조적으로 작성되어 자세히 살펴보지 않으면 흐름을 파악하기 어렵다.
            // 기존 코드는 GetNextOffset이나 GetPrevOffset으로 오프셋 값을 조정한 후 SetCaret을 호출하여 캐럿의 위치를 이동시키는 단순한 구조였다.
            // SetCaret은 내부적으로 GetCoordinate를 호출하는데 이 함수가 화면상의 좌표를 계산해서 반환하면 SetCaret은 캐럿을 옮기는 동작만 한다.
            // 여기서 중요한건 SetCaret 함수가 언제나 현재 위치, 즉 전역 변수 off를 참조한다는 것인데
            // 오프셋 값을 계산하고 캐럿의 위치를 옮기는 이러한 구조에선 현재 오프셋 값을 기준으로 분기문을 만드는 것이 좋다.
            // SetCaret은 항상 마지막에 호출되어야 하므로 순서가 정해져 있다.
            // 실행 흐름을 잘 따라가다 보면 왜 이런 구조로 코드를 작성했는지 파악할 수 있을 것이다.
        case VK_LEFT:
            if (off > 0) {
                GetRowAndColumn(off, row, column);
                start = lineInfo[row].start;
                end = lineInfo[row].end;

                toff = off;
                if (bCtrl) {
                    off = GetPrevWord(off);
                }
                else {
                    if (off == start) {
                        if (buf[GetPrevOffset(off)] == '\r') {
                            off = GetPrevOffset(off);
                            bLineEnd = FALSE;
                        }
                        else {
                            bLineEnd = TRUE;
                        }
                    }
                    else {
                        off = GetPrevOffset(off);
                        bLineEnd = FALSE;
                    }
                }

                if (bShift) {
                    ExpandSelection(toff, off);
                }
                else {
                    if (SelectStart != SelectEnd) {
                        off = min(SelectStart, SelectEnd);
                    }
                }
                SetCaret();
            }

            if (!bShift) {
                ClearSelection();
            }
            break;

        case VK_RIGHT:
            if (off < wcslen(buf)) {
                GetRowAndColumn(off, row, column);
                start = lineInfo[row].start;
                end = lineInfo[row].end;

                toff = off;
                if (bCtrl) {
                    off = GetNextWord(off);
                }
                else {
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
                }

                if (bShift) {
                    ExpandSelection(toff, off);
                }
                else {
                    if (SelectStart != SelectEnd) {
                        off = max(SelectStart, SelectEnd);
                    }
                }
                SetCaret();
            }

            if (!bShift) {
                ClearSelection();
            }
            break;

        case VK_DELETE:
            if (bCapture) { break; }

            if (bShift) {
                SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_CUT, 0), 0);
                break;
            }

            if (DeleteSelection() == FALSE) {
                if (IsCRLF(off)) {
                    Delete(off, 2);
                }
                else {
                    Delete(off, 1);
                }
            }
            Invalidate(FindParagraphStart(off));
            SetCaret();
            break;

        case VK_BACK:
            if (bCapture) { break; }
            if ((off == 0 && SelectStart == SelectEnd) || (bShift && bCtrl)) { break; }

            if (DeleteSelection() == FALSE) {
                if (off == 0) { break; }
                if (g_Option.wordWrap) {
                    off = GetPrevOffset(off);
                    GetRowAndColumn(off, row, column);

                    start = lineInfo[row].start;
                    end = lineInfo[row].end;

                    if (column > 0 && off == end && buf[off] != '\r') {
                        bLineEnd = FALSE;
                        Delete(off, 1);
                    }
                    else if (column > 0 && off == end - 1) {
                        bLineEnd = TRUE;
                        if (IsCRLF(off)) {
                            Delete(off, 2);
                        }
                        else {
                            Delete(off, 1);
                        }
                    }
                    else {
                        // 여기서는 bLineEnd를 조정할 필요가 없다.
                        if (IsCRLF(off)) {
                            Delete(off, 2);
                        }
                        else {
                            Delete(off, 1);

                        }
                    }
                }
                else {
                    off = GetPrevOffset(off);
                    if (IsCRLF(off)) {
                        Delete(off, 2);
                    }
                    else {
                        Delete(off, 1);

                    }
                }
            }
            Invalidate(FindParagraphStart(off));
            SetCaret();
            break;

        case VK_INSERT:
            if (bShift) {
                SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PASTE, 0), 0);
            }
            else if(bCtrl) {
                SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_COPY, 0), 0);
            }
            break;

        case VK_HOME:
            GetRowAndColumn(off, row, column);
            toff = off;
            if (bCtrl) {
                off = 0;
            }
            else {
                off = GetOffset(row, 0);
            }

            bLineEnd = FALSE;

            if (bShift) {
                ExpandSelection(toff, off);
            }
            else {
                ClearSelection();
            }
            SetCaret();
            break;

        case VK_END:
            GetRowAndColumn(off, row, column);
            toff = off;
            if (bCtrl) {
                off = wcslen(buf);
            }
            else {
                off = GetOffset(row, 2147483647);
            }

            if (buf[off] != '\r' && buf[off] != 0) {
                bLineEnd = TRUE;
            }

            if (bShift) {
                ExpandSelection(toff, off);
            }
            else {
                ClearSelection();
            }
            SetCaret();
            break;

        case VK_PRIOR:
            GetRowAndColumn(off, row, column);
            oldRow = row;
            row -= g_crt.bottom / LineHeight;
            row = max(row, 0);
            yPos = yPos - (oldRow - row) * LineHeight;
            yPos = max(yPos, 0);
            InvalidateRect(hWnd, NULL, TRUE);
            SetScrollPos(hWnd, SB_VERT, yPos, TRUE);

            toff = off;
            off = GetDocsXPosOnLine(row, PrevX);
            if (bShift) {
                ExpandSelection(toff, off);
            }
            else {
                ClearSelection();
            }
            SetCaret(FALSE);
            break;

        case VK_NEXT:
            GetRowAndColumn(off, row, column);
            oldRow = row;
            row += g_crt.bottom / LineHeight;
            row = min(row, lineCount - 1);
            yPos = yPos + (row - oldRow) * LineHeight;
            yPos = max(0, min(yPos, yMax - (g_crt.bottom / LineHeight) * LineHeight));
            // row = max(0, min(yPos, yMax - (g_crt.bottom / LineHeight) * LineHeight));
            InvalidateRect(hWnd, NULL, TRUE);
            SetScrollPos(hWnd, SB_VERT, yPos, TRUE);

            toff = off;
            off = GetDocsXPosOnLine(row, PrevX);
            if (bShift) {
                ExpandSelection(toff, off);
            }
            else {
                ClearSelection();
            }
            SetCaret(FALSE);
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

LRESULT OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    POINT Mouse;

    switch (wParam) {
    case 1:
        GetCursorPos(&Mouse);
        ScreenToClient(hWnd, &Mouse);
        SendMessage(hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(Mouse.x, Mouse.y));
        break;
    }

    return 0;
}

LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    if (wParam != SIZE_MINIMIZED) {
        GetClientRect(hWnd, &g_crt);
        if (g_Option.wordWrap || g_Option.KeepPunctWithWord) {
            RebuildLineInfo();
        }
        UpdateScrollInfo();
        if (GetFocus() == hWnd) {
            SetCaret();
        }

        if (hBitmap) {
            // delete hBitmap;
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }
    return 0;

}

LRESULT OnWindowPosChanged(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    LPWINDOWPOS lpWndPos = (LPWINDOWPOS)lParam;
    g_wrt.left = lpWndPos->x;
    g_wrt.top = lpWndPos->y;
    g_wrt.right = lpWndPos->x + lpWndPos->cx;
    g_wrt.bottom = lpWndPos->y + lpWndPos->cy;

    return 0;
}

LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    // if (hBitmap == NULL) {
    //    hBitmap = new Bitmap(g_crt.right, g_crt.bottom, PixelFormat32bppARGB);
    // }

    // HBITMAP bmp;
    // hBitmap->GetHBITMAP(Color(0, 0, 0, 0), &bmp);

    if (hBitmap == NULL) {
        // hBitmap = CreateCompatibleBitmap(hdc, g_crt.right, g_crt.bottom);
        
        // 24비트 포맷의 DDB는 알파 채널을 표현할 수 없다. 따라서 32비트 포맷의 DIB를 이용하기로 한다.
        // DIB 섹션은 DIB이되 HBITMAP으로 표현되는 중간 포맷인데 적당히 변환 가능하여 GDI+와 GDI 모두 호환된다.
        // DIBSECTION 구조체를 보면 첫 번째 멤버로 BITMAP 구조체를 가진다는 걸 알 수 있다.
        // 때문에 BitBlt이나 StretchBlt도 사용 가능하므로 DIB 섹션을 이용하기로 한다.
        // 단, 알파 비트를 직접 설정해야 한다는 번거로움이 있다.
        // 일단 시도해보고 실패하면 추후 GDI+에 대한 연구를 끝내고 본 프로젝트를 다시 업데이트하기로 한다.
        
        // CreateDIBSection 함수는 래스터 데이터 크기를 조사하고 이 크기만큼 메모리를 할당하여 ppvBits(네 번째 인수)로 반환한다.
        // 즉, 함수 내부적으로 래스터 데이터를 담을 버퍼를 생성하여 전달하는데 이 버퍼는 시스템에 의해 관리되므로 사용자가 신경쓸 필요 없다.
        // 별도의 해제 구문이 필요하지 않으며 DeleteObject로 비트맵 리소스를 해제할 때 함께 알아서 정리된다.
        
        // 알파 블렌딩이 가능한 비트맵을 생성하기 위해 비트 압축(또는 해석) 방식을 BI_BITFIELDS로 지정했다.
        // BI_ALPHABITFIELDS도 있지만 더 간단한 구조로 만들려면 수동으로 추가하는 것이 낫다.
        
        // bmi를 포인터 타입으로 변경했으며 RGBQUAD 구조체, 즉 팔레트 구조체를 3개분만큼 추가하여 메모리를 할당했다.
        // 이는 마스크를 지정하기 위함인데, 정확히는 RGB 비트 순서를 지정하는 것이다.
        // BI_BITFIELDS 방식으로 비트맵을 생성하면 GDI 표준 포맷인 BGRA 순서가 아니라 임의로 픽셀의 저장 방식을 변경할 수 있다.
        
        // 이때 RGBQUAD 구조체가 사용되는데 RGBQUAD 구조체는 기본적으로 팔레트용,
        // 즉 팔레트 기반 비트맵(8bpp 이하)에서 사용되었던 색상 테이블의 구성 요소 중 하나이다.
        // 24bpp, 32bpp에서는 더이상 직접적으로 사용되지 않으나 비트맵을 생성할 때
        // 해석 방식을 BI_BITFIELDS로 지정하면 시스템(GDI)이 픽셀의 포맷을 새로 정의하기 위해 RGBQUAD 구조체를 확인한다.
        
        // 이때 RGBQUAD는 팔레트라는 의미가 사라지고 일시적으로 RGB 마스크 배열로 해석되어 단순히 4바이트 크기의 메모리 공간으로만 사용된다.
        // RGBQUAD 구조체의 멤버 순서를 보면, Intel 아키텍처 기반의 LE(Little Endian) 비트 스트림 방식을 기본(Default)으로 가정하여 설계된 구조체라는 것을 알 수 있다.
        
        // BI_BITFIELDS를 지정하면 시스템(Windows GDI)은 내부적으로 bmiColors 멤버를 다음의 순서대로 해석한다.
        // - [0]: Red
        // - [1]: Green
        // - [2]: Blue

        // 기존과 같은 방식(BGRA)으로 픽셀 포맷을 지정하고 싶으면 [0] = 0x00FF0000, [1] = 0x0000FF00, [2] = 0x000000FF를 대입하면 되고,
        // 만약 Direct나 GDI+가 사용하는 픽셀 포맷(RGBA)을 원하면 [0] = 0x000000FF, [1] = 0x0000FF00, [2] = 0x00FF0000로 지정하면 된다.

        BITMAPINFO* bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3);
        memset(bmi, 0, sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3);

        LPBITMAPINFOHEADER lpInfo = &bmi->bmiHeader;
        lpInfo->biSize = sizeof(BITMAPINFOHEADER);
        lpInfo->biWidth = g_crt.right;
        
        // biHeight 멤버의 값을 음수 값으로 지정한다.
        // 기본적으로 비트맵은 Bottom-up 방향의 포맷을 가지는데
        // GDI+나 Direct2D 같은 그래픽 API에서는 Top-Down 좌표계를 사용한다.
        // 따라서 음수로 변환하여 메모리의 첫 번째 스캔라인을 화면 맨 위로 올라가도록 만들어야 한다.
        lpInfo->biHeight = -LineHeight;
        lpInfo->biPlanes = 1;
        lpInfo->biBitCount = 32;
        lpInfo->biCompression = BI_BITFIELDS;

        DWORD* pMask = (DWORD*)(bmi->bmiColors);
        pMask[0] = 0x00FF0000;
        pMask[1] = 0x0000FF00;
        pMask[2] = 0x000000FF;

        void* pvBuffer = NULL;
        hBitmap = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, &pvBuffer, NULL, 0);
    }

    HDC hMemDC = CreateCompatibleDC(hdc);
    HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);
    HBRUSH hBrush = GetSysColorBrush(COLOR_WINDOW);

    // Graphics g(hMemDC);
    // g.Clear(Color(30, 0, 0, 0));
    
    int Top, Bottom, Line, Start;
    Top = yPos / LineHeight;
    Start = (yPos + ps.rcPaint.top) / LineHeight;
    Bottom = (yPos + ps.rcPaint.bottom - 1) / LineHeight;
    Bottom = min(Bottom, lineCount - 1);

    RECT lrt;
    SetRect(&lrt, 0, 0, g_crt.right, LineHeight);

    for (Line = Top; Line <= Bottom; Line++) {
        FillRect(hMemDC, &lrt, hBrush);
        DrawLine(hMemDC, Line);

        // 줄 단위 더블 버퍼링 구조로 작성
        BitBlt(hdc, 0, (Line - Top) * LineHeight, g_crt.right, (Line - Top) * LineHeight + LineHeight, hMemDC, 0, 0, SRCCOPY);
    }

    // 남은 여백
    SetRect(&lrt, 0, (Line - Top) * LineHeight, g_crt.right, g_crt.bottom);
    FillRect(hdc, &lrt, hBrush);
    // 알파 블렌딩 
    // BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    // POINT ptLocation = { g_wrt.left, g_wrt.top };
    // SIZE szWnd = { g_wrt.right - g_wrt.left, g_wrt.bottom - g_wrt.top };
    // POINT ptSrc = { 0, 0 };

    // 윈도우 전체에 대한 알파 채널을 지원해야 하므로 AlphaBlend 대신 UpdateLayeredWindow 함수 사용
    // UpdateLayeredWindow(hWndMain, hdc, &ptLocation, &szWnd, hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

    DeleteObject(hBrush);
    SelectObject(hMemDC, hOld);
    // DeleteObject(bmp);
    DeleteDC(hMemDC);

    EndPaint(hWnd, &ps);
    return 0;
}

LRESULT OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    WCHAR Abuf[0x10];
    char ch = wParam;
    bAlphaNum = TRUE;

    if (bCapture) { return 0; }

    if (ch == 1) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_SELECTALL, 0), 0);
        return 0;
    }

    if (ch == 3) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_COPY, 0), 0);
        return 0;
    }

    if (ch == 22) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PASTE, 0), 0);
        return 0;
    }

    if (ch == 24) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_CUT, 0), 0);
        return 0;
    }

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

    DeleteSelection();
    for (int i = 0; i < LOWORD(lParam); i++) {
        // 문자 입력 코드
        Insert(off, Abuf);
        off += wcslen(Abuf);
    }

    bComp = FALSE;
    Invalidate(FindParagraphStart(off - wcslen(Abuf)));
    SetCaret();
    return 0;
}

LRESULT OnImeChar(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    WCHAR Wbuf[0x10];
    bAlphaNum = FALSE;

    Wbuf[0] = wParam;
    Wbuf[1] = 0;

    if (bComp) {
        off -= 2;
        Delete(off, 1);
    }

    Insert(off, Wbuf);
    off += wcslen(Wbuf);
    bComp = FALSE;
    Invalidate(FindParagraphStart(off - wcslen(Wbuf)));
    SetCaret();
    return 0;
}

LRESULT OnImeComposition(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    int Length = 0;
    HIMC hImc = NULL;
    WCHAR* Cbuf = NULL;

    if (bCapture) { return 0; }
    DeleteSelection();

    if (lParam & GCS_COMPSTR) {
        hImc = ImmGetContext(hWnd);
        DWORD dwConversion, dwSentence;

        if (ImmGetConversionStatus(hImc, &dwConversion, &dwSentence)) {
            if (dwConversion & IME_CMODE_ALPHANUMERIC) {
                // 영문 입력 모드
                bAlphaNum = TRUE;
            }
            else {
                bAlphaNum = FALSE;
            }
        }

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
        Invalidate(FindParagraphStart(off - Length));
        SetCaret();
    }

    return DefWindowProc(hWnd, WM_IME_COMPOSITION, wParam, lParam);
}

LRESULT OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    InitializeGDIplus();

    hWndMain = hWnd;
    bufLength = 0x400;
    docLength = 0;
    bComp = FALSE;
    PrevX = 0;
    xMax = 0x400;
    xPos = yPos = 0;
    SelectStart = SelectEnd = 0;
    bCapture = FALSE;
    HideType = 1;

    buf = (WCHAR*)malloc(sizeof(WCHAR) * bufLength);
    memset(buf, 0, sizeof(WCHAR) * bufLength);
    wcscpy_s(buf, bufLength, L"아 디버깅 하는거 힘듭니다 별 문제는 없는거 같습니다.\r\n동해물과 백두산이 마르고 닳도록 하느님이 보우하사 우리나라 만세. 무궁화 삼천리 화려강산 대한사람 대한으로 길이 보전하세.\r\nabcdefghijklmnopqrstuvwxyz\r\nabcdefghijklmnopqrstuvwxyz\r\n");
    // wcscpy_s(buf, bufLength, L"동해물과 백두산이 마르고 닳도록 하느님이 보우하사\r\nabcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz\r\n우리나라 만세 무궁화 삼천리 화려 강산 대한 사람 대한으로 길이 보전하세");
    docLength = wcslen(buf);
    off = 0;

    {
        TEXTMETRIC tm;
        HDC hdc = GetDC(hWnd);
        GetTextMetrics(hdc, &tm);
        FontHeight = tm.tmHeight;
        FontWidth = tm.tmAveCharWidth;
        ReleaseDC(hWnd, hdc);
    }

    PrecomputeCharWidths();

    TabWidth = 4;
    TabSize = AsciiCharWidth[' '] * TabWidth;

    LineRatio = 120;
    LineHeight = (int)(FontHeight * LineRatio / 100);

    lineInfoSize = 0x400;
    lineInfo = (LineInfo*)malloc(sizeof(LineInfo) * lineInfoSize);
    memset(lineInfo, 0, sizeof(lineInfo) * lineInfoSize);

    RebuildLineInfo();
    UpdateScrollInfo();
    return 0;
}

LRESULT OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    if (buf) { free(buf); }
    if (lineInfo) { free(lineInfo); }
    if (hBitmap) { 
        // delete hBitmap;
        DeleteObject(hBitmap);
    }

    ShutdownGDIPlus();
    PostQuitMessage(0);
    return 0;
}

void SetCaret(BOOL bUpdatePrevX, BOOL bScrollToCaret) {
    SIZE sz;
    int toff, x, y;
    int caretWidth;

    toff = bComp ? off - 2 : off;
    caretWidth = bComp ? GetCharWidth(buf + toff, 1) : CARET_WIDTH;

    CreateCaret(hWndMain, NULL, caretWidth, FontHeight);
    ShowCaret(hWndMain);

    GetCoordinate(toff, x, y);

    // 스크롤바를 지원하면서부터 캐럿이 화면을 벗어났을 때 스크롤 되게끔 만들어야 한다.
    BOOL bScroll = FALSE;
    if (bScrollToCaret) {
        if (!g_Option.wordWrap && !g_Option.kjcCharWrap) {
            if ((x + CARET_WIDTH > xPos + g_crt.right) || (x < xPos)) {
                xPos = max(0, x - g_crt.right / 2);
                bScroll = TRUE;
            }
        }

        if (y < yPos) {
            yPos = y;
            bScroll = TRUE;
        }

        if (y + FontHeight > yPos + g_crt.bottom) {
            int ty = (g_crt.bottom - FontHeight) / LineHeight * LineHeight;
            yPos = y - ty;
            bScroll = TRUE;
        }

        if (bScroll) {
            SetScrollPos(hWndMain, SB_HORZ, xPos, TRUE);
            SetScrollPos(hWndMain, SB_VERT, yPos, TRUE);
            InvalidateRect(hWndMain, NULL, TRUE);
        }
    }

    SetCaretPos(x - xPos, y - yPos);

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
    WCHAR ch = 0;
    for (int i = 0; i < length; i++) {
        ch = (int)(*(src + i));
        if (ch < 128) {
            if (ch == '\t') {
                width = (width / TabSize + 1) * TabSize;
            }
            else {
                width += AsciiCharWidth[ch];
            }
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

    int move = docLength + idx + length;
    memmove(buf + idx + length, buf + idx, move * sizeof(WCHAR));
    memcpy(buf + idx, str, length * sizeof(WCHAR));
    docLength += length;

    int row, column, start, end;
    GetRowAndColumn(idx, row, column);

    start = lineInfo[row].start;
    end = lineInfo[row].end;

    // bLineEnd = FALSE;
    if (g_Option.wordWrap) {
        if (column > 0 && end == idx && buf[idx] != '\r' && bAlphaNum) {
            // 한글같은 조립형 문자의 경우 IME가 자동으로 캐럿 위치를 조정해준다.
            // 알파벳은 이러한 보정이 없으므로 직접 조정해야 한다
            bLineEnd = TRUE;
        }
        else if (column == 0 && row > 0 && bAlphaNum) {
            int toff = GetPrevOffset(idx);
            bLineEnd = !IsCRLF(toff);
        }
        else {
            bLineEnd = FALSE;
        }
    }
    else {
        bLineEnd = FALSE;
    }

    RebuildLineInfo(idx, wcslen(str));
    UpdateScrollInfo();
    return TRUE;
}

BOOL Delete(int idx, int cnt) {
    if (docLength < idx + cnt) { return FALSE; }

    int move = docLength - idx - cnt + 1;
    memmove(buf + idx, buf + idx + cnt, move * sizeof(WCHAR));
    docLength -= cnt;

    RebuildLineInfo(idx, -cnt);
    UpdateScrollInfo();
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

    if (g_Option.wordWrap || g_Option.kjcCharWrap) {
        start = lineStart;
        end = FindWrapPoint(lineStart, lineEnd);
    }
    else {
        start = lineStart;
        end = lineEnd;
    }
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
            x = (x / TabSize + 1) * TabSize;
        }
        else {
            x += GetCharWidth(ptr, 1);
        }

        ptr += 1;
    }
}

BOOL IsWhiteChar(WCHAR ch) {
    return ch == L' ' ||
        ch == L'\r' ||
        ch == L'\n' || 
        ch == L'\t';
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
        if (pos < end && !IsWhiteChar(ptr[pos - 1])  && !IsWhiteChar(ptr[pos])) {
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

void RebuildLineInfo(int idx /* = -1 */, int length /* = -1 */) {
    int curLine = 0, pos = 0, wrapEnd = 0;
    int start = 0, end = 0;
    int paraStart = 0;
    WCHAR* ptr = buf;

    if (idx != -1) {
        paraStart = FindParagraphStart(idx);
        if (paraStart > 0) {
            GetRowAndColumn(paraStart - 2, curLine, end);
            curLine++;
        }
    }

    while (1) {
        if (curLine >= lineCount) {
            lineCount += 0x400;
            lineInfo = (LineInfo*)realloc(lineInfo, sizeof(LineInfo) * lineCount);
            if (lineInfo == NULL) { return; }
            memset(lineInfo + (lineCount - 0x400), 0, sizeof(LineInfo) * 0x400);
        }
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
    WCHAR* ptr = buf + start;

    if (dest == 0) { 
        return start;
    }

    int Width = 0, len = 0;
    while (ptr - buf < end) {
        if (*ptr == '\t') {
            Width = (Width / TabSize + 1) * TabSize;
        }
        else {
            Width += GetCharWidth(ptr, 1);
        }
        ptr += 1;

        if (Width >= dest) { break; }
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

void UpdateScrollInfo() {
    SCROLLINFO si;

    int line = g_crt.bottom / LineHeight;
    int needed = line / 2 + lineCount;

    yMax = needed * LineHeight;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = yMax;
    si.nPage = line * LineHeight;

    if (si.nMax < si.nPage) {
        // 스크롤 범위가 페이지 높이보다 작으면(DISABLE 조건) 0으로 맞춤
        yPos = 0;
        Invalidate(-1);
    }
    si.nPos = yPos;
    SetScrollInfo(hWndMain, SB_VERT, &si, TRUE);

    if (!g_Option.wordWrap && !g_Option.kjcCharWrap) {
        int i, MaxLength, start, end;
        i = MaxLength = start = end = 0;
        for (i; i < lineCount; i++) {
            start = lineInfo[i].start;
            end = lineInfo[i].end;

            MaxLength = max(MaxLength, end - start);
        }

        xMax = (int)(MaxLength * FontWidth * 1.5);
        si.nMax = xMax;
        si.nPage = g_crt.right;
        si.nPos = xPos;
        SetScrollInfo(hWndMain, SB_HORZ, &si, TRUE);
    }
}

int DrawLine(HDC hdc, int line) {
    int start, end, x, length, idx;

    start = lineInfo[line].start;
    end = lineInfo[line].end;
    if (start == 0 && end == 0 && line > 0) { return 0; }

    x = 0 - xPos;
    idx = start;

    BOOL bInSelect;
    COLORREF fg, bg;

    // 앞 뒤를 알 수 없으므로 정규화
    int SelectFirst = min(SelectStart, SelectEnd), SelectSecond = max(SelectStart, SelectEnd);

    while (1) {
        length = 0;
        while (1) {
            if (buf[idx + length] == '\t') { 
                if (length == 0) { length = 1; }
                if (SelectStart != SelectEnd && idx >= SelectFirst && idx < SelectSecond) {
                    bInSelect = TRUE;
                }
                else {
                    bInSelect = FALSE;
                }
                break;
            }

            if (idx + length == end) { 
                if (SelectStart != SelectEnd && idx >= SelectFirst && idx < SelectSecond) {
                    bInSelect = TRUE;
                }
                else {
                    bInSelect = FALSE;
                }
                break;
            }

            if (SelectStart != SelectEnd && length != 0 && idx + length == SelectFirst) {
                bInSelect = FALSE;
                break;
            }

            if (SelectStart != SelectEnd && length != 0 && idx + length == SelectSecond) {
                bInSelect = TRUE;
                break;
            }

            length++;
        }

        if (bInSelect && (GetFocus() == hWndMain || HideType != 0)) {
            fg = SelectFgColor;
            bg = SelectBgColor;
        }
        else {
            fg = RGB(0, 0, 0);
            bg = GetSysColor(COLOR_WINDOW);
        }

        // DrawSegment(hdc, x, line * LineHeight - yPos, idx, length, (idx + length == end), fg, bg);
        DrawSegment(hdc, x, 0, idx, length, (idx + length == end), fg, bg);

        idx += length;
        if (idx == end) { return 1; }
    }

}

// 조각이란 한 번에 같이 출력할 수 있는 성질이 같은 문자열의 집합으로 정의된다.
// 여기서는 탭만 구분자로 사용된다.
void DrawSegment(HDC hdc, int& x, int y, int idx, int length, BOOL ignore, COLORREF fg, COLORREF bg) {
    int docx;
    int oldx;
    RECT rt;
    HBRUSH hBrush;

    // 여기서 x는 화면상의 좌표인데
    // 수평 스크롤로 인해 출력해야할 문자의 좌표가 음수값이 되면
    // 탭 사이즈를 구하는 공식이 무효해진다.
    // 따라서, x를 화면상의 좌표로 받아 간단히 출력하되
    // 탭 문자를 만난 경우에는 공식을 적용하기 위해 문서상의 좌표로 변환하고 다시 화면 좌표로 되돌려야 한다.
    if (buf[idx] == '\t') {
        oldx = x;
        docx = x + xPos;
        docx = (docx / TabSize + 1) * TabSize;
        x = docx - xPos;
        SetRect(&rt, oldx, y, x + 1, y + FontHeight);
        hBrush = CreateSolidBrush(bg);
        FillRect(hdc, &rt, hBrush);
        DeleteObject(hBrush);
    }
    else {
        // GDI+의 그리기 동작은 전부 Graphics 클래스로부터 이뤄진다.
        // 새로 만든 엔진이기 때문에 GDI와 같은 로직을 사용할 순 없는데
        // 굳이 그럴 생각도 없으므로 알파 채널을 지원하는 32비트 포맷의 비트맵 생성용으로만 쓰기로 한다.
        // Graphics g(hdc);

        // 계단 현상
        // g.SetSmoothingMode(SmoothingModeAntiAlias);

        // 글자 선명도 향상
        // g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        // g.Clear(Color(255, 0, 0, 0));

        // 시스템 기본 폰트 시용
        // NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
        // SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
        // Font font(hdc, &ncm.lfMessageFont);

        // PointF pointF(x, y);
        // SolidBrush hBrush(Color(255, 0, 0, 0));

        // g.DrawString(buf + idx, length, &font, pointF, &hBrush);
        
        SetTextColor(hdc, fg);
        SetBkColor(hdc, bg);
        TextOut(hdc, x, y, buf + idx, length);

        if (ignore == FALSE) {
            x += GetCharWidth(buf + idx, length);
        }
    }
}

int GetOffsetFromPoint(int x, int y) {
    // 캡처 상태를 가정하여 음수값이 발생하지 않도록 조정 - 보통 MOUSEMOVE, LBUTTONUP 메세지에서 발생
    x = max(0, x);
    y = max(0, y);

    int row, start, end;

    row = y / LineHeight;
    row = min(row, lineCount - 1);

    start = lineInfo[row].start;
    end = lineInfo[row].end;

    int chWidth, acWidth;
    acWidth = chWidth = 0;

    WCHAR* ptr = buf + start;
    while (ptr - buf < end) {
        if (*ptr == '\t') {
            chWidth = (acWidth / TabSize + 1) * TabSize - acWidth;
        }
        else {
            chWidth = GetCharWidth(ptr, 1);
        }

        acWidth += chWidth;
        // 글자의 중앙을 기준으로 좌우를 나누어 비교한다
        if (acWidth - chWidth / 2 >= x) { break; }

        ptr += 1;
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

int GetOffsetFromPoint(POINT Mouse) {
    return GetOffsetFromPoint(Mouse.x, Mouse.y);
}

BOOL IsDelims(int idx) {
    static WCHAR delims[] = L" \t\r\n\"\'\\,.<>:;/(){}[]~!@#$%^&*-+?=";
    return (wcsrchr(delims, buf[idx]) || buf[idx] == 0);
}

int GetPrevWord(int idx) {
    if (idx == 0) { return idx; }

    while (idx--) {
        if (IsDelims(idx) == FALSE || idx == 0) { break; }
    }

    while (1) {
        if (IsDelims(idx) == TRUE || idx == 0) { break; }
        idx--;
    }

    if (idx != 0) { idx++; }
    return idx;
}

int GetNextWord(int idx) {
    while (1) {
        if (IsDelims(idx) == TRUE) { break; }
        idx++;
    }

    while (1) {
        if (IsDelims(idx) == FALSE || buf[idx] == 0) { break; }
        idx++;
    }

    return idx;
}

void ClearSelection() {
    if (SelectStart != SelectEnd) {
        Invalidate(min(SelectStart, SelectEnd), max(SelectStart, SelectEnd));
        SelectStart = SelectEnd = 0;
    }
}

void ExpandSelection(int start, int end) {
    int oldSelectEnd;

    if (SelectStart == SelectEnd) {
        SelectStart = start;
        SelectEnd = end;
        Invalidate(min(SelectStart, SelectEnd), max(SelectStart, SelectEnd));
    }
    else {
        oldSelectEnd = SelectEnd;
        SelectEnd = end;
        Invalidate(min(oldSelectEnd, SelectEnd), max(oldSelectEnd, SelectEnd));
    }

    InvalidateRect(hWndMain, NULL, TRUE);
}

BOOL DeleteSelection() {
    int SelectFirst, SelectSecond;

    if(SelectStart != SelectEnd) {
        SelectFirst = min(SelectStart, SelectEnd);
        SelectSecond = max(SelectStart, SelectEnd);
        Delete(SelectFirst, SelectSecond - SelectFirst);
        SelectStart = SelectEnd = 0;
        off = SelectFirst;
        return TRUE;
    }

    return FALSE;
}

void  InitializeGDIplus() {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_Token, &gdiplusStartupInput, NULL);
}

void ShutdownGDIPlus() {
    GdiplusShutdown(g_Token);
}

void Invalidate(int idx1, int idx2 /* = -1 */) {
    // 문서를 편집하는 동작의 함수들을 보면 작업 단위가 오프셋이다.
    // 따라서 그리기 최적화 함수 역시 오프셋을 받아 사용하기로 한다.
    // 다시 그릴 영역을 직접 지정하면 되는데 간단한 계산식으로 충분히 만들 수 있다.
    // 오프셋으로부터 픽셀 좌표를 계산하고 스크롤 값을 빼 화면상의 좌표를 얻어온다

    RECT srt;
    int x, y, y1, y2;
    if(idx1 == -1) {
        InvalidateRect(hWndMain, NULL, FALSE);
        return;
    }

    GetCoordinate(idx1, x, y);
    y1 = y - yPos;
    
    if (idx2 == -1) {
        y2 = g_crt.bottom;
    }
    else {
        GetCoordinate(idx2, x, y);
        y2 = y - yPos + LineHeight;
    }

    // 선택 영역은 작업 영역보다 커질 수 있다.
    // 따라서, 정확한 범위를 얻으려면 아래와 같이 범위를 점검해야 한다.
    // 단, InvalidateRect 함수가 클리핑 영역을 알아서 관리하므로 지금 코드에서는 그럴 필요가 없다.
    // y1 = max(y1, 0);
    // y2 = min(y2, g_crt.bottom);

    SetRect(&srt, 0, y1, g_crt.right, y2);
    InvalidateRect(hWndMain, &srt, FALSE);
}

Paragraph FindParagraphRange(int idx) {
    int paraStart, paraEnd;
    paraStart = paraEnd = idx;

    while (paraStart > 0 && !IsCRLF(paraStart)) { paraStart--; }
    while (paraEnd < docLength && !IsCRLF(paraEnd)) { paraEnd++; }
    if (paraStart > 0) { paraStart += 2; }

    Paragraph CurrentParagraph = { paraStart, paraEnd };
    return CurrentParagraph;
}

int FindParagraphStart(int idx) {
    int paraStart = idx;

    while (paraStart > 0 && !IsCRLF(paraStart)) { paraStart--; }
    if (paraStart > 0) { paraStart += 2; }

    return paraStart;
}

void SelectWord(int idx, int& start, int& end) {
    while (1) {
        if (IsDelims(idx) || idx == 0) { break; }
        idx--;
    }

    if (idx != 0 && idx != wcslen(buf) && IsDelims(idx + 1) == FALSE) {
        idx++;
    }
    start = idx;

    while (1) {
        if (IsDelims(idx) == TRUE) { break; }
        idx++;
    }
    end = idx;
}