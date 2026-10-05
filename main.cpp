/* ============================================================
PHP 配置管家 V2.3 beta1
纯 C++ + Win32 + GDI+ + 微软雅黑
32 位编译，兼容 Win7/8/10/11
编译: g++ -finput-charset=UTF-8 -fexec-charset=GBK -o PHP-GUI.exe main.cpp
-L"D:\CPP\mingw64\mingw32\i686-w64-mingw32\lib"
-lcomctl32 -lshlwapi -lgdiplus -lgdi32 -luser32 -lkernel32 -lole32 -lshell32 -mwindows
============================================================ */

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gdiplus.h>

using namespace Gdiplus;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

/* ==================== 版本 ==================== */
#define APP_VERSION   L"2.3 beta1"
#define APP_TITLE     L"PHP 配置管家 V2.3 beta1 - WINRX论坛版"
#define APP_REPO      L"https://github.com/yanghaoxvan/PHP-GUI"
#define APP_EMAIL     L"yanghaoxvan2223@outlook.com"

/* ==================== 配色 ==================== */
#define C_BG_TOP      Color(255, 0x3A, 0x42, 0x70)
#define C_BG_BOTTOM   Color(255, 0x2E, 0x34, 0x54)
#define C_TEXT        RGB(0xE8, 0xEA, 0xF6)

/* ==================== 控件ID ==================== */
#define ID_TAB            1000
#define ID_SAVE_BTN       1001
#define ID_HELP_BTN       1002
#define ID_RESTART_BTN    1003
#define ID_PROGRESS       1004
#define ID_STATUS         1005
#define ID_SCAN_BTN       1006
#define ID_MANUAL_BTN     1007
#define ID_EXT_SEARCH     1010
#define ID_EXT_LIST       1011
#define ID_EXT_PATH_EDIT  1012
#define ID_SET_VER        1020
#define ID_SET_REPO       1021
#define ID_SET_EMAIL      1022

#define ID_MEMORY_LIMIT   1100
#define ID_UPLOAD_SIZE    1101
#define ID_POST_SIZE      1102
#define ID_TIMEZONE       1103
#define ID_MAX_EXEC       1104
#define ID_MAX_INPUT      1105
#define ID_DISPLAY_ERR    1106
#define ID_LOG_ERR        1107
#define ID_MAX_FILE       1110
#define ID_ALLOW_URL      1111
#define ID_SHORT_TAG      1112
#define ID_ZLIB_COMP      1113
#define ID_SESSION_AUTO   1114
#define ID_SESSION_NAME   1115
#define ID_OPCACHE_EN     1120
#define ID_OPCACHE_MEM    1121

#define ID_TAB_PANEL_BASE 2001
#define ID_TAB_PANEL_ADV  2002
#define ID_TAB_PANEL_PERF 2003
#define ID_TAB_PANEL_UNK  2004
#define ID_TAB_PANEL_EXT  2005
#define ID_TAB_PANEL_SET  2006

/* ==================== 全局 ==================== */
HINSTANCE hInst;
HWND hMain, hTab, hProgress, hStatus;
HWND hPanelBase, hPanelAdv, hPanelPerf, hPanelUnk, hPanelExt, hPanelSet;
WCHAR g_iniPath[MAX_PATH] = {0};
WCHAR g_extDir[MAX_PATH] = {0};
WCHAR g_appDir[MAX_PATH] = {0};
WCHAR g_dataPath[MAX_PATH] = {0};
BOOL g_isAdmin = FALSE;
HFONT g_hFont = NULL;
ULONG_PTR g_gdiToken = 0;
int g_startCount = 0;

typedef struct { WCHAR name[64]; int count; } WaitItem;
WaitItem g_wait[64];
int g_waitCount = 0;

WCHAR g_log[8192] = {0};
BOOL g_logErr = FALSE;
BOOL g_logWarn = FALSE;
WCHAR g_errMsg[512] = {0};
WCHAR g_warnMsg[512] = {0};

HWND hMemory, hUpload, hPost, hTimezone, hMaxExec, hMaxInput;
HWND hDisplayErr, hLogErr;
HWND hMaxFile, hAllowUrl, hShortTag, hZlibComp, hSessionAuto, hSessionName;
HWND hOpcacheEn, hOpcacheMem, hExtPath, hExtList, hExtSearch;

const WCHAR* g_timezones[] = {
	L"Asia/Shanghai", L"Asia/Tokyo", L"Asia/Seoul", L"Asia/Singapore",
	L"Asia/Hong_Kong", L"Asia/Taipei", L"UTC",
	L"America/New_York", L"America/Chicago", L"America/Los_Angeles",
	L"Europe/London", L"Europe/Paris", L"Europe/Berlin", L"Europe/Moscow",
	L"Australia/Sydney"
};
const int g_timezoneCount = 15;

/* ==================== 前向声明 ==================== */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LayoutControls(HWND hwnd);
void LoadPhpSettings(void);
void SavePhpIni(void);
void RestartWebService(HWND hwnd);
void FindPhpIniPath(void);
void LoadDataTxt(void);
void SaveDataTxt(void);
void AppendLog(const WCHAR* step, BOOL ok);
void ShowErrorDialog(HWND parent);
void ShowHelpWindow(HWND parent);
void LoadExtensions(void);
void CheckWaitDelete(void);

/* ==================== 工具 ==================== */
BOOL IsRunningAsAdmin(void) {
	BOOL isAdmin = FALSE;
	PSID adminGroup = NULL;
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &adminGroup)) {
		CheckTokenMembership(NULL, adminGroup, &isAdmin);
		FreeSid(adminGroup);
	}
	return isAdmin;
}

void GetAppDir(void) {
	GetModuleFileNameW(NULL, g_appDir, MAX_PATH);
	WCHAR* p = wcsrchr(g_appDir, L'\\');
	if (p) *p = 0;
	wsprintfW(g_dataPath, L"%s\\DATA.txt", g_appDir);
}

HFONT CreateAppFont(void) {
	HFONT h = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						  L"Microsoft YaHei UI");
	HDC hdc = GetDC(NULL);
	HGDIOBJ old = SelectObject(hdc, h);
	WCHAR face[LF_FACESIZE] = {0};
	GetTextFaceW(hdc, LF_FACESIZE, face);
	SelectObject(hdc, old);
	ReleaseDC(NULL, hdc);
	if (wcsstr(face, L"YaHei") == NULL) {
		DeleteObject(h);
		h = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						L"Microsoft YaHei");
	}
	return h;
}

