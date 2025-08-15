# Pvz-helper

植物大战僵尸原版辅助工具源码及源码讲解分析教程

## 项目概述

这是一个基于Windows API开发的植物大战僵尸游戏内存修改器，通过直接操作游戏进程内存来修改游戏中的阳光和金币数值。该项目展示了Windows系统编程、进程间通信、内存操作等核心技术的实际应用。

## 代码结构分析

### 1. 头文件包含和依赖库

```cpp
#include "framework.h"           // Windows框架头文件
#include "Pvz hlpkit.h"         // 项目自定义头文件
#include <TlHelp32.h>           // 进程枚举和快照相关API
#include <Psapi.h>              // 进程和模块信息API
#include <vector>               // STL容器
#include <string>               // 字符串处理
#include <stdlib.h>             // 标准库函数
#include <stdio.h>              // 标准输入输出
```

**技术要点：**

- `TlHelp32.h`：提供进程枚举、快照创建等系统级操作
- `Psapi.h`：提供进程模块枚举、内存信息查询等功能
- 这些库是实现进程间通信和内存操作的基础

### 2. 全局变量设计

#### 2.1 基础Windows变量

```cpp
HINSTANCE hInst;                // 当前实例句柄
WCHAR szTitle[MAX_LOADSTRING];  // 窗口标题
WCHAR szWindowClass[MAX_LOADSTRING]; // 窗口类名
```

#### 2.2 游戏进程相关变量

```cpp
DWORD pvzProcessId = 0;         // 游戏进程ID
HANDLE pvzProcessHandle = NULL; // 游戏进程句柄
DWORD_PTR pvzBaseAddress = 0;   // 游戏基地址
```

#### 2.3 内存偏移配置

```cpp
// 阳光修改偏移
DWORD_PTR sunlightBaseAddress = 0;  // 阳光基地址
DWORD sunlightOffset1 = 0x768;      // 第一层偏移
DWORD sunlightOffset2 = 0x5560;     // 第二层偏移

// 金币修改偏移
DWORD coinOffset1 = 0x82C;          // 第一层偏移
DWORD coinOffset2 = 0x28;           // 第二层偏移
```

**设计思路：**

- 使用多层指针偏移来定位游戏数据在内存中的具体位置
- 这种设计可以适应游戏版本更新时的内存布局变化
- 偏移值通过逆向工程分析得出

## 核心功能实现分析

### 1. 主程序入口 (wWinMain)

```cpp
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PVZHLPKIT, szWindowClass, MAX_LOADSTRING);
  
    // 注册窗口类
    MyRegisterClass(hInstance);
  
    // 创建主窗口
    if (!InitInstance(hInstance, nCmdShow)) return FALSE;
  
    // 主消息循环
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PVZHLPKIT));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}
```

**技术要点：**

- 使用Windows消息循环机制处理用户交互
- 支持快捷键加速器
- 标准的Windows应用程序架构

### 2. 进程扫描与连接 (ScanPvzProcess)

这是整个程序的核心功能之一，负责自动发现并连接到植物大战僵尸游戏进程。

```cpp
BOOL ScanPvzProcess()
{
    // 1. 创建进程快照
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  
    // 2. 枚举所有进程
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
  
    // 3. 查找目标进程（支持多种进程名）
    if (_wcsicmp(pe32.szExeFile, L"PlantsVsZombies.exe") == 0 ||
        _wcsicmp(pe32.szExeFile, L"Plants vs Zombies.exe") == 0 ||
        _wcsicmp(pe32.szExeFile, L"PlantsVsZombies") == 0 ||
        _wcsicmp(pe32.szExeFile, L"pvz.exe") == 0 ||
        _wcsicmp(pe32.szExeFile, L"pvz") == 0)
    {
        // 找到目标进程
        pvzProcessId = pe32.th32ProcessID;
        // 打开进程句柄
        pvzProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pvzProcessId);
        // 获取进程基地址
        EnumProcessModules(pvzProcessHandle, &hModule, sizeof(hModule), &cbNeeded);
    }
}
```

