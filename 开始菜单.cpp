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

// 右键菜单
static HWND  g_hMenu = nullptr;
static std::vector<std::wstring> g_menuItems;
static int   g_menuHover = -1;
static int   g_menuPressedIdx = -1;
static int   g_menuTargetItem = -1;

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
    int viewH = rc.bottom - TITLE_H;
    if (g_tileContentH <= viewH) return false;

    int trackX = rc.right - SCROLLBAR_W - S(4);
    int trackTop = TITLE_H + SCROLLBAR_PAD;
    int trackBot = rc.bottom - SCROLLBAR_PAD;
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
    int viewH = rc.bottom - TITLE_H;
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
        ShellExecuteW(nullptr, L"open", it.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
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

    HBRUSH br = CreateSolidBrush(RGB(32, 32, 36));
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
    DrawText2(hdc, L"开始菜单", rtt, RGB(232, 232, 232), g_fontTitle,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    for (int b = 1; b <= 3; b++)
    {
        RECT rb = { rc.right - BTN_W * (4 - b), 0, rc.right - BTN_W * (3 - b), TITLE_H };
        if (g_hoverBtn == b)
        {
            COLORREF c = (b == 3) ? RGB(196, 43, 28) : RGB(60, 60, 66);
            HBRUSH hb = CreateSolidBrush(c);
            FillRect(hdc, &rb, hb);
            DeleteObject(hb);
        }

        int cx = (rb.left + rb.right) / 2;
        int cy = (rb.top + rb.bottom) / 2;
        HPEN pn = CreatePen(PS_SOLID, 1, RGB(230, 230, 230));
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
            MoveToEx(hdc, cx - S(5), cy - S(5), nullptr); LineTo(hdc, cx + S(5), cy + S(5));
            MoveToEx(hdc, cx + S(5), cy - S(5), nullptr); LineTo(hdc, cx - S(5), cy + S(5));
        }

        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(pn);
    }

    HPEN p = CreatePen(PS_SOLID, 1, RGB(48, 48, 54));
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

    HBRUSH br = CreateSolidBrush(RGB(28, 28, 31));
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
            DrawText2(hdc, std::wstring(1, r.letter), rh, RGB(140, 140, 150),
                g_fontSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        else
        {
            RECT ri = { LETTER_W + S(8), y, itemRight, y + ITEM_H };
            Item& it = g_items[r.itemIdx];

            // 文件缺失：背景变深红色
            if (it.missing)
            {
                COLORREF col = (i == g_hoverListIdx) ? RGB(180, 50, 50) : RGB(145, 35, 35);
                FillRoundRect(hdc, ri, S(4), col, col);
            }
            else if (i == g_hoverListIdx)
            {
                FillRoundRect(hdc, ri, S(4), RGB(48, 48, 54), RGB(48, 48, 54));
            }

            if (it.hIcon)
            {
                int sz = S(32); // 原尺寸绘制 32x32
                DrawIconEx(hdc, ri.left + S(8), ri.top + (ITEM_H - sz) / 2,
                    it.hIcon, sz, sz, 0, nullptr, DI_NORMAL);
            }

            RECT rt = { ri.left + S(48), ri.top, ri.right - S(6), ri.bottom };
            DrawText2(hdc, it.name, rt, RGB(220, 220, 225), g_fontUI,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }

    std::vector<RECT> lr;
    GetLetterRects(lr);
    for (int i = 0; i < (int)lr.size(); i++)
    {
        COLORREF c = (i == g_hoverLetter) ? RGB(90, 160, 255) : RGB(150, 150, 160);
        DrawText2(hdc, std::wstring(1, g_letters[i]), lr[i], c,
            g_fontSmall, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);

    HPEN p = CreatePen(PS_SOLID, 1, RGB(45, 45, 50));
    HGDIOBJ op = SelectObject(hdc, p);
    MoveToEx(hdc, LIST_W - 1, TITLE_H, nullptr);
    LineTo(hdc, LIST_W - 1, rc.bottom);
    SelectObject(hdc, op);
    DeleteObject(p);
}

static void PaintTiles(HDC hdc)
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    HRGN clip = CreateRectRgn(LIST_W, TITLE_H, rc.right, rc.bottom);
    SelectClipRgn(hdc, clip);

    for (int gi = 0; gi < (int)g_groups.size(); gi++)
    {
        Group& g = g_groups[gi];

        FillRoundRect(hdc, g.rc, S(10), RGB(34, 34, 38), RGB(34, 34, 38));

        // 标题栏文字：上下左右居中
        RECT rt = { g.rc.left + GROUP_PAD, g.rcTitle.top,
            g.rc.right - GROUP_PAD, g.rcTitle.bottom + S(2) };
        DrawText2(hdc, g.name, rt,
            RGB(190, 190, 200), g_fontSmall,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        for (int ii = 0; ii < (int)g.items.size(); ii++)
        {
            if (g.items[ii] == -1) continue;
            if (g_dragTile && g_dragG == gi && g_dragI == ii) continue;

            RECT r = TileRect(gi, ii);
            if (r.top > rc.bottom || r.bottom < TITLE_H) continue;

            Item& it = g_items[g.items[ii]];

            bool hot = (g_hoverTileG == gi && g_hoverTileI == ii);
            COLORREF col;
            // 文件缺失：背景变深红色
            if (it.missing)
            {
                col = hot ? RGB(180, 50, 50) : RGB(145, 35, 35);
            }
            else
            {
                col = hot ? RGB(60, 60, 68) : RGB(42, 42, 48);
            }
            FillRoundRect(hdc, r, S(6), col, col);

            if (it.hIcon)
            {
                int sz = S(32);
                DrawIconEx(hdc, r.left + (TILE_W - sz) / 2, r.top + S(12),
                    it.hIcon, sz, sz, 0, nullptr, DI_NORMAL);
            }

            RECT rt2 = { r.left + S(4), r.bottom - S(24), r.right - S(4), r.bottom - S(4) };
            DrawText2(hdc, it.name, rt2, RGB(255, 255, 255), g_fontSmall,
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
//  绘制滚动条
// ============================================================
static void PaintOneScrollBar(HDC hdc, const RECT& rcTrack, const RECT& rcThumb, bool dragging)
{
    FillRoundRect(hdc, rcTrack, SCROLLBAR_W, RGB(42, 42, 48), RGB(42, 42, 48));
    COLORREF tc = dragging ? RGB(140, 140, 155) : RGB(90, 90, 100);
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
    FillRoundRect(hdc, r, S(6), RGB(70, 70, 80), RGB(120, 120, 140));

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
            DrawText2(hdc, it.name, rt, RGB(255, 255, 255), g_fontSmall,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }
}

static void Paint(HDC hdc)
{
    RECT rc; GetClientRect(g_hWnd, &rc);

    HBRUSH bg = CreateSolidBrush(RGB(22, 22, 25));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    PaintTitleBar(hdc);
    PaintList(hdc);
    PaintTiles(hdc);
    PaintScrollBars(hdc);

    if (g_dragTile) PaintDragGhost(hdc);

    // ========================================================
    //  灰色边框（窗口外圈 1px；最大化时贴屏幕边，不绘制）
    // ========================================================
    if (!IsZoomed(g_hWnd))
    {
        HBRUSH br = CreateSolidBrush(RGB(48, 48, 54));
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

        FillRoundRect(hdc, rc, S(8), RGB(32, 32, 36), RGB(70, 70, 78));

        for (int i = 0; i < (int)g_menuItems.size(); i++)
        {
            RECT ri = { MENU_PAD, MENU_PAD + i * MENU_ITEM_H,
                        rc.right - MENU_PAD, MENU_PAD + (i + 1) * MENU_ITEM_H };
            if (i == g_menuHover)
                FillRoundRect(hdc, ri, S(4), RGB(60, 60, 68), RGB(60, 60, 68));

            RECT rt = { ri.left + S(12), ri.top, ri.right - S(8), ri.bottom };
            DrawText2(hdc, g_menuItems[i], rt, RGB(225, 225, 230), g_fontUI,
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

            if (g_hMenu == hwnd) g_hMenu = nullptr;
            ShowWindow(hwnd, SW_HIDE);
            if (IsWindow(hwnd)) DestroyWindow(hwnd);

            DoMenuAction(target, action);
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

// ============================================================
//  对话框公共辅助（合并 Rename / Missing 的重复逻辑）
// ============================================================

// 绘制对话框的标题栏、关闭按钮和分隔线
static void PaintDialogTitle(HDC hdc, HWND hwnd, const wchar_t* title, bool hoverClose)
{
    RECT rc; GetClientRect(hwnd, &rc);

    RECT rtt = { S(16), 0, rc.right - S(40), DLG_TITLE_H };
    DrawText2(hdc, title, rtt, RGB(232, 232, 232), g_fontTitle,
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
        HPEN pn = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
        HGDIOBJ op = SelectObject(hdc, pn);
        MoveToEx(hdc, cx - S(4), cy - S(4), nullptr); LineTo(hdc, cx + S(4), cy + S(4));
        MoveToEx(hdc, cx + S(4), cy - S(4), nullptr); LineTo(hdc, cx - S(4), cy + S(4));
        SelectObject(hdc, op);
        DeleteObject(pn);
    }

    {
        HPEN pl = CreatePen(PS_SOLID, 1, RGB(50, 50, 56));
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

    HBRUSH bg = CreateSolidBrush(RGB(34, 34, 38));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    PaintDialogTitle(hdc, hwnd, L"重命名", g_dlgHoverBtn == 2);

    {
        bool focused = (GetFocus() == g_hDlgEdit) || (GetFocus() == hwnd);
        COLORREF bcol = focused ? RGB(0, 174, 255) : RGB(70, 70, 78);
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
        COLORREF fill = hot ? RGB(70, 70, 80) : RGB(55, 55, 62);
        FillRoundRect(hdc, g_dlgBtnCancel, S(4), fill, fill);
        DrawText2(hdc, L"取消", g_dlgBtnCancel, RGB(225, 225, 230), g_fontUI,
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
        SetTextColor(hdc, RGB(230, 230, 235));
        SetBkColor(hdc, RGB(45, 45, 52));
        static HBRUSH hbr = CreateSolidBrush(RGB(45, 45, 52));
        return (LRESULT)hbr;
    }
    case WM_DESTROY:
        g_hDlg = nullptr;
        g_hDlgEdit = nullptr;
        g_dlgHoverBtn = -1;
        g_dlgPressedBtn = -1;
        g_dlgItem = -1;          // 清理残留
        if (g_hWnd)
        {
            EnableWindow(g_hWnd, TRUE);
            SetForegroundWindow(g_hWnd);
            // 强制重绘主窗口，修复分层窗口在弹窗期间导致的背景透明问题
            RedrawWindow(g_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE);
            InvalidateRect(g_hWnd, nullptr, FALSE);
        }
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

    HBRUSH bg = CreateSolidBrush(RGB(34, 34, 38));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    PaintDialogTitle(hdc, hwnd, L"提示", g_missingDlgHoverBtn == 2);

    // 提示文本
    {
        RECT rt = { DLG_MARGIN, DLG_TITLE_H + S(16),
                    rc.right - DLG_MARGIN,
                    rc.bottom - DLG_MARGIN - DLG_BTN_H - S(12) };
        DrawText2(hdc, g_missingDlgText, rt, RGB(220, 220, 225), g_fontUI,
            DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
    }

    // 否（左）
    {
        bool hot = (g_missingDlgHoverBtn == 1);
        COLORREF fill = hot ? RGB(70, 70, 80) : RGB(55, 55, 62);
        FillRoundRect(hdc, g_missingDlgBtnNo, S(4), fill, fill);
        DrawText2(hdc, L"否", g_missingDlgBtnNo, RGB(225, 225, 230), g_fontUI,
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
        g_missingDlgItems.clear();    // 清理残留
        if (g_hWnd)
        {
            EnableWindow(g_hWnd, TRUE);
            SetForegroundWindow(g_hWnd);
            // 强制重绘主窗口，修复分层窗口在弹窗期间导致的背景透明问题
            RedrawWindow(g_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE);
            InvalidateRect(g_hWnd, nullptr, FALSE);
        }
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
    // 因为有父窗口且去除了 TOPMOST，不需要强制 SetForegroundWindow 挡住其他软件，
    // 但需要 SetFocus 以便用户按回车或 Esc 响应
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
    bool hasGroup = false;

    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n' || line.back() == L' ' || line.back() == L'\t'))
            line.pop_back();
        while (!line.empty() && (line.front() == L' ' || line.front() == L'\t'))
            line.erase(0, 1);

        if (line.empty()) continue;

        if (line[0] == L'[') {
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

    int tg = -1, ti = -1;
    HitTile(pt, tg, ti);
    g_hoverTileG = tg;
    g_hoverTileI = ti;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        DragAcceptFiles(hwnd, TRUE);
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig);
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);
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

    case WM_NCACTIVATE:
        // 阻止系统默认的对非客户区的重绘，否则会导致 WS_EX_LAYERED 窗口失去焦点时变透明
        return TRUE;

    case WM_ENABLE:
        if (wp) {
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE);
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
                int viewH = rc.bottom - TITLE_H;
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
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
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
                    int viewH = rc.bottom - TITLE_H;
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
                if (b == 1) ShowWindow(hwnd, SW_MINIMIZE);
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
            int viewH = rc.bottom - TITLE_H;
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
        SetTextColor(hdc, RGB(230, 230, 235));
        SetBkColor(hdc, RGB(45, 45, 52));
        static HBRUSH hbr = CreateSolidBrush(RGB(45, 45, 52));
        return (LRESULT)hbr;
    }

    case WM_CLOSE:
        // 关闭前若仍处于分组名行内编辑状态，先提交
        if (g_hEdit) EndEditGroupName(true);
        SaveConfig();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        // 注销 OLE 拖放目标
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

    int w = MulDiv(DEFAULT_W, g_dpi, 96), h = MulDiv(DEFAULT_H, g_dpi, 96);
    int x = -10000, y = -10000;
    bool startMaximized = false;   // 上次关闭时窗口是否处于最大化状态
    std::wstring winContent = ReadAllText(g_cfgPath);
    if (!winContent.empty()) {
        std::wstringstream ss(winContent);
        std::wstring line;
        bool inWin = false;
        while (std::getline(ss, line)) {
            if (line == L"[Window]") { inWin = true; continue; }
            if (!line.empty() && line[0] == L'[' && line != L"[Window]") { inWin = false; }
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
        WS_OVERLAPPEDWINDOW,
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

    // 若上次关闭时处于最大化状态，则直接以最大化显示
    ShowWindow(g_hWnd, startMaximized ? SW_SHOWMAXIMIZED : SW_SHOW);
    UpdateWindow(g_hWnd);

    // ============================================================
    //  将缺失文件的检查推迟到消息循环中，确保主窗口的首次自绘已经完成。
    //  直接在这里阻塞弹窗会导致主窗口未完成初始化，进而导致样式错乱（背景透明等）。
    // ============================================================
    PostMessage(g_hWnd, WM_STARTUP_CHECK, 0, 0);

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