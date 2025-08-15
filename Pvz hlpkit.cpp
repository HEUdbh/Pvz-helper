// Pvz hlpkit.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "Pvz hlpkit.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 植物大战僵尸相关变量
DWORD pvzProcessId = 0;                         // 植物大战僵尸进程ID
HANDLE pvzProcessHandle = NULL;                 // 植物大战僵尸进程句柄
DWORD_PTR pvzBaseAddress = 0;                   // 植物大战僵尸基地址

// 阳光修改相关变量
DWORD_PTR sunlightBaseAddress = 0;             // 阳光基地址（动态获取）
DWORD sunlightOffset1 = 0x768;                 // 第一层偏移
DWORD sunlightOffset2 = 0x5560;                // 第二层偏移
DWORD currentSunlight = 0;                     // 当前阳光值
DWORD targetSunlight = 9999;                   // 目标阳光值
BOOL sunlightModifyEnabled = FALSE;            // 阳光修改是否启用
HWND hCheckSunlight; // 阳光相关控件句柄

// 金币修改相关变量
DWORD coinOffset1 = 0x82C;                     // 第一层偏移
DWORD coinOffset2 = 0x28;                      // 第二层偏移
DWORD currentCoin = 0;                         // 当前金币值
DWORD targetCoin = 99999;                      // 目标金币值
BOOL coinModifyEnabled = FALSE;                // 金币修改是否启用
HWND hCheckCoin; // 金币相关控件句柄

// 预留功能变量 (为未来扩展准备)
BOOL autoRefreshEnabled = FALSE;               // 自动刷新功能
BOOL unlimitedSunlightEnabled = FALSE;         // 无限阳光功能
BOOL godModeEnabled = FALSE;                   // 无敌模式功能
BOOL fastPlantEnabled = FALSE;                 // 快速种植功能

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 植物大战僵尸相关函数
BOOL                ScanPvzProcess();
void                CreateControls(HWND hWnd);
void                ListAllProcesses(); // 列出所有进程用于调试

// 阳光修改相关函数
DWORD_PTR           CalculateSunlightAddress();
BOOL                WriteSunlightValue(DWORD newValue);
void                EnableSunlightModify(BOOL enable);