**关键技术点：**

- **进程快照**：使用 `CreateToolhelp32Snapshot`创建系统进程快照
- **进程枚举**：通过 `Process32First`和 `Process32Next`遍历所有进程
- **进程句柄**：使用 `OpenProcess`获取目标进程的访问权限
- **模块枚举**：通过 `EnumProcessModules`获取进程的主模块基地址

### 3. 内存地址计算 (CalculateSunlightAddress/CalculateCoinAddress)

这是内存修改的核心算法，使用多层指针偏移来定位游戏数据。

```cpp
DWORD_PTR CalculateSunlightAddress()
{
    // 1. 基础地址 = 进程基地址 + 阳光基偏移
    DWORD_PTR ptr1 = pvzBaseAddress + sunlightBaseAddress;
  
    // 2. 读取第一层指针
    ReadProcessMemory(pvzProcessHandle, (LPCVOID)ptr1, &finalAddress, sizeof(DWORD_PTR), &bytesRead);
  
    // 3. 加上第一层偏移
    finalAddress += sunlightOffset1;
  
    // 4. 读取第二层指针
    ReadProcessMemory(pvzProcessHandle, (LPCVOID)finalAddress, &ptr2, sizeof(DWORD_PTR), &bytesRead);
  
    // 5. 加上第二层偏移得到最终地址
    finalAddress = ptr2 + sunlightOffset2;
  
    return finalAddress;
}
```

**内存寻址原理：**

```
游戏基地址 (0x400000) 
    ↓
+ 阳光基偏移 (0x2A9EC0)
    ↓
第一层指针地址 → 读取指针值
    ↓
+ 第一层偏移 (0x768)
    ↓
第二层指针地址 → 读取指针值
    ↓
+ 第二层偏移 (0x5560)
    ↓
最终数据地址 → 阳光数值存储位置
```

**为什么需要多层指针？**

- 游戏使用动态内存分配，数据位置不固定
- 多层指针可以适应内存布局变化
- 提高地址计算的稳定性和兼容性

### 4. 内存写入操作 (WriteSunlightValue/WriteCoinValue)

```cpp
BOOL WriteSunlightValue(DWORD newValue)
{
    // 1. 计算目标地址
    DWORD_PTR sunlightAddress = CalculateSunlightAddress();
  
    // 2. 使用WriteProcessMemory写入新值
    SIZE_T bytesWritten;
    if (WriteProcessMemory(pvzProcessHandle, (LPVOID)sunlightAddress, 
                          &newValue, sizeof(DWORD), &bytesWritten))
    {
        currentSunlight = newValue;
        return (bytesWritten == sizeof(DWORD));
    }
    return FALSE;
}
```

**技术要点：**

- `WriteProcessMemory`：跨进程内存写入的核心API
- 需要 `PROCESS_ALL_ACCESS`权限才能写入其他进程内存
- 写入成功后更新本地缓存值

### 5. 用户界面实现 (CreateControls)

```cpp
void CreateControls(HWND hWnd)
{
    // 创建主标题
    CreateWindow(L"STATIC", L"植物大战僵尸 - 游戏修改器", 
                WS_VISIBLE | WS_CHILD | SS_CENTER, 
                20, 20, 390, 35, hWnd, NULL, hInst, NULL);
  
    // 创建阳光修改勾选框
    hCheckSunlight = CreateWindow(L"BUTTON", L"修改阳光为9999", 
                                 WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                                 20, 110, 250, 25, hWnd, (HMENU)1003, hInst, NULL);
  
    // 创建金币修改勾选框
    hCheckCoin = CreateWindow(L"BUTTON", L"修改金币为99999", 
                             WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                             20, 170, 250, 25, hWnd, (HMENU)1005, hInst, NULL);
}
```

**UI设计特点：**

- 使用原生Windows控件，无需额外UI库
- 简洁的界面布局，功能明确
- 通过自定义消息ID (1003, 1005) 处理控件事件