BOOL CALLBACK SetFontProc(HWND hwnd, LPARAM lParam) {
	SendMessageW(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
	return TRUE;
}

void ApplyFontToAll(HWND hwnd) {
	EnumChildWindows(hwnd, SetFontProc, (LPARAM)g_hFont);
}

/* ==================== DATA.txt ==================== */
void LoadDataTxt(void) {
	FILE* fp = _wfopen(g_dataPath, L"r, ccs=UTF-8");
	if (!fp) { g_startCount = 0; g_waitCount = 0; return; }
	WCHAR line[1024];
	while (fgetws(line, 1024, fp)) {
		if (wcsncmp(line, L"openguino:", 10) == 0) g_startCount = _wtoi(line + 10);
		else if (wcsncmp(line, L"waitdelel[", 10) == 0) {
			WCHAR* p = line + 10;
			WCHAR* end = wcschr(p, L']');
			if (end) *end = 0;
			WCHAR* ctx = NULL;
			WCHAR* tok = wcstok(p, L",", &ctx);
			while (tok && g_waitCount < 64) {
				WCHAR* amp = wcschr(tok, L'&');
				if (amp) {
					*amp = 0;
					wcsncpy(g_wait[g_waitCount].name, tok, 63);
					g_wait[g_waitCount].count = _wtoi(amp + 1);
					g_waitCount++;
				}
				tok = wcstok(NULL, L",", &ctx);
			}
		}
	}
	fclose(fp);
}

void SaveDataTxt(void) {
	FILE* fp = _wfopen(g_dataPath, L"w, ccs=UTF-8");
	if (!fp) return;
	fwprintf(fp, L"openguino:%d\n", g_startCount);
	fwprintf(fp, L"waitdelel[");
	for (int i = 0; i < g_waitCount; i++) {
		if (i > 0) fwprintf(fp, L",");
		fwprintf(fp, L"%s&%d", g_wait[i].name, g_wait[i].count);
	}
	fwprintf(fp, L"]\n");
	fwprintf(fp, L"V:%s\n", APP_VERSION);
	fwprintf(fp, L"openlog\n");
	fwprintf(fp, L"%s\n", g_log);
	fwprintf(fp, L"ERROR: %s\n", g_logErr ? L"YES" : L"NO");
	fwprintf(fp, L"WARNING: %s\n", g_logWarn ? L"YES" : L"NO");
	fwprintf(fp, L"OK %d PHPGUI is open\n", g_logErr ? 0 : 1);
	fclose(fp);
}

void AppendLog(const WCHAR* step, BOOL ok) {
	WCHAR buf[256];
	wsprintfW(buf, L"%s:%s\n", step, ok ? L"成功" : L"失败");
	wcsncat(g_log, buf, 8192 - wcslen(g_log) - 1);
	if (!ok) {
		g_logWarn = TRUE;
		if (g_warnMsg[0]) wcsncat(g_warnMsg, L"; ", 511 - wcslen(g_warnMsg) - 1);
		wcsncat(g_warnMsg, step, 511 - wcslen(g_warnMsg) - 1);
	}
}

/* ==================== 搜索 php.ini ==================== */
BOOL GetPhpPathFromRegistry(WCHAR* outPath, DWORD size) {
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\PHP\\", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		WCHAR installDir[MAX_PATH];
		DWORD bufSize = sizeof(installDir);
		if (RegQueryValueExW(hKey, L"InstallDir", NULL, NULL, (LPBYTE)installDir, &bufSize) == ERROR_SUCCESS) {
			wsprintfW(outPath, L"%s\\php.ini", installDir);
			RegCloseKey(hKey);
			if (GetFileAttributesW(outPath) != INVALID_FILE_ATTRIBUTES) return TRUE;
		}
		RegCloseKey(hKey);
	}
	return FALSE;
}

void FindPhpIniRecursive(const WCHAR* dir, int depth) {
	if (depth > 3 || g_iniPath[0]) return;
	WCHAR search[MAX_PATH];
	wsprintfW(search, L"%s\\*", dir);
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(search, &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
		WCHAR full[MAX_PATH];
		wsprintfW(full, L"%s\\%s", dir, fd.cFileName);
		WCHAR phpExe[MAX_PATH], phpIni[MAX_PATH];
		wsprintfW(phpExe, L"%s\\php.exe", full);
		wsprintfW(phpIni, L"%s\\php.ini", full);
		if (GetFileAttributesW(phpExe) != INVALID_FILE_ATTRIBUTES) {
			if (GetFileAttributesW(phpIni) != INVALID_FILE_ATTRIBUTES) {
				wcscpy(g_iniPath, phpIni);
				FindClose(h); return;
			} else {
				WCHAR prod[MAX_PATH], dev[MAX_PATH];
				wsprintfW(prod, L"%s\\php.ini-production", full);
				wsprintfW(dev, L"%s\\php.ini-development", full);
				if (GetFileAttributesW(prod) != INVALID_FILE_ATTRIBUTES) {
					CopyFileW(prod, phpIni, FALSE);
					wcscpy(g_iniPath, phpIni);
					FindClose(h); return;
				} else if (GetFileAttributesW(dev) != INVALID_FILE_ATTRIBUTES) {
					CopyFileW(dev, phpIni, FALSE);
					wcscpy(g_iniPath, phpIni);
					FindClose(h); return;
				}
			}
		}
		FindPhpIniRecursive(full, depth + 1);
		if (g_iniPath[0]) { FindClose(h); return; }
	} while (FindNextFileW(h, &fd));
	FindClose(h);
}

void FindPhpIniPath(void) {
	if (GetPhpPathFromRegistry(g_iniPath, MAX_PATH)) { AppendLog(L"扫描 php.ini", TRUE); return; }
	WCHAR sysDrive[MAX_PATH];
	GetEnvironmentVariableW(L"SystemDrive", sysDrive, MAX_PATH);
	wsprintfW(sysDrive, L"%s\\", sysDrive);
	FindPhpIniRecursive(sysDrive, 0);
	if (g_iniPath[0]) { AppendLog(L"扫描 php.ini", TRUE); return; }
	const WCHAR* paths[] = { L"C:\\php\\php.ini", L"C:\\PHP\\php.ini", L"D:\\php\\php.ini" };
	for (int i = 0; i < 3; i++) {
		if (GetFileAttributesW(paths[i]) != INVALID_FILE_ATTRIBUTES) {
			wcscpy(g_iniPath, paths[i]);
			AppendLog(L"扫描 php.ini", TRUE);
			return;
		}
	}
	AppendLog(L"扫描 php.ini", FALSE);
}

