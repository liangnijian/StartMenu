#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <ole2.h>
#include <oleidl.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <functional>
#include <cmath>
#include <cwctype>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

// ============================================================
//  DPI 自适应
//    所有尺寸按 96 DPI 设计，运行时通过 S() 按当前 DPI 缩放。
//    优先 Per-Monitor V2 → Per-Monitor → System DPI Aware，
//    全部通过 GetProcAddress 动态加载，兼容 Win7 及以上。
// ============================================================
static int g_dpi = 96;
static inline int S(int v) { return MulDiv(v, g_dpi, 96); }

// ============================================================
//  尺寸常量
//    _BASE 后缀 = 96 DPI 下的原始设计值（永不修改）
//    无后缀      = 运行时已按 g_dpi 缩放后的实际像素值
//    UpdateDpiSizes() 每次都从 _BASE 重新计算，避免双重缩放
// ============================================================
static const int TITLE_H_BASE = 40;   // 自绘标题栏高度
static const int BTN_W_BASE = 46;   // 标题栏按钮宽度
static const int LIST_W_BASE = 280;  // 左侧列表宽度
static const int LETTER_W_BASE = 24;   // 字母索引宽度
static const int LETTER_ROW_H_BASE = 20;   // 列表内字母分组标题高度
static const int ITEM_H_BASE = 38;   // 列表项高度 (32x32 图标专用)
static const int TILE_W_BASE = 72;   // 磁贴宽
static const int TILE_H_BASE = 72;   // 磁贴高
static const int TILE_GAP_BASE = 4;    // 磁贴间距
static const int GROUP_PAD_BASE = 8;   // 分组水平内边距
static const int GROUP_HDR_BASE = 28;   // 分组标题高度
static const int GROUP_GAP_BASE = 18;   // 分组之间的间距
static const int MAX_COLS = 4;    // 每组横向最多磁贴数
static const int MAX_ROWS = 6;    // 每组竖向最多磁贴数
static const int DEFAULT_W = 982;    // 默认宽度（逻辑单位，wWinMain 中手动 MulDiv）
static const int DEFAULT_H = 680;    // 默认高度（逻辑单位，wWinMain 中手动 MulDiv）
static const int MIN_W_BASE = 650;    // 最小宽度
static const int MIN_H_BASE = 460;    // 最小高度

// 滚动条尺寸
static const int SCROLLBAR_W_BASE = 8;    // 滚动条宽度
static const int SCROLLBAR_PAD_BASE = 4;    // 上下边距
static const int SCROLLBAR_MIN_THUMB_BASE = 20;   // 滑块最小高度

// 搜索框尺寸（位于右侧磁贴区域底部，左右居中）
static const int SEARCH_W_BASE = 340;      // 搜索框宽度
static const int SEARCH_H_BASE = 34;       // 搜索框高度（两端为半圆）
static const int SEARCH_BOTTOM_BASE = 16;  // 搜索框距窗口底部的距离
static const int SEARCH_GAP_BASE = 12;     // 搜索框顶部与磁贴内容的间隙
static const int SEARCH_ICON_BASE = 18;    // 搜索框右侧图标尺寸（放大镜 / 叉）
static const int GEAR_ICON_BASE = 20;      // 齿轮图标尺寸（按钮直径与搜索框高度一致）

// 设置窗口尺寸
static const int SET_W_BASE = 620;
static const int SET_H_BASE = 440;
static const int SET_NAV_W_BASE = 150;     // 左侧选项列表宽度
static const int SET_NAV_ITEM_H_BASE = 42;
static const int SET_SWITCH_W_BASE = 46;   // 开关宽度
static const int SET_SWITCH_H_BASE = 24;   // 开关高度
static const int SET_TRACK_H_BASE = 10;    // 滑条高度（比搜索框矮）
static const int SET_KNOB_R_BASE = 8;      // 滑条圆点半径

// 主题默认值（可在 .cfg 的 [Color] 段中保存）
static const int  THEME_OPACITY_DEF = 94;   // 窗口不透明度 0~100
static const int  THEME_HUE_DEF = 220;      // 主题色相 0~360
static const int  THEME_BRIGHT_DEF = 10;    // 颜色亮度 0~100
static const bool THEME_GRAYMODE_DEF = true; // 默认开启黑白模式
static const int  THEME_GRAY_DEF = 20;      // 黑白模式默认灰度 0~100

// 重命名对话框尺寸
static const int DLG_W_BASE = 400;
static const int DLG_H_BASE = 180;
static const int DLG_TITLE_H_BASE = 34;
static const int DLG_MARGIN_BASE = 20;
static const int DLG_BTN_W_BASE = 76;
static const int DLG_BTN_H_BASE = 30;

// 右键菜单尺寸
static const int MENU_ITEM_H_BASE = 32;
static const int MENU_PAD_BASE = 4;
static const int MENU_W_BASE = 168;

// 运行时尺寸（经 S() 缩放）
static int TITLE_H = TITLE_H_BASE;
static int BTN_W = BTN_W_BASE;
static int LIST_W = LIST_W_BASE;
static int LETTER_W = LETTER_W_BASE;
static int LETTER_ROW_H = LETTER_ROW_H_BASE;
static int ITEM_H = ITEM_H_BASE;
static int TILE_W = TILE_W_BASE;
static int TILE_H = TILE_H_BASE;
static int TILE_GAP = TILE_GAP_BASE;
static int GROUP_PAD = GROUP_PAD_BASE;
static int GROUP_HDR = GROUP_HDR_BASE;
static int GROUP_GAP = GROUP_GAP_BASE;
static int MIN_W = MIN_W_BASE;
static int MIN_H = MIN_H_BASE;

static int SCROLLBAR_W = SCROLLBAR_W_BASE;
static int SCROLLBAR_PAD = SCROLLBAR_PAD_BASE;
static int SCROLLBAR_MIN_THUMB = SCROLLBAR_MIN_THUMB_BASE;

static int DLG_W = DLG_W_BASE;
static int DLG_H = DLG_H_BASE;
static int DLG_TITLE_H = DLG_TITLE_H_BASE;
static int DLG_MARGIN = DLG_MARGIN_BASE;
static int DLG_BTN_W = DLG_BTN_W_BASE;
static int DLG_BTN_H = DLG_BTN_H_BASE;

static int MENU_ITEM_H = MENU_ITEM_H_BASE;
static int MENU_PAD = MENU_PAD_BASE;
static int MENU_W = MENU_W_BASE;

// 按当前 g_dpi 从 _BASE 原始值重新计算所有尺寸常量
// ★ 关键修复：每次从 _BASE 计算，即便多次调用也不会双重缩放
static void UpdateDpiSizes()
{
    TITLE_H = S(TITLE_H_BASE);
    BTN_W = S(BTN_W_BASE);
    LIST_W = S(LIST_W_BASE);
    LETTER_W = S(LETTER_W_BASE);
    LETTER_ROW_H = S(LETTER_ROW_H_BASE);
    ITEM_H = S(ITEM_H_BASE);
    TILE_W = S(TILE_W_BASE);
    TILE_H = S(TILE_H_BASE);
    TILE_GAP = S(TILE_GAP_BASE);
    GROUP_PAD = S(GROUP_PAD_BASE);
    GROUP_HDR = S(GROUP_HDR_BASE);
    GROUP_GAP = S(GROUP_GAP_BASE);
    MIN_W = S(MIN_W_BASE);
    MIN_H = S(MIN_H_BASE);

    SCROLLBAR_W = S(SCROLLBAR_W_BASE);
    SCROLLBAR_PAD = S(SCROLLBAR_PAD_BASE);
    SCROLLBAR_MIN_THUMB = S(SCROLLBAR_MIN_THUMB_BASE);

    DLG_W = S(DLG_W_BASE);
    DLG_H = S(DLG_H_BASE);
    DLG_TITLE_H = S(DLG_TITLE_H_BASE);
    DLG_MARGIN = S(DLG_MARGIN_BASE);
    DLG_BTN_W = S(DLG_BTN_W_BASE);
    DLG_BTN_H = S(DLG_BTN_H_BASE);

    MENU_ITEM_H = S(MENU_ITEM_H_BASE);
    MENU_PAD = S(MENU_PAD_BASE);
    MENU_W = S(MENU_W_BASE);
}

// 兼容旧版 SDK 的 DPI_AWARENESS_CONTEXT 常量定义
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((HANDLE)-3)
#endif

// 启用 DPI 感知，多级回退：
//   1. SetProcessDpiAwarenessContext (Per-Monitor V2, Win10 1703+)
//   2. SetProcessDpiAwareness        (Per-Monitor,    Win8.1+)
//   3. SetProcessDPIAware            (System Aware,   Vista/Win7+)
static void EnableDpiAwareness()
{
    bool done = false;

    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32)
    {
        typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(HANDLE);
        PFN_SetProcessDpiAwarenessContext pfn =
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(
                hUser32, "SetProcessDpiAwarenessContext");
        if (pfn)
        {
            if (pfn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) done = true;
        }
    }

    if (!done)
    {
        HMODULE hShcore = LoadLibraryW(L"shcore.dll");
        if (hShcore)
        {
            typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(int);
            PFN_SetProcessDpiAwareness pfn =
                (PFN_SetProcessDpiAwareness)GetProcAddress(
                    hShcore, "SetProcessDpiAwareness");
            if (pfn)
            {
                // PROCESS_PER_MONITOR_DPI_AWARE = 2
                if (SUCCEEDED(pfn(2))) done = true;
            }
            FreeLibrary(hShcore);
        }
    }

    if (!done && hUser32)
    {
        typedef BOOL(WINAPI* PFN_SetProcessDPIAware)();
        PFN_SetProcessDPIAware pfn =
            (PFN_SetProcessDPIAware)GetProcAddress(
                hUser32, "SetProcessDPIAware");
        if (pfn) pfn();
    }
}

// 获取窗口 DPI（Win10 使用 GetDpiForWindow，否则取系统 DPI）
static UINT GetWindowDpiCompat(HWND hwnd)
{
    if (hwnd)
    {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32)
        {
            typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
            PFN_GetDpiForWindow pfn =
                (PFN_GetDpiForWindow)GetProcAddress(hUser32, "GetDpiForWindow");
            if (pfn)
            {
                UINT d = pfn(hwnd);
                if (d) return d;
            }
        }
    }
    HDC dc = GetDC(nullptr);
    UINT dpi = 96;
    if (dc) { dpi = (UINT)GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(nullptr, dc); }
    return dpi ? dpi : 96;
}

// ============================================================
//  数据结构
// ============================================================
struct Item {
    std::wstring name;     // 显示名称 (快捷方式的名称)
    std::wstring path;     // 实际路径 (解析后的 exe 或真实文件)
    std::wstring args;     // 解析后的参数
    HICON        hIcon = nullptr;
    bool         missing = false;  // 文件是否已不存在（运行时检查）
};

struct Group {
    std::wstring     name;
    std::vector<int> items;      // 指向 g_items 的索引，-1 表示空位（不自动补位）
    RECT             rc = { 0,0,0,0 };
    RECT             rcTitle = { 0,0,0,0 };
    RECT             rcBody = { 0,0,0,0 };
};

struct ListRow {
    bool    header;
    wchar_t letter;
    int     itemIdx;
    int     y;
};

// ============================================================
//  全局变量
// ============================================================
static HINSTANCE g_hInst = nullptr;
static HWND      g_hWnd = nullptr;
static HICON     g_hIconBig = nullptr, g_hIconSmall = nullptr;

static std::wstring g_exeDir;
static std::wstring g_cfgPath;

static HFONT g_fontUI = nullptr;
static HFONT g_fontSmall = nullptr;
static HFONT g_fontTitle = nullptr;

static std::vector<Item>  g_items;
static std::vector<Group> g_groups;

static std::vector<ListRow> g_listRows;
static std::vector<wchar_t> g_letters;
static int g_listHeight = 0;
static int g_listScroll = 0;
static int g_tileScroll = 0;
static int g_tileContentH = 0;

static POINT g_mouse = { -1000, -1000 };
static int   g_hoverBtn = 0;
static int   g_hoverLetter = -1;
static int   g_hoverListIdx = -1;
static int   g_hoverTileG = -1;
static int   g_hoverTileI = -1;

// 标题栏按钮按下状态（松开鼠标左键时再执行动作）
static int   g_pressedTitleBtn = 0;    // 0=无 1=最小化 2=最大化 3=关闭

// 分组标题栏按下/拖动状态
//   按下时 g_dragGroupPending = true
//   移动超过阈值 -> 进入拖动模式 (g_dragGroup = true)
//   未移动就松开 -> 视为单击，进入重命名
static bool  g_dragGroupPending = false;
static POINT g_dragGroupStart = { 0, 0 };

// 滚动条拖动状态
static bool  g_listScrollDrag = false;
static int   g_listScrollDragStartY = 0;
static int   g_listScrollDragStartOffset = 0;
static bool  g_tileScrollDrag = false;
static int   g_tileScrollDragStartY = 0;
static int   g_tileScrollDragStartOffset = 0;

// 拖动磁贴
static bool  g_dragTile = false;
static bool  g_dragPending = false;
static int   g_dragG = -1, g_dragI = -1;
static POINT g_dragStart = { 0,0 };
static POINT g_dragOffset = { 0,0 };
static RECT  g_dragRect = { 0,0,0,0 };
static int   g_dragTargetG = -1;
static int   g_dragTargetPos = -1;
static int   g_dragTargetMode = 0;   // 0=无 1=竖线(插入已占用格之间) 2=框框(放到空白格)
static bool  g_dragNewGroup = false;

// 外部拖入（资源管理器拖文件进程序窗口）
static bool  g_dragExternal = false;     // 当前是否处于外部拖放过程中
static IDropTarget* g_pDropTarget = nullptr;

// 拖动分组
static bool  g_dragGroup = false;
static int   g_dragGroupIdx = -1;

// 行内编辑分组名（g_hEdit != nullptr 即表示处于编辑状态）
static HWND  g_hEdit = nullptr;
static int   g_editGroup = -1;

// 搜索框（右侧磁贴区域底部）
//   输入内容后，列表与磁贴中名称包含该内容的项背景高亮为橙黄色
static HWND         g_hSearchEdit = nullptr;   // 搜索输入框（EDIT 子窗口）
static std::wstring g_searchText;              // 当前搜索内容（空表示未搜索）
static bool         g_hoverSearch = false;     // 鼠标是否悬停在搜索框上
static bool         g_hoverSearchClear = false;// 鼠标是否悬停在搜索框右侧的叉上
static bool         g_searchComposing = false; // 搜索框是否正在输入法组字
static bool         g_hoverSettingsBtn = false;// 鼠标是否悬停在齿轮设置按钮上

// 主题（保存于 .cfg 的 [Color] 段）
static int      g_winOpacity = THEME_OPACITY_DEF;  // 窗口不透明度 0~100
static int      g_themeHue = THEME_HUE_DEF;        // 主题色相 0~360
static int      g_themeBright = THEME_BRIGHT_DEF;  // 颜色亮度 0~100
static bool     g_grayMode = THEME_GRAYMODE_DEF;   // 黑白模式
static int      g_grayLevel = THEME_GRAY_DEF;      // 黑白模式下的灰度 0~100
static COLORREF g_themeBg = RGB(22, 22, 25);       // 由上面几个值算出的窗口背景色

// 由主题色派生出的整套界面配色（标题栏 / 列表 / 磁贴 / 对话框等）
static bool     g_themeLight = false;              // 当前主题是否为浅色
static COLORREF g_uiTitle = RGB(32, 32, 36);
static COLORREF g_uiTitleHot = RGB(60, 60, 66);
static COLORREF g_uiList = RGB(28, 28, 31);
static COLORREF g_uiHover = RGB(48, 48, 54);
static COLORREF g_uiCard = RGB(34, 34, 38);
static COLORREF g_uiTile = RGB(42, 42, 48);
static COLORREF g_uiTileHot = RGB(60, 60, 68);
static COLORREF g_uiSurface = RGB(45, 45, 52);
static COLORREF g_uiSurfaceBorder = RGB(70, 70, 82);
static COLORREF g_uiSurfaceBorderHover = RGB(112, 112, 128);
static COLORREF g_uiText = RGB(232, 232, 236);
static COLORREF g_uiTextDim = RGB(185, 185, 195);
static COLORREF g_uiTextFaint = RGB(145, 145, 158);
static COLORREF g_uiAccentText = RGB(120, 190, 255);
static COLORREF g_uiIcon = RGB(176, 176, 188);
static COLORREF g_uiSeparator = RGB(48, 48, 54);
static COLORREF g_uiScrollTrack = RGB(42, 42, 48);
static COLORREF g_uiScrollThumb = RGB(90, 90, 100);
static COLORREF g_uiScrollThumbHot = RGB(140, 140, 155);
static COLORREF g_uiDialogBg = RGB(34, 34, 38);
static COLORREF g_uiNavSel = RGB(56, 56, 64);
static COLORREF g_uiNavHover = RGB(45, 45, 52);
static COLORREF g_uiBtn = RGB(55, 55, 62);
static COLORREF g_uiBtnHot = RGB(70, 70, 80);
static COLORREF g_uiSwitchOff = RGB(72, 72, 82);
static HBRUSH   g_hbrEdit = nullptr;               // 输入框背景刷（随主题更新）

// 托盘模式
static bool  g_trayMode = false;
static bool  g_reallyExit = false;         // true 表示真正退出（托盘菜单「退出」）
static bool  g_trayIconAdded = false;
static HHOOK g_kbHook = nullptr;
static bool  g_kbCtrlDown = false;
static bool  g_kbWinDown = false;
static bool  g_kbChordOther = false;       // 组合键过程中按了其它键
static bool  g_kbChordDone = false;        // 本次组合键已经触发过
static UINT  g_wmTaskbarCreated = 0;
static bool  g_mainWasActive = false;      // 主窗口是否曾经真正激活（避免启动即被隐藏）

#define WM_TRAYICON     (WM_APP + 110)
#define WM_TOGGLE_MAIN  (WM_APP + 111)

// 设置窗口
static HWND g_hSettings = nullptr;
static int  g_setPage = 0;                 // 0=窗口模式 1=主题调节
static int  g_setHover = -1;               // 悬停控件 id
static int  g_setPressed = -1;             // 按下控件 id
static int  g_setDragSlider = -1;          // 正在拖动的滑条 0~2

// 右键菜单
static HWND  g_hMenu = nullptr;
static std::vector<std::wstring> g_menuItems;
static int   g_menuHover = -1;
static int   g_menuPressedIdx = -1;
static int   g_menuTargetItem = -1;
static int   g_menuMode = 0;   // 0=快捷方式右键菜单 1=托盘图标菜单

// 重命名对话框（自绘、直角）
static HWND  g_hDlg = nullptr;
static HWND  g_hDlgEdit = nullptr;
static int   g_dlgItem = -1;
static int   g_dlgHoverBtn = -1;
static int   g_dlgPressedBtn = -1;
static RECT  g_dlgBtnOk = { 0,0,0,0 };
static RECT  g_dlgBtnCancel = { 0,0,0,0 };
static RECT  g_dlgEditRect = { 0,0,0,0 };

// 文件缺失提示对话框（复用重命名窗口的直角自绘样式）
static HWND         g_hMissingDlg = nullptr;
static int          g_missingDlgHoverBtn = -1;
static int          g_missingDlgPressedBtn = -1;
static RECT         g_missingDlgBtnYes = { 0,0,0,0 };
static RECT         g_missingDlgBtnNo = { 0,0,0,0 };
static std::wstring g_missingDlgText;
static std::vector<int> g_missingDlgItems;

// 启动文件缺失检查的自定义消息
#define WM_STARTUP_CHECK (WM_APP + 100)

// ============================================================
//  前置声明
// ============================================================
static void RebuildList();
static void LayoutGroups();
static void ShowRenameDialog(int itemIdx);
static void ToggleMaximize();
static void SaveConfig();
static void ShowMissingFilesDialog(const std::vector<int>& items);
static bool FileExistsForItem(const Item& it);
static void CheckAllItems();
static bool EnsureItemAvailable(int itemIdx);
static void ShowMainWindow();
static void HideMainWindow();
static void ExitApplication();
static void ApplyTrayMode(bool enable);
static void ShowSettingsDialog();

// ============================================================
//  工具函数
// ============================================================
static std::wstring GetExeDir()
{
    wchar_t buf[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    PathRemoveFileSpecW(buf);
    return buf;
}

static std::wstring GetSpecialPath(int csidl)
{
    wchar_t buf[MAX_PATH] = { 0 };
    SHGetFolderPathW(nullptr, csidl, nullptr, 0, buf);
    return buf;
}

static std::wstring ExpandPath(const std::wstring& s)
{
    std::wstring r = s;
    auto repl = [&](const wchar_t* tok, const std::wstring& val) {
        if (val.empty()) return;
        size_t p;
        while ((p = r.find(tok)) != std::wstring::npos)
            r.replace(p, wcslen(tok), val);
        };
    wchar_t sys[MAX_PATH] = { 0 };
    GetWindowsDirectoryW(sys, MAX_PATH);
    repl(L"%SystemRoot%", sys);
    repl(L"%ProgramFiles(x86)%", GetSpecialPath(CSIDL_PROGRAM_FILESX86));
    repl(L"%ProgramFiles%", GetSpecialPath(CSIDL_PROGRAM_FILES));
    repl(L"%Desktop%", GetSpecialPath(CSIDL_DESKTOPDIRECTORY));
    repl(L"%Programs%", GetSpecialPath(CSIDL_PROGRAMS));
    repl(L"%APPDATA%", GetSpecialPath(CSIDL_APPDATA));
    repl(L"%LOCALAPPDATA%", GetSpecialPath(CSIDL_LOCAL_APPDATA));
    repl(L"%USERPROFILE%", GetSpecialPath(CSIDL_PROFILE));

    // 相对路径（保存在程序同目录/子目录下的文件）→ 拼回程序所在目录
    if (!r.empty() && PathIsRelativeW(r.c_str()))
    {
        std::wstring base = g_exeDir;
        if (!base.empty() && base.back() != L'\\') base += L'\\';
        r = base + r;
    }
    return r;
}

static std::wstring MakePortable(const std::wstring& s)
{
    if (s.empty()) return s;

    // 程序所在目录（含子目录）内的文件：只保存相对路径，便于整体迁移
    if (!g_exeDir.empty())
    {
        std::wstring prefix = g_exeDir;
        if (prefix.back() != L'\\') prefix += L'\\';
        if (_wcsnicmp(s.c_str(), prefix.c_str(), prefix.size()) == 0)
            return s.substr(prefix.size());
    }

    static std::vector<std::pair<std::wstring, std::wstring>> vars;
    if (vars.empty())
    {
        wchar_t sys[MAX_PATH] = { 0 };
        GetWindowsDirectoryW(sys, MAX_PATH);
        vars.push_back({ sys, L"%SystemRoot%" });
        vars.push_back({ GetSpecialPath(CSIDL_PROGRAM_FILESX86), L"%ProgramFiles(x86)%" });
        vars.push_back({ GetSpecialPath(CSIDL_PROGRAM_FILES), L"%ProgramFiles%" });
        vars.push_back({ GetSpecialPath(CSIDL_DESKTOPDIRECTORY), L"%Desktop%" });
        vars.push_back({ GetSpecialPath(CSIDL_PROGRAMS), L"%Programs%" });
        vars.push_back({ GetSpecialPath(CSIDL_APPDATA), L"%APPDATA%" });
        vars.push_back({ GetSpecialPath(CSIDL_LOCAL_APPDATA), L"%LOCALAPPDATA%" });
        vars.push_back({ GetSpecialPath(CSIDL_PROFILE), L"%USERPROFILE%" });
        std::sort(vars.begin(), vars.end(),
            [](const std::pair<std::wstring, std::wstring>& a,
                const std::pair<std::wstring, std::wstring>& b)
            { return a.first.size() > b.first.size(); });
    }
    for (auto& v : vars)
    {
        if (v.first.empty()) continue;
        if (_wcsnicmp(s.c_str(), v.first.c_str(), v.first.size()) == 0)
            return v.second + s.substr(v.first.size());
    }
    return s;
}

// 读取整个文件 (自动识别编码，支持 ANSI / UTF-8 BOM / UTF-16 BOM)
static std::wstring ReadAllText(const std::wstring& path)
{
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return L"";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string s(sz, 0);
    fread(&s[0], 1, sz, f);
    fclose(f);

    if (sz >= 2 && (unsigned char)s[0] == 0xFF && (unsigned char)s[1] == 0xFE) {
        // UTF-16LE BOM
        return std::wstring((wchar_t*)&s[2], (sz - 2) / 2);
    }
    else if (sz >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
        // UTF-8 BOM
        int len = MultiByteToWideChar(CP_UTF8, 0, &s[3], sz - 3, NULL, 0);
        std::wstring w(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, &s[3], sz - 3, &w[0], len);
        return w;
    }
    else {
        // ANSI
        int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), sz, NULL, 0);
        std::wstring w(len, 0);
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), sz, &w[0], len);
        return w;
    }
}