### 6. 消息处理 (WndProc)

```cpp
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == 1003) // 阳光修改勾选框
        {
            BOOL checked = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (checked) {
                // 启用阳光修改功能
                DWORD newSunlight = 9999;
                if (WriteSunlightValue(newSunlight)) {
                    MessageBox(hWnd, L"阳光修改为9999！", L"修改成功", MB_OK | MB_ICONINFORMATION);
                }
            }
        }
        break;
    }
}
```

**事件处理机制：**

- 通过 `WM_COMMAND`消息处理控件事件
- 使用 `SendMessage`获取控件状态
- 实时响应用户操作并执行相应功能

## 技术难点与解决方案

### 1. 64位兼容性问题

**问题描述：** 在64位系统上，指针大小从4字节变为8字节，可能导致内存读取错误。

**解决方案：**

```cpp
// 使用DWORD_PTR类型确保指针大小兼容性
DWORD_PTR pvzBaseAddress = 0;
DWORD_PTR finalAddress = 0;

// 使用sizeof(DWORD_PTR)确保读取正确的字节数
ReadProcessMemory(pvzProcessHandle, (LPCVOID)ptr1, &finalAddress, sizeof(DWORD_PTR), &bytesRead);
```

### 2. 进程权限问题

**问题描述：** 默认情况下，程序无法访问其他进程的内存。

**解决方案：**

```cpp
// 使用PROCESS_ALL_ACCESS获取完整访问权限
pvzProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pvzProcessId);

// 注意：这需要管理员权限或目标进程允许访问
```

### 3. 内存地址稳定性

**问题描述：** 游戏重启后，内存地址可能发生变化。

**解决方案：**

```cpp
// 使用相对偏移而非绝对地址
DWORD_PTR ptr1 = pvzBaseAddress + sunlightBaseAddress;

// 通过多层指针提高地址稳定性
finalAddress = ptr2 + sunlightOffset2;
```

## 安全性与注意事项

### 1. 权限要求

- 程序需要管理员权限才能访问其他进程内存
- 目标游戏进程不能有反调试保护

### 2. 内存操作风险

- 错误的地址计算可能导致程序崩溃
- 写入错误的内存位置可能影响系统稳定性

### 3. 反病毒软件

- 内存修改行为可能被误报为恶意软件
- 建议添加数字签名或白名单

## 扩展功能设计

代码中预留了多个功能变量，为未来扩展做准备：

```cpp
// 预留功能变量
BOOL autoRefreshEnabled = FALSE;        // 自动刷新功能
BOOL unlimitedSunlightEnabled = FALSE;  // 无限阳光功能
BOOL godModeEnabled = FALSE;            // 无敌模式功能
BOOL fastPlantEnabled = FALSE;          // 快速种植功能
```

**扩展思路：**

- 自动刷新：定时扫描游戏状态并自动修改
- 无限阳光：持续监控阳光值，低于阈值时自动补充
- 无敌模式：修改僵尸伤害或植物生命值
- 快速种植：修改种植冷却时间或资源消耗

## 编译与部署

### 1. 开发环境

- Visual Studio 2019/2022
- Windows SDK 10.0
- 支持x86和x64架构

### 2. 编译配置

- Release模式优化性能
- 静态链接减少依赖
- 支持Unicode字符集

### 3. 部署要求

- Windows 7及以上系统
- 管理员权限运行
- 目标游戏进程正在运行

## 总结

这个项目展示了Windows系统编程的多个核心技术：

1. **进程管理**：进程枚举、句柄操作、模块信息获取
2. **内存操作**：跨进程内存读写、地址计算、指针操作
3. **用户界面**：原生Windows控件、消息处理、事件驱动
4. **系统集成**：权限管理、错误处理、调试输出

通过分析这个代码，可以学习到：

- Windows API的实际应用
- 游戏逆向工程的基本原理
- 系统级编程的安全考虑
- 软件架构设计的实践经验

这是一个很好的学习Windows系统编程和游戏修改技术的示例项目。