// 金币修改相关函数
DWORD_PTR           CalculateCoinAddress();
BOOL                WriteCoinValue(DWORD newValue);
void                EnableCoinModify(BOOL enable);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PVZHLPKIT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PVZHLPKIT));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PVZHLPKIT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PVZHLPKIT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

       HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, 0, 450, 250, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // 创建控件
   CreateControls(hWnd);

               // 扫描植物大战僵尸进程
        if (!ScanPvzProcess())
        {
            MessageBox(hWnd, L"未找到PlantsVsZombies.exe进程！\n请先启动植物大战僵尸游戏，然后重新运行本程序。", 
                       L"连接失败", MB_OK | MB_ICONWARNING);
        }

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                // 处理自定义控件消息
                if (LOWORD(wParam) == 1003) // 阳光修改勾选框
                {
                    // 获取勾选框当前状态
                    BOOL checked = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    sunlightModifyEnabled = checked;
                    EnableSunlightModify(sunlightModifyEnabled);
                    
                    if (sunlightModifyEnabled)
                    {
                        // 功能已启用，自动修改阳光为9999
                        if (pvzProcessHandle != NULL)
                        {
                            DWORD newSunlight = 9999;
                            if (WriteSunlightValue(newSunlight))
                            {
                                MessageBox(hWnd, L"阳光修改为9999！", L"修改成功", MB_OK | MB_ICONINFORMATION);
                            }
                            else
                            {
                                MessageBox(hWnd, L"阳光修改失败！请确保游戏正在运行。", L"修改失败", MB_OK | MB_ICONERROR);
                            }
                        }
                        else
                        {
                            MessageBox(hWnd, L"未连接到游戏进程！请先启动植物大战僵尸游戏。", L"连接失败", MB_OK | MB_ICONWARNING);
                        }
                    }
                }
                else if (LOWORD(wParam) == 1005) // 金币修改勾选框
                {
                    // 获取勾选框当前状态
                    BOOL checked = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    coinModifyEnabled = checked;
                    EnableCoinModify(coinModifyEnabled);
                    
                    if (coinModifyEnabled)
                    {
                        // 功能已启用，自动修改金币为99999
                        if (pvzProcessHandle != NULL)
                        {
                            DWORD newCoin = 99999;
                            if (WriteCoinValue(newCoin))
                            {
                                MessageBox(hWnd, L"金币修改为99999！", L"修改成功", MB_OK | MB_ICONINFORMATION);
                            }
                            else
                            {
                                MessageBox(hWnd, L"金币修改失败！请确保游戏正在运行。", L"修改失败", MB_OK | MB_ICONERROR);
                            }
                        }
                        else
                        {
                            MessageBox(hWnd, L"未连接到游戏进程！请先启动植物大战僵尸游戏。", L"连接失败", MB_OK | MB_ICONWARNING);
                        }
                    }
                }
                else if (LOWORD(wParam) == 1004) // 手动刷新按钮
                {
                    // 先列出所有进程用于调试
                    ListAllProcesses();
                    
                    // 重新扫描植物大战僵尸进程
                    if (!ScanPvzProcess())
                    {
                        MessageBox(hWnd, L"未找到植物大战僵尸进程！\n请先启动植物大战僵尸游戏，然后点击此按钮重试。\n\n已列出所有进程到调试输出，请检查进程名称。", 
                                   L"连接失败", MB_OK | MB_ICONWARNING);
                    }
                    else
                    {
                        MessageBox(hWnd, L"植物大战僵尸进程连接成功！", L"连接成功", MB_OK | MB_ICONINFORMATION);
                    }
                }
                else
                {
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        if (pvzProcessHandle)
        {
            CloseHandle(pvzProcessHandle);
        }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// 扫描植物大战僵尸进程
// 修复扫描进程函数
BOOL ScanPvzProcess()
{
    // 打印开始扫描信息
    OutputDebugStringW(L"开始扫描植物大战僵尸进程...\n");
    wprintf(L"开始扫描植物大战僵尸进程...\n");
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"创建进程快照失败! 错误代码: %lu", error);
        OutputDebugStringW(errorMsg);
        wprintf(L"%s\n", errorMsg);
        return FALSE;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    BOOL found = FALSE;
    DWORD processCount = 0;
    
    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            processCount++;
            
            // 支持多种可能的进程名称
            if (_wcsicmp(pe32.szExeFile, L"PlantsVsZombies.exe") == 0 ||
                _wcsicmp(pe32.szExeFile, L"Plants vs Zombies.exe") == 0 ||
                _wcsicmp(pe32.szExeFile, L"PlantsVsZombies") == 0 ||
                _wcsicmp(pe32.szExeFile, L"pvz.exe") == 0 ||
                _wcsicmp(pe32.szExeFile, L"pvz") == 0)
            {
                pvzProcessId = pe32.th32ProcessID;
                found = TRUE;
                
                wchar_t foundMsg[256];
                swprintf_s(foundMsg, L"找到植物大战僵尸进程! 名称: %s, 进程ID: %lu", 
                           pe32.szExeFile, pvzProcessId);
                OutputDebugStringW(foundMsg);
                wprintf(L"%s\n", foundMsg);
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    else
    {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"枚举进程失败! 错误代码: %lu", error);
        OutputDebugStringW(errorMsg);
        wprintf(L"%s\n", errorMsg);
    }

    CloseHandle(hSnapshot);

    if (!found)
    {
        wchar_t notFoundMsg[256];
        swprintf_s(notFoundMsg, L"未找到植物大战僵尸进程! 已扫描 %lu 个进程", processCount);
        OutputDebugStringW(notFoundMsg);
        wprintf(L"%s\n", notFoundMsg);
        
        // 列出所有进程名称以便调试
        OutputDebugStringW(L"当前运行的进程列表:");
        wprintf(L"当前运行的进程列表:\n");
        
        HANDLE hSnapshot2 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot2 != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32 pe32_2;
            pe32_2.dwSize = sizeof(PROCESSENTRY32);
            
            if (Process32First(hSnapshot2, &pe32_2))
            {
                do
                {
                    wchar_t processInfo[128];
                    swprintf_s(processInfo, L"进程: %s (ID: %lu)", pe32_2.szExeFile, pe32_2.th32ProcessID);
                    OutputDebugStringW(processInfo);
                    wprintf(L"%s\n", processInfo);
                } while (Process32Next(hSnapshot2, &pe32_2));
            }
            CloseHandle(hSnapshot2);
        }
        
        return FALSE;
    }

    // 打开进程句柄
    pvzProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pvzProcessId);
    if (pvzProcessHandle == NULL)
    {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"打开进程句柄失败! 进程ID: %lu, 错误代码: %lu", pvzProcessId, error);
        OutputDebugStringW(errorMsg);
        wprintf(L"%s\n", errorMsg);
        return FALSE;
    }

    // 获取进程基地址 - 修复64位兼容问题
    HMODULE hModule;
    DWORD cbNeeded;
    if (EnumProcessModules(pvzProcessHandle, &hModule, sizeof(hModule), &cbNeeded))
    {
        // 获取第一个模块（主模块）
        if (cbNeeded > 0)
        {
            pvzBaseAddress = (DWORD_PTR)hModule;
            
            // 直接使用默认偏移，避免栈溢出
            sunlightBaseAddress = 0x2A9EC0; // 默认偏移 (基于实际扫描)
            
            // 打印进程连接成功信息
            wchar_t connectInfo[256];
            swprintf_s(connectInfo, L"植物大战僵尸进程连接成功!\n进程ID: %lu\n基地址: 0x%p\n阳光偏移: 0x%X", 
                       pvzProcessId, (void*)pvzBaseAddress, sunlightBaseAddress);
            OutputDebugStringW(connectInfo);
            wprintf(L"%s\n", connectInfo);
            
            return TRUE;
        }
        else
        {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"获取进程模块失败! cbNeeded: %lu", cbNeeded);
            OutputDebugStringW(errorMsg);
            wprintf(L"%s\n", errorMsg);
            
            CloseHandle(pvzProcessHandle);
            pvzProcessHandle = NULL;
            return FALSE;
        }
    }
    else
    {
        DWORD err = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"枚举进程模块失败! 错误代码: %lu", err);
        OutputDebugStringW(errorMsg);
        wprintf(L"%s\n", errorMsg);
        
        CloseHandle(pvzProcessHandle);
        pvzProcessHandle = NULL;
        return FALSE;
    }

    return TRUE;
}