/* ==================== 读写 ==================== */
char* ReadFileContent(const WCHAR* path, long* outSize) {
	FILE* fp = _wfopen(path, L"rb");
	if (!fp) return NULL;
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* content = (char*)malloc(size + 2);
	if (!content) { fclose(fp); return NULL; }
	fread(content, 1, size, fp);
	content[size] = 0;
	fclose(fp);
	if (outSize) *outSize = size;
	return content;
}

BOOL WriteFileContent(const WCHAR* path, const char* content) {
	FILE* fp = _wfopen(path, L"wb");
	if (!fp) return FALSE;
	fwrite(content, 1, strlen(content), fp);
	fclose(fp);
	return TRUE;
}

void ExtractValue(const char* line, char* out, int maxLen) {
	const char* eq = strchr(line, '=');
	if (!eq) { out[0] = 0; return; }
	const char* s = eq + 1;
	while (*s == ' ' || *s == '\t') s++;
	const char* e = s;
	while (*e && *e != '\r' && *e != '\n') e++;
	int len = (int)(e - s);
	if (len >= maxLen) len = maxLen - 1;
	strncpy(out, s, len);
	out[len] = 0;
}

void LoadPhpSettings(void) {
	if (g_iniPath[0] == 0) return;
	char* content = ReadFileContent(g_iniPath, NULL);
	if (!content) return;
	char line[2048], value[512];
	const char* ptr = content;
	while (*ptr) {
		const char* eol = strchr(ptr, '\n');
		int len = eol ? (int)(eol - ptr) : (int)strlen(ptr);
		if (len >= (int)sizeof(line)) len = sizeof(line) - 1;
		strncpy(line, ptr, len);
		line[len] = 0;
		ptr = eol ? eol + 1 : ptr + strlen(ptr);
		if (line[0] == 0 || line[0] == ';' || line[0] == '#') continue;
		
		if (strstr(line, "memory_limit")) { ExtractValue(line, value, 512); SetWindowTextA(hMemory, value); }
		else if (strstr(line, "upload_max_filesize")) { ExtractValue(line, value, 512); SetWindowTextA(hUpload, value); }
		else if (strstr(line, "post_max_size")) { ExtractValue(line, value, 512); SetWindowTextA(hPost, value); }
		else if (strstr(line, "max_execution_time")) { ExtractValue(line, value, 512); SetWindowTextA(hMaxExec, value); }
		else if (strstr(line, "max_input_time")) { ExtractValue(line, value, 512); SetWindowTextA(hMaxInput, value); }
		else if (strstr(line, "date.timezone")) {
			ExtractValue(line, value, 512);
			WCHAR wv[128];
			MultiByteToWideChar(CP_UTF8, 0, value, -1, wv, 128);
			for (int i = 0; i < g_timezoneCount; i++) {
				if (wcscmp(g_timezones[i], wv) == 0) { SendMessageW(hTimezone, CB_SETCURSEL, i, 0); break; }
			}
		}
		else if (strstr(line, "display_errors")) { ExtractValue(line, value, 512); SendMessageW(hDisplayErr, CB_SETCURSEL, (stricmp(value,"On")==0)?0:1, 0); }
		else if (strstr(line, "log_errors")) { ExtractValue(line, value, 512); SendMessageW(hLogErr, CB_SETCURSEL, (stricmp(value,"On")==0)?0:1, 0); }
		else if (strstr(line, "allow_url_fopen")) { ExtractValue(line, value, 512); SendMessageW(hAllowUrl, CB_SETCURSEL, (stricmp(value,"On")==0)?0:1, 0); }
		else if (strstr(line, "short_open_tag")) { ExtractValue(line, value, 512); SendMessageW(hShortTag, CB_SETCURSEL, (stricmp(value,"On")==0)?0:1, 0); }
		else if (strstr(line, "zlib.output_compression")) { ExtractValue(line, value, 512); SendMessageW(hZlibComp, CB_SETCURSEL, (stricmp(value,"On")==0)?0:1, 0); }
		else if (strstr(line, "session.auto_start")) { ExtractValue(line, value, 512); SendMessageW(hSessionAuto, CB_SETCURSEL, (stricmp(value,"On")==0)?0:1, 0); }
		else if (strstr(line, "session.name")) { ExtractValue(line, value, 512); SetWindowTextA(hSessionName, value); }
		else if (strstr(line, "opcache.enable")) { ExtractValue(line, value, 512); SendMessageW(hOpcacheEn, CB_SETCURSEL, (stricmp(value,"On")==0)?0:1, 0); }
		else if (strstr(line, "opcache.memory_consumption")) { ExtractValue(line, value, 512); SetWindowTextA(hOpcacheMem, value); }
		else if (strstr(line, "max_file_uploads")) { ExtractValue(line, value, 512); SetWindowTextA(hMaxFile, value); }
		else if (strstr(line, "extension_dir")) { ExtractValue(line, value, 512); SetWindowTextA(hExtPath, value); }
	}
	free(content);
}

