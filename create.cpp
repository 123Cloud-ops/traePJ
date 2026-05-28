// SimplePaint.cpp
// 编译环境：Windows + MinGW/MSVC
// 编译命令（MinGW）：g++ SimplePaint.cpp -o SimplePaint.exe -lgdi32 -luser32 -mwindows

#define UNICODE
#include <windows.h>

// 全局变量
HINSTANCE hInst;
HWND hWnd;
bool isDrawing = false;
POINT prevPoint;

// 画笔颜色（可自行修改）
COLORREF penColor = RGB(0, 0, 0);   // 黑色
int penWidth = 2;                    // 画笔粗细

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN: {
        isDrawing = true;
        prevPoint.x = LOWORD(lParam);
        prevPoint.y = HIWORD(lParam);
        // 捕获鼠标，确保在窗口外松开也能收到消息
        SetCapture(hwnd);
        break;
    }
    case WM_MOUSEMOVE: {
        if (isDrawing) {
            HDC hdc = GetDC(hwnd);
            HPEN hPen = CreatePen(PS_SOLID, penWidth, penColor);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            // 移动到上一个点并画线到当前点
            MoveToEx(hdc, prevPoint.x, prevPoint.y, NULL);
            LineTo(hdc, LOWORD(lParam), HIWORD(lParam));

            // 更新上一个点
            prevPoint.x = LOWORD(lParam);
            prevPoint.y = HIWORD(lParam);

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            ReleaseDC(hwnd, hdc);
        }
        break;
    }
    case WM_LBUTTONUP: {
        if (isDrawing) {
            isDrawing = false;
            ReleaseCapture();
        }
        break;
    }
    case WM_KEYDOWN: {
        switch (wParam) {
        case 'C':  // 清屏
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case VK_ESCAPE:  // 退出
            DestroyWindow(hwnd);
            break;
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 用白色填充整个客户区（清屏时的背景色）
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_ERASEBKGND:
        // 返回非0值，表示已处理背景擦除，避免闪烁
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);   // 十字光标，更像绘图工具
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"SimplePaintClass";
    RegisterClassEx(&wc);

    // 创建窗口
    hWnd = CreateWindowEx(
        0,
        L"SimplePaintClass",
        L"简易画板 - 左键拖动绘图，C 清屏，ESC 退出",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