// 写入整个文件 (统一保存为 UTF-16LE 带 BOM，避免乱码)
static void WriteAllText(const std::wstring& path, const std::wstring& text)
{
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) return;
    unsigned char bom[2] = { 0xFF, 0xFE };
    fwrite(bom, 1, 2, f);
    fwrite(text.c_str(), sizeof(wchar_t), text.size(), f);
    fclose(f);
}

static bool ResolveShortcut(const std::wstring& lnk, std::wstring& target,
    std::wstring& args, std::wstring& workDir)
{
    IShellLinkW* psl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, (void**)&psl)))
        return false;

    IPersistFile* ppf = nullptr;
    bool ok = false;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)))
    {
        if (SUCCEEDED(ppf->Load(lnk.c_str(), STGM_READ)))
        {
            wchar_t buf[MAX_PATH * 2] = { 0 };
            psl->GetPath(buf, MAX_PATH * 2, nullptr, SLGP_RAWPATH);
            target = buf;
            psl->GetArguments(buf, MAX_PATH * 2);
            args = buf;
            psl->GetWorkingDirectory(buf, MAX_PATH * 2);
            workDir = buf;
            ok = true;
        }
        ppf->Release();
    }
    psl->Release();
    return ok;
}

static HICON GetItemIcon(const std::wstring& path)
{
    SHFILEINFOW sfi = { 0 };
    if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi),
        SHGFI_ICON | SHGFI_LARGEICON))
        return sfi.hIcon;
    return nullptr;
}

static std::wstring StripExt(const std::wstring& fn)
{
    size_t p = fn.find_last_of(L'.');
    if (p != std::wstring::npos && p > 0) return fn.substr(0, p);
    return fn;
}

// 获取首字母，支持中文拼音映射
static wchar_t GetFirstLetter(wchar_t c)
{
    if (c >= L'a' && c <= L'z') return (wchar_t)(c - 32);
    if (c >= L'A' && c <= L'Z') return c;

    char buf[4] = { 0 };
    if (WideCharToMultiByte(936, 0, &c, 1, buf, 4, NULL, NULL) == 2)
    {
        unsigned char b1 = (unsigned char)buf[0];
        unsigned char b2 = (unsigned char)buf[1];
        if (b1 >= 0xB0 && b1 <= 0xD7)
        {
            int code = (b1 << 8) | b2;
            static const unsigned short table[] = {
                0xB0A1, 0xB0C5, 0xB2C1, 0xB4EE, 0xB6EA, 0xB7A2, 0xB8C1, 0xB9FE, 0xBBF7,
                0xBFA6, 0xC0AC, 0xC2E8, 0xC4C3, 0xC5B6, 0xC5BE, 0xC6DA, 0xC8BB, 0xC8F6,
                0xCBFA, 0xCDDA, 0xCEF4, 0xD1B9, 0xD4D1, 0xD7FA
            };
            static const wchar_t letters[] = L"ABCDEFGHJKLMNOPQRSTWXYZ";
            for (int i = 0; i < 23; i++)
            {
                if (code >= table[i] && code < table[i + 1])
                    return letters[i];
            }
        }
    }
    return L'#';
}

static wchar_t FirstLetter(const std::wstring& s)
{
    if (s.empty()) return L'#';
    return GetFirstLetter(s[0]);
}

static void DrawText2(HDC hdc, const std::wstring& t, RECT rc, COLORREF col,
    HFONT f, UINT flags)
{
    SetTextColor(hdc, col);
    HGDIOBJ old = SelectObject(hdc, f);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, t.c_str(), -1, &rc, flags);
    SelectObject(hdc, old);
}

static void FillRoundRect(HDC hdc, RECT rc, int rad, COLORREF fill, COLORREF border)
{
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(hdc, br);
    HGDIOBJ op = SelectObject(hdc, pn);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, rad, rad);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
    DeleteObject(pn);
}

// ------------------------------------------------------------
//  字体管理（合并 wWinMain 与 WM_DPICHANGED 中重复的创建逻辑）
// ------------------------------------------------------------
static void DestroyFonts()
{
    if (g_fontUI) { DeleteObject(g_fontUI);    g_fontUI = nullptr; }
    if (g_fontSmall) { DeleteObject(g_fontSmall); g_fontSmall = nullptr; }
    if (g_fontTitle) { DeleteObject(g_fontTitle); g_fontTitle = nullptr; }
}

static void CreateFonts()
{
    DestroyFonts();
    g_fontUI = CreateFontW(-S(12), 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"微软雅黑");
    g_fontSmall = CreateFontW(-S(11), 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"微软雅黑");
    g_fontTitle = CreateFontW(-S(12), 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"微软雅黑");
}

// ------------------------------------------------------------
//  文件存在性检查
// ------------------------------------------------------------
static bool FileExistsForItem(const Item& it)
{
    if (it.path.empty()) return false;
    std::wstring full = ExpandPath(it.path);
    return PathFileExistsW(full.c_str()) != FALSE;
}

static void CheckAllItems()
{
    for (auto& it : g_items)
        it.missing = !FileExistsForItem(it);
}

// 统一封装「检查文件是否存在 → 缺失则弹窗」逻辑
// 返回 true 表示文件可用，可以继续执行；false 表示已弹出缺失提示
static bool EnsureItemAvailable(int itemIdx)
{
    if (itemIdx < 0 || itemIdx >= (int)g_items.size()) return false;
    Item& it = g_items[itemIdx];
    if (!FileExistsForItem(it))
    {
        it.missing = true;
        if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
        std::vector<int> v;
        v.push_back(itemIdx);
        ShowMissingFilesDialog(v);
        return false;
    }
    it.missing = false;
    return true;
}

// ============================================================
//  主题（窗口背景色 / 不透明度）
// ============================================================
static void HSVToRGB(double h, double s, double v, int& r, int& g, int& b)
{
    if (h < 0.0) h = 0.0;
    h = fmod(h, 360.0);
    if (s < 0.0) s = 0.0;
    if (s > 1.0) s = 1.0;
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;

    double c = v * s;
    double x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    double rr = 0, gg = 0, bb = 0;

    if (h < 60)       { rr = c; gg = x; }
    else if (h < 120) { rr = x; gg = c; }
    else if (h < 180) { gg = c; bb = x; }
    else if (h < 240) { gg = x; bb = c; }
    else if (h < 300) { rr = x; bb = c; }
    else              { rr = c; bb = x; }

    r = (int)((rr + m) * 255.0 + 0.5);
    g = (int)((gg + m) * 255.0 + 0.5);
    b = (int)((bb + m) * 255.0 + 0.5);
}

static COLORREF MixColor(COLORREF a, COLORREF b, double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    int r = (int)(GetRValue(a) * (1.0 - t) + GetRValue(b) * t + 0.5);
    int g = (int)(GetGValue(a) * (1.0 - t) + GetGValue(b) * t + 0.5);
    int b2 = (int)(GetBValue(a) * (1.0 - t) + GetBValue(b) * t + 0.5);
    return RGB(r, g, b2);
}

static COLORREF LightenC(COLORREF c, double t)
{
    return MixColor(c, RGB(255, 255, 255), t);
}

static COLORREF DarkenC(COLORREF c, double t)
{
    return MixColor(c, RGB(0, 0, 0), t);
}

// 由色相/亮度（或黑白模式下的灰度/亮度）算出主题底色
static COLORREF ThemeColorAt(int hue, int bright)
{
    if (g_grayMode)
    {
        double gv = (g_grayLevel / 100.0) * (bright / 100.0);
        int v = (int)(gv * 255.0 + 0.5);
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        return RGB(v, v, v);
    }

    int r, g, b;
    HSVToRGB((double)hue, 0.45, bright / 100.0, r, g, b);
    return RGB(r, g, b);
}

// 根据主题底色派生所有界面元素颜色（标题栏、列表、磁贴、对话框……）
static void UpdateThemeColor()
{
    g_themeBg = ThemeColorAt(g_themeHue, g_themeBright);

    int lum = (GetRValue(g_themeBg) * 299 + GetGValue(g_themeBg) * 587 +
        GetBValue(g_themeBg) * 114) / 1000;
    g_themeLight = (lum > 140);

    if (g_themeLight)
    {
        g_uiTitle = DarkenC(g_themeBg, 0.12);
        g_uiTitleHot = DarkenC(g_themeBg, 0.22);
        g_uiList = DarkenC(g_themeBg, 0.07);
        g_uiHover = DarkenC(g_themeBg, 0.12);
        g_uiCard = DarkenC(g_themeBg, 0.10);
        g_uiTile = DarkenC(g_themeBg, 0.16);
        g_uiTileHot = DarkenC(g_themeBg, 0.26);
        g_uiSurface = DarkenC(g_themeBg, 0.18);
        g_uiSurfaceBorder = DarkenC(g_themeBg, 0.45);
        g_uiSurfaceBorderHover = DarkenC(g_themeBg, 0.62);
        g_uiText = RGB(26, 26, 30);
        g_uiTextDim = RGB(60, 60, 68);
        g_uiTextFaint = RGB(100, 100, 110);
        g_uiAccentText = RGB(0, 110, 200);
        g_uiIcon = RGB(90, 90, 100);
        g_uiSeparator = DarkenC(g_themeBg, 0.25);
        g_uiScrollTrack = DarkenC(g_themeBg, 0.15);
        g_uiScrollThumb = DarkenC(g_themeBg, 0.45);
        g_uiScrollThumbHot = DarkenC(g_themeBg, 0.62);
        g_uiDialogBg = DarkenC(g_themeBg, 0.08);
        g_uiNavSel = DarkenC(g_themeBg, 0.18);
        g_uiNavHover = DarkenC(g_themeBg, 0.12);
        g_uiBtn = DarkenC(g_themeBg, 0.18);
        g_uiBtnHot = DarkenC(g_themeBg, 0.28);
        g_uiSwitchOff = DarkenC(g_themeBg, 0.35);
    }
    else
    {
        g_uiTitle = LightenC(g_themeBg, 0.06);
        g_uiTitleHot = LightenC(g_themeBg, 0.16);
        g_uiList = LightenC(g_themeBg, 0.03);
        g_uiHover = LightenC(g_themeBg, 0.09);
        g_uiCard = LightenC(g_themeBg, 0.05);
        g_uiTile = LightenC(g_themeBg, 0.08);
        g_uiTileHot = LightenC(g_themeBg, 0.14);
        g_uiSurface = LightenC(g_themeBg, 0.10);
        g_uiSurfaceBorder = LightenC(g_themeBg, 0.20);
        g_uiSurfaceBorderHover = LightenC(g_themeBg, 0.32);
        g_uiText = RGB(232, 232, 236);
        g_uiTextDim = RGB(185, 185, 195);
        g_uiTextFaint = RGB(145, 145, 158);
        g_uiAccentText = RGB(120, 190, 255);
        g_uiIcon = RGB(176, 176, 188);
        g_uiSeparator = LightenC(g_themeBg, 0.10);
        g_uiScrollTrack = LightenC(g_themeBg, 0.08);
        g_uiScrollThumb = LightenC(g_themeBg, 0.28);
        g_uiScrollThumbHot = LightenC(g_themeBg, 0.42);
        g_uiDialogBg = LightenC(g_themeBg, 0.05);
        g_uiNavSel = LightenC(g_themeBg, 0.12);
        g_uiNavHover = LightenC(g_themeBg, 0.07);
        g_uiBtn = LightenC(g_themeBg, 0.14);
        g_uiBtnHot = LightenC(g_themeBg, 0.20);
        g_uiSwitchOff = LightenC(g_themeBg, 0.20);
    }

    if (g_hbrEdit) DeleteObject(g_hbrEdit);
    g_hbrEdit = CreateSolidBrush(g_uiSurface);
}

// 应用窗口整体不透明度（0~100，100 为不透明）
static void ApplyWindowAlpha()
{
    if (!g_hWnd) return;
    int a = g_winOpacity * 255 / 100;
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    SetLayeredWindowAttributes(g_hWnd, 0, (BYTE)a, LWA_ALPHA);
}

// 颜色 / 透明度改动后：重算背景色、应用透明度并重绘所有窗口
static void ApplyThemeToMainWindow()
{
    UpdateThemeColor();
    ApplyWindowAlpha();
    if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
    if (g_hSearchEdit) InvalidateRect(g_hSearchEdit, nullptr, TRUE);
    if (g_hSettings) InvalidateRect(g_hSettings, nullptr, FALSE);
    if (g_hDlg) InvalidateRect(g_hDlg, nullptr, FALSE);
    if (g_hMissingDlg) InvalidateRect(g_hMissingDlg, nullptr, FALSE);
    if (g_hMenu) InvalidateRect(g_hMenu, nullptr, FALSE);
}

// ============================================================
//  搜索框布局
//    搜索框固定在右侧磁贴区域底部、左右居中，两端为半圆。
//    左侧是圆形的齿轮设置按钮；磁贴区的可视高度需要扣掉底部搜索条占用的高度。
// ============================================================
static int SearchBoxH() { return S(SEARCH_H_BASE); }
static int SearchBottomMargin() { return S(SEARCH_BOTTOM_BASE); }
static int SearchStripH()
{
    return SearchBoxH() + SearchBottomMargin() + S(SEARCH_GAP_BASE);
}

static RECT GetSearchBoxRect()
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    int paneW = rc.right - LIST_W;
    int boxH = SearchBoxH();
    int boxW = S(SEARCH_W_BASE);

    // 齿轮按钮 + 间隙 + 搜索框视为一个整体，整体在磁贴区内左右居中
    const int gap = S(12);
    int maxW = paneW - S(48) - boxH - gap;
    if (boxW > maxW) boxW = maxW;
    if (boxW < S(120)) boxW = S(120);

    int groupW = boxH + gap + boxW;
    int groupLeft = LIST_W + (paneW - groupW) / 2;
    if (groupLeft < LIST_W + S(8)) groupLeft = LIST_W + S(8);

    int left = groupLeft + boxH + gap;
    int top = rc.bottom - SearchBottomMargin() - boxH;
    RECT r = { left, top, left + boxW, top + boxH };
    return r;
}

// 搜索框左侧的圆形齿轮设置按钮（直径与搜索框高度一致）
static RECT GetSettingsButtonRect()
{
    RECT b = GetSearchBoxRect();
    int d = b.bottom - b.top;
    int cy = (b.top + b.bottom) / 2;
    int x = b.left - S(12) - d;
    RECT r = { x, cy - d / 2, x + d, cy - d / 2 + d };
    return r;
}

static bool HitSettingsButton(POINT pt)
{
    RECT r = GetSettingsButtonRect();
    return PtInRect(&r, pt) != FALSE;
}

// 右侧图标（放大镜 / 叉）的绘制矩形
static RECT GetSearchIconRect()
{
    RECT b = GetSearchBoxRect();
    int sz = S(SEARCH_ICON_BASE);
    int cx = b.right - S(24);
    int cy = (b.top + b.bottom) / 2;
    RECT r = { cx - sz / 2, cy - sz / 2, cx - sz / 2 + sz, cy - sz / 2 + sz };
    return r;
}

// 右侧图标（放大镜 / 叉）的可点击区域（搜索框右侧半圆内）
static RECT GetSearchClearRect()
{
    RECT b = GetSearchBoxRect();
    int h = b.bottom - b.top;
    RECT r = { b.right - h, b.top, b.right, b.bottom };
    return r;
}

static bool HitSearchBox(POINT pt)
{
    RECT b = GetSearchBoxRect();
    return PtInRect(&b, pt) != FALSE;
}

// 只有已输入内容时，右侧图标才变成叉并可点击
static bool HitSearchClear(POINT pt)
{
    if (g_searchText.empty()) return false;
    RECT r = GetSearchClearRect();
    return PtInRect(&r, pt) != FALSE;
}

// 根据光标位置刷新搜索框 / 叉的悬停状态。
// 鼠标移到子窗口 EDIT 上时父窗口会收到 WM_MOUSELEAVE，
// 这里用真实光标位置判断，避免边框在进入 / 离开时闪一下。
static void RefreshSearchHover(POINT pt)
{
    bool hs = HitSearchBox(pt);
    bool hc = HitSearchClear(pt);
    if (hs == g_hoverSearch && hc == g_hoverSearchClear) return;

    g_hoverSearch = hs;
    g_hoverSearchClear = hc;
    if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
}

// 磁贴区可视区域的下边界（扣掉底部搜索框）
static int TileViewBottom()
{
    RECT rc; GetClientRect(g_hWnd, &rc);
    int b = rc.bottom - SearchStripH();
    if (b < TITLE_H + S(40)) b = TITLE_H + S(40);
    if (b > rc.bottom) b = rc.bottom;
    return b;
}

static int TileViewH() { return TileViewBottom() - TITLE_H; }

// ============================================================
//  搜索匹配（不区分大小写；中文等字符按原样比较）
// ============================================================
static bool ItemMatchesSearch(int idx)
{
    if (g_searchText.empty()) return false;
    if (idx < 0 || idx >= (int)g_items.size()) return false;

    const std::wstring& name = g_items[idx].name;
    if (g_searchText.size() > name.size()) return false;

    for (size_t i = 0; i + g_searchText.size() <= name.size(); i++)
    {
        size_t j = 0;
        for (; j < g_searchText.size(); j++)
        {
            if (towlower(name[i + j]) != towlower(g_searchText[j])) break;
        }
        if (j == g_searchText.size()) return true;
    }
    return false;
}

// 搜索命中高亮色（橙黄色）
static const COLORREF SEARCH_HIT_BG = RGB(250, 176, 36);
static const COLORREF SEARCH_HIT_BG_HOT = RGB(255, 199, 76);
static const COLORREF SEARCH_HIT_TEXT = RGB(36, 26, 6);

// ============================================================
//  滚动条位置计算
// ============================================================
static bool GetListScrollBar(RECT& rcTrack, RECT& rcThumb)
{
    RECT rc; GetClientRect(g_hWnd, &rc);
    int viewH = rc.bottom - TITLE_H;
    if (g_listHeight <= viewH) return false;

    int trackX = LIST_W - SCROLLBAR_W - S(4);
    int trackTop = TITLE_H + SCROLLBAR_PAD;
    int trackBot = rc.bottom - SCROLLBAR_PAD;
    if (trackBot - trackTop < SCROLLBAR_MIN_THUMB) return false;

    rcTrack = { trackX, trackTop, trackX + SCROLLBAR_W, trackBot };

    int trackH = rcTrack.bottom - rcTrack.top;
    int thumbH = trackH * viewH / g_listHeight;
    if (thumbH < SCROLLBAR_MIN_THUMB) thumbH = SCROLLBAR_MIN_THUMB;
    if (thumbH > trackH) thumbH = trackH;

    int maxScroll = g_listHeight - viewH;
    int span = trackH - thumbH;
    int thumbY = rcTrack.top + (maxScroll > 0 ? span * g_listScroll / maxScroll : 0);

    rcThumb = { trackX, thumbY, trackX + SCROLLBAR_W, thumbY + thumbH };
    return true;
}

static bool GetTileScrollBar(RECT& rcTrack, RECT& rcThumb)
{
    RECT rc; GetClientRect(g_hWnd, &rc);
    int viewH = TileViewH();
    if (g_tileContentH <= viewH) return false;

    int trackX = rc.right - SCROLLBAR_W - S(4);
    int trackTop = TITLE_H + SCROLLBAR_PAD;
    int trackBot = TileViewBottom() - SCROLLBAR_PAD;
    if (trackBot - trackTop < SCROLLBAR_MIN_THUMB) return false;

    rcTrack = { trackX, trackTop, trackX + SCROLLBAR_W, trackBot };

    int trackH = rcTrack.bottom - rcTrack.top;
    int thumbH = trackH * viewH / g_tileContentH;
    if (thumbH < SCROLLBAR_MIN_THUMB) thumbH = SCROLLBAR_MIN_THUMB;
    if (thumbH > trackH) thumbH = trackH;

    int maxScroll = g_tileContentH - viewH;
    int span = trackH - thumbH;
    int thumbY = rcTrack.top + (maxScroll > 0 ? span * g_tileScroll / maxScroll : 0);

    rcThumb = { trackX, thumbY, trackX + SCROLLBAR_W, thumbY + thumbH };
    return true;
}

// ============================================================
//  数据维护
// ============================================================

// 计算分组内实际占用的最大格数（尾部 -1 不计，返回末尾非空索引 + 1）
static int GetGroupMaxItemCount(const Group& g)
{
    for (int i = (int)g.items.size() - 1; i >= 0; i--) {
        if (g.items[i] != -1) return i + 1;
    }
    return 0;
}

// 根据 maxItemCount 计算分组所需高度（单行及以上）
static int CalcGroupHeight(int maxItemCount)
{
    int rows = maxItemCount > 0 ? ((maxItemCount + MAX_COLS - 1) / MAX_COLS) : 1;
    if (rows > MAX_ROWS) rows = MAX_ROWS;
    return GROUP_HDR + S(2) + rows * TILE_H + (rows - 1) * TILE_GAP + GROUP_PAD;
}

// 从"显示名 + 已展开绝对路径 + 参数"构造一个 Item（AddItem 与 LoadConfig 共用）
static Item BuildItem(const std::wstring& displayName,
    const std::wstring& expandedPath,
    const std::wstring& args)
{
    Item it;
    it.name = displayName.empty()
        ? StripExt(PathFindFileNameW(expandedPath.c_str()))
        : displayName;
    it.path = expandedPath;
    it.args = args;

    // .lnk 快捷方式：解析目标，仅在目标存在时才用目标路径（保持与拖入行为一致）
    if (expandedPath.size() > 4 &&
        _wcsicmp(expandedPath.c_str() + expandedPath.size() - 4, L".lnk") == 0)
    {
        std::wstring t, a, wd;
        if (ResolveShortcut(expandedPath, t, a, wd) && !t.empty())
        {
            if (it.args.empty()) it.args = a;
            if (PathFileExistsW(t.c_str())) it.path = t;
        }
    }
    it.hIcon = GetItemIcon(it.path);
    // 微优化：仅当路径被 .lnk 解析替换后才需要第二次图标查找
    if (!it.hIcon && it.path != expandedPath) it.hIcon = GetItemIcon(expandedPath);
    it.missing = !FileExistsForItem(it);
    return it;
}

static void RemoveEmptyGroups()
{
    g_groups.erase(std::remove_if(g_groups.begin(), g_groups.end(),
        [](const Group& g) {
            for (int x : g.items) {
                if (x != -1) return false;
            }
            return true;
        }), g_groups.end());
}

static int AddItem(const std::wstring& original_path)
{
    g_items.push_back(BuildItem(L"", original_path, L""));
    return (int)g_items.size() - 1;
}

static void RemoveItem(int idx)
{
    if (idx < 0 || idx >= (int)g_items.size()) return;

    for (auto& g : g_groups)
    {
        for (auto& x : g.items)
        {
            if (x == idx) x = -1;
            else if (x > idx) x--;
        }
        while (!g.items.empty() && g.items.back() == -1)
            g.items.pop_back();
    }

    if (g_items[idx].hIcon) DestroyIcon(g_items[idx].hIcon);
    g_items.erase(g_items.begin() + idx);

    RemoveEmptyGroups();
    RebuildList();
    LayoutGroups();
}