void SavePhpIni(void) {
	if (g_iniPath[0] == 0) {
		int r = MessageBoxW(NULL, L"未找到 php.ini，是否手动选择？", L"提示", MB_YESNO|MB_ICONQUESTION);
		if (r != IDYES) return;
		OPENFILENAMEW ofn = {0};
		WCHAR fp[MAX_PATH] = {0};
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = L"php.ini\0*.ini\0所有文件\0*.*\0";
		ofn.lpstrFile = fp;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
		if (!GetOpenFileNameW(&ofn)) return;
		wcscpy(g_iniPath, fp);
	}
	
	char mem[64]="128M", up[64]="128M", post[64]="128M", tz[256]="Asia/Shanghai";
	char me[64]="30", mi[64]="60", mf[64]="20", sn[64]="PHPSESSID", om[64]="128";
	GetWindowTextA(hMemory, mem, 64);
	GetWindowTextA(hUpload, up, 64);
	GetWindowTextA(hPost, post, 64);
	GetWindowTextA(hMaxExec, me, 64);
	GetWindowTextA(hMaxInput, mi, 64);
	GetWindowTextA(hMaxFile, mf, 64);
	GetWindowTextA(hSessionName, sn, 64);
	GetWindowTextA(hOpcacheMem, om, 64);
	int tzSel = (int)SendMessageW(hTimezone, CB_GETCURSEL, 0, 0);
	if (tzSel >= 0 && tzSel < g_timezoneCount) {
		WideCharToMultiByte(CP_UTF8, 0, g_timezones[tzSel], -1, tz, 256, NULL, NULL);
	}
	int de = (int)SendMessageW(hDisplayErr, CB_GETCURSEL, 0, 0);
	int le = (int)SendMessageW(hLogErr, CB_GETCURSEL, 0, 0);
	int au = (int)SendMessageW(hAllowUrl, CB_GETCURSEL, 0, 0);
	int st = (int)SendMessageW(hShortTag, CB_GETCURSEL, 0, 0);
	int zc = (int)SendMessageW(hZlibComp, CB_GETCURSEL, 0, 0);
	int sa = (int)SendMessageW(hSessionAuto, CB_GETCURSEL, 0, 0);
	int oe = (int)SendMessageW(hOpcacheEn, CB_GETCURSEL, 0, 0);
	
	WCHAR bak[MAX_PATH];
	wsprintfW(bak, L"%s.bak", g_iniPath);
	if (GetFileAttributesW(bak) == INVALID_FILE_ATTRIBUTES) CopyFileW(g_iniPath, bak, FALSE);
	
	long fs = 0;
	char* content = ReadFileContent(g_iniPath, &fs);
	if (!content) { MessageBoxW(NULL, L"读取 php.ini 失败", L"错误", MB_OK|MB_ICONERROR); return; }
	
	char* nw = (char*)malloc(fs + 16384);
	if (!nw) { free(content); return; }
	nw[0] = 0;
	
	int rep[20] = {0};
	char* lines[2048];
	int lc = 0;
	char* sp;
	char* ln = strtok_s(content, "\n", &sp);
	while (ln && lc < 2048) { lines[lc++] = ln; ln = strtok_s(NULL, "\n", &sp); }
	
	for (int i = 0; i < lc; i++) {
		char* cur = lines[i];
		int matched = 0;
		if (!rep[0] && strstr(cur, "memory_limit") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "memory_limit = %s\n", mem); rep[0]=1; matched=1; }
		else if (!rep[1] && strstr(cur, "upload_max_filesize") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "upload_max_filesize = %s\n", up); rep[1]=1; matched=1; }
		else if (!rep[2] && strstr(cur, "post_max_size") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "post_max_size = %s\n", post); rep[2]=1; matched=1; }
		else if (!rep[3] && strstr(cur, "max_execution_time") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "max_execution_time = %s\n", me); rep[3]=1; matched=1; }
		else if (!rep[4] && strstr(cur, "max_input_time") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "max_input_time = %s\n", mi); rep[4]=1; matched=1; }
		else if (!rep[5] && strstr(cur, "date.timezone") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "date.timezone = %s\n", tz); rep[5]=1; matched=1; }
		else if (!rep[6] && strstr(cur, "display_errors") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "display_errors = %s\n", de==0?"On":"Off"); rep[6]=1; matched=1; }
		else if (!rep[7] && strstr(cur, "log_errors") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "log_errors = %s\n", le==0?"On":"Off"); rep[7]=1; matched=1; }
		else if (!rep[8] && strstr(cur, "allow_url_fopen") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "allow_url_fopen = %s\n", au==0?"On":"Off"); rep[8]=1; matched=1; }
		else if (!rep[9] && strstr(cur, "short_open_tag") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "short_open_tag = %s\n", st==0?"On":"Off"); rep[9]=1; matched=1; }
		else if (!rep[10] && strstr(cur, "zlib.output_compression") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "zlib.output_compression = %s\n", zc==0?"On":"Off"); rep[10]=1; matched=1; }
		else if (!rep[11] && strstr(cur, "session.auto_start") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "session.auto_start = %s\n", sa==0?"On":"Off"); rep[11]=1; matched=1; }
		else if (!rep[12] && strstr(cur, "session.name") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "session.name = %s\n", sn); rep[12]=1; matched=1; }
		else if (!rep[13] && strstr(cur, "max_file_uploads") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "max_file_uploads = %s\n", mf); rep[13]=1; matched=1; }
		else if (!rep[14] && strstr(cur, "opcache.enable") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "opcache.enable = %s\n", oe==0?"On":"Off"); rep[14]=1; matched=1; }
		else if (!rep[15] && strstr(cur, "opcache.memory_consumption") && strchr(cur, '=')) { sprintf(nw+strlen(nw), "opcache.memory_consumption = %s\n", om); rep[15]=1; matched=1; }
		if (!matched) { sprintf(nw+strlen(nw), "%s\n", cur); }
	}
	
	if (WriteFileContent(g_iniPath, nw)) {
		MessageBoxW(NULL, L"配置已保存！请点击「重启 Web 服务」生效", L"成功", MB_OK|MB_ICONINFORMATION);
		AppendLog(L"保存 php.ini", TRUE);
	} else {
		MessageBoxW(NULL, L"保存失败，请检查权限", L"错误", MB_OK|MB_ICONERROR);
		CopyFileW(bak, g_iniPath, FALSE);
		AppendLog(L"保存 php.ini", FALSE);
	}
	free(content);
	free(nw);
}

void RestartWebService(HWND hwnd) {
	if (!g_isAdmin) {
		int r = MessageBoxW(NULL, L"未以管理员身份运行，可能失败。继续？", L"权限提示", MB_YESNO|MB_ICONWARNING);
		if (r != IDYES) return;
	}
	EnableWindow(GetDlgItem(hwnd, ID_RESTART_BTN), FALSE);
	SetWindowTextW(GetDlgItem(hwnd, ID_RESTART_BTN), L"正在重启...");
	SendMessageW(hProgress, PBM_SETPOS, 30, 0);
	SetWindowTextW(hStatus, L"正在重启 Web 服务...");
	
	int result = 0;
	if (system("sc query W3SVC >nul 2>&1") == 0) {
		result = system("iisreset /noforce >nul 2>&1");
	} else if (system("tasklist /fi \"imagename eq httpd.exe\" 2>nul | find /i \"httpd.exe\" >nul") == 0) {
		result = system("httpd.exe -k restart >nul 2>&1");
	} else if (system("tasklist /fi \"imagename eq nginx.exe\" 2>nul | find /i \"nginx.exe\" >nul") == 0) {
		result = system("nginx -s reload >nul 2>&1");
	} else {
		result = system("taskkill /f /im php-cgi.exe >nul 2>&1 & taskkill /f /im php.exe >nul 2>&1");
	}
	
	if (result == 0) {
		SendMessageW(hProgress, PBM_SETPOS, 100, 0);
		SetWindowTextW(hStatus, L"重启成功！");
		MessageBoxW(NULL, L"Web 服务已重启，PHP 配置已生效", L"成功", MB_OK|MB_ICONINFORMATION);
		AppendLog(L"重启 Web 服务", TRUE);
	} else {
		SetWindowTextW(hStatus, L"重启失败");
		MessageBoxW(NULL, L"重启失败，请手动重启", L"错误", MB_OK|MB_ICONERROR);
		AppendLog(L"重启 Web 服务", FALSE);
	}
	EnableWindow(GetDlgItem(hwnd, ID_RESTART_BTN), TRUE);
	SetWindowTextW(GetDlgItem(hwnd, ID_RESTART_BTN), L"重启 Web 服务");
	SetTimer(hwnd, 1, 2000, NULL);
}