// 列出所有进程用于调试
void ListAllProcesses()
{
    OutputDebugStringW(L"=== 当前系统运行的所有进程 ===\n");
    wprintf(L"=== 当前系统运行的所有进程 ===\n");
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringW(L"创建进程快照失败!\n");
        wprintf(L"创建进程快照失败!\n");
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    DWORD processCount = 0;
    
    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            processCount++;
            wchar_t processInfo[256];
            swprintf_s(processInfo, L"%4lu: %s (PID: %lu)", 
                       processCount, pe32.szExeFile, pe32.th32ProcessID);
            OutputDebugStringW(processInfo);
            wprintf(L"%s\n", processInfo);
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    wchar_t summary[128];
    swprintf_s(summary, L"总共找到 %lu 个进程\n", processCount);
    OutputDebugStringW(summary);
    wprintf(L"%s\n", summary);
    
    CloseHandle(hSnapshot);
}

// 修复计算阳光地址函数
DWORD_PTR CalculateSunlightAddress()
{
    if (pvzProcessHandle == NULL || pvzBaseAddress == 0)
    {
        return 0;
    }

    DWORD_PTR finalAddress = 0;
    SIZE_T bytesRead;

    // 使用动态模块基址 + 相对偏移
    DWORD_PTR ptr1 = pvzBaseAddress + sunlightBaseAddress;
    if (!ReadProcessMemory(pvzProcessHandle, (LPCVOID)ptr1, &finalAddress, sizeof(DWORD_PTR), &bytesRead) ||
        bytesRead != sizeof(DWORD_PTR))
    {
        return 0;
    }

    // 检查地址有效性
    if (finalAddress == 0 || finalAddress < pvzBaseAddress)
    {
        return 0;
    }

    // 加上第一层偏移
    finalAddress += sunlightOffset1;

    // 读取第二层指针
    DWORD_PTR ptr2 = 0;
    if (!ReadProcessMemory(pvzProcessHandle, (LPCVOID)finalAddress, &ptr2, sizeof(DWORD_PTR), &bytesRead) ||
        bytesRead != sizeof(DWORD_PTR))
    {
        return 0;
    }

    // 检查地址有效性
    if (ptr2 == 0 || ptr2 < pvzBaseAddress)
    {
        return 0;
    }

    // 加上第二层偏移
    finalAddress = ptr2 + sunlightOffset2;

    // 最终地址验证
    if (finalAddress == 0 || finalAddress < pvzBaseAddress)
    {
        return 0;
    }

    // 打印阳光存储地址信息
    wchar_t debugInfo[256];
    swprintf_s(debugInfo, L"阳光存储地址信息:\n基地址: 0x%p\n第一层指针: 0x%p\n第二层指针: 0x%p\n最终地址: 0x%p", 
               (void*)pvzBaseAddress, (void*)(pvzBaseAddress + sunlightBaseAddress), (void*)(ptr2), (void*)finalAddress);
    OutputDebugStringW(debugInfo);
    
    // 同时输出到控制台（如果存在）
    wprintf(L"%s\n", debugInfo);

    return finalAddress;
}