static void RebuildList()
{
    g_listRows.clear();
    g_letters.clear();

    std::vector<std::pair<wchar_t, int>> v;
    v.reserve(g_items.size());
    for (int i = 0; i < (int)g_items.size(); i++)
    {
        wchar_t first = FirstLetter(g_items[i].name);
        v.push_back({ first, i });
    }

    std::sort(v.begin(), v.end(), [](const std::pair<wchar_t, int>& a,
        const std::pair<wchar_t, int>& b)
        {
            if (a.first != b.first)
            {
                if (a.first == L'#') return true;
                if (b.first == L'#') return false;
                return a.first < b.first;
            }
            int cmp = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE,
                g_items[a.second].name.c_str(), -1,
                g_items[b.second].name.c_str(), -1, NULL, NULL, 0);
            return cmp == CSTR_LESS_THAN;
        });

    int y = S(8);
    wchar_t last = 0;
    for (size_t i = 0; i < v.size(); i++)
    {
        auto& p = v[i];
        if (p.first != last)
        {
            last = p.first;
            g_letters.push_back(p.first);
            ListRow h; h.header = true; h.letter = p.first; h.itemIdx = -1; h.y = y;
            g_listRows.push_back(h);
            y += LETTER_ROW_H;
        }
        ListRow r; r.header = false; r.letter = p.first; r.itemIdx = p.second; r.y = y;
        g_listRows.push_back(r);
        y += ITEM_H;

        // 同一个字母分组的最后一项后，加点空隙
        if (i == v.size() - 1 || v[i + 1].first != p.first) {
            y += S(12);
        }
    }
    g_listHeight = y + S(16);
}

// ============================================================
//  布局
// ============================================================
static void LayoutGroups()
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    int left = LIST_W + S(20);
    int right = rc.right - S(32);   // 给右侧滚动条留出空间
    if (right < left + S(100)) right = left + S(100);

    int gw = GROUP_PAD * 2 + MAX_COLS * TILE_W + (MAX_COLS - 1) * TILE_GAP;

    // 1. 计算内容总高度
    int tempX = left, tempY = 0, tempRowH = 0;
    for (int gi = 0; gi < (int)g_groups.size(); gi++)
    {
        Group& g = g_groups[gi];
        int maxItemCount = GetGroupMaxItemCount(g);

        // 动态展开：如果是拖拽目标分组，把拖拽目标的位置也纳入高度计算
        if ((g_dragTile || g_dragExternal) && g_dragTargetG == gi && g_dragTargetPos >= 0)
        {
            if (g_dragTargetPos + 1 > maxItemCount)
                maxItemCount = g_dragTargetPos + 1;
        }

        int gh = CalcGroupHeight(maxItemCount);

        if (tempX + gw > right && tempX > left)
        {
            tempX = left;
            tempY += tempRowH + GROUP_GAP;
            tempRowH = 0;
        }
        if (gh > tempRowH) tempRowH = gh;
        tempX += gw + GROUP_GAP;
    }
    g_tileContentH = tempY + tempRowH + S(40);

    // 2. 修正滚动值
    int viewH = TileViewH();
    int maxScroll = g_tileContentH - viewH;
    if (maxScroll < 0) maxScroll = 0;
    if (g_tileScroll > maxScroll) g_tileScroll = maxScroll;
    if (g_tileScroll < 0) g_tileScroll = 0;

    // 3. 实际分配位置
    int top = TITLE_H + S(20) - g_tileScroll;
    int x = left, y = top, rowH = 0;

    for (int gi = 0; gi < (int)g_groups.size(); gi++)
    {
        Group& g = g_groups[gi];
        int maxItemCount = GetGroupMaxItemCount(g);

        if ((g_dragTile || g_dragExternal) && g_dragTargetG == gi && g_dragTargetPos >= 0)
        {
            if (g_dragTargetPos + 1 > maxItemCount)
                maxItemCount = g_dragTargetPos + 1;
        }

        int gh = CalcGroupHeight(maxItemCount);

        if (x + gw > right && x > left)
        {
            x = left;
            y += rowH + GROUP_GAP;
            rowH = 0;
        }

        g.rc = { x, y, x + gw, y + gh };
        g.rcTitle = { x, y, x + gw, y + GROUP_HDR };
        g.rcBody = { x, y + GROUP_HDR, x + gw, y + gh };

        if (gh > rowH) rowH = gh;
        x += gw + GROUP_GAP;
    }
}

static RECT TileRect(int gi, int ii)
{
    Group& g = g_groups[gi];
    int col = ii % MAX_COLS;
    int row = ii / MAX_COLS;
    int x = g.rc.left + GROUP_PAD + col * (TILE_W + TILE_GAP);
    int y = g.rc.top + GROUP_HDR + S(2) + row * (TILE_H + TILE_GAP);
    RECT r = { x, y, x + TILE_W, y + TILE_H };
    return r;
}

// 计算"新建分组"占位符在磁贴区的显示矩形（拖动到空白区域时显示）
// 位置与 LayoutGroups 的排布逻辑保持一致，尺寸与单行默认分组相同
static RECT ComputeNewGroupPlaceholderRect()
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    int left = LIST_W + S(20);
    int right = rc.right - S(32);
    if (right < left + S(100)) right = left + S(100);

    int gw = GROUP_PAD * 2 + MAX_COLS * TILE_W + (MAX_COLS - 1) * TILE_GAP;

    int top = TITLE_H + S(20) - g_tileScroll;
    int x = left, y = top, rowH = 0;

    for (int gi = 0; gi < (int)g_groups.size(); gi++)
    {
        Group& g = g_groups[gi];

        // 拖动源分组若只剩被拖动的那一项，放置后会被移除，这里视为已删除，
        // 让占位符位置与最终布局一致
        if (g_dragTile && gi == g_dragG)
        {
            int nonEmpty = 0;
            for (int i = 0; i < (int)g.items.size(); i++)
                if (g.items[i] != -1 && i != g_dragI) nonEmpty++;
            if (nonEmpty == 0) continue;
        }

        int maxItemCount = GetGroupMaxItemCount(g);

        // 动态展开
        if ((g_dragTile || g_dragExternal) && g_dragTargetG == gi && g_dragTargetPos >= 0)
        {
            if (g_dragTargetPos + 1 > maxItemCount)
                maxItemCount = g_dragTargetPos + 1;
        }

        int gh = CalcGroupHeight(maxItemCount);

        if (x + gw > right && x > left)
        {
            x = left;
            y += rowH + GROUP_GAP;
            rowH = 0;
        }
        if (gh > rowH) rowH = gh;
        x += gw + GROUP_GAP;
    }

    // 新分组默认高度：单行
    int gh = CalcGroupHeight(0);

    if (x + gw > right && x > left)
    {
        x = left;
        y += rowH + GROUP_GAP;
    }

    RECT r = { x, y, x + gw, y + gh };
    return r;
}

static void GetLetterRects(std::vector<RECT>& out)
{
    out.clear();
    int n = (int)g_letters.size();
    if (n == 0) return;

    RECT rc; GetClientRect(g_hWnd, &rc);
    int x = S(4);
    int y0 = TITLE_H + S(8);
    int avail = rc.bottom - y0 - S(8);
    if (avail < S(20)) avail = S(20);

    int h = avail / n;
    if (h > S(24)) h = S(24);
    if (h < S(10)) h = S(10);
    int total = h * n;
    int y = y0 + (avail - total) / 2;

    for (int i = 0; i < n; i++)
    {
        RECT r = { x, y, x + LETTER_W, y + h };
        out.push_back(r);
        y += h;
    }
}

// ============================================================
//  命中测试
// ============================================================
static int HitButton(POINT pt)
{
    RECT rc; GetClientRect(g_hWnd, &rc);
    if (pt.y < 0 || pt.y >= TITLE_H) return 0;
    if (pt.x < 0) return 0;
    // 从右到左依次判断：关闭(3) / 最大化(2) / 最小化(1)
    for (int b = 3; b >= 1; b--)
    {
        if (pt.x >= rc.right - BTN_W * (4 - b)) return b;
    }
    return 0;
}

static int HitLetter(POINT pt)
{
    std::vector<RECT> v;
    GetLetterRects(v);
    for (int i = 0; i < (int)v.size(); i++)
        if (PtInRect(&v[i], pt)) return i;
    return -1;
}

static int HitListItem(POINT pt)
{
    if (pt.x < LETTER_W + S(4)) return -1;
    if (pt.x >= LIST_W - SCROLLBAR_W - S(4)) return -1;
    if (pt.y < TITLE_H) return -1;
    int ly = pt.y - TITLE_H + g_listScroll;
    for (int i = 0; i < (int)g_listRows.size(); i++)
    {
        int h = g_listRows[i].header ? LETTER_ROW_H : ITEM_H;
        if (ly >= g_listRows[i].y && ly < g_listRows[i].y + h)
            return g_listRows[i].header ? -1 : i;
    }
    return -1;
}

static bool HitTile(POINT pt, int& gi, int& ii)
{
    gi = ii = -1;
    for (int g = 0; g < (int)g_groups.size(); g++)
    {
        for (int i = 0; i < (int)g_groups[g].items.size(); i++)
        {
            if (g_groups[g].items[i] == -1) continue;
            RECT r = TileRect(g, i);
            if (PtInRect(&r, pt)) { gi = g; ii = i; return true; }
        }
    }
    return false;
}

static int HitGroupTitle(POINT pt)
{
    for (int i = 0; i < (int)g_groups.size(); i++)
        if (PtInRect(&g_groups[i].rcTitle, pt)) return i;
    return -1;
}