void LoadExtensions(void) {
	SendMessageW(hExtList, LB_RESETCONTENT, 0, 0);
	if (g_extDir[0] == 0) {
		if (g_iniPath[0]) {
			wcscpy(g_extDir, g_iniPath);
			WCHAR* p = wcsrchr(g_extDir, L'\\');
			if (p) { *p = 0; wcscat(g_extDir, L"\\ext"); }
		}
	}
	if (g_extDir[0] == 0) return;
	WCHAR search[MAX_PATH];
	wsprintfW(search, L"%s\\php_*.dll", g_extDir);
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(search, &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		WCHAR name[128];
		wcsncpy(name, fd.cFileName + 4, 127);
		WCHAR* dot = wcsrchr(name, L'.');
		if (dot) *dot = 0;
		SendMessageW(hExtList, LB_ADDSTRING, 0, (LPARAM)name);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
}

void CheckWaitDelete(void) {
	for (int i = g_waitCount - 1; i >= 0; i--) {
		g_wait[i].count++;
		if (g_wait[i].count >= 3) {
			for (int j = i; j < g_waitCount - 1; j++) g_wait[j] = g_wait[j+1];
			g_waitCount--;
		}
	}
}

void ShowHelpWindow(HWND parent) {
	MessageBoxW(parent,
				L"PHP 配置管家 V2.3 beta1\n\n"
				L"1. 自动查找 php.ini\n"
				L"2. 六页配置：基础 / 高级 / 性能 / 未知 / 扩展 / 设置\n"
				L"3. 支持 IIS / Apache / Nginx\n"
				L"4. 出错时导出 DATA.txt 发给开发者\n\n"
				L"官方下载: https://windows.php.net/download/\n"
				L"IIS + FastCGI 请下载 Non-Thread-Safe (nts) 版本",
				L"帮助", MB_OK|MB_ICONINFORMATION);
}

void ShowErrorDialog(HWND parent) {
	int r = MessageBoxW(parent,
						L"OOPS！\n\n"
						L"程序出现了一点错误，众所周知，重启是能解决 80% 的问题的，\n"
						L"所以你看到就请重启，如果失败，请把 DATA.txt 发给开发者。\n\n"
						L"（不要把这个平白无故的窗口发给开发者啊！）\n\n"
						L"是否导出 DATA.txt？",
						L"OOPS！", MB_YESNO|MB_ICONERROR);
	if (r == IDYES) {
		OPENFILENAMEW ofn = {0};
		WCHAR fp[MAX_PATH] = L"DATA.txt";
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = L"文本文件\0*.txt\0所有文件\0*.*\0";
		ofn.lpstrFile = fp;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
		if (GetSaveFileNameW(&ofn)) CopyFileW(g_dataPath, fp, FALSE);
	}
}

void ShowTab(int idx) {
	ShowWindow(hPanelBase, idx==0?SW_SHOW:SW_HIDE);
	ShowWindow(hPanelAdv,  idx==1?SW_SHOW:SW_HIDE);
	ShowWindow(hPanelPerf, idx==2?SW_SHOW:SW_HIDE);
	ShowWindow(hPanelUnk,  idx==3?SW_SHOW:SW_HIDE);
	ShowWindow(hPanelExt,  idx==4?SW_SHOW:SW_HIDE);
	ShowWindow(hPanelSet,  idx==5?SW_SHOW:SW_HIDE);
	TabCtrl_SetCurSel(hTab, idx);
}

/* ==================== 控件创建 ==================== */
void CreateControls(HWND hwnd) {
	hTab = CreateWindowW(WC_TABCONTROLW, NULL, WS_CHILD|WS_VISIBLE,
						 10, 10, 700, 420, hwnd, (HMENU)ID_TAB, hInst, NULL);
	TCITEMW tie = { TCIF_TEXT };
	WCHAR t1[]=L"基础设置", t2[]=L"高级设置", t3[]=L"性能优化";
	WCHAR t4[]=L"未知选项", t5[]=L"扩展", t6[]=L"设置";
	tie.pszText=t1; TabCtrl_InsertItem(hTab,0,&tie);
	tie.pszText=t2; TabCtrl_InsertItem(hTab,1,&tie);
	tie.pszText=t3; TabCtrl_InsertItem(hTab,2,&tie);
	tie.pszText=t4; TabCtrl_InsertItem(hTab,3,&tie);
	tie.pszText=t5; TabCtrl_InsertItem(hTab,4,&tie);
	tie.pszText=t6; TabCtrl_InsertItem(hTab,5,&tie);
	
	/* ===== 基础设置 ===== */
	hPanelBase = CreateWindowW(L"STATIC", NULL, WS_CHILD|WS_VISIBLE, 10,45,700,370, hwnd, (HMENU)ID_TAB_PANEL_BASE, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"memory_limit", WS_CHILD|WS_VISIBLE, 20,30,140,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"内存限制", WS_CHILD|WS_VISIBLE, 20,50,140,20, hPanelBase, NULL, hInst, NULL);
	hMemory = CreateWindowW(L"EDIT", L"128M", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 170,30,120,24, hPanelBase, (HMENU)ID_MEMORY_LIMIT, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"upload_max_filesize", WS_CHILD|WS_VISIBLE, 20,90,160,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"上传文件大小限制", WS_CHILD|WS_VISIBLE, 20,110,160,20, hPanelBase, NULL, hInst, NULL);
	hUpload = CreateWindowW(L"EDIT", L"128M", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 190,90,120,24, hPanelBase, (HMENU)ID_UPLOAD_SIZE, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"post_max_size", WS_CHILD|WS_VISIBLE, 20,150,140,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"POST 数据大小限制", WS_CHILD|WS_VISIBLE, 20,170,160,20, hPanelBase, NULL, hInst, NULL);
	hPost = CreateWindowW(L"EDIT", L"128M", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 170,150,120,24, hPanelBase, (HMENU)ID_POST_SIZE, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"max_execution_time", WS_CHILD|WS_VISIBLE, 20,210,160,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"脚本最大执行时间", WS_CHILD|WS_VISIBLE, 20,230,160,20, hPanelBase, NULL, hInst, NULL);
	hMaxExec = CreateWindowW(L"EDIT", L"30", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 190,210,80,24, hPanelBase, (HMENU)ID_MAX_EXEC, hInst, NULL);
	CreateWindowW(L"STATIC", L"秒", WS_CHILD|WS_VISIBLE, 280,212,30,20, hPanelBase, NULL, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"max_input_time", WS_CHILD|WS_VISIBLE, 20,270,140,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"脚本解析输入时间", WS_CHILD|WS_VISIBLE, 20,290,160,20, hPanelBase, NULL, hInst, NULL);
	hMaxInput = CreateWindowW(L"EDIT", L"60", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 170,270,80,24, hPanelBase, (HMENU)ID_MAX_INPUT, hInst, NULL);
	CreateWindowW(L"STATIC", L"秒", WS_CHILD|WS_VISIBLE, 260,272,30,20, hPanelBase, NULL, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"date.timezone", WS_CHILD|WS_VISIBLE, 20,330,140,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"时区设置", WS_CHILD|WS_VISIBLE, 20,350,140,20, hPanelBase, NULL, hInst, NULL);
	hTimezone = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 170,330,180,200, hPanelBase, (HMENU)ID_TIMEZONE, hInst, NULL);
	for (int i = 0; i < g_timezoneCount; i++) SendMessageW(hTimezone, CB_ADDSTRING, 0, (LPARAM)g_timezones[i]);
	SendMessageW(hTimezone, CB_SETCURSEL, 0, 0);
	
	CreateWindowW(L"STATIC", L"display_errors", WS_CHILD|WS_VISIBLE, 400,30,140,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"是否显示错误", WS_CHILD|WS_VISIBLE, 400,50,140,20, hPanelBase, NULL, hInst, NULL);
	hDisplayErr = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 550,30,100,100, hPanelBase, (HMENU)ID_DISPLAY_ERR, hInst, NULL);
	SendMessageW(hDisplayErr, CB_ADDSTRING, 0, (LPARAM)L"开启");
	SendMessageW(hDisplayErr, CB_ADDSTRING, 0, (LPARAM)L"关闭");
	SendMessageW(hDisplayErr, CB_SETCURSEL, 0, 0);
	
	CreateWindowW(L"STATIC", L"log_errors", WS_CHILD|WS_VISIBLE, 400,90,140,20, hPanelBase, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"是否记录错误日志", WS_CHILD|WS_VISIBLE, 400,110,160,20, hPanelBase, NULL, hInst, NULL);
	hLogErr = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 550,90,100,100, hPanelBase, (HMENU)ID_LOG_ERR, hInst, NULL);
	SendMessageW(hLogErr, CB_ADDSTRING, 0, (LPARAM)L"开启");
	SendMessageW(hLogErr, CB_ADDSTRING, 0, (LPARAM)L"关闭");
	SendMessageW(hLogErr, CB_SETCURSEL, 0, 0);
	
	/* ===== 高级设置 ===== */
	hPanelAdv = CreateWindowW(L"STATIC", NULL, WS_CHILD, 10,45,700,370, hwnd, (HMENU)ID_TAB_PANEL_ADV, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"max_file_uploads", WS_CHILD|WS_VISIBLE, 20,30,160,20, hPanelAdv, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"单次最多上传文件数", WS_CHILD|WS_VISIBLE, 20,50,160,20, hPanelAdv, NULL, hInst, NULL);
	hMaxFile = CreateWindowW(L"EDIT", L"20", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 190,30,80,24, hPanelAdv, (HMENU)ID_MAX_FILE, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"allow_url_fopen", WS_CHILD|WS_VISIBLE, 20,90,140,20, hPanelAdv, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"允许打开远程文件", WS_CHILD|WS_VISIBLE, 20,110,160,20, hPanelAdv, NULL, hInst, NULL);
	hAllowUrl = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 190,90,100,100, hPanelAdv, (HMENU)ID_ALLOW_URL, hInst, NULL);
	SendMessageW(hAllowUrl, CB_ADDSTRING, 0, (LPARAM)L"开启");
	SendMessageW(hAllowUrl, CB_ADDSTRING, 0, (LPARAM)L"关闭");
	SendMessageW(hAllowUrl, CB_SETCURSEL, 0, 0);
	
	CreateWindowW(L"STATIC", L"short_open_tag", WS_CHILD|WS_VISIBLE, 20,150,140,20, hPanelAdv, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"短标签支持", WS_CHILD|WS_VISIBLE, 20,170,140,20, hPanelAdv, NULL, hInst, NULL);
	hShortTag = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 170,150,100,100, hPanelAdv, (HMENU)ID_SHORT_TAG, hInst, NULL);
	SendMessageW(hShortTag, CB_ADDSTRING, 0, (LPARAM)L"开启");
	SendMessageW(hShortTag, CB_ADDSTRING, 0, (LPARAM)L"关闭");
	SendMessageW(hShortTag, CB_SETCURSEL, 1, 0);
	
	CreateWindowW(L"STATIC", L"zlib.output_compression", WS_CHILD|WS_VISIBLE, 20,210,180,20, hPanelAdv, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"启用 Gzip 压缩输出", WS_CHILD|WS_VISIBLE, 20,230,180,20, hPanelAdv, NULL, hInst, NULL);
	hZlibComp = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 210,210,100,100, hPanelAdv, (HMENU)ID_ZLIB_COMP, hInst, NULL);
	SendMessageW(hZlibComp, CB_ADDSTRING, 0, (LPARAM)L"开启");
	SendMessageW(hZlibComp, CB_ADDSTRING, 0, (LPARAM)L"关闭");
	SendMessageW(hZlibComp, CB_SETCURSEL, 1, 0);
	
	CreateWindowW(L"STATIC", L"session.auto_start", WS_CHILD|WS_VISIBLE, 400,30,140,20, hPanelAdv, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"自动启动会话", WS_CHILD|WS_VISIBLE, 400,50,140,20, hPanelAdv, NULL, hInst, NULL);
	hSessionAuto = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 550,30,100,100, hPanelAdv, (HMENU)ID_SESSION_AUTO, hInst, NULL);
	SendMessageW(hSessionAuto, CB_ADDSTRING, 0, (LPARAM)L"开启");
	SendMessageW(hSessionAuto, CB_ADDSTRING, 0, (LPARAM)L"关闭");
	SendMessageW(hSessionAuto, CB_SETCURSEL, 1, 0);
	
	CreateWindowW(L"STATIC", L"session.name", WS_CHILD|WS_VISIBLE, 400,90,140,20, hPanelAdv, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"会话名称", WS_CHILD|WS_VISIBLE, 400,110,140,20, hPanelAdv, NULL, hInst, NULL);
	hSessionName = CreateWindowW(L"EDIT", L"PHPSESSID", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 550,90,150,24, hPanelAdv, (HMENU)ID_SESSION_NAME, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"扩展存放路径", WS_CHILD|WS_VISIBLE, 400,150,140,20, hPanelAdv, NULL, hInst, NULL);
	hExtPath = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 400,170,300,24, hPanelAdv, (HMENU)ID_EXT_PATH_EDIT, hInst, NULL);
	
	/* ===== 性能优化 ===== */
	hPanelPerf = CreateWindowW(L"STATIC", NULL, WS_CHILD, 10,45,700,370, hwnd, (HMENU)ID_TAB_PANEL_PERF, hInst, NULL);
	
	CreateWindowW(L"STATIC", L"opcache.enable", WS_CHILD|WS_VISIBLE, 20,30,140,20, hPanelPerf, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"开启 OPcache 加速", WS_CHILD|WS_VISIBLE, 20,50,160,20, hPanelPerf, NULL, hInst, NULL);
	hOpcacheEn = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 190,30,100,100, hPanelPerf, (HMENU)ID_OPCACHE_EN, hInst, NULL);
	SendMessageW(hOpcacheEn, CB_ADDSTRING, 0, (LPARAM)L"开启");
	SendMessageW(hOpcacheEn, CB_ADDSTRING, 0, (LPARAM)L"关闭");
	SendMessageW(hOpcacheEn, CB_SETCURSEL, 0, 0);
	
	CreateWindowW(L"STATIC", L"opcache.memory_consumption", WS_CHILD|WS_VISIBLE, 20,90,200,20, hPanelPerf, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"OPcache 内存大小", WS_CHILD|WS_VISIBLE, 20,110,160,20, hPanelPerf, NULL, hInst, NULL);
	hOpcacheMem = CreateWindowW(L"EDIT", L"128", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 230,90,80,24, hPanelPerf, (HMENU)ID_OPCACHE_MEM, hInst, NULL);
	CreateWindowW(L"STATIC", L"MB", WS_CHILD|WS_VISIBLE, 320,92,40,20, hPanelPerf, NULL, hInst, NULL);
	
	/* ===== 未知选项 ===== */
	hPanelUnk = CreateWindowW(L"STATIC", NULL, WS_CHILD, 10,45,700,370, hwnd, (HMENU)ID_TAB_PANEL_UNK, hInst, NULL);
	CreateWindowW(L"STATIC", L"（本页显示 php.ini 中已生效但未预设的配置项）", WS_CHILD|WS_VISIBLE, 20,20,600,20, hPanelUnk, NULL, hInst, NULL);
	
	/* ===== 扩展 ===== */
	hPanelExt = CreateWindowW(L"STATIC", NULL, WS_CHILD, 10,45,700,370, hwnd, (HMENU)ID_TAB_PANEL_EXT, hInst, NULL);
	CreateWindowW(L"STATIC", L"扩展搜索:", WS_CHILD|WS_VISIBLE, 20,20,80,20, hPanelExt, NULL, hInst, NULL);
	hExtSearch = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 100,18,200,24, hPanelExt, (HMENU)ID_EXT_SEARCH, hInst, NULL);
	hExtList = CreateWindowW(L"LISTBOX", NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|LBS_NOTIFY, 20,55,300,250, hPanelExt, (HMENU)ID_EXT_LIST, hInst, NULL);
	CreateWindowW(L"BUTTON", L"扫描", WS_CHILD|WS_VISIBLE, 340,55,80,30, hPanelExt, (HMENU)ID_SCAN_BTN, hInst, NULL);
	CreateWindowW(L"BUTTON", L"手动选择", WS_CHILD|WS_VISIBLE, 340,95,80,30, hPanelExt, (HMENU)ID_MANUAL_BTN, hInst, NULL);
	
	/* ===== 设置 ===== */
	hPanelSet = CreateWindowW(L"STATIC", NULL, WS_CHILD, 10,45,700,370, hwnd, (HMENU)ID_TAB_PANEL_SET, hInst, NULL);
	CreateWindowW(L"STATIC", L"当前版本:", WS_CHILD|WS_VISIBLE, 30,40,100,20, hPanelSet, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", APP_VERSION, WS_CHILD|WS_VISIBLE, 140,40,200,20, hPanelSet, (HMENU)ID_SET_VER, hInst, NULL);
	CreateWindowW(L"STATIC", L"仓库主页:", WS_CHILD|WS_VISIBLE, 30,80,100,20, hPanelSet, NULL, hInst, NULL);
	CreateWindowW(L"BUTTON", L"前往 GitHub", WS_CHILD|WS_VISIBLE, 140,78,150,28, hPanelSet, (HMENU)ID_SET_REPO, hInst, NULL);
	CreateWindowW(L"STATIC", L"作者邮箱:", WS_CHILD|WS_VISIBLE, 30,120,100,20, hPanelSet, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", APP_EMAIL, WS_CHILD|WS_VISIBLE, 140,120,300,20, hPanelSet, NULL, hInst, NULL);
	
	/* ===== 底部 ===== */
	CreateWindowW(L"BUTTON", L"保存配置", WS_CHILD|WS_VISIBLE, 20,435,100,35, hwnd, (HMENU)ID_SAVE_BTN, hInst, NULL);
	CreateWindowW(L"BUTTON", L"帮助", WS_CHILD|WS_VISIBLE, 140,435,80,35, hwnd, (HMENU)ID_HELP_BTN, hInst, NULL);
	CreateWindowW(L"BUTTON", L"重启 Web 服务", WS_CHILD|WS_VISIBLE, 530,435,150,35, hwnd, (HMENU)ID_RESTART_BTN, hInst, NULL);
	
	hProgress = CreateWindowW(PROGRESS_CLASSW, NULL, WS_CHILD|WS_VISIBLE, 20,485,660,20, hwnd, (HMENU)ID_PROGRESS, hInst, NULL);
	SendMessageW(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0,100));
	hStatus = CreateWindowW(L"STATIC", L"状态: 就绪", WS_CHILD|WS_VISIBLE, 20,520,500,25, hwnd, (HMENU)ID_STATUS, hInst, NULL);
}