// 计算金币地址函数
DWORD_PTR CalculateCoinAddress()
{
    if (pvzProcessHandle == NULL || pvzBaseAddress == 0)
    {
        return 0;
    }

    DWORD_PTR finalAddress = 0;
    SIZE_T bytesRead;

    // 使用动态模块基址 + 相对偏移（与阳光相同）
    DWORD_PTR ptr1 = pvzBaseAddress + sunlightBaseAddress;
    if (!ReadProcessMemory(pvzProcessHandle, (LPCVOID)ptr1, &finalAddress, sizeof(DWORD_PTR), &bytesRead) ||
        bytesRead != sizeof(DWORD_PTR))
    {
        return 0;
    }

    // 检查地址有效性
    if (finalAddress == 0 || finalAddress < pvzBaseAddress)
    {
        return 0;
    }

    // 加上第一层偏移（金币专用）
    finalAddress += coinOffset1;

    // 读取第二层指针
    DWORD_PTR ptr2 = 0;
    if (!ReadProcessMemory(pvzProcessHandle, (LPCVOID)finalAddress, &ptr2, sizeof(DWORD_PTR), &bytesRead) ||
        bytesRead != sizeof(DWORD_PTR))
    {
        return 0;
    }

    // 检查地址有效性
    if (ptr2 == 0 || ptr2 < pvzBaseAddress)
    {
        return 0;
    }

    // 加上第二层偏移（金币专用）
    finalAddress = ptr2 + coinOffset2;

    // 最终地址验证
    if (finalAddress == 0 || finalAddress < pvzBaseAddress)
    {
        return 0;
    }

    // 打印金币存储地址信息
    wchar_t debugInfo[256];
    swprintf_s(debugInfo, L"金币存储地址信息:\n基地址: 0x%p\n第一层指针: 0x%p\n第二层指针: 0x%p\n最终地址: 0x%p", 
               (void*)pvzBaseAddress, (void*)(pvzBaseAddress + sunlightBaseAddress), (void*)(ptr2), (void*)finalAddress);
    OutputDebugStringW(debugInfo);
    
    // 同时输出到控制台（如果存在）
    wprintf(L"%s\n", debugInfo);

    return finalAddress;
}



// 创建控件
void CreateControls(HWND hWnd)
{
    // 创建主标题
    CreateWindow(L"STATIC", L"植物大战僵尸 - 游戏修改器", 
                WS_VISIBLE | WS_CHILD | SS_CENTER, 
                20, 20, 390, 35, hWnd, NULL, hInst, NULL);

    // 创建分隔线1
    CreateWindow(L"STATIC", L"", 
                WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ, 
                20, 65, 390, 2, hWnd, NULL, hInst, NULL);

    // 创建功能区域标题
    CreateWindow(L"STATIC", L"阳光修改功能", 
                WS_VISIBLE | WS_CHILD | SS_LEFT, 
                20, 80, 150, 25, hWnd, NULL, hInst, NULL);

    // 创建阳光修改勾选框
    hCheckSunlight = CreateWindow(L"BUTTON", L"修改阳光为9999", 
                                 WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                                 20, 110, 250, 25, hWnd, (HMENU)1003, hInst, NULL);

    // 创建金币修改功能标题
    CreateWindow(L"STATIC", L"金币修改功能", 
                WS_VISIBLE | WS_CHILD | SS_LEFT, 
                20, 140, 150, 25, hWnd, NULL, hInst, NULL);

    // 创建金币修改勾选框
    hCheckCoin = CreateWindow(L"BUTTON", L"修改金币为99999", 
                             WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                             20, 170, 250, 25, hWnd, (HMENU)1005, hInst, NULL);

    // 创建手动刷新按钮
    CreateWindow(L"BUTTON", L"手动刷新", 
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 
                20, 205, 120, 25, hWnd, (HMENU)1004, hInst, NULL);

    // 创建版本信息
    CreateWindow(L"STATIC", L"版本 1.0 | 植物大战僵尸游戏修改器", 
                WS_VISIBLE | WS_CHILD | SS_RIGHT, 
                20, 210, 390, 20, hWnd, NULL, hInst, NULL);

    // 不设置字体，使用系统默认字体避免TextShaping.dll栈溢出
}