// ============================================================
//  启动项
// ============================================================
static void LaunchItem(int idx)
{
    if (!EnsureItemAvailable(idx)) return;

    Item& it = g_items[idx];
    std::wstring full = ExpandPath(it.path);
    HINSTANCE r = ShellExecuteW(nullptr, L"open", full.c_str(),
        it.args.empty() ? nullptr : it.args.c_str(),
        nullptr, SW_SHOWNORMAL);
    if ((INT_PTR)r <= 32)
    {
        r = ShellExecuteW(nullptr, L"open", it.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    // 托盘模式：从程序内启动快捷方式后自动隐藏主窗口
    if (g_trayMode && (INT_PTR)r > 32)
        HideMainWindow();
}

static void OpenFileLocation(const std::wstring& path)
{
    std::wstring full = ExpandPath(path);
    std::wstring args = L"/select,\"" + full + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
}

// ============================================================
//  绘制
// ============================================================
static void PaintTitleBar(HDC hdc)
{
    RECT rc; GetClientRect(g_hWnd, &rc);
    RECT rt = { 0, 0, rc.right, TITLE_H };

    HBRUSH br = CreateSolidBrush(g_uiTitle);
    FillRect(hdc, &rt, br);
    DeleteObject(br);

    // 标题栏图标调大到 24x24
    int iconSize = S(24);
    if (g_hIconBig)
        DrawIconEx(hdc, S(12), (TITLE_H - iconSize) / 2, g_hIconBig, iconSize, iconSize, 0, nullptr, DI_NORMAL);
    else if (g_hIconSmall)
        DrawIconEx(hdc, S(12), (TITLE_H - iconSize) / 2, g_hIconSmall, iconSize, iconSize, 0, nullptr, DI_NORMAL);

    // 文字起点向后移动，留出图标宽度+8px的间隙
    RECT rtt = { S(44), 0, rc.right - BTN_W * 3 - S(10), TITLE_H };
    DrawText2(hdc, L"开始菜单", rtt, g_uiText, g_fontTitle,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    for (int b = 1; b <= 3; b++)
    {
        RECT rb = { rc.right - BTN_W * (4 - b), 0, rc.right - BTN_W * (3 - b), TITLE_H };
        if (g_hoverBtn == b)
        {
            COLORREF c = (b == 3) ? RGB(196, 43, 28) : g_uiTitleHot;
            HBRUSH hb = CreateSolidBrush(c);
            FillRect(hdc, &rb, hb);
            DeleteObject(hb);
        }

        int cx = (rb.left + rb.right) / 2;
        int cy = (rb.top + rb.bottom) / 2;
        HPEN pn = CreatePen(PS_SOLID, 1, g_uiText);
        HGDIOBJ op = SelectObject(hdc, pn);
        HGDIOBJ ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));

        if (b == 1)
        {
            MoveToEx(hdc, cx - S(5), cy + S(4), nullptr);
            LineTo(hdc, cx + S(5), cy + S(4));
        }
        else if (b == 2)
        {
            if (IsZoomed(g_hWnd))
            {
                // 资源管理器经典的还原按钮样式
                MoveToEx(hdc, cx - S(3), cy - S(6), nullptr);
                LineTo(hdc, cx + S(6), cy - S(6));
                LineTo(hdc, cx + S(6), cy + S(3));

                Rectangle(hdc, cx - S(6), cy - S(3), cx + S(3), cy + S(6));
            }
            else
            {
                Rectangle(hdc, cx - S(5), cy - S(5), cx + S(5), cy + S(5));
            }
        }
        else
        {
            // 补上终点像素，确保交叉点绝对居中
            MoveToEx(hdc, cx - S(5), cy - S(5), nullptr); LineTo(hdc, cx + S(5) + 1, cy + S(5) + 1);
            MoveToEx(hdc, cx + S(5), cy - S(5), nullptr); LineTo(hdc, cx - S(5) - 1, cy + S(5) + 1);
        }

        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(pn);
    }

    HPEN p = CreatePen(PS_SOLID, 1, g_uiSeparator);
    HGDIOBJ op = SelectObject(hdc, p);
    MoveToEx(hdc, 0, TITLE_H - 1, nullptr);
    LineTo(hdc, rc.right, TITLE_H - 1);
    SelectObject(hdc, op);
    DeleteObject(p);
}

static void PaintList(HDC hdc)
{
    RECT rc; GetClientRect(g_hWnd, &rc);
    RECT rl = { 0, TITLE_H, LIST_W, rc.bottom };

    HBRUSH br = CreateSolidBrush(g_uiList);
    FillRect(hdc, &rl, br);
    DeleteObject(br);

    HRGN clip = CreateRectRgn(rl.left, rl.top, rl.right, rl.bottom);
    SelectClipRgn(hdc, clip);

    int itemRight = LIST_W - SCROLLBAR_W - S(8);

    for (int i = 0; i < (int)g_listRows.size(); i++)
    {
        ListRow& r = g_listRows[i];
        int y = TITLE_H + r.y - g_listScroll;
        if (y < TITLE_H - S(50) || y > rc.bottom) continue;

        if (r.header)
        {
            RECT rh = { LETTER_W + S(12), y, itemRight, y + LETTER_ROW_H };
            DrawText2(hdc, std::wstring(1, r.letter), rh, g_uiTextFaint,
                g_fontSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        else
        {
            RECT ri = { LETTER_W + S(8), y, itemRight, y + ITEM_H };
            Item& it = g_items[r.itemIdx];
            bool hit = ItemMatchesSearch(r.itemIdx);

            // 文件缺失：背景变深红色（优先于搜索高亮）
            if (it.missing)
            {
                COLORREF col = (i == g_hoverListIdx) ? RGB(180, 50, 50) : RGB(145, 35, 35);
                FillRoundRect(hdc, ri, S(4), col, col);
            }
            else if (hit)
            {
                // 搜索命中：背景变为橙黄色
                COLORREF col = (i == g_hoverListIdx) ? SEARCH_HIT_BG_HOT : SEARCH_HIT_BG;
                FillRoundRect(hdc, ri, S(4), col, col);
            }
            else if (i == g_hoverListIdx)
            {
                FillRoundRect(hdc, ri, S(4), g_uiHover, g_uiHover);
            }

            if (it.hIcon)
            {
                int sz = S(32); // 原尺寸绘制 32x32
                DrawIconEx(hdc, ri.left + S(8), ri.top + (ITEM_H - sz) / 2,
                    it.hIcon, sz, sz, 0, nullptr, DI_NORMAL);
            }

            RECT rt = { ri.left + S(48), ri.top, ri.right - S(6), ri.bottom };
            COLORREF tcol = (hit && !it.missing) ? SEARCH_HIT_TEXT : g_uiText;
            DrawText2(hdc, it.name, rt, tcol, g_fontUI,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }

    std::vector<RECT> lr;
    GetLetterRects(lr);
    for (int i = 0; i < (int)lr.size(); i++)
    {
        COLORREF c = (i == g_hoverLetter) ? RGB(90, 160, 255) : g_uiTextFaint;
        DrawText2(hdc, std::wstring(1, g_letters[i]), lr[i], c,
            g_fontSmall, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);

    HPEN p = CreatePen(PS_SOLID, 1, g_uiSeparator);
    HGDIOBJ op = SelectObject(hdc, p);
    MoveToEx(hdc, LIST_W - 1, TITLE_H, nullptr);
    LineTo(hdc, LIST_W - 1, rc.bottom);
    SelectObject(hdc, op);
    DeleteObject(p);
}

static void PaintTiles(HDC hdc)
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    // 底部留给搜索框，磁贴不绘制到搜索框区域
    HRGN clip = CreateRectRgn(LIST_W, TITLE_H, rc.right, TileViewBottom());
    SelectClipRgn(hdc, clip);

    for (int gi = 0; gi < (int)g_groups.size(); gi++)
    {
        Group& g = g_groups[gi];

        FillRoundRect(hdc, g.rc, S(10), g_uiCard, g_uiCard);

        // 标题栏文字：上下左右居中
        RECT rt = { g.rc.left + GROUP_PAD, g.rcTitle.top,
            g.rc.right - GROUP_PAD, g.rcTitle.bottom + S(2) };
        DrawText2(hdc, g.name, rt,
            g_uiTextDim, g_fontSmall,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        for (int ii = 0; ii < (int)g.items.size(); ii++)
        {
            if (g.items[ii] == -1) continue;
            if (g_dragTile && g_dragG == gi && g_dragI == ii) continue;

            RECT r = TileRect(gi, ii);
            if (r.top > rc.bottom || r.bottom < TITLE_H) continue;

            Item& it = g_items[g.items[ii]];

            bool hot = (g_hoverTileG == gi && g_hoverTileI == ii);
            bool hit = ItemMatchesSearch(g.items[ii]);
            COLORREF col;
            // 文件缺失：背景变深红色（优先于搜索高亮）
            if (it.missing)
            {
                col = hot ? RGB(180, 50, 50) : RGB(145, 35, 35);
            }
            else if (hit)
            {
                // 搜索命中：背景变为橙黄色
                col = hot ? SEARCH_HIT_BG_HOT : SEARCH_HIT_BG;
            }
            else
            {
                col = hot ? g_uiTileHot : g_uiTile;
            }
            FillRoundRect(hdc, r, S(6), col, col);

            if (it.hIcon)
            {
                int sz = S(32);
                DrawIconEx(hdc, r.left + (TILE_W - sz) / 2, r.top + S(12),
                    it.hIcon, sz, sz, 0, nullptr, DI_NORMAL);
            }

            RECT rt2 = { r.left + S(4), r.bottom - S(24), r.right - S(4), r.bottom - S(4) };
            COLORREF tcol = (hit && !it.missing) ? SEARCH_HIT_TEXT : g_uiText;
            DrawText2(hdc, it.name, rt2, tcol, g_fontSmall,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }

    // ========================================================
    //  "新建分组"占位符（拖动磁贴 / 外部文件拖入 到空白区域时显示）
    //  样式：与单行默认分组大小一致，蓝色边框 + 中央加号
    // ========================================================
    if ((g_dragTile || g_dragExternal) && g_dragNewGroup)
    {
        RECT r = ComputeNewGroupPlaceholderRect();
        if (r.bottom > TITLE_H && r.top < rc.bottom)
        {
            // 淡蓝填充
            HBRUSH br = CreateSolidBrush(RGB(28, 50, 72));
            FillRect(hdc, &r, br);
            DeleteObject(br);

            // 蓝色边框
            HPEN pn = CreatePen(PS_SOLID, S(2), RGB(0, 174, 255));
            HGDIOBJ op = SelectObject(hdc, pn);
            HGDIOBJ ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1);
            SelectObject(hdc, ob);
            SelectObject(hdc, op);
            DeleteObject(pn);

            // 中间加号
            int cx = (r.left + r.right) / 2;
            int cy = (r.top + r.bottom) / 2;
            int sz = S(20);
            HPEN pn2 = CreatePen(PS_SOLID, S(3), RGB(0, 174, 255));
            HGDIOBJ op2 = SelectObject(hdc, pn2);
            MoveToEx(hdc, cx - sz / 2, cy, nullptr);
            LineTo(hdc, cx + sz / 2 + 1, cy);
            MoveToEx(hdc, cx, cy - sz / 2, nullptr);
            LineTo(hdc, cx, cy + sz / 2 + 1);
            SelectObject(hdc, op2);
            DeleteObject(pn2);
        }
    }

    if ((g_dragTile || g_dragExternal) &&
        g_dragTargetG >= 0 && g_dragTargetG < (int)g_groups.size())
    {
        Group& g = g_groups[g_dragTargetG];
        int pos = g_dragTargetPos;
        int maxCells = MAX_COLS * MAX_ROWS;
        if (pos >= maxCells) pos = maxCells - 1;

        if (pos >= 0)
        {
            int col = pos % MAX_COLS;
            int row = pos / MAX_COLS;
            int x = g.rc.left + GROUP_PAD + col * (TILE_W + TILE_GAP);
            int y = g.rc.top + GROUP_HDR + S(2) + row * (TILE_H + TILE_GAP);

            if (g_dragTargetMode == 2)
            {
                RECT r = { x, y, x + TILE_W, y + TILE_H };

                HBRUSH br = CreateSolidBrush(RGB(30, 65, 95));
                FillRect(hdc, &r, br);
                DeleteObject(br);

                HPEN pn = CreatePen(PS_SOLID, S(2), RGB(0, 174, 255));
                HGDIOBJ op = SelectObject(hdc, pn);
                HGDIOBJ ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1);
                SelectObject(hdc, ob);
                SelectObject(hdc, op);
                DeleteObject(pn);
            }
            else if (g_dragTargetMode == 1)
            {
                int lx = x - S(2);
                RECT r = { lx, y, lx + S(3), y + TILE_H };
                HBRUSH br = CreateSolidBrush(RGB(0, 174, 255));
                FillRect(hdc, &r, br);
                DeleteObject(br);
            }
        }
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);
}

// ============================================================
//  搜索框绘制（两端半圆的胶囊形）
// ============================================================
// 点到线段的距离（用于绘制抗锯齿的放大镜手柄、叉的两条斜线）
static double DistToSegment(double px, double py,
    double ax, double ay, double bx, double by)
{
    double dx = bx - ax, dy = by - ay;
    double len2 = dx * dx + dy * dy;
    double t = 0.0;
    if (len2 > 0.0) t = ((px - ax) * dx + (py - ay) * dy) / len2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    double qx = ax + t * dx, qy = ay + t * dy;
    double ddx = px - qx, ddy = py - qy;
    return sqrt(ddx * ddx + ddy * ddy);
}

// 把到图形中心线的距离折算成覆盖率（0~1），让边缘平滑过渡
static double ShapeCoverage(double dist, double halfWidth)
{
    double v = halfWidth + 0.5 - dist;
    if (v <= 0.0) return 0.0;
    if (v >= 1.0) return 1.0;
    return v;
}

// 通用抗锯齿图标绘制：逐像素 4x4 超采样，按覆盖率与背景混色
static void DrawAAIcon(HDC hdc, const RECT& rc, COLORREF fg, COLORREF bg,
    const std::function<double(double, double)>& cover)
{
    const int SS = 4;
    const double inv = 1.0 / (SS * SS);

    for (int y = rc.top; y < rc.bottom; y++)
    {
        for (int x = rc.left; x < rc.right; x++)
        {
            double sum = 0.0;
            for (int sy = 0; sy < SS; sy++)
            {
                for (int sx = 0; sx < SS; sx++)
                {
                    double px = x + (sx + 0.5) / SS;
                    double py = y + (sy + 0.5) / SS;
                    sum += cover(px, py);
                }
            }

            double a = sum * inv;
            if (a <= 0.002) continue;
            if (a > 1.0) a = 1.0;

            int r = (int)(GetRValue(bg) * (1.0 - a) + GetRValue(fg) * a + 0.5);
            int g = (int)(GetGValue(bg) * (1.0 - a) + GetGValue(fg) * a + 0.5);
            int b = (int)(GetBValue(bg) * (1.0 - a) + GetBValue(fg) * a + 0.5);
            SetPixelV(hdc, x, y, RGB(r, g, b));
        }
    }
}

// 放大镜图标（抗锯齿）：圆环 + 右下方手柄
static void DrawSearchIconAA(HDC hdc, const RECT& rc, COLORREF fg, COLORREF bg)
{
    double w = rc.right - rc.left;
    double h = rc.bottom - rc.top;
    if (w <= 2 || h <= 2) return;

    double cx = rc.left + w * 0.42;
    double cy = rc.top + h * 0.42;
    double rad = w * 0.27;
    double half = w * 0.055;          // 线宽的一半
    if (half < 0.5) half = 0.5;

    double hx0 = cx + rad * 0.70, hy0 = cy + rad * 0.70;
    double hx1 = rc.right - w * 0.10, hy1 = rc.bottom - h * 0.10;

    DrawAAIcon(hdc, rc, fg, bg, [=](double px, double py) -> double {
        double ddx = px - cx, ddy = py - cy;
        double dRing = fabs(sqrt(ddx * ddx + ddy * ddy) - rad);
        double dSeg = DistToSegment(px, py, hx0, hy0, hx1, hy1);
        double c1 = ShapeCoverage(dRing, half);
        double c2 = ShapeCoverage(dSeg, half);
        return c1 > c2 ? c1 : c2;
    });
}

// 叉图标（抗锯齿）：两条对角线
static void DrawClearIconAA(HDC hdc, const RECT& rc, COLORREF fg, COLORREF bg)
{
    double w = rc.right - rc.left;
    double h = rc.bottom - rc.top;
    if (w <= 2 || h <= 2) return;

    double cx = rc.left + w / 2.0;
    double cy = rc.top + h / 2.0;
    double a = w * 0.26;
    double half = w * 0.055;
    if (half < 0.5) half = 0.5;

    DrawAAIcon(hdc, rc, fg, bg, [=](double px, double py) -> double {
        double d1 = DistToSegment(px, py, cx - a, cy - a, cx + a, cy + a);
        double d2 = DistToSegment(px, py, cx + a, cy - a, cx - a, cy + a);
        double c1 = ShapeCoverage(d1, half);
        double c2 = ShapeCoverage(d2, half);
        return c1 > c2 ? c1 : c2;
    });
}

static inline double Clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

// 齿轮图标（抗锯齿）：8 个圆角齿 + 中心圆孔
static void DrawGearIconAA(HDC hdc, const RECT& rc, COLORREF fg, COLORREF bg)
{
    double w = rc.right - rc.left;
    double h = rc.bottom - rc.top;
    if (w <= 4.0 || h <= 4.0) return;

    double cx = rc.left + w / 2.0;
    double cy = rc.top + h / 2.0;
    double rBase = w * 0.30;      // 齿根半径
    double rTooth = w * 0.095;    // 齿高
    double rHole = w * 0.115;     // 中心孔半径

    DrawAAIcon(hdc, rc, fg, bg, [=](double px, double py) -> double {
        double dx = px - cx, dy = py - cy;
        double d = sqrt(dx * dx + dy * dy);
        double ang = atan2(dy, dx);

        // 用 cos 的平滑调制生成 8 个圆角齿
        double t = (cos(ang * 8.0) + 1.0) * 0.5;
        double tooth = t * t * t;
        double rOut = rBase + rTooth * tooth;

        double covOuter = Clamp01(rOut - d + 0.5);
        double covHole = Clamp01(d - rHole + 0.5);
        return covOuter * covHole;
    });
}

// 抗锯齿胶囊形：左右两端为半圆，1px 边框也做抗锯齿。
// 背景固定为窗口底色，直接在内存 DC 上按覆盖率混色。
static void FillCapsuleAA(HDC hdc, const RECT& rc,
    COLORREF fill, COLORREF border, COLORREF bg)
{
    double h = rc.bottom - rc.top;
    if (h <= 2.0) return;

    double rad = h / 2.0;
    double cy = (rc.top + rc.bottom) / 2.0;
    double ax = rc.left + rad;
    double bx = rc.right - rad;
    if (bx < ax) { ax = bx = (rc.left + rc.right) / 2.0; }

    const double bw = 1.0;   // 边框宽度

    for (int y = rc.top; y < rc.bottom; y++)
    {
        for (int x = rc.left; x < rc.right; x++)
        {
            double d = DistToSegment(x + 0.5, y + 0.5, ax, cy, bx, cy);

            // 外形（含边框外沿）与内形（填充区域）的覆盖率
            double outer = Clamp01(rad - d + 0.5);
            if (outer <= 0.0) continue;
            double inner = Clamp01((rad - bw) - d + 0.5);

            double r = GetRValue(bg) * (1.0 - outer) + GetRValue(border) * outer;
            double g = GetGValue(bg) * (1.0 - outer) + GetGValue(border) * outer;
            double b = GetBValue(bg) * (1.0 - outer) + GetBValue(border) * outer;

            r = r * (1.0 - inner) + GetRValue(fill) * inner;
            g = g * (1.0 - inner) + GetGValue(fill) * inner;
            b = b * (1.0 - inner) + GetBValue(fill) * inner;

            SetPixelV(hdc, x, y, RGB((int)(r + 0.5), (int)(g + 0.5), (int)(b + 0.5)));
        }
    }
}

static void PaintSearchBox(HDC hdc)
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    HRGN clip = CreateRectRgn(LIST_W, TITLE_H, rc.right, rc.bottom);
    SelectClipRgn(hdc, clip);

    RECT box = GetSearchBoxRect();
    const COLORREF fill = g_uiSurface;

    bool focused = (g_hSearchEdit != nullptr && GetFocus() == g_hSearchEdit);
    COLORREF border;
    if (focused) border = RGB(0, 174, 255);
    else if (g_hoverSearch) border = g_uiSurfaceBorderHover;
    else border = g_uiSurfaceBorder;

    // 胶囊形底：左右两端为半圆。
    // 用逐像素覆盖率做抗锯齿（GDI 的 RoundRect 两端是硬锯齿）
    FillCapsuleAA(hdc, box, fill, border, g_themeBg);

    // 右侧图标：无内容时为放大镜；输入内容后自动变成叉
    RECT ir = GetSearchIconRect();
    if (g_searchText.empty())
    {
        COLORREF ic = g_hoverSearch ? g_uiText : g_uiIcon;
        DrawSearchIconAA(hdc, ir, ic, fill);
    }
    else
    {
        COLORREF ic;
        if (g_hoverSearchClear) ic = RGB(255, 108, 88);
        else if (g_hoverSearch) ic = g_uiText;
        else ic = g_uiIcon;
        DrawClearIconAA(hdc, ir, ic, fill);
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);
}

// 搜索框左侧的圆形齿轮按钮（圆形 + 齿轮均抗锯齿）
static void PaintSettingsButton(HDC hdc)
{
    RECT r = GetSettingsButtonRect();
    int d = r.right - r.left;
    int isz = S(GEAR_ICON_BASE);

    const COLORREF bg = g_themeBg;
    const COLORREF fill = g_uiSurface;
    COLORREF border = g_hoverSettingsBtn ? g_uiSurfaceBorderHover : g_uiSurfaceBorder;

    // 与搜索框完全相同的填充与边框，只是形状为圆形
    FillCapsuleAA(hdc, r, fill, border, bg);

    RECT ir = { r.left + (d - isz) / 2, r.top + (d - isz) / 2,
                r.left + (d - isz) / 2 + isz, r.top + (d - isz) / 2 + isz };
    COLORREF ic = g_hoverSettingsBtn ? g_uiText : g_uiIcon;
    DrawGearIconAA(hdc, ir, ic, fill);
}

// ============================================================
//  绘制滚动条
// ============================================================
static void PaintOneScrollBar(HDC hdc, const RECT& rcTrack, const RECT& rcThumb, bool dragging)
{
    FillRoundRect(hdc, rcTrack, SCROLLBAR_W, g_uiScrollTrack, g_uiScrollTrack);
    COLORREF tc = dragging ? g_uiScrollThumbHot : g_uiScrollThumb;
    FillRoundRect(hdc, rcThumb, SCROLLBAR_W, tc, tc);
}

static void PaintScrollBars(HDC hdc)
{
    RECT rcTrack, rcThumb;

    // 列表滚动条
    if (GetListScrollBar(rcTrack, rcThumb))
        PaintOneScrollBar(hdc, rcTrack, rcThumb, g_listScrollDrag);

    // 磁贴滚动条
    if (GetTileScrollBar(rcTrack, rcThumb))
        PaintOneScrollBar(hdc, rcTrack, rcThumb, g_tileScrollDrag);
}

static void PaintDragGhost(HDC hdc)
{
    RECT r = g_dragRect;
    FillRoundRect(hdc, r, S(6), g_uiBtnHot, g_uiSurfaceBorderHover);

    if (g_dragG >= 0 && g_dragG < (int)g_groups.size() &&
        g_dragI >= 0 && g_dragI < (int)g_groups[g_dragG].items.size())
    {
        int id = g_groups[g_dragG].items[g_dragI];
        if (id >= 0 && id < (int)g_items.size()) {
            Item& it = g_items[id];
            if (it.hIcon)
            {
                int sz = S(32);
                DrawIconEx(hdc, r.left + (TILE_W - sz) / 2, r.top + S(12),
                    it.hIcon, sz, sz, 0, nullptr, DI_NORMAL);
            }
            RECT rt = { r.left + S(4), r.bottom - S(24), r.right - S(4), r.bottom - S(4) };
            DrawText2(hdc, it.name, rt, g_uiText, g_fontSmall,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }
}

static void Paint(HDC hdc)
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    HBRUSH bg = CreateSolidBrush(g_themeBg);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    PaintTitleBar(hdc);
    PaintList(hdc);
    PaintTiles(hdc);
    PaintScrollBars(hdc);
    PaintSearchBox(hdc);
    PaintSettingsButton(hdc);

    if (g_dragTile) PaintDragGhost(hdc);

    // ========================================================
    //  灰色边框（窗口外圈 1px；最大化时贴屏幕边，不绘制）
    // ========================================================
    if (!IsZoomed(g_hWnd))
    {
        HBRUSH br = CreateSolidBrush(g_uiSeparator);
        FrameRect(hdc, &rc, br);
        DeleteObject(br);
    }
}

// ============================================================
//  右键菜单窗口
// ============================================================
static void DoMenuAction(int itemIdx, int action)
{
    if (itemIdx < 0 || itemIdx >= (int)g_items.size()) return;

    if (action == 0)
    {
        ShowRenameDialog(itemIdx);
    }
    else if (action == 1)
    {
        // 复用统一的存在性检查与缺失弹窗
        if (!EnsureItemAvailable(itemIdx)) return;
        OpenFileLocation(g_items[itemIdx].path);
    }
    else if (action == 2)
    {
        RemoveItem(itemIdx);   // RemoveItem 内部已调用 LayoutGroups
        SaveConfig();          // ★ 新增：移除快捷方式后立即写入 cfg，不必等关闭程序
        InvalidateRect(g_hWnd, nullptr, FALSE);
    }
}

static LRESULT CALLBACK MenuProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        FillRoundRect(hdc, rc, S(8), g_uiCard, g_uiSurfaceBorder);

        for (int i = 0; i < (int)g_menuItems.size(); i++)
        {
            RECT ri = { MENU_PAD, MENU_PAD + i * MENU_ITEM_H,
                        rc.right - MENU_PAD, MENU_PAD + (i + 1) * MENU_ITEM_H };
            if (i == g_menuHover)
                FillRoundRect(hdc, ri, S(4), g_uiHover, g_uiHover);

            RECT rt = { ri.left + S(12), ri.top, ri.right - S(8), ri.bottom };
            DrawText2(hdc, g_menuItems[i], rt, g_uiText, g_fontUI,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int idx = (pt.y - MENU_PAD) / MENU_ITEM_H;
        if (pt.y < MENU_PAD) idx = -1;
        if (idx >= (int)g_menuItems.size()) idx = -1;
        if (idx != g_menuHover)
        {
            g_menuHover = idx;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int idx = (pt.y - MENU_PAD) / MENU_ITEM_H;
        if (pt.y < MENU_PAD) idx = -1;
        if (idx >= (int)g_menuItems.size()) idx = -1;

        if (idx < 0)
        {
            if (g_hMenu == hwnd) g_hMenu = nullptr;
            DestroyWindow(hwnd);
            return 0;
        }

        g_menuPressedIdx = idx;
        if (GetCapture() != hwnd) SetCapture(hwnd);
        return 0;
    }
    case WM_LBUTTONUP:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int idx = (pt.y - MENU_PAD) / MENU_ITEM_H;
        if (pt.y < MENU_PAD) idx = -1;
        if (idx >= (int)g_menuItems.size()) idx = -1;

        int pressed = g_menuPressedIdx;
        g_menuPressedIdx = -1;

        bool hit = (idx >= 0 && idx == pressed);

        if (hit)
        {
            int target = g_menuTargetItem;
            int action = idx;
            int mode = g_menuMode;

            if (g_hMenu == hwnd) g_hMenu = nullptr;
            ShowWindow(hwnd, SW_HIDE);
            if (IsWindow(hwnd)) DestroyWindow(hwnd);

            if (mode == 1)
            {
                // 托盘菜单：打开窗口 / 退出托盘模式 / 退出
                if (action == 0)
                {
                    ShowMainWindow();
                }
                else if (action == 1)
                {
                    ApplyTrayMode(false);
                    SaveConfig();
                    ShowMainWindow();
                }
                else if (action == 2)
                {
                    ExitApplication();
                }
            }
            else
            {
                DoMenuAction(target, action);
            }
        }
        else
        {
            if (g_hMenu == hwnd) g_hMenu = nullptr;
            if (IsWindow(hwnd)) DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CAPTURECHANGED:
        g_menuPressedIdx = -1;
        if ((HWND)lp != hwnd && IsWindow(hwnd) && g_hMenu == hwnd)
            DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (g_hMenu == hwnd) g_hMenu = nullptr;
        g_menuPressedIdx = -1;
        g_menuTargetItem = -1;   // 清理残留
        g_menuMode = 0;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void ShowContextMenu(POINT ptClient)
{
    if (g_hMenu) { DestroyWindow(g_hMenu); g_hMenu = nullptr; }

    g_menuItems = { L"重命名", L"打开文件位置", L"移除快捷方式" };
    g_menuHover = -1;
    g_menuPressedIdx = -1;
    g_menuMode = 0;

    POINT pt = ptClient;
    ClientToScreen(g_hWnd, &pt);

    int w = MENU_W;
    int h = MENU_PAD * 2 + MENU_ITEM_H * (int)g_menuItems.size();

    // 去除 WS_EX_TOPMOST，防止右键菜单强行置顶
    g_hMenu = CreateWindowExW(WS_EX_TOOLWINDOW,
        L"SMRightMenu", L"", WS_POPUP,
        pt.x, pt.y, w, h, g_hWnd, nullptr, g_hInst, nullptr);

    if (g_hMenu)
    {
        ShowWindow(g_hMenu, SW_SHOW);
        SetCapture(g_hMenu);
    }
}

// 托盘图标菜单：打开窗口 / 退出（左键、右键点击托盘图标都弹出这个菜单）
static void ShowTrayMenu(POINT ptScreen)
{
    if (g_hMenu) { DestroyWindow(g_hMenu); g_hMenu = nullptr; }

    g_menuItems = { L"打开窗口", L"退出托盘模式", L"退出" };
    g_menuHover = -1;
    g_menuPressedIdx = -1;
    g_menuTargetItem = -1;
    g_menuMode = 1;

    int w = MENU_W;
    int h = MENU_PAD * 2 + MENU_ITEM_H * (int)g_menuItems.size();

    // 保证菜单完整显示在屏幕工作区内
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromPoint(ptScreen, MONITOR_DEFAULTTONEAREST), &mi);
    int x = ptScreen.x, y = ptScreen.y;
    if (x + w > mi.rcWork.right) x = mi.rcWork.right - w;
    if (y + h > mi.rcWork.bottom) y = mi.rcWork.bottom - h;
    if (x < mi.rcWork.left) x = mi.rcWork.left;
    if (y < mi.rcWork.top) y = mi.rcWork.top;

    g_hMenu = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"SMRightMenu", L"", WS_POPUP,
        x, y, w, h, g_hWnd, nullptr, g_hInst, nullptr);

    if (g_hMenu)
    {
        ShowWindow(g_hMenu, SW_SHOW);
        SetForegroundWindow(g_hMenu);
        SetCapture(g_hMenu);
    }
}

// ============================================================
//  统一激活并前置主窗口
//  弹窗关闭后调用，确保主窗口不会被其它程序的窗口挡住
// ============================================================
static void ActivateMainWindow()
{
    if (!g_hWnd) return;

    EnableWindow(g_hWnd, TRUE);

    if (IsIconic(g_hWnd))
        ShowWindow(g_hWnd, SW_RESTORE);

    // 先尝试普通激活
    SetForegroundWindow(g_hWnd);
    BringWindowToTop(g_hWnd);

    // 如果主窗口仍未获得焦点（可能被其它进程的前台窗口抢占），
    // 则使用 AttachThreadInput 短暂附加到前台线程进行强制激活
    if (GetForegroundWindow() != g_hWnd)
    {
        DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        DWORD myThread = GetCurrentThreadId();
        if (fgThread && fgThread != myThread)
        {
            AttachThreadInput(myThread, fgThread, TRUE);
            SetForegroundWindow(g_hWnd);
            BringWindowToTop(g_hWnd);
            AttachThreadInput(myThread, fgThread, FALSE);
        }
    }

    // 确保主窗口在 Z 序顶部
    SetWindowPos(g_hWnd, HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // 标记重绘（不强制立即刷新，避免闪烁）
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

// ============================================================
//  托盘模式
// ============================================================
static void ShowMainWindow()
{
    if (!g_hWnd) return;

    if (IsIconic(g_hWnd)) ShowWindow(g_hWnd, SW_RESTORE);
    else ShowWindow(g_hWnd, SW_SHOW);

    // 托盘模式下的显示通常由全局热键 / 托盘菜单触发，
    // 进程此时不是前台进程，SetForegroundWindow 可能失败。
    // 若不强制激活窗口，它就一直是“未激活”状态，
    // 此时点击窗口外不会产生 WM_ACTIVATE(WA_INACTIVE)，也就不会自动隐藏。
    SetForegroundWindow(g_hWnd);
    BringWindowToTop(g_hWnd);

    if (GetForegroundWindow() != g_hWnd)
    {
        DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        DWORD myThread = GetCurrentThreadId();
        if (fgThread && fgThread != myThread)
        {
            AttachThreadInput(myThread, fgThread, TRUE);
            SetForegroundWindow(g_hWnd);
            BringWindowToTop(g_hWnd);
            AttachThreadInput(myThread, fgThread, FALSE);
        }
    }

    SetWindowPos(g_hWnd, HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE);

    // 已经主动激活过，允许“点击窗口外自动隐藏”
    g_mainWasActive = true;
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

static void HideMainWindow()
{
    if (!g_hWnd) return;
    if (g_hMenu) { DestroyWindow(g_hMenu); g_hMenu = nullptr; }
    ShowWindow(g_hWnd, SW_HIDE);
    g_mainWasActive = false;
}

static void ToggleMainWindow()
{
    if (!g_hWnd) return;
    if (IsWindowVisible(g_hWnd)) HideMainWindow();
    else ShowMainWindow();
}

static void ExitApplication()
{
    if (!g_hWnd) return;
    g_reallyExit = true;
    SaveConfig();
    DestroyWindow(g_hWnd);
}

static void AddTrayIcon()
{
    if (!g_hWnd || g_trayIconAdded) return;

    NOTIFYICONDATAW nid = { 0 };
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = g_hIconSmall ? g_hIconSmall : LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(nid.szTip, L"开始菜单",
        (int)(sizeof(nid.szTip) / sizeof(nid.szTip[0])));

    if (Shell_NotifyIconW(NIM_ADD, &nid))
        g_trayIconAdded = true;
}

static void RemoveTrayIcon()
{
    if (!g_hWnd || !g_trayIconAdded) return;

    NOTIFYICONDATAW nid = { 0 };
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_hWnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    g_trayIconAdded = false;
}

// 低层键盘钩子：检测 Ctrl + Win 组合键（两者都按下并在没有其它键时松开）
static LRESULT CALLBACK TrayKeyboardProc(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lp;
        DWORD vk = k->vkCode;
        bool isDown = (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN);
        bool isUp = (wp == WM_KEYUP || wp == WM_SYSKEYUP);
        bool isCtrl = (vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL);
        bool isWin = (vk == VK_LWIN || vk == VK_RWIN);

        // 每次事件前用真实按键状态校准，避免漏掉某个 keyup 后状态卡死
        if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000)) g_kbCtrlDown = false;
        if (!(GetAsyncKeyState(VK_LWIN) & 0x8000) &&
            !(GetAsyncKeyState(VK_RWIN) & 0x8000)) g_kbWinDown = false;

        if (isDown)
        {
            // 新一次组合键从干净的状态开始
            if ((isCtrl && !g_kbWinDown) || (isWin && !g_kbCtrlDown))
            {
                g_kbChordOther = false;
                g_kbChordDone = false;
            }

            if (isCtrl) g_kbCtrlDown = true;
            else if (isWin) g_kbWinDown = true;
            else if (g_kbCtrlDown || g_kbWinDown) g_kbChordOther = true;
        }
        else if (isUp && (isCtrl || isWin))
        {
            bool wasChord = g_kbCtrlDown && g_kbWinDown;
            if (isCtrl) g_kbCtrlDown = false;
            if (isWin) g_kbWinDown = false;

            if (wasChord && !g_kbChordOther && !g_kbChordDone && g_trayMode)
            {
                g_kbChordDone = true;
                if (g_hWnd) PostMessageW(g_hWnd, WM_TOGGLE_MAIN, 0, 0);
            }

            if (!g_kbCtrlDown && !g_kbWinDown)
            {
                g_kbChordOther = false;
                g_kbChordDone = false;
            }
        }
    }
    return CallNextHookEx(g_kbHook, code, wp, lp);
}

static DWORD  g_hookThreadId = 0;
static HANDLE g_hookThread = nullptr;

static DWORD WINAPI TrayHotkeyThreadProc(LPVOID)
{
    // 先创建消息队列，再安装钩子
    MSG msg;
    PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE);

    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, TrayKeyboardProc, nullptr, 0);

    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_kbHook)
    {
        UnhookWindowsHookEx(g_kbHook);
        g_kbHook = nullptr;
    }
    return 0;
}

static void InstallTrayHotkey()
{
    if (g_hookThread) return;

    g_kbCtrlDown = g_kbWinDown = false;
    g_kbChordOther = g_kbChordDone = false;

    g_hookThread = CreateThread(nullptr, 0, TrayHotkeyThreadProc, nullptr, 0,
        &g_hookThreadId);
    if (g_hookThread) Sleep(20);
}

static void UninstallTrayHotkey()
{
    if (!g_hookThread) return;

    PostThreadMessageW(g_hookThreadId, WM_QUIT, 0, 0);
    WaitForSingleObject(g_hookThread, 1000);
    CloseHandle(g_hookThread);
    g_hookThread = nullptr;
    g_hookThreadId = 0;
}

// 开启 / 关闭托盘模式：任务栏按钮、托盘图标、Ctrl+Win 热键、窗口扩展样式
static void ApplyTrayMode(bool enable)
{
    g_trayMode = enable;
    if (!g_hWnd) return;

    LONG_PTR ex = GetWindowLongPtrW(g_hWnd, GWL_EXSTYLE);
    if (enable)
    {
        ex &= ~WS_EX_APPWINDOW;
        ex |= WS_EX_TOOLWINDOW;
    }
    else
    {
        ex &= ~WS_EX_TOOLWINDOW;
        ex |= WS_EX_APPWINDOW;
    }
    SetWindowLongPtrW(g_hWnd, GWL_EXSTYLE, ex);
    SetWindowPos(g_hWnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    if (enable)
    {
        AddTrayIcon();
        InstallTrayHotkey();
    }
    else
    {
        RemoveTrayIcon();
        UninstallTrayHotkey();
    }
}

// ============================================================
//  对话框公共辅助（合并 Rename / Missing 的重复逻辑）
// ============================================================

// 绘制对话框的标题栏、关闭按钮和分隔线
static void PaintDialogTitle(HDC hdc, HWND hwnd, const wchar_t* title, bool hoverClose)
{
    RECT rc; GetClientRect(hwnd, &rc);

    RECT rtt = { S(16), 0, rc.right - S(40), DLG_TITLE_H };
    DrawText2(hdc, title, rtt, g_uiText, g_fontTitle,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcClose = { rc.right - DLG_TITLE_H, 0, rc.right, DLG_TITLE_H };
    if (hoverClose)
    {
        HBRUSH br = CreateSolidBrush(RGB(196, 43, 28));
        FillRect(hdc, &rcClose, br);
        DeleteObject(br);
    }
    int cx = (rcClose.left + rcClose.right) / 2;
    int cy = (rcClose.top + rcClose.bottom) / 2;
    {
        HPEN pn = CreatePen(PS_SOLID, 1, g_uiText);
        HGDIOBJ op = SelectObject(hdc, pn);
        // 补上终点像素，确保交叉点绝对居中
        MoveToEx(hdc, cx - S(4), cy - S(4), nullptr); LineTo(hdc, cx + S(4) + 1, cy + S(4) + 1);
        MoveToEx(hdc, cx + S(4), cy - S(4), nullptr); LineTo(hdc, cx - S(4) - 1, cy + S(4) + 1);
        SelectObject(hdc, op);
        DeleteObject(pn);
    }

    {
        HPEN pl = CreatePen(PS_SOLID, 1, g_uiSeparator);
        HGDIOBJ op = SelectObject(hdc, pl);
        MoveToEx(hdc, 1, DLG_TITLE_H, nullptr);
        LineTo(hdc, rc.right - 1, DLG_TITLE_H);
        SelectObject(hdc, op);
        DeleteObject(pl);
    }
}

// 统一模态消息循环；遇到 WM_QUIT 时强制清理模态窗口，避免全局状态悬空
static void RunModalLoop(HWND& modalWnd)
{
    MSG msg;
    while (modalWnd && GetMessageW(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_QUIT)
        {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (modalWnd)
    {
        HWND h = modalWnd;
        modalWnd = nullptr;
        if (IsWindow(h)) DestroyWindow(h);
    }

    // 弹窗已彻底销毁，确保主窗口回到前台
    ActivateMainWindow();
}

// ============================================================
//  自绘重命名对话框（直角风格）
// ============================================================
static int HitDlgButton(HWND hwnd, POINT pt)
{
    RECT rc; GetClientRect(hwnd, &rc);
    RECT rcClose = { rc.right - DLG_TITLE_H, 0, rc.right, DLG_TITLE_H };

    if (PtInRect(&g_dlgBtnOk, pt))          return 0;
    if (PtInRect(&g_dlgBtnCancel, pt))      return 1;
    if (PtInRect(&rcClose, pt))             return 2;
    return -1;
}

static void PaintRenameDlg(HDC hdc, HWND hwnd)
{
    RECT rc; GetClientRect(hwnd, &rc);

    HBRUSH bg = CreateSolidBrush(g_uiDialogBg);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    PaintDialogTitle(hdc, hwnd, L"重命名", g_dlgHoverBtn == 2);

    {
        bool focused = (GetFocus() == g_hDlgEdit) || (GetFocus() == hwnd);
        COLORREF bcol = focused ? RGB(0, 174, 255) : g_uiSurfaceBorder;
        HPEN pn = CreatePen(PS_SOLID, 1, bcol);
        HGDIOBJ op = SelectObject(hdc, pn);
        HGDIOBJ ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, g_dlgEditRect.left, g_dlgEditRect.top,
            g_dlgEditRect.right, g_dlgEditRect.bottom);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(pn);
    }

    {
        bool hot = (g_dlgHoverBtn == 1);
        COLORREF fill = hot ? g_uiBtnHot : g_uiBtn;
        FillRoundRect(hdc, g_dlgBtnCancel, S(4), fill, fill);
        DrawText2(hdc, L"取消", g_dlgBtnCancel, g_uiText, g_fontUI,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    {
        bool hot = (g_dlgHoverBtn == 0);
        COLORREF fill = hot ? RGB(0, 140, 235) : RGB(0, 120, 215);
        FillRoundRect(hdc, g_dlgBtnOk, S(4), fill, fill);
        DrawText2(hdc, L"确定", g_dlgBtnOk, RGB(255, 255, 255), g_fontUI,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

static LRESULT CALLBACK DlgEditSubclassProc(HWND hwnd, UINT msg, WPARAM wp,
    LPARAM lp, UINT_PTR id, DWORD_PTR ref)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wp == VK_RETURN)
        {
            if (g_hDlg) SendMessageW(g_hDlg, WM_COMMAND, IDOK, 0);
            return 0;
        }
        if (wp == VK_ESCAPE)
        {
            if (g_hDlg) SendMessageW(g_hDlg, WM_COMMAND, IDCANCEL, 0);
            return 0;
        }
        break;
    case WM_CHAR:
        if (wp == VK_RETURN) return 0;
        break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        if (g_hDlg) InvalidateRect(g_hDlg, &g_dlgEditRect, FALSE);
        break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK RenameProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc,
            max(1, rc.right), max(1, rc.bottom));
        HGDIOBJ ob = SelectObject(memDC, bmp);

        PaintRenameDlg(memDC, hwnd);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, ob);
        DeleteObject(bmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_NCHITTEST:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        RECT rc; GetClientRect(hwnd, &rc);
        if (pt.y < DLG_TITLE_H && pt.x < rc.right - DLG_TITLE_H)
            return HTCAPTION;
        return HTCLIENT;
    }
    case WM_MOUSEMOVE:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        int newHover = HitDlgButton(hwnd, pt);
        if (newHover != g_dlgHoverBtn)
        {
            g_dlgHoverBtn = newHover;
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (g_dlgHoverBtn != -1)
        {
            g_dlgHoverBtn = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int b = HitDlgButton(hwnd, pt);
        if (b >= 0)
        {
            g_dlgPressedBtn = b;
            SetCapture(hwnd);
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        if (g_dlgPressedBtn >= 0)
        {
            int pressed = g_dlgPressedBtn;
            g_dlgPressedBtn = -1;
            if (GetCapture() == hwnd) ReleaseCapture();

            int b = HitDlgButton(hwnd, pt);
            if (b == pressed)
            {
                if (b == 0)
                    SendMessageW(hwnd, WM_COMMAND, IDOK, 0);
                else
                    SendMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
            }
        }
        return 0;
    }
    case WM_CAPTURECHANGED:
        g_dlgPressedBtn = -1;
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK)
        {
            wchar_t buf[512] = { 0 };
            if (g_hDlgEdit) GetWindowTextW(g_hDlgEdit, buf, 512);
            if (g_dlgItem >= 0 && g_dlgItem < (int)g_items.size() && buf[0])
                g_items[g_dlgItem].name = buf;
            RebuildList();
            SaveConfig();          // ★ 新增：重命名后立即写入 cfg，不必等关闭程序
            InvalidateRect(g_hWnd, nullptr, FALSE);
            DestroyWindow(hwnd);
        }
        else if (LOWORD(wp) == IDCANCEL)
        {
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, g_uiText);
        SetBkColor(hdc, g_uiSurface);
        return (LRESULT)g_hbrEdit;
    }
    case WM_DESTROY:
        g_hDlg = nullptr;
        g_hDlgEdit = nullptr;
        g_dlgHoverBtn = -1;
        g_dlgPressedBtn = -1;
        g_dlgItem = -1;
        ActivateMainWindow();
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void ShowRenameDialog(int itemIdx)
{
    if (itemIdx < 0 || itemIdx >= (int)g_items.size()) return;
    if (g_hDlg) return;

    g_dlgItem = itemIdx;
    g_dlgHoverBtn = -1;
    g_dlgPressedBtn = -1;

    RECT rc; GetWindowRect(g_hWnd, &rc);
    int w = DLG_W, h = DLG_H;
    int x = rc.left + (rc.right - rc.left - w) / 2;
    int y = rc.top + (rc.bottom - rc.top - h) / 2;

    // 去除 WS_EX_TOPMOST，防止重命名窗口强行置顶
    g_hDlg = CreateWindowExW(WS_EX_TOOLWINDOW,
        L"SMRenameDlg", L"",
        WS_POPUP,
        x, y, w, h, g_hWnd, nullptr, g_hInst, nullptr);

    if (!g_hDlg) return;

    int by = h - DLG_MARGIN - DLG_BTN_H;
    g_dlgBtnOk = { w - DLG_MARGIN - DLG_BTN_W, by,
                   w - DLG_MARGIN, by + DLG_BTN_H };
    g_dlgBtnCancel = { g_dlgBtnOk.left - S(10) - DLG_BTN_W, by,
                       g_dlgBtnOk.left - S(10), by + DLG_BTN_H };

    int ebX = DLG_MARGIN;
    int ebY = DLG_TITLE_H + S(20);
    int ebW = w - DLG_MARGIN * 2;
    int ebH = S(36);
    g_dlgEditRect = { ebX, ebY, ebX + ebW, ebY + ebH };

    g_hDlgEdit = CreateWindowExW(0, L"EDIT",
        g_items[itemIdx].name.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        ebX + S(4), ebY + S(8), ebW - S(8), ebH - S(16),
        g_hDlg, (HMENU)1000, g_hInst, nullptr);

    if (g_hDlgEdit)
    {
        SendMessageW(g_hDlgEdit, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
        SendMessageW(g_hDlgEdit, EM_SETSEL, 0, -1);
        SetWindowSubclass(g_hDlgEdit, DlgEditSubclassProc, 1, 0);
    }

    EnableWindow(g_hWnd, FALSE);
    ShowWindow(g_hDlg, SW_SHOW);
    if (g_hDlgEdit) SetFocus(g_hDlgEdit);

    RunModalLoop(g_hDlg);
}

// ============================================================
//  设置窗口（自绘，风格与重命名对话框一致）
//    左侧：选项列表（窗口模式 / 主题调节）
//    右侧：对应内容
// ============================================================
static int SetNavW() { return S(SET_NAV_W_BASE); }

static RECT SettingsNavItemRect(HWND hwnd, int i)
{
    RECT rc; GetClientRect(hwnd, &rc);
    int x = S(16);
    int y = DLG_TITLE_H + S(18) + i * (S(SET_NAV_ITEM_H_BASE) + S(6));
    RECT r = { x, y, x + SetNavW(), y + S(SET_NAV_ITEM_H_BASE) };
    return r;
}

static RECT SettingsContentRect(HWND hwnd)
{
    RECT rc; GetClientRect(hwnd, &rc);
    RECT r = { S(16) + SetNavW() + S(22), DLG_TITLE_H + S(20),
               rc.right - S(22), rc.bottom - S(18) };
    return r;
}

static RECT SettingsSwitchRect(HWND hwnd)
{
    RECT c = SettingsContentRect(hwnd);
    int w = S(SET_SWITCH_W_BASE), h = S(SET_SWITCH_H_BASE);
    int top = c.top + S(62);
    RECT r = { c.right - w, top, c.right, top + h };
    return r;
}

// 三个滑条：0=透明度 1=颜色 2=亮度
// 0 号与 1 号之间留出“黑白”开关的位置
static void SettingsSliderRects(HWND hwnd, RECT out[3])
{
    RECT c = SettingsContentRect(hwnd);
    int y = c.top + S(44);
    for (int k = 0; k < 3; k++)
    {
        out[k].left = c.left + S(2);
        out[k].right = c.right - S(2);
        out[k].top = y + S(26);
        out[k].bottom = out[k].top + S(SET_TRACK_H_BASE);
        y = out[k].bottom + (k == 0 ? S(56) : S(34));
    }
}

// “主题颜色”上方的黑白开关
static RECT SettingsGraySwitchRect(HWND hwnd)
{
    RECT c = SettingsContentRect(hwnd);
    RECT tracks[3];
    SettingsSliderRects(hwnd, tracks);

    int w = S(38), h = S(20);
    int top = tracks[0].bottom + S(26);
    RECT r = { c.right - w, top, c.right, top + h };
    return r;
}

// 主题页右下角的“恢复默认”按钮
static RECT SettingsResetRect(HWND hwnd)
{
    RECT c = SettingsContentRect(hwnd);
    int w = S(96), h = S(30);
    RECT r = { c.right - w, c.bottom - h, c.right, c.bottom };
    return r;
}

static int SettingsSliderValue(int k)
{
    if (k == 0) return g_winOpacity;
    if (k == 1) return g_grayMode ? g_grayLevel : g_themeHue;
    return g_themeBright;
}

static int SettingsSliderMax(int k)
{
    return (k == 1 && !g_grayMode) ? 360 : 100;
}

// 滑条某个位置的颜色
static COLORREF SettingsSliderColorAt(int k, double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    if (k == 0)
    {
        // 已填充部分为蓝色，未填充部分为灰色
        double v = g_winOpacity / 100.0;
        return (t <= v) ? RGB(0, 150, 230) : g_uiSwitchOff;
    }
    if (k == 1)
    {
        if (g_grayMode)
        {
            int v = (int)(t * 255.0 + 0.5);
            return RGB(v, v, v);
        }
        int r, g, b;
        HSVToRGB(t * 360.0, 0.85, 1.0, r, g, b);
        return RGB(r, g, b);
    }

    int v = (int)(t * 255.0 + 0.5);
    return RGB(v, v, v);
}

// 抗锯齿地绘制一根滑条
static void DrawSettingsTrack(HDC hdc, const RECT& rc, int k)
{
    double h = rc.bottom - rc.top;
    if (h <= 2.0) return;

    double rad = h / 2.0;
    double cy = (rc.top + rc.bottom) / 2.0;
    double ax = rc.left + rad;
    double bx = rc.right - rad;
    if (bx < ax) { ax = bx = (rc.left + rc.right) / 2.0; }

    const COLORREF bg = g_uiDialogBg;
    double span = (double)(rc.right - rc.left);

    for (int y = rc.top; y < rc.bottom; y++)
    {
        for (int x = rc.left; x < rc.right; x++)
        {
            double d = DistToSegment(x + 0.5, y + 0.5, ax, cy, bx, cy);
            double cov = Clamp01(rad - d + 0.5);
            if (cov <= 0.0) continue;

            double t = (x + 0.5 - rc.left) / span;
            COLORREF c = SettingsSliderColorAt(k, t);

            int r = (int)(GetRValue(bg) * (1.0 - cov) + GetRValue(c) * cov + 0.5);
            int g = (int)(GetGValue(bg) * (1.0 - cov) + GetGValue(c) * cov + 0.5);
            int b = (int)(GetBValue(bg) * (1.0 - cov) + GetBValue(c) * cov + 0.5);
            SetPixelV(hdc, x, y, RGB(r, g, b));
        }
    }
}

static void DrawSettingsKnob(HDC hdc, const RECT& track, int k)
{
    int maxv = SettingsSliderMax(k);
    double t = maxv > 0 ? (double)SettingsSliderValue(k) / maxv : 0.0;
    int rad = S(SET_KNOB_R_BASE);
    int span = (track.right - track.left) - rad * 2;
    int cx = track.left + rad + (int)(span * t + 0.5);
    int cy = (track.top + track.bottom) / 2;

    RECT kr = { cx - rad, cy - rad, cx + rad, cy + rad };
    COLORREF under = SettingsSliderColorAt(k, t);
    FillCapsuleAA(hdc, kr, RGB(250, 250, 252), g_uiSurfaceBorder, under);
}

static void PaintToggle(HDC hdc, const RECT& sw, bool on, bool hover)
{
    COLORREF track = on ? RGB(0, 150, 230) : g_uiSwitchOff;
    if (hover)
        track = on ? RGB(0, 168, 250) : g_uiScrollThumb;

    FillCapsuleAA(hdc, sw, track, track, g_uiDialogBg);

    int d = (sw.bottom - sw.top) - S(4);
    int cy = (sw.top + sw.bottom) / 2;
    int cx = on ? (sw.right - S(2) - d / 2) : (sw.left + S(2) + d / 2);
    RECT kr = { cx - d / 2, cy - d / 2, cx - d / 2 + d, cy - d / 2 + d };
    FillCapsuleAA(hdc, kr, RGB(250, 250, 255), g_uiSurfaceBorder, track);
}

static void PaintSettingsSwitch(HDC hdc, HWND hwnd)
{
    PaintToggle(hdc, SettingsSwitchRect(hwnd), g_trayMode, g_setHover == 3);
}

static void PaintSettingsTrayPage(HDC hdc, HWND hwnd)
{
    RECT c = SettingsContentRect(hwnd);

    RECT rt = { c.left, c.top, c.right, c.top + S(28) };
    DrawText2(hdc, L"窗口模式", rt, g_uiText, g_fontTitle,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rd = { c.left, c.top + S(32), c.right, c.top + S(54) };
    DrawText2(hdc, L"隐藏任务栏图标，改用通知区域图标运行。", rd,
        g_uiTextFaint, g_fontSmall,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    RECT rs = { c.left, c.top + S(62), c.right - S(SET_SWITCH_W_BASE) - S(12),
                c.top + S(62) + S(SET_SWITCH_H_BASE) };
    DrawText2(hdc, L"切换到托盘模式", rs, g_uiText, g_fontUI,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    PaintSettingsSwitch(hdc, hwnd);

    const wchar_t* lines[] = {
        L"开启后：",
        L"· 隐藏任务栏图标，通知区域显示程序图标",
        L"· 左键或右键点击托盘图标：打开窗口 / 退出托盘模式 / 退出",
        L"· 按 Ctrl + Win 键显示或隐藏主窗口",
        L"· 点击最小化、关闭按钮只隐藏窗口",
        L"· 启动程序内的快捷方式后自动隐藏",
        L"· 点击主窗口以外的任意位置自动隐藏",
    };

    int y = c.top + S(110);
    for (int i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++)
    {
        RECT rl = { c.left, y, c.right, y + S(24) };
        DrawText2(hdc, lines[i], rl,
            i == 0 ? g_uiTextDim : g_uiTextFaint,
            g_fontSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        y += S(24);
    }
}

static void PaintSettingsThemePage(HDC hdc, HWND hwnd)
{
    RECT c = SettingsContentRect(hwnd);

    RECT rt = { c.left, c.top, c.right, c.top + S(28) };
    DrawText2(hdc, L"主题调节", rt, g_uiText, g_fontTitle,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT tracks[3];
    SettingsSliderRects(hwnd, tracks);

    // 黑白开关（位于“主题颜色”上方）
    RECT gsw = SettingsGraySwitchRect(hwnd);
    {
        RECT rl = { c.left, gsw.top, gsw.left - S(12), gsw.bottom };
        DrawText2(hdc, L"黑白模式", rl, g_uiTextDim, g_fontUI,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        PaintToggle(hdc, gsw, g_grayMode, g_setHover == 4);
    }

    const wchar_t* labels[3] = { L"窗口透明度", L"主题颜色", L"颜色亮度" };
    if (g_grayMode) labels[1] = L"灰度";
    wchar_t buf[32];

    for (int k = 0; k < 3; k++)
    {
        RECT rl = { tracks[k].left, tracks[k].top - S(26),
                    tracks[k].right, tracks[k].top - S(6) };
        DrawText2(hdc, labels[k], rl, g_uiTextDim, g_fontUI,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        if (k == 1)
        {
            if (g_grayMode) swprintf_s(buf, L"%d%%", g_grayLevel);
            else swprintf_s(buf, L"%d°", g_themeHue);
        }
        else if (k == 0)
            swprintf_s(buf, L"%d%%", g_winOpacity);
        else
            swprintf_s(buf, L"%d%%", g_themeBright);

        DrawText2(hdc, buf, rl, g_uiAccentText, g_fontUI,
            DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        DrawSettingsTrack(hdc, tracks[k], k);
        DrawSettingsKnob(hdc, tracks[k], k);
    }

    // 右下角：恢复默认
    RECT rb = SettingsResetRect(hwnd);
    COLORREF fill = (g_setHover == 5) ? g_uiBtnHot : g_uiBtn;
    FillRoundRect(hdc, rb, S(4), fill, fill);
    DrawText2(hdc, L"恢复默认", rb, g_uiText, g_fontUI,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void PaintSettingsDlg(HDC hdc, HWND hwnd)
{
    RECT rc; GetClientRect(hwnd, &rc);

    HBRUSH bg = CreateSolidBrush(g_uiDialogBg);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    PaintDialogTitle(hdc, hwnd, L"设置", g_setHover == 2);

    const wchar_t* nav[2] = { L"窗口模式", L"主题调节" };
    for (int i = 0; i < 2; i++)
    {
        RECT ri = SettingsNavItemRect(hwnd, i);
        if (i == g_setPage)
            FillRoundRect(hdc, ri, S(6), g_uiNavSel, g_uiNavSel);
        else if (i == g_setHover)
            FillRoundRect(hdc, ri, S(6), g_uiNavHover, g_uiNavHover);

        RECT rt = { ri.left + S(14), ri.top, ri.right - S(8), ri.bottom };
        DrawText2(hdc, nav[i], rt,
            (i == g_setPage) ? g_uiText : g_uiTextDim,
            g_fontUI, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // 左侧列表与右侧内容的分隔线
    {
        int x = S(16) + SetNavW() + S(11);
        HPEN p = CreatePen(PS_SOLID, 1, g_uiSeparator);
        HGDIOBJ op = SelectObject(hdc, p);
        MoveToEx(hdc, x, DLG_TITLE_H + S(16), nullptr);
        LineTo(hdc, x, rc.bottom - S(16));
        SelectObject(hdc, op);
        DeleteObject(p);
    }

    if (g_setPage == 0)
        PaintSettingsTrayPage(hdc, hwnd);
    else
        PaintSettingsThemePage(hdc, hwnd);
}

// 命中测试：0/1=左侧选项 2=关闭 3=托盘开关 4=黑白开关 5=恢复默认
//           10/11/12=滑条
static int HitSettingsControl(HWND hwnd, POINT pt)
{
    RECT rc; GetClientRect(hwnd, &rc);
    RECT rcClose = { rc.right - DLG_TITLE_H, 0, rc.right, DLG_TITLE_H };
    if (PtInRect(&rcClose, pt)) return 2;

    for (int i = 0; i < 2; i++)
    {
        RECT ri = SettingsNavItemRect(hwnd, i);
        if (PtInRect(&ri, pt)) return i;
    }

    if (g_setPage == 0)
    {
        RECT sw = SettingsSwitchRect(hwnd);
        InflateRect(&sw, S(4), S(6));
        if (PtInRect(&sw, pt)) return 3;
    }
    else
    {
        RECT gsw = SettingsGraySwitchRect(hwnd);
        InflateRect(&gsw, S(4), S(6));
        if (PtInRect(&gsw, pt)) return 4;

        RECT rb = SettingsResetRect(hwnd);
        if (PtInRect(&rb, pt)) return 5;

        RECT tracks[3];
        SettingsSliderRects(hwnd, tracks);
        for (int k = 0; k < 3; k++)
        {
            RECT hit = tracks[k];
            hit.top -= S(14);
            hit.bottom += S(14);
            hit.left -= S(8);
            hit.right += S(8);
            if (PtInRect(&hit, pt)) return 10 + k;
        }
    }
    return -1;
}

static void SetSettingsSliderFromX(HWND hwnd, int k, int x)
{
    RECT tracks[3];
    SettingsSliderRects(hwnd, tracks);

    int rad = S(SET_KNOB_R_BASE);
    int left = tracks[k].left + rad;
    int right = tracks[k].right - rad;
    double t = (right > left) ? (double)(x - left) / (right - left) : 0.0;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    if (k == 0) g_winOpacity = (int)(t * 100.0 + 0.5);
    else if (k == 1)
    {
        if (g_grayMode) g_grayLevel = (int)(t * 100.0 + 0.5);
        else g_themeHue = (int)(t * 360.0 + 0.5);
    }
    else g_themeBright = (int)(t * 100.0 + 0.5);

    if (g_winOpacity < 0) g_winOpacity = 0;
    if (g_winOpacity > 100) g_winOpacity = 100;
    if (g_themeHue < 0) g_themeHue = 0;
    if (g_themeHue > 360) g_themeHue = 360;
    if (g_grayLevel < 0) g_grayLevel = 0;
    if (g_grayLevel > 100) g_grayLevel = 100;
    if (g_themeBright < 0) g_themeBright = 0;
    if (g_themeBright > 100) g_themeBright = 100;

    ApplyThemeToMainWindow();
    InvalidateRect(hwnd, nullptr, FALSE);
}

static void ToggleTrayModeFromSettings(HWND hwnd)
{
    ApplyTrayMode(!g_trayMode);
    SaveConfig();
    InvalidateRect(hwnd, nullptr, FALSE);
}

static LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, max(1, rc.right), max(1, rc.bottom));
        HGDIOBJ ob = SelectObject(memDC, bmp);

        PaintSettingsDlg(memDC, hwnd);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, ob);
        DeleteObject(bmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_NCHITTEST:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        RECT rc; GetClientRect(hwnd, &rc);
        if (pt.y < DLG_TITLE_H && pt.x < rc.right - DLG_TITLE_H)
            return HTCAPTION;
        return HTCLIENT;
    }

    case WM_MOUSEMOVE:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        if (g_setDragSlider >= 0)
        {
            SetSettingsSliderFromX(hwnd, g_setDragSlider, pt.x);
        }
        else
        {
            int h = HitSettingsControl(hwnd, pt);
            if (h != g_setHover)
            {
                g_setHover = h;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }

        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_MOUSELEAVE:
        if (g_setHover != -1)
        {
            g_setHover = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int id = HitSettingsControl(hwnd, pt);

        if (id >= 10)
        {
            g_setDragSlider = id - 10;
            SetSettingsSliderFromX(hwnd, g_setDragSlider, pt.x);
            SetCapture(hwnd);
        }
        else if (id >= 0)
        {
            g_setPressed = id;
            SetCapture(hwnd);
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        if (g_setDragSlider >= 0)
        {
            g_setDragSlider = -1;
            if (GetCapture() == hwnd) ReleaseCapture();
            SaveConfig();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (g_setPressed >= 0)
        {
            int pressed = g_setPressed;
            g_setPressed = -1;
            if (GetCapture() == hwnd) ReleaseCapture();

            if (HitSettingsControl(hwnd, pt) == pressed)
            {
                if (pressed == 0 || pressed == 1)
                {
                    g_setPage = pressed;
                    g_setHover = -1;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                else if (pressed == 2)
                {
                    DestroyWindow(hwnd);
                }
                else if (pressed == 3)
                {
                    ToggleTrayModeFromSettings(hwnd);
                }
                else if (pressed == 4)
                {
                    // 黑白模式开关
                    g_grayMode = !g_grayMode;
                    ApplyThemeToMainWindow();
                    SaveConfig();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                else if (pressed == 5)
                {
                    // 恢复默认主题
                    g_winOpacity = THEME_OPACITY_DEF;
                    g_themeHue = THEME_HUE_DEF;
                    g_themeBright = THEME_BRIGHT_DEF;
                    g_grayMode = THEME_GRAYMODE_DEF;
                    g_grayLevel = THEME_GRAY_DEF;
                    ApplyThemeToMainWindow();
                    SaveConfig();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
        }
        return 0;
    }

    case WM_CAPTURECHANGED:
        g_setPressed = -1;
        g_setDragSlider = -1;
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { DestroyWindow(hwnd); return 0; }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        g_hSettings = nullptr;
        g_setHover = -1;
        g_setPressed = -1;
        g_setDragSlider = -1;
        SaveConfig();
        ActivateMainWindow();
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void ShowSettingsDialog()
{
    if (g_hSettings) return;
    if (!g_hWnd) return;

    RECT rc; GetWindowRect(g_hWnd, &rc);
    int w = S(SET_W_BASE), h = S(SET_H_BASE);
    int x = rc.left + (rc.right - rc.left - w) / 2;
    int y = rc.top + (rc.bottom - rc.top - h) / 2;

    g_setPage = 0;
    g_setHover = -1;
    g_setPressed = -1;
    g_setDragSlider = -1;

    g_hSettings = CreateWindowExW(WS_EX_TOOLWINDOW,
        L"SMSettingsDlg", L"设置", WS_POPUP,
        x, y, w, h, g_hWnd, nullptr, g_hInst, nullptr);

    if (!g_hSettings) return;

    EnableWindow(g_hWnd, FALSE);
    ShowWindow(g_hSettings, SW_SHOW);
    SetFocus(g_hSettings);

    RunModalLoop(g_hSettings);
}

// ============================================================
//  文件缺失提示对话框（复用重命名窗口的直角自绘样式）
// ============================================================
static int HitMissingDlgButton(HWND hwnd, POINT pt)
{
    RECT rc; GetClientRect(hwnd, &rc);
    RECT rcClose = { rc.right - DLG_TITLE_H, 0, rc.right, DLG_TITLE_H };

    if (PtInRect(&g_missingDlgBtnYes, pt)) return 0;  // 是
    if (PtInRect(&g_missingDlgBtnNo, pt))  return 1;  // 否
    if (PtInRect(&rcClose, pt))            return 2;  // 关闭
    return -1;
}

static void PaintMissingDlg(HDC hdc, HWND hwnd)
{
    RECT rc; GetClientRect(hwnd, &rc);

    HBRUSH bg = CreateSolidBrush(g_uiDialogBg);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    PaintDialogTitle(hdc, hwnd, L"提示", g_missingDlgHoverBtn == 2);

    // 提示文本
    {
        RECT rt = { DLG_MARGIN, DLG_TITLE_H + S(16),
                    rc.right - DLG_MARGIN,
                    rc.bottom - DLG_MARGIN - DLG_BTN_H - S(12) };
        DrawText2(hdc, g_missingDlgText, rt, g_uiText, g_fontUI,
            DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
    }

    // 否（左）
    {
        bool hot = (g_missingDlgHoverBtn == 1);
        COLORREF fill = hot ? g_uiBtnHot : g_uiBtn;
        FillRoundRect(hdc, g_missingDlgBtnNo, S(4), fill, fill);
        DrawText2(hdc, L"否", g_missingDlgBtnNo, g_uiText, g_fontUI,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 是（右，主操作）
    {
        bool hot = (g_missingDlgHoverBtn == 0);
        COLORREF fill = hot ? RGB(0, 140, 235) : RGB(0, 120, 215);
        FillRoundRect(hdc, g_missingDlgBtnYes, S(4), fill, fill);
        DrawText2(hdc, L"是", g_missingDlgBtnYes, RGB(255, 255, 255), g_fontUI,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

static LRESULT CALLBACK MissingDlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc,
            max(1, rc.right), max(1, rc.bottom));
        HGDIOBJ ob = SelectObject(memDC, bmp);

        PaintMissingDlg(memDC, hwnd);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, ob);
        DeleteObject(bmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_NCHITTEST:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        RECT rc; GetClientRect(hwnd, &rc);
        if (pt.y < DLG_TITLE_H && pt.x < rc.right - DLG_TITLE_H)
            return HTCAPTION;
        return HTCLIENT;
    }
    case WM_MOUSEMOVE:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int newHover = HitMissingDlgButton(hwnd, pt);
        if (newHover != g_missingDlgHoverBtn)
        {
            g_missingDlgHoverBtn = newHover;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (g_missingDlgHoverBtn != -1)
        {
            g_missingDlgHoverBtn = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int b = HitMissingDlgButton(hwnd, pt);
        if (b >= 0)
        {
            g_missingDlgPressedBtn = b;
            SetCapture(hwnd);
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        if (g_missingDlgPressedBtn >= 0)
        {
            int pressed = g_missingDlgPressedBtn;
            g_missingDlgPressedBtn = -1;
            if (GetCapture() == hwnd) ReleaseCapture();

            int b = HitMissingDlgButton(hwnd, pt);
            if (b == pressed)
            {
                if (b == 0)
                    SendMessageW(hwnd, WM_COMMAND, IDYES, 0);
                else if (b == 1)
                    SendMessageW(hwnd, WM_COMMAND, IDNO, 0);
                else if (b == 2)
                    SendMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
            }
        }
        return 0;
    }
    case WM_CAPTURECHANGED:
        g_missingDlgPressedBtn = -1;
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE)
        {
            SendMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
            return 0;
        }
        if (wp == VK_RETURN)
        {
            SendMessageW(hwnd, WM_COMMAND, IDYES, 0);
            return 0;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == IDYES)
        {
            // 移除所有缺失项（降序删除，避免索引错位）
            std::vector<int> v = g_missingDlgItems;
            std::sort(v.begin(), v.end(), std::greater<int>());
            v.erase(std::unique(v.begin(), v.end()), v.end());
            for (int idx : v)
                RemoveItem(idx);       // RemoveItem 内部已调用 RebuildList + LayoutGroups
            SaveConfig();
            DestroyWindow(hwnd);
        }
        else if (LOWORD(wp) == IDNO || LOWORD(wp) == IDCANCEL)
        {
            // 不移除，保留深红色背景
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        g_hMissingDlg = nullptr;
        g_missingDlgHoverBtn = -1;
        g_missingDlgPressedBtn = -1;
        g_missingDlgItems.clear();
        ActivateMainWindow();
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void ShowMissingFilesDialog(const std::vector<int>& items)
{
    if (items.empty()) return;
    if (g_hMissingDlg) return;
    if (!g_hWnd) return;

    g_missingDlgItems = items;
    g_missingDlgHoverBtn = -1;
    g_missingDlgPressedBtn = -1;

    if (items.size() == 1 && items[0] >= 0 && items[0] < (int)g_items.size())
    {
        g_missingDlgText = L"该快捷方式找不到对应路径：\r\n"
            + g_items[items[0]].name
            + L"\r\n\r\n是否移除该快捷方式？";
    }
    else
    {
        g_missingDlgText = L"有 " + std::to_wstring(items.size())
            + L" 个文件找不到对应路径，\r\n是否移除这些快捷方式？";
    }

    RECT rc; GetWindowRect(g_hWnd, &rc);
    int w = DLG_W;
    int h = DLG_H + S(40);
    int x = rc.left + (rc.right - rc.left - w) / 2;
    int y = rc.top + (rc.bottom - rc.top - h) / 2;

    // 去除 WS_EX_TOPMOST，防止提示框强行置顶
    g_hMissingDlg = CreateWindowExW(WS_EX_TOOLWINDOW,
        L"SMMissingDlg", L"",
        WS_POPUP,
        x, y, w, h, g_hWnd, nullptr, g_hInst, nullptr);

    if (!g_hMissingDlg) return;

    int by = h - DLG_MARGIN - DLG_BTN_H;
    g_missingDlgBtnYes = { w - DLG_MARGIN - DLG_BTN_W, by,
                           w - DLG_MARGIN, by + DLG_BTN_H };
    g_missingDlgBtnNo = { g_missingDlgBtnYes.left - S(10) - DLG_BTN_W, by,
                          g_missingDlgBtnYes.left - S(10), by + DLG_BTN_H };

    EnableWindow(g_hWnd, FALSE);
    ShowWindow(g_hMissingDlg, SW_SHOW);

    // 强制把提示窗提到最前（包括主窗口的显示动画层之上）
    SetWindowPos(g_hMissingDlg, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    // 立即取消 topmost，但 Z 序保持在主窗口上方
    SetWindowPos(g_hMissingDlg, HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE);

    SetFocus(g_hMissingDlg);

    RunModalLoop(g_hMissingDlg);
}

// ============================================================
//  分组名行内编辑
// ============================================================
static void EndEditGroupName(bool commit)
{
    if (!g_hEdit) return;

    wchar_t buf[512] = { 0 };
    GetWindowTextW(g_hEdit, buf, 512);

    // 防止用户粘贴多行文本导致换行
    std::wstring newName = buf;
    for (auto& c : newName) {
        if (c == L'\r' || c == L'\n') c = L' ';
    }

    int gi = g_editGroup;
    g_editGroup = -1;

    // 先置空 g_hEdit 再 DestroyWindow，避免 WM_KILLFOCUS 重入 EndEditGroupName
    HWND hEdit = g_hEdit;
    g_hEdit = nullptr;
    DestroyWindow(hEdit);

    // 分组名称可以为空，不再限制 buf[0] 必须有字符
    if (commit && gi >= 0 && gi < (int)g_groups.size())
        g_groups[gi].name = newName;

    // 名称变更后立即保存
    SaveConfig();

    SetFocus(g_hWnd);
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wp,
    LPARAM lp, UINT_PTR id, DWORD_PTR ref)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wp == VK_RETURN) { EndEditGroupName(true);  return 0; }
        if (wp == VK_ESCAPE) { EndEditGroupName(false); return 0; }
        break;
    case WM_CHAR:
        // 拦截回车和换行符，防止多行编辑框换行
        if (wp == VK_RETURN || wp == L'\r' || wp == L'\n') return 0;
        break;
    case WM_KILLFOCUS:
        if (g_hEdit) EndEditGroupName(true);
        break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static void StartEditGroupName(int gi)
{
    if (gi < 0 || gi >= (int)g_groups.size()) return;
    if (g_hEdit) EndEditGroupName(true);

    g_editGroup = gi;

    RECT r = g_groups[gi].rcTitle;
    int editH = r.bottom - r.top - S(6);
    int editW = (r.right - r.left) - GROUP_PAD * 2;
    if (editW < S(10)) editW = S(10);

    // 必须使用 ES_MULTILINE 才能通过 EM_SETRECT 精确控制垂直居中
    g_hEdit = CreateWindowExW(0, L"EDIT", g_groups[gi].name.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | ES_MULTILINE,
        r.left + GROUP_PAD, r.top + S(3),
        editW, editH,
        g_hWnd, nullptr, g_hInst, nullptr);

    if (!g_hEdit) return;

    SendMessage(g_hEdit, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    // 计算文本的垂直居中位置
    HDC hdc = GetDC(g_hEdit);
    HFONT hOld = (HFONT)SelectObject(hdc, g_fontSmall);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, hOld);
    ReleaseDC(g_hEdit, hdc);

    int textH = tm.tmHeight;
    int yOff = (editH - textH) / 2;
    if (yOff < 0) yOff = 0;

    // 设置编辑框文本绘制矩形（左2，上偏移，右-2，下偏移+文本高度）
    RECT rcText = { S(2), yOff, editW - S(2), yOff + textH };
    SendMessage(g_hEdit, EM_SETRECT, 0, (LPARAM)&rcText);

    SetWindowSubclass(g_hEdit, EditSubclassProc, 1, 0);
    SetFocus(g_hEdit);
    SendMessage(g_hEdit, EM_SETSEL, 0, -1);
}

// ============================================================
//  拖拽逻辑（含外部拖入）
// ============================================================

// 根据客户区坐标计算拖放目标：分组索引 / 插入位置 / 提示模式 / 是否新建分组
static void ComputeDropTargetAt(POINT pt, int& outG, int& outPos,
    int& outMode, bool& outNewGroup)
{
    outG = -1;
    outPos = -1;
    outMode = 0;
    outNewGroup = false;

    int maxCells = MAX_COLS * MAX_ROWS;
    int targetGi = -1;

    // 1. 先找鼠标是否直接在任何分组的实际矩形内（避免下方分组标题栏被上方分组的扩展区误判）
    for (int gi = 0; gi < (int)g_groups.size(); gi++)
    {
        if (PtInRect(&g_groups[gi].rc, pt)) {
            targetGi = gi;
            break;
        }
    }

    // 2. 如果不在任何分组内，再找是否在某分组的底部扩展区
    if (targetGi == -1) {
        for (int gi = 0; gi < (int)g_groups.size(); gi++)
        {
            RECT hitRect = g_groups[gi].rc;
            hitRect.bottom += (TILE_H + TILE_GAP); // 向下容忍一行的高度
            if (PtInRect(&hitRect, pt)) {
                targetGi = gi;
                break;
            }
        }
    }

    if (targetGi >= 0)
    {
        Group& g = g_groups[targetGi];
        outG = targetGi;

        int rx = pt.x - (g.rc.left + GROUP_PAD);
        int ry = pt.y - (g.rc.top + GROUP_HDR + S(2));

        int col = rx / (TILE_W + TILE_GAP);
        int row = ry / (TILE_H + TILE_GAP);
        if (col < 0) col = 0;
        if (col >= MAX_COLS) col = MAX_COLS - 1;
        if (row < 0) row = 0;

        // 限制最大展开行数，最多比当前原有多出 1 行
        int origMaxItemCount = GetGroupMaxItemCount(g);
        int origRows = (origMaxItemCount > 0) ? ((origMaxItemCount + MAX_COLS - 1) / MAX_COLS) : 1;
        int maxAllowedRow = origRows; // 允许在原基础上向下多出 1 行

        if (row > maxAllowedRow) row = maxAllowedRow;
        if (row >= MAX_ROWS) row = MAX_ROWS - 1;

        int cellIdx = row * MAX_COLS + col;
        if (cellIdx >= maxCells) cellIdx = maxCells - 1;

        int sz = (int)g.items.size();
        bool isEmpty = (cellIdx >= sz) || (g.items[cellIdx] == -1);

        if (isEmpty)
        {
            outPos = cellIdx;
            outMode = 2;
        }
        else
        {
            int localX = rx % (TILE_W + TILE_GAP);
            int pos = cellIdx;
            if (localX > TILE_W / 2) pos = cellIdx + 1;
            if (pos > sz) pos = sz;
            if (pos > maxCells) pos = maxCells;
            outPos = pos;
            outMode = 1;
        }
        return;
    }

    // 不在任何分组里，且不在标题栏内 —— 视为"新建分组"
    if (pt.y > TITLE_H)
        outNewGroup = true;
}

// 把 itemIdx 插入到某分组的指定位置（pos/mode 与 ComputeDropTargetAt 输出一致）
static void InsertItemAtTarget(int itemIdx, int gi, int pos, int mode)
{
    if (gi < 0 || gi >= (int)g_groups.size()) return;

    int maxCells = MAX_COLS * MAX_ROWS;
    Group& g = g_groups[gi];

    if (mode == 2)
    {
        if (pos < 0) pos = 0;
        if (pos >= maxCells) pos = maxCells - 1;

        if (pos >= (int)g.items.size())
        {
            g.items.resize(pos + 1, -1);
            g.items[pos] = itemIdx;
            return;
        }
        if (g.items[pos] == -1)
        {
            g.items[pos] = itemIdx;
            return;
        }
    }

    if (pos < 0) pos = 0;
    if (pos > (int)g.items.size()) pos = (int)g.items.size();
    if (pos > maxCells) pos = maxCells;
    g.items.insert(g.items.begin() + pos, itemIdx);
}

// 把一组文件依次插入到目标分组；若 tg < 0 则每组新建一个分组
static void InsertItemsAtTarget(const std::vector<std::wstring>& files, int tg, int tpos, int tmode)
{
    for (auto& f : files)
    {
        int idx = AddItem(f);
        if (tg >= 0 && tg < (int)g_groups.size())
        {
            InsertItemAtTarget(idx, tg, tpos, tmode);
            tpos++;
            if (tmode == 2) tmode = 1;
        }
        else
        {
            Group ng2;
            ng2.name = L""; // 新建分组默认为空名
            ng2.items.push_back(idx);
            g_groups.push_back(ng2);
            tg = (int)g_groups.size() - 1;
            tpos = 1;
            tmode = 1;
        }
    }
}

// 从 IDataObject 中取出 CF_HDROP 文件列表
static bool GetHDROPFiles(IDataObject* pDataObj, std::vector<std::wstring>& files)
{
    if (!pDataObj) return false;

    FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg;
    ZeroMemory(&stg, sizeof(stg));
    if (FAILED(pDataObj->GetData(&fmt, &stg))) return false;

    bool ok = false;
    if (stg.tymed == TYMED_HGLOBAL)
    {
        HDROP hd = (HDROP)GlobalLock(stg.hGlobal);
        if (hd)
        {
            UINT n = DragQueryFileW(hd, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < n; i++)
            {
                wchar_t buf[MAX_PATH * 2] = { 0 };
                if (DragQueryFileW(hd, i, buf, MAX_PATH * 2) > 0)
                    files.push_back(buf);
            }
            GlobalUnlock(stg.hGlobal);
            ok = true;
        }
    }
    ReleaseStgMedium(&stg);
    return ok;
}

// 处理拖入文件（Drop / WM_DROPFILES 共用）
static void ProcessDroppedFiles(const std::vector<std::wstring>& files, POINT ptClient)
{
    if (files.empty()) return;

    int tg = -1, tpos = -1, tmode = 0;
    bool ng = false;
    ComputeDropTargetAt(ptClient, tg, tpos, tmode, ng);

    InsertItemsAtTarget(files, tg, tpos, tmode);

    RebuildList();
    LayoutGroups();
    SaveConfig();
    if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
}

// ------------------------------------------------------------
//  OLE 拖放目标：支持外部拖入过程中的实时高亮
// ------------------------------------------------------------
class CStartMenuDropTarget : public IDropTarget
{
public:
    CStartMenuDropTarget() : m_ref(1) {}
    virtual ~CStartMenuDropTarget() {}

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
    {
        if (!ppvObject) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IDropTarget)
        {
            *ppvObject = static_cast<IDropTarget*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)()
    {
        return (ULONG)InterlockedIncrement(&m_ref);
    }
    STDMETHOD_(ULONG, Release)()
    {
        LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return (ULONG)r;
    }

    STDMETHOD(DragEnter)(IDataObject* pDataObj, DWORD grfKeyState,
        POINTL pt, DWORD* pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(IDataObject* pDataObj, DWORD grfKeyState,
        POINTL pt, DWORD* pdwEffect);

private:
    LONG m_ref;
};

STDMETHODIMP CStartMenuDropTarget::DragEnter(IDataObject* pDataObj,
    DWORD /*grfKeyState*/, POINTL ptl, DWORD* pdwEffect)
{
    FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    if (pDataObj && pDataObj->QueryGetData(&fmt) == S_OK)
    {
        *pdwEffect = DROPEFFECT_COPY;

        g_dragExternal = true;

        POINT pt = { ptl.x, ptl.y };
        if (g_hWnd) ScreenToClient(g_hWnd, &pt);
        ComputeDropTargetAt(pt, g_dragTargetG, g_dragTargetPos,
            g_dragTargetMode, g_dragNewGroup);

        if (g_hWnd)
        {
            LayoutGroups();
            InvalidateRect(g_hWnd, nullptr, FALSE);
        }
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
        g_dragExternal = false;
    }
    return S_OK;
}

STDMETHODIMP CStartMenuDropTarget::DragOver(DWORD /*grfKeyState*/,
    POINTL ptl, DWORD* pdwEffect)
{
    if (!g_dragExternal)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    *pdwEffect = DROPEFFECT_COPY;

    POINT pt = { ptl.x, ptl.y };
    if (g_hWnd) ScreenToClient(g_hWnd, &pt);

    int tg, tpos, tmode;
    bool ng;
    ComputeDropTargetAt(pt, tg, tpos, tmode, ng);

    if (tg != g_dragTargetG || tpos != g_dragTargetPos ||
        tmode != g_dragTargetMode || ng != g_dragNewGroup)
    {
        g_dragTargetG = tg;
        g_dragTargetPos = tpos;
        g_dragTargetMode = tmode;
        g_dragNewGroup = ng;
        if (g_hWnd)
        {
            LayoutGroups(); // 触发底部动态扩展重排
            InvalidateRect(g_hWnd, nullptr, FALSE);
        }
    }
    return S_OK;
}

STDMETHODIMP CStartMenuDropTarget::DragLeave()
{
    g_dragExternal = false;
    g_dragTargetG = -1;
    g_dragTargetPos = -1;
    g_dragTargetMode = 0;
    g_dragNewGroup = false;

    if (g_hWnd)
    {
        LayoutGroups(); // 恢复原高度
        InvalidateRect(g_hWnd, nullptr, FALSE);
    }
    return S_OK;
}

STDMETHODIMP CStartMenuDropTarget::Drop(IDataObject* pDataObj,
    DWORD /*grfKeyState*/, POINTL ptl, DWORD* pdwEffect)
{
    POINT pt = { ptl.x, ptl.y };
    if (g_hWnd) ScreenToClient(g_hWnd, &pt);

    // 结束外部拖动状态，并清除高亮
    g_dragExternal = false;
    g_dragTargetG = -1;
    g_dragTargetPos = -1;
    g_dragTargetMode = 0;
    g_dragNewGroup = false;

    std::vector<std::wstring> files;
    if (GetHDROPFiles(pDataObj, files) && !files.empty())
    {
        ProcessDroppedFiles(files, pt);
        *pdwEffect = DROPEFFECT_COPY;
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
        if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
    }
    return S_OK;
}

// ------------------------------------------------------------
static void UpdateTileDrag(POINT pt)
{
    g_dragRect.left = pt.x - g_dragOffset.x;
    g_dragRect.top = pt.y - g_dragOffset.y;
    g_dragRect.right = g_dragRect.left + TILE_W;
    g_dragRect.bottom = g_dragRect.top + TILE_H;

    ComputeDropTargetAt(pt, g_dragTargetG, g_dragTargetPos,
        g_dragTargetMode, g_dragNewGroup);
}

static void DropTile(POINT pt)
{
    int sgi = g_dragG, sii = g_dragI;
    if (sgi < 0 || sgi >= (int)g_groups.size()) return;
    if (sii < 0 || sii >= (int)g_groups[sgi].items.size()) return;

    int itemId = g_groups[sgi].items[sii];
    if (itemId == -1) return;

    int maxCells = MAX_COLS * MAX_ROWS;

    if (g_dragNewGroup)
    {
        g_groups[sgi].items[sii] = -1;
        Group ng;
        ng.name = L""; // 新建分组默认为空名
        ng.items.push_back(itemId);
        g_groups.push_back(ng);
    }
    else if (g_dragTargetG >= 0 && g_dragTargetG < (int)g_groups.size())
    {
        int pos = g_dragTargetPos;
        if (pos < 0) pos = 0;
        if (pos > maxCells) pos = maxCells;

        if (g_dragTargetG == sgi)
        {
            if (g_dragTargetMode == 2)
            {
                if (pos == sii) return;
                g_groups[sgi].items[sii] = -1;
                while ((int)g_groups[sgi].items.size() <= pos &&
                    (int)g_groups[sgi].items.size() < maxCells)
                    g_groups[sgi].items.push_back(-1);
                if (pos < (int)g_groups[sgi].items.size())
                    g_groups[sgi].items[pos] = itemId;
                else
                    g_groups[sgi].items.push_back(itemId);
            }
            else
            {
                if (pos == sii || pos == sii + 1) return;
                g_groups[sgi].items.erase(g_groups[sgi].items.begin() + sii);
                if (pos > sii) pos -= 1;
                int sz = (int)g_groups[sgi].items.size();
                if (pos > sz) pos = sz;
                g_groups[sgi].items.insert(g_groups[sgi].items.begin() + pos, itemId);
            }
        }
        else
        {
            Group& tg = g_groups[g_dragTargetG];

            if (g_dragTargetMode == 2)
            {
                g_groups[sgi].items[sii] = -1;
                while ((int)tg.items.size() <= pos && (int)tg.items.size() < maxCells)
                    tg.items.push_back(-1);
                if (pos < (int)tg.items.size() && tg.items[pos] == -1)
                    tg.items[pos] = itemId;
                else if (pos >= (int)tg.items.size())
                    tg.items.push_back(itemId);
                else
                    tg.items.insert(tg.items.begin() + pos, itemId);
            }
            else
            {
                g_groups[sgi].items[sii] = -1;
                if (pos > (int)tg.items.size()) pos = (int)tg.items.size();
                tg.items.insert(tg.items.begin() + pos, itemId);
            }
        }
    }
    else
    {
        return;
    }

    for (auto& g : g_groups) {
        while (!g.items.empty() && g.items.back() == -1)
            g.items.pop_back();
    }

    RemoveEmptyGroups();
    LayoutGroups();
}

static void UpdateGroupDrag(POINT pt)
{
    if (g_dragGroupIdx < 0 || g_dragGroupIdx >= (int)g_groups.size()) return;

    int target = -1;
    for (int i = 0; i < (int)g_groups.size(); i++)
    {
        if (PtInRect(&g_groups[i].rc, pt)) { target = i; break; }
    }

    if (target >= 0 && target != g_dragGroupIdx)
    {
        Group tmp = g_groups[g_dragGroupIdx];
        g_groups.erase(g_groups.begin() + g_dragGroupIdx);
        g_groups.insert(g_groups.begin() + target, tmp);
        g_dragGroupIdx = target;
        LayoutGroups();
        InvalidateRect(g_hWnd, nullptr, FALSE);
    }
}

// ============================================================
//  配置读写（自定义 .cfg 格式）
//    窗口宽高按 96 DPI 逻辑单位保存，跨 DPI 迁移时观感一致
//    窗口最大化（或最小化）时只记录 Maximized 标志，保留原有 Width/Height/X/Y
// ============================================================

// 从配置内容中提取 [Window] 段内指定键的原始行（如 "Width=982"），不存在则返回空串
static std::wstring GetWindowKeyLine(const std::wstring& content, const std::wstring& key)
{
    if (content.empty() || key.empty()) return L"";
    std::wstringstream ss(content);
    std::wstring line;
    bool inWin = false;
    while (std::getline(ss, line))
    {
        // 去掉行尾的 \r / \n / 空白
        while (!line.empty() &&
            (line.back() == L'\r' || line.back() == L'\n' ||
                line.back() == L' ' || line.back() == L'\t'))
            line.pop_back();

        if (line == L"[Window]") { inWin = true; continue; }
        if (!line.empty() && line[0] == L'[') { inWin = false; continue; }

        if (inWin && line.size() >= key.size() &&
            line.compare(0, key.size(), key) == 0)
        {
            return line;
        }
    }
    return L"";
}

static void SaveConfig()
{
    RECT rc; GetWindowRect(g_hWnd, &rc);
    bool maximized = IsZoomed(g_hWnd) != FALSE;
    bool minimized = IsIconic(g_hWnd) != FALSE;

    std::wstringstream ss;
    ss << L"[Window]\n";
    // 记录是否处于最大化，用于下次启动时还原窗口状态
    ss << L"Maximized=" << (maximized ? 1 : 0) << L"\n";
    // 是否开启托盘模式
    ss << L"TrayMode=" << (g_trayMode ? 1 : 0) << L"\n";

    if (!maximized && !minimized)
    {
        // 正常状态：写入当前尺寸 / 位置（按 96 DPI 逻辑单位保存）
        int logicalW = MulDiv(rc.right - rc.left, 96, g_dpi);
        int logicalH = MulDiv(rc.bottom - rc.top, 96, g_dpi);
        ss << L"Width=" << logicalW << L"\n";
        ss << L"Height=" << logicalH << L"\n";
        ss << L"X=" << rc.left << L"\n";
        ss << L"Y=" << rc.top << L"\n";
    }
    else
    {
        // 最大化 / 最小化：不覆盖已有的 Width / Height / X / Y。
        // 若旧配置中这些键存在，则原样保留；若不存在，则不写入。
        std::wstring old = ReadAllText(g_cfgPath);
        std::wstring wLine = GetWindowKeyLine(old, L"Width=");
        std::wstring hLine = GetWindowKeyLine(old, L"Height=");
        std::wstring xLine = GetWindowKeyLine(old, L"X=");
        std::wstring yLine = GetWindowKeyLine(old, L"Y=");
        if (!wLine.empty()) ss << wLine << L"\n";
        if (!hLine.empty()) ss << hLine << L"\n";
        if (!xLine.empty()) ss << xLine << L"\n";
        if (!yLine.empty()) ss << yLine << L"\n";
    }

    // 主题（透明度 / 色相 / 亮度 / 黑白模式）紧跟 [Window] 段
    ss << L"\n[Color]\n";
    ss << L"Opacity=" << g_winOpacity << L"\n";
    ss << L"Hue=" << g_themeHue << L"\n";
    ss << L"Brightness=" << g_themeBright << L"\n";
    ss << L"Gray=" << (g_grayMode ? 1 : 0) << L"\n";
    ss << L"GrayLevel=" << g_grayLevel << L"\n";

    for (size_t gi = 0; gi < g_groups.size(); ++gi) {
        const auto& group = g_groups[gi];
        ss << L"\n[grouping]\n";
        ss << L"number=" << (gi + 1) << L"\n";
        ss << L"name=" << group.name << L"\n";
        for (size_t i = 0; i < group.items.size(); ++i) {
            int itemIdx = group.items[i];
            if (itemIdx >= 0 && itemIdx < (int)g_items.size()) {
                const auto& item = g_items[itemIdx];
                std::wstring safeName = item.name;
                std::replace(safeName.begin(), safeName.end(), L'|', L' '); // 避免字符冲突
                std::wstring safeArgs = item.args;
                std::replace(safeArgs.begin(), safeArgs.end(), L'|', L' ');

                ss << (i + 1) << L"|"
                    << safeName << L"|"
                    << MakePortable(item.path) << L"|"
                    << safeArgs << L"\n";
            }
        }
    }

    WriteAllText(g_cfgPath, ss.str());
}

static void LoadConfig()
{
    g_items.clear();
    g_groups.clear();

    std::wstring content = ReadAllText(g_cfgPath);
    if (content.empty()) return;

    std::wstringstream ss(content);
    std::wstring line;

    Group currentGroup;
    bool inGrouping = false;
    bool inWindow = false;
    bool inColor = false;
    bool hasGroup = false;

    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n' || line.back() == L' ' || line.back() == L'\t'))
            line.pop_back();
        while (!line.empty() && (line.front() == L' ' || line.front() == L'\t'))
            line.erase(0, 1);

        if (line.empty()) continue;

        if (line[0] == L'[') {
            inWindow = (line == L"[Window]" || line == L"[Windows]");
            inColor = (line == L"[Color]");
            if (line == L"[grouping]") {
                if (hasGroup) {
                    g_groups.push_back(currentGroup);
                }
                currentGroup = Group();
                inGrouping = true;
                hasGroup = true;
            }
            else {
                inGrouping = false;
            }
            continue;
        }

        if (inWindow) {
            if (line.find(L"TrayMode=") == 0)
                g_trayMode = (_wtoi(line.substr(9).c_str()) != 0);
            continue;
        }

        if (inColor) {
            if (line.find(L"Opacity=") == 0)
                g_winOpacity = _wtoi(line.substr(8).c_str());
            else if (line.find(L"Hue=") == 0)
                g_themeHue = _wtoi(line.substr(4).c_str());
            else if (line.find(L"Brightness=") == 0)
                g_themeBright = _wtoi(line.substr(11).c_str());
            else if (line.find(L"Gray=") == 0)
                g_grayMode = (_wtoi(line.substr(5).c_str()) != 0);
            else if (line.find(L"GrayLevel=") == 0)
                g_grayLevel = _wtoi(line.substr(10).c_str());
            continue;
        }

        if (!inGrouping) continue;

        if (line.find(L"number=") == 0) {
            continue;
        }

        if (line.find(L"name=") == 0) {
            currentGroup.name = line.substr(5);
            continue;
        }

        if (line.find(L'|') != std::wstring::npos) {
            std::vector<std::wstring> parts;
            size_t start = 0, pos = 0;
            while ((pos = line.find(L'|', start)) != std::wstring::npos) {
                parts.push_back(line.substr(start, pos - start));
                start = pos + 1;
            }
            parts.push_back(line.substr(start));

            if (parts.size() >= 3) {
                int seq = _wtoi(parts[0].c_str());
                std::wstring name = parts[1];
                std::wstring path = parts[2];
                std::wstring args = parts.size() >= 4 ? parts[3] : L"";

                if (!path.empty()) {
                    std::wstring realPath = ExpandPath(path);
                    // BuildItem 内部完成 .lnk 解析 / 图标 / missing 检查
                    g_items.push_back(BuildItem(name, realPath, args));
                    int idx = (int)g_items.size() - 1;

                    if (seq > 0) {
                        int targetIdx = seq - 1;
                        if ((int)currentGroup.items.size() <= targetIdx) {
                            currentGroup.items.resize(targetIdx + 1, -1);
                        }
                        currentGroup.items[targetIdx] = idx;
                    }
                    else {
                        currentGroup.items.push_back(idx);
                    }
                }
            }
        }
    }

    if (hasGroup) {
        g_groups.push_back(currentGroup);
    }

    for (auto& g : g_groups) {
        while (!g.items.empty() && g.items.back() == -1) {
            g.items.pop_back();
        }
    }

    RemoveEmptyGroups();

    // 修正主题参数到合法范围
    if (g_winOpacity < 0) g_winOpacity = 0;
    if (g_winOpacity > 100) g_winOpacity = 100;
    if (g_themeHue < 0) g_themeHue = 0;
    if (g_themeHue > 360) g_themeHue = 360;
    if (g_themeBright < 0) g_themeBright = 0;
    if (g_themeBright > 100) g_themeBright = 100;
    if (g_grayLevel < 0) g_grayLevel = 0;
    if (g_grayLevel > 100) g_grayLevel = 100;
}

// ============================================================
//  主窗口消息处理
// ============================================================
static void ToggleMaximize()
{
    if (IsZoomed(g_hWnd))
        ShowWindow(g_hWnd, SW_RESTORE);
    else
        ShowWindow(g_hWnd, SW_MAXIMIZE);
    // 最大化/还原状态变化后，立即更新配置文件
    SaveConfig();
}

static void ScrollToLetter(wchar_t c)
{
    for (auto& r : g_listRows)
    {
        if (r.header && r.letter == c)
        {
            RECT rc; GetClientRect(g_hWnd, &rc);
            int viewH = rc.bottom - TITLE_H;
            int maxScroll = g_listHeight - viewH;
            if (maxScroll < 0) maxScroll = 0;

            g_listScroll = r.y - S(8);
            if (g_listScroll > maxScroll) g_listScroll = maxScroll;
            if (g_listScroll < 0) g_listScroll = 0;
            return;
        }
    }
}

static void UpdateHover(POINT pt)
{
    g_mouse = pt;

    g_hoverBtn = HitButton(pt);
    g_hoverLetter = HitLetter(pt);
    g_hoverListIdx = HitListItem(pt);
    g_hoverSearch = HitSearchBox(pt);
    g_hoverSearchClear = HitSearchClear(pt);
    g_hoverSettingsBtn = HitSettingsButton(pt);

    int tg = -1, ti = -1;
    HitTile(pt, tg, ti);
    g_hoverTileG = tg;
    g_hoverTileI = ti;
}

// ============================================================
//  搜索输入框（EDIT 子窗口，外壳由父窗口自绘）
// ============================================================
static void ClearSearch();

static void OnSearchChanged()
{
    if (!g_hSearchEdit) return;

    wchar_t buf[256] = { 0 };
    GetWindowTextW(g_hSearchEdit, buf, 256);

    std::wstring s = buf;
    if (s == g_searchText) return;
    g_searchText = s;

    if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
}

static void ClearSearch()
{
    if (!g_hSearchEdit) return;

    // 置空文本会触发 EN_CHANGE，从而清空 g_searchText 并重绘
    SetWindowTextW(g_hSearchEdit, L"");
    if (!g_searchText.empty())
    {
        g_searchText.clear();
        if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
    }
}

// 把输入框摆到胶囊形外壳内部（左侧文字区）
static void UpdateSearchEditLayout()
{
    if (!g_hSearchEdit) return;

    RECT box = GetSearchBoxRect();
    RECT clearRc = GetSearchClearRect();

    int x = box.left + S(22);
    int right = clearRc.left - S(4);
    if (right < x + S(40)) right = x + S(40);

    HDC hdc = GetDC(g_hSearchEdit);
    TEXTMETRICW tm = { 0 };
    if (hdc)
    {
        HFONT old = (HFONT)SelectObject(hdc, g_fontUI);
        GetTextMetricsW(hdc, &tm);
        SelectObject(hdc, old);
        ReleaseDC(g_hSearchEdit, hdc);
    }

    // 单行 EDIT 的文本总是从客户区顶部开始绘制，所以这里按字体行高
    // 反推窗口位置，让整行文字（含光标的行盒）正好落在搜索框垂直中线上。
    int textH = tm.tmHeight;
    if (textH <= 0) textH = S(16);

    int th = textH + S(2);
    if (th < S(16)) th = S(16);

    // 四舍五入，避免 /2 截断导致文字整体偏上 1~2 像素
    int y = (box.top + box.bottom - textH + 1) / 2;

    SendMessageW(g_hSearchEdit, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SetWindowPos(g_hSearchEdit, nullptr, x, y, right - x, th,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

static LRESULT CALLBACK SearchEditSubclassProc(HWND hwnd, UINT msg, WPARAM wp,
    LPARAM lp, UINT_PTR id, DWORD_PTR ref)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { ClearSearch(); return 0; }
        if (wp == VK_RETURN) return 0;
        break;

    case WM_CHAR:
        // 单行搜索框：屏蔽回车/换行
        if (wp == VK_RETURN || wp == L'\r' || wp == L'\n') return 0;
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE);
        break;

    case WM_MOUSEMOVE:
    {
        // 鼠标在输入框上移动时也要刷新搜索框的悬停高亮
        POINT pt; GetCursorPos(&pt);
        if (g_hWnd) ScreenToClient(g_hWnd, &pt);
        RefreshSearchHover(pt);

        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE:
    {
        POINT pt; GetCursorPos(&pt);
        if (g_hWnd) ScreenToClient(g_hWnd, &pt);
        RefreshSearchHover(pt);
        break;
    }

    case WM_IME_STARTCOMPOSITION:
        g_searchComposing = true;
        break;

    case WM_IME_ENDCOMPOSITION:
        g_searchComposing = false;
        break;

    case WM_PAINT:
    {
        LRESULT r = DefSubclassProc(hwnd, msg, wp, lp);

        // 内容为空时在左侧显示「搜索」提示文字；
        // 输入法组字过程中不绘制，避免与组字串重叠
        if (!g_searchComposing)
        {
            wchar_t buf[2] = { 0 };
            if (GetWindowTextW(hwnd, buf, 2) == 0)
            {
                HDC hdc = GetDC(hwnd);
                if (hdc)
                {
                    RECT rct; GetClientRect(hwnd, &rct);
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, g_uiTextFaint);
                    HFONT old = (HFONT)SelectObject(hdc, g_fontUI);
                    // 与 EDIT 自身的文字一样从客户区顶部开始绘制，保证提示文字
                    // 与输入后的文字在同一基线上
                    DrawTextW(hdc, L"搜索", -1, &rct,
                        DT_LEFT | DT_TOP | DT_SINGLELINE);
                    SelectObject(hdc, old);
                    ReleaseDC(hwnd, hdc);
                }
            }
        }
        return r;
    }
    }

    return DefSubclassProc(hwnd, msg, wp, lp);
}

static void CreateSearchEdit()
{
    if (g_hSearchEdit) return;

    RECT box = GetSearchBoxRect();
    g_hSearchEdit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        box.left + S(22), box.top + S(8), S(80), S(20),
        g_hWnd, (HMENU)(INT_PTR)1001, g_hInst, nullptr);
    if (!g_hSearchEdit) return;

    SendMessageW(g_hSearchEdit, WM_SETFONT, (WPARAM)g_fontUI, TRUE);
    SetWindowSubclass(g_hSearchEdit, SearchEditSubclassProc, 1, 0);
    UpdateSearchEditLayout();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    // 资源管理器重启后会广播 TaskbarCreated，需要重新添加托盘图标
    if (g_wmTaskbarCreated && msg == g_wmTaskbarCreated)
    {
        if (g_trayMode)
        {
            g_trayIconAdded = false;
            AddTrayIcon();
        }
        return 0;
    }

    switch (msg)
    {
    case WM_CREATE:
        DragAcceptFiles(hwnd, TRUE);
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig);
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);
        return 0;

    case WM_ACTIVATE:
    {
        if (LOWORD(wp) == WA_INACTIVE)
        {
            // 托盘模式：点击主窗口以外的位置时自动隐藏
            // g_mainWasActive 用于避免程序刚启动、尚未获得焦点时就被隐藏
            if (g_trayMode && g_mainWasActive && !g_reallyExit && !g_hSettings)
            {
                HWND next = (HWND)lp;
                DWORD pid = 0;
                if (next) GetWindowThreadProcessId(next, &pid);
                if (pid != GetCurrentProcessId())
                    HideMainWindow();
            }
        }
        else
        {
            g_mainWasActive = true;
        }
        break;
    }

    case WM_TRAYICON:
        if (LOWORD(lp) == WM_LBUTTONUP || LOWORD(lp) == WM_RBUTTONUP)
        {
            POINT pt; GetCursorPos(&pt);
            ShowTrayMenu(pt);
        }
        return 0;

    case WM_TOGGLE_MAIN:
        ToggleMainWindow();
        return 0;

    case WM_GETICON:
        if (wp == ICON_BIG && g_hIconBig) return (LRESULT)g_hIconBig;
        if (wp == ICON_SMALL && g_hIconSmall) return (LRESULT)g_hIconSmall;
        break;

    case WM_STARTUP_CHECK:
    {
        CheckAllItems();
        std::vector<int> miss;
        for (int i = 0; i < (int)g_items.size(); i++)
            if (g_items[i].missing) miss.push_back(i);

        if (!miss.empty())
        {
            InvalidateRect(g_hWnd, nullptr, FALSE);
            UpdateWindow(g_hWnd);
            ShowMissingFilesDialog(miss);

            // 弹窗关闭后再次检查，确保状态一致
            CheckAllItems();
            RebuildList();
            LayoutGroups();
            InvalidateRect(g_hWnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_TIMER:
    {
        if (wp == 1001)
        {
            static int nTick = 0;
            nTick++;

            // 至少等 3 个 tick（约 300ms），覆盖 DWM 窗口显示动画
            if (nTick < 3) return 0;

            // 500ms 后如果主窗口还不是前台，继续等（再等约 2.5 秒兜底）
            if (GetForegroundWindow() != hwnd && nTick < 30)
                return 0;

            nTick = 0;
            KillTimer(hwnd, 1001);
            PostMessage(hwnd, WM_STARTUP_CHECK, 0, 0);
        }
        return 0;
    }

    case WM_NCACTIVATE:
        // 阻止系统默认的对非客户区的重绘，否则会导致 WS_EX_LAYERED 窗口失去焦点时变透明
        return TRUE;

    case WM_ENABLE:
        if (wp) {
            // 仅标记重绘，避免弹窗关闭激活主窗口时强制刷新引起闪烁
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

        // ========================================================
        //  DPI 变化：重算尺寸、重建字体、按系统建议调整窗口大小
        // ========================================================
    case WM_DPICHANGED:
    {
        int newDpi = (int)HIWORD(wp);
        if (newDpi == 0) newDpi = (int)LOWORD(wp);
        int oldDpi = g_dpi;
        if (newDpi > 0 && newDpi != g_dpi)
        {
            g_dpi = newDpi;
            UpdateDpiSizes();
            CreateFonts();

            // 滚动位置按比例迁移
            g_listScroll = MulDiv(g_listScroll, g_dpi, oldDpi);
            g_tileScroll = MulDiv(g_tileScroll, g_dpi, oldDpi);
        }

        RECT* prc = (RECT*)lp;
        if (prc)
        {
            SetWindowPos(hwnd, nullptr,
                prc->left, prc->top,
                prc->right - prc->left, prc->bottom - prc->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }

        LayoutGroups();
        UpdateSearchEditLayout();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_SIZE:
    {
        RECT rc; GetClientRect(hwnd, &rc);
        int viewH = rc.bottom - TITLE_H;
        int maxScroll = g_listHeight - viewH;
        if (maxScroll < 0) maxScroll = 0;
        if (g_listScroll > maxScroll) g_listScroll = maxScroll;
        if (g_listScroll < 0) g_listScroll = 0;

        LayoutGroups();
        UpdateSearchEditLayout();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    // ========================================================
    //  用户拖动调整大小或移动窗口结束后，立即保存窗口配置
    // ========================================================
    case WM_EXITSIZEMOVE:
    {
        SaveConfig();
        return 0;
    }

    // ========================================================
    //  拦截系统菜单触发的最大化 / 还原（例如任务栏右键菜单、
    //  Alt+Space 系统菜单等），处理完成后立即保存窗口配置
    // ========================================================
    case WM_SYSCOMMAND:
    {
        UINT cmd = (UINT)(wp & 0xFFF0);

        // 托盘模式：系统菜单的最小化/关闭也只隐藏窗口
        if (g_trayMode && cmd == SC_MINIMIZE)
        {
            HideMainWindow();
            return 0;
        }

        if (cmd == SC_MAXIMIZE || cmd == SC_RESTORE)
        {
            LRESULT r = DefWindowProcW(hwnd, msg, wp, lp);
            SaveConfig();
            return r;
        }
        break;
    }

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lp;
        MONITORINFO mi; mi.cbSize = sizeof(mi);
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
        mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
        mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
        mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
        mmi->ptMinTrackSize.x = MIN_W;
        mmi->ptMinTrackSize.y = MIN_H;
        return 0;
    }

    case WM_NCCALCSIZE:
        if (wp) {
            NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lp;
            if (IsZoomed(hwnd)) {
                // 最大化时强制客户区对齐到工作区，解决超屏和溢出问题
                MONITORINFO mi = { sizeof(mi) };
                GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
                params->rgrc[0] = mi.rcWork;
            }
            return 0;
        }
        break;

    case WM_NCHITTEST:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        RECT rc; GetClientRect(hwnd, &rc);

        if (!IsZoomed(hwnd))
        {
            int B = S(6);
            bool L = pt.x < B, R = pt.x >= rc.right - B;
            bool T = pt.y < B, Bm = pt.y >= rc.bottom - B;

            if (T && L)  return HTTOPLEFT;
            if (T && R)  return HTTOPRIGHT;
            if (Bm && L) return HTBOTTOMLEFT;
            if (Bm && R) return HTBOTTOMRIGHT;
            if (L)  return HTLEFT;
            if (R)  return HTRIGHT;
            if (T)  return HTTOP;
            if (Bm) return HTBOTTOM;
        }

        if (pt.y < TITLE_H)
        {
            if (pt.x >= rc.right - BTN_W * 3) return HTCLIENT;
            return HTCAPTION;
        }
        return HTCLIENT;
    }

    case WM_NCLBUTTONDBLCLK:
        if (wp == HTCAPTION)
        {
            ToggleMaximize();
            return 0;
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc; GetClientRect(hwnd, &rc);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc,
            max(1, rc.right), max(1, rc.bottom));
        HGDIOBJ ob = SelectObject(memDC, bmp);

        Paint(memDC);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, ob);
        DeleteObject(bmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        // 请求鼠标离开通知，确保移出窗口后能清除悬停高亮
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);

        if (g_listScrollDrag)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            RECT rcTrack, rcThumb;
            if (GetListScrollBar(rcTrack, rcThumb))
            {
                int trackH = rcTrack.bottom - rcTrack.top;
                int thumbH = rcThumb.bottom - rcThumb.top;
                int viewH = rc.bottom - TITLE_H;
                int maxScroll = g_listHeight - viewH;
                int span = trackH - thumbH;
                if (span > 0 && maxScroll > 0)
                {
                    int dy = pt.y - g_listScrollDragStartY;
                    int newScroll = g_listScrollDragStartOffset + dy * maxScroll / span;
                    if (newScroll < 0) newScroll = 0;
                    if (newScroll > maxScroll) newScroll = maxScroll;
                    if (newScroll != g_listScroll)
                    {
                        g_listScroll = newScroll;
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                }
            }
            return 0;
        }

        if (g_tileScrollDrag)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            RECT rcTrack, rcThumb;
            if (GetTileScrollBar(rcTrack, rcThumb))
            {
                int trackH = rcTrack.bottom - rcTrack.top;
                int thumbH = rcThumb.bottom - rcThumb.top;
                int viewH = TileViewH();
                int maxScroll = g_tileContentH - viewH;
                int span = trackH - thumbH;
                if (span > 0 && maxScroll > 0)
                {
                    int dy = pt.y - g_tileScrollDragStartY;
                    int newScroll = g_tileScrollDragStartOffset + dy * maxScroll / span;
                    if (newScroll < 0) newScroll = 0;
                    if (newScroll > maxScroll) newScroll = maxScroll;
                    if (newScroll != g_tileScroll)
                    {
                        g_tileScroll = newScroll;
                        LayoutGroups();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                }
            }
            return 0;
        }

        if (g_hEdit)
        {
            g_mouse = pt;
            return 0;
        }

        if (g_dragTile)
        {
            UpdateTileDrag(pt);
            LayoutGroups(); // 实时重排（实现高度自动拉伸）
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        // 分组标题栏：按下后移动超过阈值 -> 进入拖动模式
        if (g_dragGroupPending && (wp & MK_LBUTTON))
        {
            int dragThreshold = S(4);
            if (abs(pt.x - g_dragGroupStart.x) > dragThreshold ||
                abs(pt.y - g_dragGroupStart.y) > dragThreshold)
            {
                g_dragGroupPending = false;
                g_dragGroup = true;
                UpdateGroupDrag(pt);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }

        if (g_dragGroup)
        {
            UpdateGroupDrag(pt);
            return 0;
        }

        if (g_dragPending && (wp & MK_LBUTTON))
        {
            int dragThreshold = S(4);
            if (abs(pt.x - g_dragStart.x) > dragThreshold ||
                abs(pt.y - g_dragStart.y) > dragThreshold)
            {
                g_dragPending = false;
                g_dragTile = true;
                g_dragNewGroup = false;
                g_dragTargetG = -1;
                g_dragTargetPos = -1;
                g_dragTargetMode = 0;
                RECT r = TileRect(g_dragG, g_dragI);
                g_dragOffset.x = g_dragStart.x - r.left;
                g_dragOffset.y = g_dragStart.y - r.top;
                g_dragRect = r;

                SetCapture(hwnd);
                UpdateTileDrag(pt);
                LayoutGroups();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }

        UpdateHover(pt);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    // 鼠标移出客户区时清除所有悬停状态
    case WM_MOUSELEAVE:
    {
        g_mouse = { -1000, -1000 };
        g_hoverBtn = 0;
        g_hoverLetter = -1;
        g_hoverListIdx = -1;
        g_hoverTileG = -1;
        g_hoverTileI = -1;
        g_hoverSettingsBtn = false;

        // 鼠标可能只是移动到了搜索框的 EDIT 子窗口上，
        // 用真实光标位置判断，避免边框闪烁
        POINT pt; GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        RefreshSearchHover(pt);

        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        // 搜索框左侧的齿轮设置按钮
        if (HitSettingsButton(pt))
        {
            SetFocus(hwnd);
            ShowSettingsDialog();
            return 0;
        }

        // 搜索框：点右侧的叉清空搜索；点其它位置把焦点交给输入框
        if (g_hSearchEdit && HitSearchBox(pt))
        {
            if (HitSearchClear(pt))
            {
                ClearSearch();
                SetFocus(g_hSearchEdit);
                return 0;
            }
            SetFocus(g_hSearchEdit);
            SendMessageW(g_hSearchEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            return 0;
        }

        SetFocus(hwnd);

        int b = HitButton(pt);
        if (b > 0)
        {
            g_pressedTitleBtn = b;
            SetCapture(hwnd);
            return 0;
        }

        {
            RECT rcTrack, rcThumb;
            if (GetListScrollBar(rcTrack, rcThumb))
            {
                if (PtInRect(&rcThumb, pt))
                {
                    g_listScrollDrag = true;
                    g_listScrollDragStartY = pt.y;
                    g_listScrollDragStartOffset = g_listScroll;
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                else if (PtInRect(&rcTrack, pt))
                {
                    RECT rc; GetClientRect(hwnd, &rc);
                    int viewH = rc.bottom - TITLE_H;
                    int maxScroll = g_listHeight - viewH;
                    if (pt.y < rcThumb.top)
                        g_listScroll -= viewH;
                    else
                        g_listScroll += viewH;
                    if (g_listScroll < 0) g_listScroll = 0;
                    if (g_listScroll > maxScroll) g_listScroll = maxScroll;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
        }

        {
            RECT rcTrack, rcThumb;
            if (GetTileScrollBar(rcTrack, rcThumb))
            {
                if (PtInRect(&rcThumb, pt))
                {
                    g_tileScrollDrag = true;
                    g_tileScrollDragStartY = pt.y;
                    g_tileScrollDragStartOffset = g_tileScroll;
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                else if (PtInRect(&rcTrack, pt))
                {
                    RECT rc; GetClientRect(hwnd, &rc);
                    int viewH = TileViewH();
                    int maxScroll = g_tileContentH - viewH;
                    if (pt.y < rcThumb.top)
                        g_tileScroll -= viewH;
                    else
                        g_tileScroll += viewH;
                    if (g_tileScroll < 0) g_tileScroll = 0;
                    if (g_tileScroll > maxScroll) g_tileScroll = maxScroll;
                    LayoutGroups();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
        }

        int li = HitLetter(pt);
        if (li >= 0)
        {
            ScrollToLetter(g_letters[li]);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        int gt = HitGroupTitle(pt);
        if (gt >= 0)
        {
            // 标题栏：先进入"待定"状态；
            //   移动超过阈值 -> 拖动调整位置
            //   原地松开 -> 单击重命名
            g_dragGroupPending = true;
            g_dragGroupIdx = gt;
            g_dragGroupStart = pt;
            SetCapture(hwnd);
            return 0;
        }

        int tg, ti;
        if (HitTile(pt, tg, ti))
        {
            g_dragPending = true;
            g_dragG = tg;
            g_dragI = ti;
            g_dragStart = pt;
            g_dragTargetMode = 0;
            SetCapture(hwnd);
            return 0;
        }

        return 0;
    }

    case WM_LBUTTONUP:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        if (g_listScrollDrag || g_tileScrollDrag)
        {
            g_listScrollDrag = false;
            g_tileScrollDrag = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (g_pressedTitleBtn > 0)
        {
            int pressed = g_pressedTitleBtn;
            g_pressedTitleBtn = 0;
            if (GetCapture() == hwnd) ReleaseCapture();

            int b = HitButton(pt);
            if (b == pressed)
            {
                if (b == 1)
                {
                    // 托盘模式：最小化按钮只隐藏窗口
                    if (g_trayMode) HideMainWindow();
                    else ShowWindow(hwnd, SW_MINIMIZE);
                }
                else if (b == 2) ToggleMaximize();
                else if (b == 3) PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;
        }

        // 分组标题栏：未移动即松开 => 单击 -> 进入重命名
        if (g_dragGroupPending)
        {
            g_dragGroupPending = false;
            if (GetCapture() == hwnd) ReleaseCapture();

            int gt = HitGroupTitle(pt);
            if (gt >= 0 && gt == g_dragGroupIdx)
                StartEditGroupName(gt);
            g_dragGroupIdx = -1;
            return 0;
        }

        if (g_dragTile)
        {
            UpdateTileDrag(pt);
            DropTile(pt);
            g_dragTile = false;
            g_dragPending = false;
            g_dragG = g_dragI = -1;
            g_dragTargetG = -1;
            g_dragTargetPos = -1;
            g_dragTargetMode = 0;
            g_dragNewGroup = false;
            ReleaseCapture();
            LayoutGroups();
            SaveConfig();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (g_dragGroup)
        {
            g_dragGroup = false;
            g_dragGroupIdx = -1;
            ReleaseCapture();
            LayoutGroups();
            SaveConfig();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (g_dragPending)
        {
            g_dragPending = false;
            ReleaseCapture();

            if (g_dragG >= 0 && g_dragG < (int)g_groups.size() &&
                g_dragI >= 0 && g_dragI < (int)g_groups[g_dragG].items.size())
            {
                int idx = g_groups[g_dragG].items[g_dragI];
                if (idx != -1) LaunchItem(idx);
            }
            g_dragG = g_dragI = -1;
            g_dragTargetMode = 0;
            return 0;
        }

        int li = HitListItem(pt);
        if (li >= 0 && li < (int)g_listRows.size())
        {
            int idx = g_listRows[li].itemIdx;
            LaunchItem(idx);
        }
        return 0;
    }

    case WM_CAPTURECHANGED:
        if (g_pressedTitleBtn > 0) g_pressedTitleBtn = 0;
        if (g_dragGroupPending) g_dragGroupPending = false;
        if (g_dragGroup) { g_dragGroup = false; g_dragGroupIdx = -1; }
        if (g_listScrollDrag) { g_listScrollDrag = false; InvalidateRect(hwnd, nullptr, FALSE); }
        if (g_tileScrollDrag) { g_tileScrollDrag = false; InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;

    case WM_RBUTTONUP:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

        int tg, ti;
        if (HitTile(pt, tg, ti))
        {
            g_menuTargetItem = g_groups[tg].items[ti];
            ShowContextMenu(pt);
            return 0;
        }

        int li = HitListItem(pt);
        if (li >= 0 && li < (int)g_listRows.size())
        {
            g_menuTargetItem = g_listRows[li].itemIdx;
            ShowContextMenu(pt);
            return 0;
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        int delta = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;

        if (pt.x < LIST_W)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            int viewH = rc.bottom - TITLE_H;
            int maxScroll = g_listHeight - viewH;
            if (maxScroll < 0) maxScroll = 0;

            g_listScroll -= delta * S(60);
            if (g_listScroll < 0) g_listScroll = 0;
            if (g_listScroll > maxScroll) g_listScroll = maxScroll;
        }
        else
        {
            RECT rc; GetClientRect(hwnd, &rc);
            int viewH = TileViewH();
            int maxScroll = g_tileContentH - viewH;
            if (maxScroll < 0) maxScroll = 0;

            g_tileScroll -= delta * S(60);
            if (g_tileScroll < 0) g_tileScroll = 0;
            if (g_tileScroll > maxScroll) g_tileScroll = maxScroll;

            LayoutGroups();
        }

        // 滚动后列表内容移动，鼠标下的项可能变了，需要重新计算悬停状态
        UpdateHover(pt);

        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_DROPFILES:
    {
        HDROP hd = (HDROP)wp;
        UINT n = DragQueryFileW(hd, 0xFFFFFFFF, nullptr, 0);

        std::vector<std::wstring> files;
        for (UINT i = 0; i < n; i++)
        {
            wchar_t buf[MAX_PATH * 2] = { 0 };
            DragQueryFileW(hd, i, buf, MAX_PATH * 2);
            files.push_back(buf);
        }

        POINT pt; DragQueryPoint(hd, &pt);
        DragFinish(hd);

        // 直接调用 ProcessDroppedFiles（原 HandleDroppedFiles 仅为一行转发，已删除）
        ProcessDroppedFiles(files, pt);
        return 0;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, g_uiText);
        SetBkColor(hdc, g_uiSurface);
        return (LRESULT)g_hbrEdit;
    }

    case WM_COMMAND:
    {
        if (g_hSearchEdit && (HWND)lp == g_hSearchEdit)
        {
            WORD code = HIWORD(wp);
            if (code == EN_CHANGE) OnSearchChanged();
            else if (code == EN_SETFOCUS || code == EN_KILLFOCUS)
                InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        // 关闭前若仍处于分组名行内编辑状态，先提交
        if (g_hEdit) EndEditGroupName(true);
        SaveConfig();

        // 托盘模式：关闭按钮只隐藏窗口，真正退出请用托盘菜单
        if (g_trayMode && !g_reallyExit)
        {
            HideMainWindow();
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1001);   // 清理启动检查定时器
        RemoveTrayIcon();
        UninstallTrayHotkey();
        RevokeDragDrop(hwnd);
        if (g_pDropTarget)
        {
            g_pDropTarget->Release();
            g_pDropTarget = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ============================================================
//  WinMain
// ============================================================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    g_hInst = hInst;

    // ========================================================
    //  ★ 新增：单实例检查
    //  若已存在运行中的实例，把它的窗口激活到前台，然后直接退出。
    //  互斥量名默认即为 Local 命名空间（当前登录会话）。
    // ========================================================
    HANDLE hSingleMutex = CreateMutexW(nullptr, FALSE,
        L"SMStartMenu_SingleInstance");
    if (hSingleMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // 轮询等待已运行实例的主窗口出现（最多约 1 秒），避免先启动进程
        // 还在初始化阶段时因 FindWindow 找不到而误判
        HWND hPrev = nullptr;
        for (int i = 0; i < 20 && !hPrev; i++)
        {
            hPrev = FindWindowW(L"SMStartMenu", nullptr);
            if (!hPrev) Sleep(50);
        }

        if (hPrev)
        {
            // 若最小化，先还原；否则仅保证可见
            if (IsIconic(hPrev))
                ShowWindow(hPrev, SW_RESTORE);
            else
                ShowWindow(hPrev, SW_SHOW);

            // 尝试激活到前台
            SetForegroundWindow(hPrev);

            // 某些场景下（如另一个进程正在前台）SetForegroundWindow 会失败，
            // 用 AttachThreadInput 短暂附加到前台线程作为兜底
            if (GetForegroundWindow() != hPrev)
            {
                DWORD fgThread = GetWindowThreadProcessId(
                    GetForegroundWindow(), nullptr);
                DWORD myThread = GetCurrentThreadId();
                if (fgThread && fgThread != myThread)
                {
                    AttachThreadInput(myThread, fgThread, TRUE);
                    SetForegroundWindow(hPrev);
                    BringWindowToTop(hPrev);
                    AttachThreadInput(myThread, fgThread, FALSE);
                }
            }
        }

        CloseHandle(hSingleMutex);
        return 0;
    }

    // 第一步：在创建任何窗口之前启用 DPI 感知
    // （Win7/8.1/10+ 会自动选用最合适的一种）
    EnableDpiAwareness();

    // 读取初始 DPI 并计算所有尺寸
    g_dpi = (int)GetWindowDpiCompat(nullptr);
    if (g_dpi <= 0) g_dpi = 96;
    UpdateDpiSizes();

    // 创建字体（已按 DPI 缩放）
    CreateFonts();

    // OLE 初始化（RegisterDragDrop 需要 OLE 初始化）
    if (FAILED(OleInitialize(nullptr)))
    {
        if (hSingleMutex) CloseHandle(hSingleMutex);
        return 0;
    }
    InitCommonControls();

    g_exeDir = GetExeDir();

    // 生成程序名.cfg
    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeName = PathFindFileNameW(exePath);
    std::wstring baseName = StripExt(exeName);
    g_cfgPath = g_exeDir + L"\\" + baseName + L".cfg";

    // 使用程序内嵌的资源图标作为标题栏和任务栏图标
    g_hIconBig = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    g_hIconSmall = LoadIconW(hInst, MAKEINTRESOURCEW(1));

    if (!g_hIconBig)   g_hIconBig = LoadIconW(nullptr, IDI_APPLICATION);
    if (!g_hIconSmall) g_hIconSmall = LoadIconW(nullptr, IDI_APPLICATION);

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"SMStartMenu";
    wc.hIcon = g_hIconBig;
    wc.hIconSm = g_hIconSmall;
    RegisterClassExW(&wc);

    WNDCLASSEXW mc = { 0 };
    mc.cbSize = sizeof(mc);
    mc.lpfnWndProc = MenuProc;
    mc.hInstance = hInst;
    mc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    mc.lpszClassName = L"SMRightMenu";
    mc.hbrBackground = nullptr;
    RegisterClassExW(&mc);

    WNDCLASSEXW rc2 = { 0 };
    rc2.cbSize = sizeof(rc2);
    rc2.lpfnWndProc = RenameProc;
    rc2.hInstance = hInst;
    rc2.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    rc2.lpszClassName = L"SMRenameDlg";
    rc2.hbrBackground = nullptr;
    rc2.hIcon = g_hIconBig;
    RegisterClassExW(&rc2);

    // 新增：文件缺失提示对话框窗口类（复用重命名窗口的直角自绘样式）
    WNDCLASSEXW mdc = { 0 };
    mdc.cbSize = sizeof(mdc);
    mdc.lpfnWndProc = MissingDlgProc;
    mdc.hInstance = hInst;
    mdc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    mdc.lpszClassName = L"SMMissingDlg";
    mdc.hbrBackground = nullptr;
    mdc.hIcon = g_hIconBig;
    RegisterClassExW(&mdc);

    // 设置窗口类
    WNDCLASSEXW sdc = { 0 };
    sdc.cbSize = sizeof(sdc);
    sdc.lpfnWndProc = SettingsProc;
    sdc.hInstance = hInst;
    sdc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    sdc.lpszClassName = L"SMSettingsDlg";
    sdc.hbrBackground = nullptr;
    sdc.hIcon = g_hIconBig;
    RegisterClassExW(&sdc);

    // 资源管理器重启广播：用于重新添加托盘图标
    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    int w = MulDiv(DEFAULT_W, g_dpi, 96), h = MulDiv(DEFAULT_H, g_dpi, 96);
    int x = -10000, y = -10000;
    bool startMaximized = false;   // 上次关闭时窗口是否处于最大化状态
    std::wstring winContent = ReadAllText(g_cfgPath);
    if (!winContent.empty()) {
        std::wstringstream ss(winContent);
        std::wstring line;
        bool inWin = false;
        while (std::getline(ss, line)) {
            if (line == L"[Window]" || line == L"[Windows]") { inWin = true; continue; }
            if (!line.empty() && line[0] == L'[' &&
                line != L"[Window]" && line != L"[Windows]") { inWin = false; }
            if (inWin) {
                // 读取最大化状态；宽高保存在配置文件中为 96 DPI 逻辑单位，此处还原为物理像素
                if (line.find(L"Maximized=") == 0)
                    startMaximized = (_wtoi(line.substr(10).c_str()) != 0);
                else if (line.find(L"Width=") == 0)
                    w = MulDiv(_wtoi(line.substr(6).c_str()), g_dpi, 96);
                else if (line.find(L"Height=") == 0)
                    h = MulDiv(_wtoi(line.substr(7).c_str()), g_dpi, 96);
                else if (line.find(L"X=") == 0)
                    x = _wtoi(line.substr(2).c_str());
                else if (line.find(L"Y=") == 0)
                    y = _wtoi(line.substr(2).c_str());
            }
        }
    }

    if (w < MIN_W) w = MIN_W;
    if (h < MIN_H) h = MIN_H;

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    if (x < -5000 || x > sw) x = (sw - w) / 2;
    if (y < -5000 || y > sh) y = (sh - h) / 2;

    g_hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_APPWINDOW,
        L"SMStartMenu", L"开始菜单",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        x, y, w, h,
        nullptr, nullptr, hInst, nullptr);

    if (!g_hWnd)
    {
        OleUninitialize();
        if (hSingleMutex) CloseHandle(hSingleMutex);
        return 0;
    }

    SetLayeredWindowAttributes(g_hWnd, 0, 238, LWA_ALPHA);

    // 注册 OLE 拖放目标（让外部拖入时有实时的高亮反馈）
    {
        CStartMenuDropTarget* pdt = new CStartMenuDropTarget();
        if (SUCCEEDED(RegisterDragDrop(g_hWnd, pdt)))
        {
            pdt->Release();          // OLE 已 AddRef，释放本地引用
            g_pDropTarget = pdt;     // 保存原始指针供退出时使用
            pdt->AddRef();           // 自己保留一份引用，便于退出时 Release
        }
        else
        {
            pdt->Release();
        }
    }

    LoadConfig();
    RebuildList();
    LayoutGroups();

    // 创建后核对一次 DPI（多显示器下窗口可能落在不同 DPI 的屏幕上）
    // ★ 修复：若窗口实际 DPI 与初始不同，必须重新计算尺寸并重建字体，
    //        否则字体 / 布局仍按错误 DPI 缩放
    {
        UINT d = GetWindowDpiCompat(g_hWnd);
        if (d && (int)d != g_dpi)
        {
            g_dpi = (int)d;
            UpdateDpiSizes();
            CreateFonts();
            LayoutGroups();
        }
    }

    // 创建搜索框（右侧磁贴区域底部）
    CreateSearchEdit();

    // 应用主题（窗口背景色 / 不透明度）与托盘模式设置
    UpdateThemeColor();
    ApplyWindowAlpha();
    if (g_trayMode) ApplyTrayMode(true);

    // 若上次关闭时处于最大化状态，则直接以最大化显示
    ShowWindow(g_hWnd, startMaximized ? SW_SHOWMAXIMIZED : SW_SHOW);
    UpdateWindow(g_hWnd);

    // 延迟触发启动检查：等主窗口显示动画/激活完成后再弹提示框，
    // 避免提示框先创建在主窗口下面、动画结束后又翻到上面的视觉跳动
    SetTimer(g_hWnd, 1001, 100, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyFonts();

    OleUninitialize();

    // 释放单实例互斥量（进程结束也会自动释放，这里显式关闭更清晰）
    if (hSingleMutex) CloseHandle(hSingleMutex);

    return 0;
}