/* ==================== 布局 ==================== */
void LayoutControls(HWND hwnd) {
	RECT rc;
	GetClientRect(hwnd, &rc);
	int w = rc.right, h = rc.bottom;
	int pad = 10;
	
	MoveWindow(hTab, pad, pad, w - pad*2, h - 120, TRUE);
	MoveWindow(hPanelBase, pad+5, pad+30, 700, 370, TRUE);
	MoveWindow(hPanelAdv, pad+5, pad+30, 700, 370, TRUE);
	MoveWindow(hPanelPerf, pad+5, pad+30, 700, 370, TRUE);
	MoveWindow(hPanelUnk, pad+5, pad+30, 700, 370, TRUE);
	MoveWindow(hPanelExt, pad+5, pad+30, 700, 370, TRUE);
	MoveWindow(hPanelSet, pad+5, pad+30, 700, 370, TRUE);
	
	MoveWindow(GetDlgItem(hwnd, ID_SAVE_BTN), pad+10, h-100, 100, 35, TRUE);
	MoveWindow(GetDlgItem(hwnd, ID_HELP_BTN), pad+120, h-100, 80, 35, TRUE);
	MoveWindow(GetDlgItem(hwnd, ID_RESTART_BTN), w-pad-170, h-100, 160, 35, TRUE);
	MoveWindow(hProgress, pad+10, h-55, w-pad*2-20, 20, TRUE);
	MoveWindow(hStatus, pad+10, h-28, w-pad*2-20, 25, TRUE);
}