// 写入阳光值
BOOL WriteSunlightValue(DWORD newValue)
{
    if (pvzProcessHandle == NULL)
    {
        return FALSE;
    }

    DWORD_PTR sunlightAddress = CalculateSunlightAddress();
    if (sunlightAddress == 0)
    {
        return FALSE;
    }

    // 打印写入阳光值的地址信息
    wchar_t writeInfo[128];
    swprintf_s(writeInfo, L"正在写入阳光值 %d 到地址: 0x%p", newValue, (void*)sunlightAddress);
    OutputDebugStringW(writeInfo);
    wprintf(L"%s\n", writeInfo);

    SIZE_T bytesWritten;
    if (WriteProcessMemory(pvzProcessHandle, (LPVOID)sunlightAddress, &newValue, sizeof(DWORD), &bytesWritten))
    {
        currentSunlight = newValue;
        
        // 打印写入成功信息
        wchar_t successInfo[128];
        swprintf_s(successInfo, L"阳光值写入成功! 新值: %d, 写入字节数: %zu", newValue, bytesWritten);
        OutputDebugStringW(successInfo);
        wprintf(L"%s\n", successInfo);
        
        return (bytesWritten == sizeof(DWORD));
    }
    
    // 打印写入失败信息
    wchar_t failInfo[128];
    swprintf_s(failInfo, L"阳光值写入失败! 目标地址: 0x%p, 新值: %d", (void*)sunlightAddress, newValue);
    OutputDebugStringW(failInfo);
    wprintf(L"%s\n", failInfo);
    
    return FALSE;
}

// 写入金币值
BOOL WriteCoinValue(DWORD newValue)
{
    if (pvzProcessHandle == NULL)
    {
        return FALSE;
    }

    DWORD_PTR coinAddress = CalculateCoinAddress();
    if (coinAddress == 0)
    {
        return FALSE;
    }

    // 打印写入金币值的地址信息
    wchar_t writeInfo[128];
    swprintf_s(writeInfo, L"正在写入金币值 %d 到地址: 0x%p", newValue, (void*)coinAddress);
    OutputDebugStringW(writeInfo);
    wprintf(L"%s\n", writeInfo);

    SIZE_T bytesWritten;
    if (WriteProcessMemory(pvzProcessHandle, (LPVOID)coinAddress, &newValue, sizeof(DWORD), &bytesWritten))
    {
        currentCoin = newValue;
        
        // 打印写入成功信息
        wchar_t successInfo[128];
        swprintf_s(successInfo, L"金币值写入成功! 新值: %d, 写入字节数: %zu", newValue, bytesWritten);
        OutputDebugStringW(successInfo);
        wprintf(L"%s\n", successInfo);
        
        return (bytesWritten == sizeof(DWORD));
    }
    
    // 打印写入失败信息
    wchar_t failInfo[128];
    swprintf_s(failInfo, L"金币值写入失败! 目标地址: 0x%p, 新值: %d", (void*)coinAddress, newValue);
    OutputDebugStringW(failInfo);
    wprintf(L"%s\n", failInfo);
    
    return FALSE;
}



// 启用/禁用阳光修改
void EnableSunlightModify(BOOL enable)
{
    // 简化版本，不需要启用/禁用控件
}

// 启用/禁用金币修改
void EnableCoinModify(BOOL enable)
{
    // 简化版本，不需要启用/禁用控件
}




