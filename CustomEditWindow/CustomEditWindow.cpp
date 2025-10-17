#include "pch.h"
#include "CustomEditWindow.h"
#define CARET_WIDTH 2

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define GET_X_LPARAM(lParam) (int)(short)LOWORD(lParam)
#define GET_Y_LPARAM(lParam) (int)(short)HIWORD(lParam)

#define IDM_CUT         2001
#define IDM_COPY        2002
#define IDM_PASTE       2003
#define IDM_SELECTALL   2004

LRESULT CustomEditWindow::OnMessage(UINT iMessage, WPARAM wParam, LPARAM lParam) {
    for (DWORD i = 0; i < sizeof(MainMsg) / sizeof(MainMsg[0]); i++) {
        if (MainMsg[i].iMessage == iMessage) {
            return (this->*MainMsg[i].lpfnWndProc)(wParam, lParam);
        }
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

LRESULT CustomEditWindow::OnPaint(WPARAM wParam, LPARAM lParam) {
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
        BitBlt(hdc, 0, (Line - Top) * LineHeight, g_crt.right, LineHeight, hMemDC, 0, 0, SRCCOPY);
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
    // UpdateLayeredWindow(hWnd, hdc, &ptLocation, &szWnd, hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

    DeleteObject(hBrush);
    SelectObject(hMemDC, hOld);
    // DeleteObject(bmp);
    DeleteDC(hMemDC);

    EndPaint(hWnd, &ps);
	return 0;
}

LRESULT CustomEditWindow::OnTimer(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnSize(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnKeyDown(WPARAM wParam, LPARAM lParam) {
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
        else if (bCtrl) {
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

LRESULT CustomEditWindow::OnChar(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnImeChar(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnImeComposition(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnImeStartComposition(WPARAM wParam, LPARAM lParam) {
	return 0;
}

LRESULT CustomEditWindow::OnLButtonDblClk(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnLButtonDown(WPARAM wParam, LPARAM lParam) {
    int toff;
    int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

    SetFocus(hWnd);

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

LRESULT CustomEditWindow::OnMouseMove(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnLButtonUp(WPARAM wParam, LPARAM lParam) {
    bCapture = FALSE;
    ReleaseCapture();
    KillTimer(hWnd, 1);

    // 드래그 중 입력한 조립 문자를 취소한다.
    HIMC hImc = ImmGetContext(hWnd);
    ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmReleaseContext(hWnd, hImc);

    return 0;
}

LRESULT CustomEditWindow::OnSetFocus(WPARAM wParam, LPARAM lParam) {
    SetCaret(FALSE, FALSE);

    SelectFgColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
    SelectBgColor = GetSysColor(COLOR_HIGHLIGHT);

    if (HideType != 2 && SelectStart != SelectEnd) {
        InvalidateRect(hWnd, NULL, TRUE);
    }

	return 0;
}

LRESULT CustomEditWindow::OnKillFocus(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnMouseWheel(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnContextMenu(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnHScroll(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnVScroll(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
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

LRESULT CustomEditWindow::OnWindowPosChanged(WPARAM wParam, LPARAM lParam) {
    LPWINDOWPOS lpWndPos = (LPWINDOWPOS)lParam;
    g_wrt.left = lpWndPos->x;
    g_wrt.top = lpWndPos->y;
    g_wrt.right = lpWndPos->x + lpWndPos->cx;
    g_wrt.bottom = lpWndPos->y + lpWndPos->cy;

	return 0;
}

LRESULT CustomEditWindow::OnGetDlgCode(WPARAM wParam, LPARAM lParam) {
    LPMSG lpMsg = (LPMSG)lParam;
    if (lpMsg) {
        if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == '\t' && bWantTab == FALSE) { return 0; }
    }

    return DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTALLKEYS | DLGC_WANTCHARS;
}

LRESULT CustomEditWindow::OnCreate(WPARAM wParam, LPARAM lParam) {
    // InitializeGDIplus();
    SelectFgColor = RGB(0, 0, 0);
    SelectBgColor = RGB(0, 0, 0);

    Sum = 0;
    bWantTab = TRUE;
    bufLength = 0x400;
    docLength = 0;
    off = 0;
    bLineEnd = FALSE;
    bLineFirst = FALSE;
    bComp = FALSE;
    PrevX = 0;
    HangulCharWidth = 0;
    xMax = 0x400;
    yMax = 0x400;
    xPos = yPos = 0;
    SelectStart = SelectEnd = 0;
    bCapture = FALSE;
    HideType = 1;
    bAlphaNum = TRUE;

    buf = (WCHAR*)malloc(sizeof(WCHAR) * bufLength);
    if (buf != NULL) {
        memset(buf, 0, sizeof(WCHAR) * bufLength);
        wcscpy_s(buf, bufLength, L"아 디버깅 하는거 힘듭니다 별 문제는 없는거 같습니다.\r\n동해물과 백두산이 마르고 닳도록 하느님이 보우하사 우리나라 만세. 무궁화 삼천리 화려강산 대한사람 대한으로 길이 보전하세.\r\nabcdefghijklmnopqrstuvwxyz\r\nabcdefghijklmnopqrstuvwxyz\r\n");
        // wcscpy_s(buf, bufLength, L"동해물과 백두산이 마르고 닳도록 하느님이 보우하사\r\nabcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz\r\n우리나라 만세 무궁화 삼천리 화려 강산 대한 사람 대한으로 길이 보전하세");
        docLength = wcslen(buf);
        off = docLength;
    }
    else {
        return -1;
    }

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

    lineCount = 0;
    lineInfoSize = 0x400;
    lineInfo = (LineInfo*)malloc(sizeof(LineInfo) * lineInfoSize);
    if (lineInfo != NULL) {
        memset(lineInfo, 0, sizeof(lineInfo) * lineInfoSize);
    }
    else {
        if (buf) { free(buf); }
        return -1;
    }
    
    RebuildLineInfo();
    UpdateScrollInfo();

	return 0;
}

LRESULT CustomEditWindow::OnDestroy(WPARAM wParam, LPARAM lParam) {
	if (buf) { free(buf); }
	if (lineInfo) { free(lineInfo); }
	if (hBitmap) {
		// delete hBitmap;
		DeleteObject(hBitmap);
	}

	// PostQuitMessage(0);
	return 0;
}


void CustomEditWindow::SetCaret(BOOL bUpdatePrevX, BOOL bScrollToCaret) {
    SIZE sz;
    int toff, x, y;
    int caretWidth;

    toff = bComp ? off - 2 : off;
    caretWidth = bComp ? GetCharWidth(buf + toff, 1) : CARET_WIDTH;

    CreateCaret(hWnd, NULL, caretWidth, FontHeight);
    ShowCaret(hWnd);

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
            SetScrollPos(hWnd, SB_HORZ, xPos, TRUE);
            SetScrollPos(hWnd, SB_VERT, yPos, TRUE);
            InvalidateRect(hWnd, NULL, TRUE);
        }
    }

    SetCaretPos(x - xPos, y - yPos);

    if (bUpdatePrevX != FALSE) {
        PrevX = x;
    }
}

void CustomEditWindow::PrecomputeCharWidths() {
    SIZE sz;
    HDC hdc = GetDC(hWnd);

    WCHAR ch[2] = { 0, };
    for (int i = 1; i < 128; i++) {
        ch[0] = (WCHAR)i;
        GetTextExtentPoint32(hdc, ch, 1, &sz);
        AsciiCharWidth[i] = sz.cx;
    }
    AsciiCharWidth[0] = AsciiCharWidth[32];

    GetTextExtentPoint32(hdc, Sample, 1, &sz);
    HangulCharWidth = sz.cx;

    ReleaseDC(hWnd, hdc);
}

int CustomEditWindow::GetCharWidth(WCHAR* src, int length) {
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

BOOL CustomEditWindow::Insert(int idx, WCHAR* str) {
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

BOOL CustomEditWindow::Delete(int idx, int cnt) {
    if (docLength < idx + cnt) { return FALSE; }

    int move = docLength - idx - cnt + 1;
    memmove(buf + idx, buf + idx + cnt, move * sizeof(WCHAR));
    docLength -= cnt;

    RebuildLineInfo(idx, -cnt);
    UpdateScrollInfo();
    return TRUE;
}

BOOL CustomEditWindow::IsCRLF(int idx) {
    if (buf[idx] == '\r' && buf[idx + 1] == '\n') {
        return TRUE;
    }
    return FALSE;
}

int CustomEditWindow::GetPrevOffset(int idx) {
    if (idx <= 0) { return 0; }
    if (IsCRLF(idx - 2)) { return idx - 2; }
    return idx - 1;
}

int CustomEditWindow::GetNextOffset(int idx) {
    if (docLength < idx) { return docLength; }
    if (IsCRLF(idx)) { return idx + 2; }
    return idx + 1;
}

void CustomEditWindow::GetLine(int line, int& start, int& end) {
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
void CustomEditWindow::GetRowAndColumn(int idx, int& row, int& column) {
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

int CustomEditWindow::GetOffset(int row, int column) {
    int start = lineInfo[row].start;
    int end = lineInfo[row].end;
    column = min(column, end - start);
    return column + start;
}

void CustomEditWindow::GetCoordinate(int idx, int& x, int& y) {
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

BOOL CustomEditWindow::IsWhiteChar(WCHAR ch) {
    return ch == L' ' ||
        ch == L'\r' ||
        ch == L'\n' ||
        ch == L'\t';
}

BOOL CustomEditWindow::IsAlnumChar(WCHAR ch) {
    return (ch >= L'0' && ch <= L'9') ||
        (ch >= L'A' && ch <= L'Z') ||
        (ch >= L'a' && ch <= L'z');
}

BOOL CustomEditWindow::IsPunctChar(WCHAR ch) {
    return (ch >= 0x21 && ch <= 0x2F) ||    // ! " # $ % & ' ( ) * + , - . /
        (ch >= 0x3A && ch <= 0x40) ||        // : ; < = > ? @ 
        (ch >= 0x5B && ch <= 0x60) ||       // [ \ ] ^ _ \ `
        (ch >= 0x7B && ch <= 0x7E);         // { | } ~
}

BOOL CustomEditWindow::IsKJCChar(WCHAR ch) {
    return (ch >= 0xAC00 && ch <= 0xD7AF) ||    /* 한글 */
        (ch >= 0x3040 && ch <= 0x30FF) ||       /* 일본어 */
        (ch >= 0x4E00 && ch <= 0x9FFF);         /* 중국어  */
}

enum CustomEditWindow::CustomCharset CustomEditWindow::GetCustomCharset(WCHAR ch) {
    if (IsWhiteChar(ch)) { return (enum CustomCharset)CC_WHITE; }
    if (IsAlnumChar(ch)) { return (enum CustomCharset)CC_ALNUM; }
    if (IsPunctChar(ch)) { return (enum CustomCharset)CC_PUNCT; }
    if (IsKJCChar(ch)) { return (enum CustomCharset)CC_KJC; }
    return (enum CustomCharset)CC_OTHER;
}

int CustomEditWindow::FindWrapPoint(int start, int end) {
    if (start >= end) { return start; }

    WCHAR* ptr = buf;

    int left = start + 1, right = end, fit = left;
    int maxWidth = g_crt.right - g_crt.left - CARET_WIDTH;

    // if (maxWidth < FontHeight * 4) { return; }

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

int CustomEditWindow::WordBreakProc(int pos, int start, WBPType type) {
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

void CustomEditWindow::RebuildLineInfo(int idx /* = -1 */, int length /* = -1 */) {
    int curLine = 0, pos = 0, wrapEnd = 0;
    int start = 0, end = 0;
    int paraStart = 0;
    int prevStart = -1, prevEnd = -1;
    WCHAR* ptr = buf;

    BOOL bAll = (idx == -1);

    if (!bAll) {
        paraStart = FindParagraphStart(idx);
        if (paraStart > 0) {
            GetRowAndColumn(paraStart - 2, curLine, end);
            curLine++;
        }
    }

    while (1) {
        if (curLine >= lineInfoSize) {
            // lineCount -> lineInfoSize : 오타 수정
            lineInfoSize += 0x400;
            lineInfo = (LineInfo*)realloc(lineInfo, sizeof(LineInfo) * lineInfoSize);
            if (lineInfo == NULL) { return; }
            memset(lineInfo + (lineInfoSize - 0x400), -1, sizeof(LineInfo) * 0x400);
        }
        GetLine(curLine, start, end);

        // 앞에서 문단의 시작 오프셋을 반환하면 이 오프셋을 GetLine으로 넘겨 문단의 첫 줄부터 start, end 오프셋을 가져온다.
        // idx < end 조건은 앞 줄을 건너뛰고 현재 줄을 찾는 조건식이다.
        // 편집이 발생한 idx 값보다 end 값이 작은 경우에는 굳이 볼 필요 없는 줄(전방 줄)이므로 그냥 건너뛴다.
        // 이때, idx < end 조건은 또 하나의 처리를 하는데 start와 end가 -1로 초기화되는 경우, 즉 문서의 끝(마지막 줄)인 경우도 자연스럽게 처리한다.

        // lineInfo[curLine].start != -1 조건식은 realloc에 의해 새로 할당된 줄을 간소화 범위에서 제외한다.
        // 새로 할당된 줄은 정렬되지 않은 상태인데 이를 간소화한다는건 말이 안된다.
        // 간소화 범위에서 제외하기 위해선 특이값이 필요하므로 memset 호출문의 두 번째 인수를 0에서 -1로 변경했다.

        if (!bAll && idx < end && lineInfo[curLine].start != -1) {
            // 1번 패턴 : 한 줄 내에서 편집이 발생한 경우
            // 현재 줄은 건너뛰고 다음 줄부터 length만큼 오프셋 증감
            int i = 0;
            if (lineInfo[curLine].start + length == start && lineInfo[curLine].end + length == end) {
                i = curLine;
                while (lineInfo[i].start != -1) {
                    lineInfo[i].start += length;
                    lineInfo[i].end += length;
                    i++;
                }
                break;
            }

            // 3번 패턴 : 삭제로 인해 전체 줄 개수가 줄어든 경우
            // 글로 쓰려니 좀 복잡한데, 이전에 만들어둔 줄 정보에서 편집되어 변화가 생긴 줄(curLine)의 뒷줄 정보(curLine + 1)를 가져온다.
            // 이때 뒷줄 정보와 GetLine으로 가져온 start, end 값, 즉 현재 정렬한 결과가 서로 같다면 중간에 있던 한 줄이 사라진 것이다.

            // 그림을 다시 보면서 되짚어보자.
            //                                                                                            ↓문자 삭제("■■■ ")
            // "■■ ■■■■ ■■■ ■■ ■■■■■ ■■ ■■■■ "       WRAP    ->      "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"   CRLF     (0, 29  -> 0, 29)
            // "■■■■"                                                  CRLF    ->                                                                        (29, 33 -> NULL)
            // "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"      CRLF    ->      "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"   CRLF     (33, 62 -> 29, 58) ↑ 이 줄이 위로 올라가고 curLine인 상태

            // 위 그림에서 curLine은 오른쪽 그림의 두 번째 줄이다.
            // 즉, 편집이 발생하여 변화가 생긴 줄에서 다음 예외 분기를 만족하고 아래 구문이 실행된다.
            // 이전 줄 정보에서 curLine + 1을 참조하면 왼쪽 그림에서 세 번째 줄의 정보(33, 62)를 가져온다.
            // 이 값에 삭제된 문자("■■■ ")의 개수 4만큼 빼면 변화가 생긴 줄 curLine(29, 58)의 정보와 완전히 동일하다는 것을 알 수 있다.
            // 이 줄을 찾은 다음에는 이후의 줄에 똑같이 length만큼 오프셋 증감을 적용하면 간소화가 완료된다.
            if (lineInfo[curLine + 1].start + length == start && lineInfo[curLine + 1].end + length == end) {
                i = curLine;
                while (1) {
                    if (lineInfo[i + 1].start == -1) {
                        lineInfo[i].start = -1;
                        break;
                    }

                    lineInfo[i].start = lineInfo[i + 1].start + length;
                    lineInfo[i].end = lineInfo[i + 1].end + length;
                    i++;
                }

                lineCount -= 1;
                break;
            }

            // 마지막으로 2번 패턴인데 이건 좀 까다롭다.
            // 이전 줄 정보에서 curLine - 1, 즉 앞 줄 정보를 가져와야 하는데 이 정보가 파괴되기 때문이다.
            // 다행히 딱 한 줄의 정보만 필요하기 때문에 임시 변수 한쌍으로 해결할 수 있다.

            // 다시 예시를 보자.                                                                                              ↓ 문자 삽입("■■■ ")
            // "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"                          CRLF    ->      "■■ ■■■■ ■■■ ■■ ■■■■■ ■■ ■■■■ "    WRAP     (0, 29  -> 0, 29)
            //                                                                                         ->      "■■■■"                                               CRLF     (NULL   -> 29, 33)
            // "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■" ↑원래 2번줄(29, 58)     CRLF    ->      "■■ ■■■■ ■■ ■■■■■ ■■ ■■■■ ■■■■"   CRLF     (29, 58 -> 33, 62) 

            // 우선 순서를 파악하는게 중요하다. 앞에 있는 예외 분기는 조건이 명확하여 2번 패턴의 분기를 작성할 때는 신경쓸 필요가 없다.
            // 즉, 현재 상황에만 집중하란 얘기다.

            // curLine부터 명확히 하자.
            // 아래 분기가 시작되는 시점에 curLine은 오른쪽 그림의 세 번째 줄이다.

            // 이때 이전 줄 정보에서 curLine - 1 행의 줄 정보를 조사하면 왼쪽 그림에서 두 번째 줄의 정보(29, 58)를 가져온다.
            // 헷갈릴 수 있는데, 왼쪽 그림은 사실 줄이 두 개뿐인 것이지만 설명을 위해 빈 줄(NULL)을 추가하여 물리적인 줄이 있는 것처럼 그려놨다.
            // 3번 패턴에서는 줄이 있다가 사라진거라 어색하지 않았는데 2번 패턴은 원래 없던 줄을 있는 것처럼 끼워놓은 상태라 어색한 부분이 있다.

            // 이전 줄 정보에서 curLine - 1 행의 값을 가져오려고 보니, 직전의 루프에서 줄 정보가 갱신되어 버렸다.
            // 즉, 값이 파괴되어 조사할 수 없는 상태인 것이다.
            // 이러한 이유로 prevStart와 prevEnd 임시 변수가 필요해졌으며 줄 정보를 갱신하는 코드 직전에 대입문을 작성해뒀다.

            // 위 예시에서 prevStart와 prevEnd는 각각 29, 58의 값을 가진다.
            // 여기에 삽입된 문자("■■■ ") 길이만큼 더하면 현재 조사된 줄 정보 start, end와 같다는 것을 알 수 있다. 
            // 곧, 이전 줄 정보(29, 58)에 삽입된 문자열의 길이(4)만큼 더했을 때
            // 현재 조사한 줄 정보(33, 62)와 그 값이 같다면 중간에 줄이 추가된 것이다.

            if (prevStart + length == start && prevEnd + length == end) {
                // 
                if (lineCount + 1 >= lineInfoSize) {
                    lineInfoSize += 0x400;
                    lineInfo = (LineInfo*)realloc(lineInfo, sizeof(LineInfo) * lineInfoSize);
                    if (lineInfo != NULL) {
                        memset(lineInfo + (lineInfoSize - 0x400), -1, sizeof(LineInfo) * 0x400);
                    }
                }

                // lineCount는 문서에 작성되어 있는 마지막 줄의 번호이자 줄의 개수이다.
                // lineCount + 1을 -1로 초기화 하여 문서의 마지막임을 알린다.
                // 이렇게 해야 분기문을 피할 수 있다.

                // 그다음 마지막 줄부터 현재 편집되고 있는 줄까지 뒤에서부터 차례대로 정보를 갱신하는데
                // 앞에서부터 차례대로 정보를 갱신하면 어떤 일이 벌어질지 설명안해도 알 것이다.
                lineInfo[lineCount + 1].start = -1;
                for (i = lineCount; i >= curLine; i--) {
                    lineInfo[i].start = lineInfo[i - 1].start + length;
                    lineInfo[i].end = lineInfo[i - 1].end + length;
                }

                lineInfo[curLine].start = start;
                lineInfo[curLine].end = end;
                lineCount += 1;
                break;
            }
        }

        prevStart = lineInfo[curLine].start;
        prevEnd = lineInfo[curLine].end;

        lineInfo[curLine].start = start;
        lineInfo[curLine].end = end;

        if (start == -1) { 
            lineCount = curLine;
            break;
        }

        curLine++;
    }
}

int CustomEditWindow::GetDocsXPosOnLine(int row, int dest) {
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

void CustomEditWindow::TraceFormat(LPCWSTR format, ...)
{
    WCHAR buffer[512];
    va_list args;
    va_start(args, format);
    vswprintf(buffer, 512, format, args);
    va_end(args);
    OutputDebugString(buffer);
}

void CustomEditWindow::UpdateScrollInfo() {
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
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

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
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
    }
}

int CustomEditWindow::DrawLine(HDC hdc, int line) {
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

        if (bInSelect && (GetFocus() == hWnd || HideType != 0)) {
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
void CustomEditWindow::DrawSegment(HDC hdc, int& x, int y, int idx, int length, BOOL ignore, COLORREF fg, COLORREF bg) {
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

int CustomEditWindow::GetOffsetFromPoint(int x, int y) {
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

int CustomEditWindow::GetOffsetFromPoint(POINT Mouse) {
    return GetOffsetFromPoint(Mouse.x, Mouse.y);
}

BOOL CustomEditWindow::IsDelims(int idx) {
    static WCHAR delims[] = L" \t\r\n\"\'\\,.<>:;/(){}[]~!@#$%^&*-+?=";
    return (wcsrchr(delims, buf[idx]) || buf[idx] == 0);
}

int CustomEditWindow::GetPrevWord(int idx) {
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

int CustomEditWindow::GetNextWord(int idx) {
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

void CustomEditWindow::ClearSelection() {
    if (SelectStart != SelectEnd) {
        Invalidate(min(SelectStart, SelectEnd), max(SelectStart, SelectEnd));
        SelectStart = SelectEnd = 0;
    }
}

void CustomEditWindow::ExpandSelection(int start, int end) {
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

    InvalidateRect(hWnd, NULL, TRUE);
}

BOOL CustomEditWindow::DeleteSelection() {
    int SelectFirst, SelectSecond;

    if (SelectStart != SelectEnd) {
        SelectFirst = min(SelectStart, SelectEnd);
        SelectSecond = max(SelectStart, SelectEnd);
        Delete(SelectFirst, SelectSecond - SelectFirst);
        SelectStart = SelectEnd = 0;
        off = SelectFirst;
        return TRUE;
    }

    return FALSE;
}

void CustomEditWindow::Invalidate(int idx1, int idx2 /* = -1 */) {
    // 문서를 편집하는 동작의 함수들을 보면 작업 단위가 오프셋이다.
    // 따라서 그리기 최적화 함수 역시 오프셋을 받아 사용하기로 한다.
    // 다시 그릴 영역을 직접 지정하면 되는데 간단한 계산식으로 충분히 만들 수 있다.
    // 오프셋으로부터 픽셀 좌표를 계산하고 스크롤 값을 빼 화면상의 좌표를 얻어온다

    RECT srt;
    int x, y, y1, y2;
    if (idx1 == -1) {
        InvalidateRect(hWnd, NULL, FALSE);
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
    InvalidateRect(hWnd, &srt, FALSE);
}

int CustomEditWindow::FindParagraphStart(int idx) {
    int paraStart = idx;

    while (paraStart > 0 && !IsCRLF(paraStart)) { paraStart--; }
    if (paraStart > 0) { paraStart += 2; }

    return paraStart;
}

void CustomEditWindow::SelectWord(int idx, int& start, int& end) {
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