/* ==================== 窗口过程 ==================== */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE: {
		GetAppDir();
		LoadDataTxt();
		g_startCount++;
		g_isAdmin = IsRunningAsAdmin();
		g_log[0] = 0;
		g_logErr = FALSE; g_logWarn = FALSE;
		g_errMsg[0] = 0; g_warnMsg[0] = 0;
		AppendLog(L"创建消息循环", TRUE);
		
		InitCommonControls();
		g_hFont = CreateAppFont();
		
		FindPhpIniPath();
		CreateControls(hwnd);
		ApplyFontToAll(hwnd);
		if (hTab) SendMessageW(hTab, WM_SETFONT, (WPARAM)g_hFont, TRUE);
		
		CheckWaitDelete();
		LoadPhpSettings();
		LayoutControls(hwnd);
		
		SetWindowTextW(hwnd, APP_TITLE);
		SetTimer(hwnd, 100, 100, NULL);
		break;
	}
	case WM_SIZE:
		LayoutControls(hwnd);
		break;
		case WM_NOTIFY: {
			NMHDR* nh = (NMHDR*)lParam;
			if (nh->idFrom == ID_TAB && nh->code == TCN_SELCHANGE) {
				ShowTab(TabCtrl_GetCurSel(hTab));
			}
			break;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_SAVE_BTN) SavePhpIni();
		else if (LOWORD(wParam) == ID_RESTART_BTN) RestartWebService(hwnd);
		else if (LOWORD(wParam) == ID_HELP_BTN) ShowHelpWindow(hwnd);
		else if (LOWORD(wParam) == ID_SCAN_BTN) LoadExtensions();
		else if (LOWORD(wParam) == ID_MANUAL_BTN) {
			OPENFILENAMEW ofn = {0};
			WCHAR fp[MAX_PATH] = {0};
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFilter = L"php.ini\0*.ini\0所有文件\0*.*\0";
			ofn.lpstrFile = fp;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
			if (GetOpenFileNameW(&ofn)) { wcscpy(g_iniPath, fp); LoadPhpSettings(); }
		}
		else if (LOWORD(wParam) == ID_SET_REPO) ShellExecuteW(NULL, L"open", APP_REPO, NULL, NULL, SW_SHOWNORMAL);
		break;
		
		case WM_CTLCOLORSTATIC: {
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, C_TEXT);
			SetBkMode(hdc, TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
		case WM_CTLCOLORBTN: {
			HDC hdc = (HDC)wParam;
			SetBkMode(hdc, TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
		case WM_CTLCOLOREDIT: {
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, RGB(0,0,0));
			SetBkColor(hdc, RGB(255,255,255));
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		}
		case WM_CTLCOLORLISTBOX: {
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, RGB(0,0,0));
			SetBkColor(hdc, RGB(255,255,255));
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		}
		
	case WM_TIMER:
		if (wParam == 100) {
			KillTimer(hwnd, 100);
			if (g_logErr) { ShowErrorDialog(hwnd); DestroyWindow(hwnd); }
		} else if (wParam == 1) {
			KillTimer(hwnd, 1);
			SetWindowTextW(hStatus, L"状态: 就绪");
			SendMessageW(hProgress, PBM_SETPOS, 0, 0);
		}
		break;
		
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			RECT rc;
			GetClientRect(hwnd, &rc);
			HDC memDC = CreateCompatibleDC(hdc);
			HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
			HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
			Graphics g(memDC);
			g.SetSmoothingMode(SmoothingModeAntiAlias);
			LinearGradientBrush brush(
									  Point(0, 0), Point(0, rc.bottom), C_BG_TOP, C_BG_BOTTOM);
			g.FillRectangle(&brush, 0, 0, rc.right, rc.bottom);
			BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
			SelectObject(memDC, oldBmp);
			DeleteObject(memBmp);
			DeleteDC(memDC);
			EndPaint(hwnd, &ps);
			break;
		}
	case WM_ERASEBKGND:
		return 1;
	case WM_DESTROY:
		SaveDataTxt();
		if (g_hFont) DeleteObject(g_hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}
	return 0;
}

/* ==================== 入口 ==================== */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
	hInst = hInstance;
	
	GdiplusStartupInput gsi;
	GdiplusStartup(&g_gdiToken, &gsi, NULL);
	
	WNDCLASSW wc = {0};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"PHPConfigTool";
	wc.hbrBackground = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClassW(&wc);
	
	HWND hwnd = CreateWindowExW(0, L"PHPConfigTool", APP_TITLE,
								WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 780, 620,
								NULL, NULL, hInstance, NULL);
	if (!hwnd) { GdiplusShutdown(g_gdiToken); return 0; }
	
	ShowWindow(hwnd, nShow);
	UpdateWindow(hwnd);
	
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	GdiplusShutdown(g_gdiToken);
	return (int)msg.wParam;
}
