#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>
#include <string.h>
#include <winreg.h>

#ifndef PROGRESS_CLASSW
#define PROGRESS_CLASSW L"msctls_progress32"
#endif

// 控件ID
#define ID_TAB           1000
#define ID_RESTART_BTN   1001
#define ID_PROGRESS      1002
#define ID_SAVE_BTN      1003
#define ID_STATUS        1004
#define ID_HELP_BTN      1005

// 基础设置
#define ID_MEMORY_LIMIT  1010
#define ID_UPLOAD_SIZE   1011
#define ID_POST_SIZE     1012
#define ID_TIMEZONE      1013
#define ID_MAX_EXEC      1014
#define ID_MAX_INPUT     1015
#define ID_DISPLAY_ERRORS 1016
#define ID_ERROR_REPORTING 1017
#define ID_LOG_ERRORS    1018

// 高级设置
#define ID_MAX_FILE      1020
#define ID_ALLOW_URL_FOPEN 1021
#define ID_SHORT_OPEN_TAG 1022
#define ID_ASP_TAGS      1023
#define ID_ZLIB_COMPRESS 1024
#define ID_SESSION_AUTO  1025
#define ID_SESSION_NAME  1026

// 性能优化
#define ID_OPCACHE_ENABLE 1030
#define ID_OPCACHE_MEM   1031

#define ID_TAB1_PANEL    2000
#define ID_TAB2_PANEL    2001
#define ID_TAB3_PANEL    2002

HINSTANCE hInst;
HWND hTab, hProgress, hStatus;
HWND hPanel1, hPanel2, hPanel3;
WCHAR g_iniPath[MAX_PATH] = {0};

// 基础设置控件
HWND hMemoryEdit, hUploadEdit, hPostEdit, hTimezoneEdit, hMaxExecEdit, hMaxInputEdit;
HWND hDisplayErrors, hErrorReporting, hLogErrors;

// 高级设置控件
HWND hMaxFileEdit, hAllowUrlFopen, hShortOpenTag, hAspTags, hZlibCompress;
HWND hSessionAuto, hSessionNameEdit;

// 性能优化控件
HWND hOpcacheEnable, hOpcacheMemEdit;

// 查找 php.ini
void FindPhpIniPath(void) {
	WCHAR testPath[MAX_PATH];
	
	wsprintfW(testPath, L"C:\\php-8.2.31-nts-Win32-vs16-x64\\php.ini");
	if (GetFileAttributesW(testPath) != INVALID_FILE_ATTRIBUTES) {
		wcscpy(g_iniPath, testPath);
		return;
	}
	
	wsprintfW(testPath, L"C:\\php\\php.ini");
	if (GetFileAttributesW(testPath) != INVALID_FILE_ATTRIBUTES) {
		wcscpy(g_iniPath, testPath);
		return;
	}
	
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\PHP\\", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		WCHAR phpDir[MAX_PATH];
		DWORD size = sizeof(phpDir);
		if (RegQueryValueExW(hKey, L"InstallDir", NULL, NULL, (LPBYTE)phpDir, &size) == ERROR_SUCCESS) {
			wsprintfW(g_iniPath, L"%s\\php.ini", phpDir);
			if (GetFileAttributesW(g_iniPath) != INVALID_FILE_ATTRIBUTES) {
				RegCloseKey(hKey);
				return;
			}
		}
		RegCloseKey(hKey);
	}
	
	g_iniPath[0] = 0;
}

// 读取配置
void LoadPhpSettings(void) {
	if (g_iniPath[0] == 0) return;
	
	FILE* fp = _wfopen(g_iniPath, L"r");
	if (!fp) return;
	
	char line[1024];
	char value[256];
	
	while (fgets(line, sizeof(line), fp)) {
		if (line[0] == ';' || line[0] == '#') continue;
		
		if (strstr(line, "memory_limit")) {
			sscanf(line, "memory_limit = %255s", value);
			SetWindowTextA(hMemoryEdit, value);
		}
		else if (strstr(line, "upload_max_filesize")) {
			sscanf(line, "upload_max_filesize = %255s", value);
			SetWindowTextA(hUploadEdit, value);
		}
		else if (strstr(line, "post_max_size")) {
			sscanf(line, "post_max_size = %255s", value);
			SetWindowTextA(hPostEdit, value);
		}
		else if (strstr(line, "max_execution_time")) {
			sscanf(line, "max_execution_time = %255s", value);
			SetWindowTextA(hMaxExecEdit, value);
		}
		else if (strstr(line, "max_input_time")) {
			sscanf(line, "max_input_time = %255s", value);
			SetWindowTextA(hMaxInputEdit, value);
		}
		else if (strstr(line, "max_file_uploads")) {
			sscanf(line, "max_file_uploads = %255s", value);
			SetWindowTextA(hMaxFileEdit, value);
		}
		else if (strstr(line, "session.name")) {
			sscanf(line, "session.name = %255s", value);
			SetWindowTextA(hSessionNameEdit, value);
		}
		else if (strstr(line, "opcache.memory_consumption")) {
			sscanf(line, "opcache.memory_consumption = %255s", value);
			SetWindowTextA(hOpcacheMemEdit, value);
		}
		else if (strstr(line, "date.timezone")) {
			char* eq = strchr(line, '=');
			if (eq) {
				char* start = eq + 1;
				while (*start == ' ' || *start == '\t') start++;
				char* end = start;
				while (*end && *end != '\r' && *end != '\n') end++;
				*end = 0;
				SetWindowTextA(hTimezoneEdit, start);
			}
		}
		else if (strstr(line, "display_errors")) {
			sscanf(line, "display_errors = %255s", value);
			SendMessageA(hDisplayErrors, CB_SETCURSEL, (strstr(value, "On") || strstr(value, "on")) ? 0 : 1, 0);
		}
		else if (strstr(line, "log_errors")) {
			sscanf(line, "log_errors = %255s", value);
			SendMessageA(hLogErrors, CB_SETCURSEL, (strstr(value, "On") || strstr(value, "on")) ? 0 : 1, 0);
		}
		else if (strstr(line, "allow_url_fopen")) {
			sscanf(line, "allow_url_fopen = %255s", value);
			SendMessageA(hAllowUrlFopen, CB_SETCURSEL, (strstr(value, "On") || strstr(value, "on")) ? 0 : 1, 0);
		}
		else if (strstr(line, "short_open_tag")) {
			sscanf(line, "short_open_tag = %255s", value);
			SendMessageA(hShortOpenTag, CB_SETCURSEL, (strstr(value, "On") || strstr(value, "on")) ? 0 : 1, 0);
		}
		else if (strstr(line, "zlib.output_compression")) {
			sscanf(line, "zlib.output_compression = %255s", value);
			SendMessageA(hZlibCompress, CB_SETCURSEL, (strstr(value, "On") || strstr(value, "on")) ? 0 : 1, 0);
		}
		else if (strstr(line, "session.auto_start")) {
			sscanf(line, "session.auto_start = %255s", value);
			SendMessageA(hSessionAuto, CB_SETCURSEL, (strstr(value, "On") || strstr(value, "on")) ? 0 : 1, 0);
		}
		else if (strstr(line, "opcache.enable")) {
			sscanf(line, "opcache.enable = %255s", value);
			SendMessageA(hOpcacheEnable, CB_SETCURSEL, (strstr(value, "On") || strstr(value, "on")) ? 0 : 1, 0);
		}
	}
	fclose(fp);
}

// 保存配置（真正的替换模式）
void SavePhpIni(void) {
	if (g_iniPath[0] == 0) {
		MessageBoxW(NULL, L"未找到 php.ini 文件，请手动选择", L"提示", MB_OK);
		OPENFILENAMEW ofn = {0};
		WCHAR filePath[MAX_PATH] = {0};
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = L"php.ini\0*.ini\0\0";
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST;
		if (GetOpenFileNameW(&ofn)) {
			wcscpy(g_iniPath, filePath);
		} else {
			return;
		}
	}
	
	// 获取用户输入
	char memoryLimit[64] = "128M";
	char uploadSize[64] = "128M";
	char postSize[64] = "128M";
	char timezone[256] = "Asia/Shanghai";
	char maxExec[64] = "30";
	char maxInput[64] = "60";
	char maxFile[64] = "20";
	char sessionName[64] = "PHPSESSID";
	char opcacheMem[64] = "128";
	
	GetWindowTextA(hMemoryEdit, memoryLimit, 64);
	GetWindowTextA(hUploadEdit, uploadSize, 64);
	GetWindowTextA(hPostEdit, postSize, 64);
	GetWindowTextA(hTimezoneEdit, timezone, 256);
	GetWindowTextA(hMaxExecEdit, maxExec, 64);
	GetWindowTextA(hMaxInputEdit, maxInput, 64);
	GetWindowTextA(hMaxFileEdit, maxFile, 64);
	GetWindowTextA(hSessionNameEdit, sessionName, 64);
	GetWindowTextA(hOpcacheMemEdit, opcacheMem, 64);
	
	int displayErrors = SendMessageA(hDisplayErrors, CB_GETCURSEL, 0, 0);
	int logErrors = SendMessageA(hLogErrors, CB_GETCURSEL, 0, 0);
	int allowUrlFopen = SendMessageA(hAllowUrlFopen, CB_GETCURSEL, 0, 0);
	int shortOpenTag = SendMessageA(hShortOpenTag, CB_GETCURSEL, 0, 0);
	int zlibCompress = SendMessageA(hZlibCompress, CB_GETCURSEL, 0, 0);
	int sessionAuto = SendMessageA(hSessionAuto, CB_GETCURSEL, 0, 0);
	int opcacheEnable = SendMessageA(hOpcacheEnable, CB_GETCURSEL, 0, 0);
	
	// 备份原文件
	WCHAR backupPath[MAX_PATH];
	wsprintfW(backupPath, L"%s.bak", g_iniPath);
	CopyFileW(g_iniPath, backupPath, FALSE);
	
	// 读取原文件
	FILE* fp = _wfopen(g_iniPath, L"rb");
	if (!fp) {
		MessageBoxW(NULL, L"无法打开 php.ini", L"错误", MB_OK);
		return;
	}
	
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* content = (char*)malloc(size + 2);
	if (!content) {
		fclose(fp);
		MessageBoxW(NULL, L"内存不足", L"错误", MB_OK);
		return;
	}
	fread(content, 1, size, fp);
	content[size] = 0;
	fclose(fp);
	
	// 逐行处理并替换
	char* newContent = (char*)malloc(size + 8192);
	if (!newContent) {
		free(content);
		MessageBoxW(NULL, L"内存不足", L"错误", MB_OK);
		return;
	}
	newContent[0] = 0;
	
	char* line = strtok(content, "\n");
	int replaced[20] = {0};
	
	while (line) {
		// 跳过之前添加的标记行
		if (strstr(line, "PHP配置管家修改")) {
			line = strtok(NULL, "\n");
			continue;
		}
		
		if ((strstr(line, "memory_limit") || strncmp(line, "memory_limit", 12) == 0) && !replaced[0] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "memory_limit = %s\n", memoryLimit);
			replaced[0] = 1;
		}
		else if ((strstr(line, "upload_max_filesize") || strncmp(line, "upload_max_filesize", 19) == 0) && !replaced[1] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "upload_max_filesize = %s\n", uploadSize);
			replaced[1] = 1;
		}
		else if ((strstr(line, "post_max_size") || strncmp(line, "post_max_size", 13) == 0) && !replaced[2] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "post_max_size = %s\n", postSize);
			replaced[2] = 1;
		}
		else if ((strstr(line, "max_execution_time") || strncmp(line, "max_execution_time", 18) == 0) && !replaced[3] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "max_execution_time = %s\n", maxExec);
			replaced[3] = 1;
		}
		else if ((strstr(line, "max_input_time") || strncmp(line, "max_input_time", 14) == 0) && !replaced[4] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "max_input_time = %s\n", maxInput);
			replaced[4] = 1;
		}
		else if ((strstr(line, "max_file_uploads") || strncmp(line, "max_file_uploads", 16) == 0) && !replaced[5] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "max_file_uploads = %s\n", maxFile);
			replaced[5] = 1;
		}
		else if ((strstr(line, "date.timezone") || strncmp(line, "date.timezone", 13) == 0) && !replaced[6] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "date.timezone = %s\n", timezone);
			replaced[6] = 1;
		}
		else if ((strstr(line, "session.name") || strncmp(line, "session.name", 12) == 0) && !replaced[7] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "session.name = %s\n", sessionName);
			replaced[7] = 1;
		}
		else if ((strstr(line, "display_errors") || strncmp(line, "display_errors", 14) == 0) && !replaced[8] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "display_errors = %s\n", displayErrors == 0 ? "On" : "Off");
			replaced[8] = 1;
		}
		else if ((strstr(line, "log_errors") || strncmp(line, "log_errors", 10) == 0) && !replaced[9] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "log_errors = %s\n", logErrors == 0 ? "On" : "Off");
			replaced[9] = 1;
		}
		else if ((strstr(line, "allow_url_fopen") || strncmp(line, "allow_url_fopen", 15) == 0) && !replaced[10] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "allow_url_fopen = %s\n", allowUrlFopen == 0 ? "On" : "Off");
			replaced[10] = 1;
		}
		else if ((strstr(line, "short_open_tag") || strncmp(line, "short_open_tag", 14) == 0) && !replaced[11] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "short_open_tag = %s\n", shortOpenTag == 0 ? "On" : "Off");
			replaced[11] = 1;
		}
		else if ((strstr(line, "zlib.output_compression")) && !replaced[12] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "zlib.output_compression = %s\n", zlibCompress == 0 ? "On" : "Off");
			replaced[12] = 1;
		}
		else if ((strstr(line, "session.auto_start") || strncmp(line, "session.auto_start", 18) == 0) && !replaced[13] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "session.auto_start = %s\n", sessionAuto == 0 ? "On" : "Off");
			replaced[13] = 1;
		}
		else if ((strstr(line, "opcache.enable") || strncmp(line, "opcache.enable", 14) == 0) && !replaced[14] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "opcache.enable = %s\n", opcacheEnable == 0 ? "On" : "Off");
			replaced[14] = 1;
		}
		else if ((strstr(line, "opcache.memory_consumption") || strncmp(line, "opcache.memory_consumption", 26) == 0) && !replaced[15] && strchr(line, '=')) {
			sprintf(newContent + strlen(newContent), "opcache.memory_consumption = %s\n", opcacheMem);
			replaced[15] = 1;
		}
		else {
			// 保留其他所有行
			sprintf(newContent + strlen(newContent), "%s\n", line);
		}
		
		line = strtok(NULL, "\n");
	}
	
	// 写回文件
	fp = _wfopen(g_iniPath, L"wb");
	if (fp) {
		fwrite(newContent, 1, strlen(newContent), fp);
		fclose(fp);
		MessageBoxW(NULL, L"配置已修改保存！请重启 PHP 生效", L"成功", MB_OK);
	} else {
		MessageBoxW(NULL, L"保存失败，请检查文件权限", L"错误", MB_OK);
	}
	
	free(content);
	free(newContent);
}

// 重启 IIS
void RestartPHP(HWND hwnd) {
	EnableWindow(GetDlgItem(hwnd, ID_RESTART_BTN), FALSE);
	SetWindowTextW(GetDlgItem(hwnd, ID_RESTART_BTN), L"正在重启...");
	SendMessageW(hProgress, PBM_SETPOS, 30, 0);
	SetWindowTextW(GetDlgItem(hwnd, ID_STATUS), L"正在重启 IIS...");
	
	system("iisreset");
	
	SendMessageW(hProgress, PBM_SETPOS, 100, 0);
	SetWindowTextW(GetDlgItem(hwnd, ID_STATUS), L"重启成功！");
	EnableWindow(GetDlgItem(hwnd, ID_RESTART_BTN), TRUE);
	SetWindowTextW(GetDlgItem(hwnd, ID_RESTART_BTN), L"重启 PHP");
	SetTimer(hwnd, 1, 2000, NULL);
}

// 获取当前 PHP 版本
void GetPhpVersion(WCHAR* version, int size) {
	wcscpy(version, L"未检测到");
	
	// 从注册表读取
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\PHP\\", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		DWORD bufSize = size;
		RegQueryValueExW(hKey, L"Version", NULL, NULL, (LPBYTE)version, &bufSize);
		RegCloseKey(hKey);
		if (version[0] != 0 && wcscmp(version, L"未检测到") != 0) return;
	}
	
	// 从 php.ini 路径推断
	if (g_iniPath[0] != 0) {
		WCHAR* p = wcsstr(g_iniPath, L"php-");
		if (p) {
			wcscpy(version, p + 4);
			WCHAR* end = wcschr(version, L'\\');
			if (end) *end = 0;
		}
	}
}

// 显示帮助窗口
void ShowHelpWindow(HWND hwndParent) {
	WCHAR phpVersion[128] = {0};
	GetPhpVersion(phpVersion, 128);
	
	// 创建帮助对话框
	HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"STATIC", L"帮助 - PHP配置管家",
								WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU,
								CW_USEDEFAULT, CW_USEDEFAULT, 480, 420,
								hwndParent, NULL, hInst, NULL);
	
	// 设置字体
	HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
							  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							  DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
	
	// PHP官方下载地址
	CreateWindowW(L"STATIC", L"PHP 官方下载地址：", WS_CHILD | WS_VISIBLE,
				  20, 20, 200, 25, hDlg, NULL, hInst, NULL);
	HWND hUrl = CreateWindowW(L"STATIC", L"https://windows.php.net/download/", WS_CHILD | WS_VISIBLE,
							  20, 45, 400, 25, hDlg, NULL, hInst, NULL);
	SendMessageW(hUrl, WM_SETFONT, (WPARAM)hFont, TRUE);
	
	// 当前 PHP 版本
	CreateWindowW(L"STATIC", L"▸ 当前 PHP 版本", WS_CHILD | WS_VISIBLE,
				  20, 80, 200, 20, hDlg, NULL, hInst, NULL);
	WCHAR versionText[256];
	wsprintfW(versionText, L"  %s", phpVersion);
	HWND hVersion = CreateWindowW(L"STATIC", versionText, WS_CHILD | WS_VISIBLE,
								  20, 103, 350, 20, hDlg, NULL, hInst, NULL);
	SendMessageW(hVersion, WM_SETFONT, (WPARAM)hFont, TRUE);
	
	// 线程安全说明
	CreateWindowW(L"STATIC", L"▸ 版本选择", WS_CHILD | WS_VISIBLE,
				  20, 135, 200, 20, hDlg, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"  IIS + FastCGI 请下载 Non-Thread-Safe (nts) 版本", WS_CHILD | WS_VISIBLE,
				  20, 158, 380, 20, hDlg, NULL, hInst, NULL);
	
	// 安装步骤
	CreateWindowW(L"STATIC", L"▸ 安装步骤", WS_CHILD | WS_VISIBLE,
				  20, 190, 200, 20, hDlg, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"  1. 下载 PHP 压缩包（推荐 nts-x64 版本）", WS_CHILD | WS_VISIBLE,
				  20, 213, 350, 20, hDlg, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"  2. 解压到 C:\\php 或 C:\\php-版本号", WS_CHILD | WS_VISIBLE,
				  20, 236, 350, 20, hDlg, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"  3. 复制 php.ini-development 为 php.ini", WS_CHILD | WS_VISIBLE,
				  20, 259, 350, 20, hDlg, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"  4. 用本工具修改配置，然后点击「重启 PHP」", WS_CHILD | WS_VISIBLE,
				  20, 282, 380, 20, hDlg, NULL, hInst, NULL);
	
	// 版本切换说明
	CreateWindowW(L"STATIC", L"▸ 版本切换方法", WS_CHILD | WS_VISIBLE,
				  20, 315, 200, 20, hDlg, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"  方法一：修改系统 PATH 环境变量中的 PHP 路径", WS_CHILD | WS_VISIBLE,
				  20, 338, 380, 20, hDlg, NULL, hInst, NULL);
	CreateWindowW(L"STATIC", L"  方法二：在本工具中手动选择新版本的 php.ini", WS_CHILD | WS_VISIBLE,
				  20, 361, 380, 20, hDlg, NULL, hInst, NULL);
	
	// 关闭按钮
	HWND hBtn = CreateWindowW(L"BUTTON", L"关闭", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							  190, 395, 80, 30, hDlg, (HMENU)IDOK, hInst, NULL);
	SendMessageW(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
	
	// 消息循环
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		if (msg.hwnd == hDlg && msg.message == WM_COMMAND && LOWORD(msg.wParam) == IDOK) {
			DestroyWindow(hDlg);
			DeleteObject(hFont);
			break;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

void ShowTab(int tabIndex) {
	ShowWindow(hPanel1, tabIndex == 0 ? SW_SHOW : SW_HIDE);
	ShowWindow(hPanel2, tabIndex == 1 ? SW_SHOW : SW_HIDE);
	ShowWindow(hPanel3, tabIndex == 2 ? SW_SHOW : SW_HIDE);
	TabCtrl_SetCurSel(hTab, tabIndex);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE: {
		InitCommonControls();
		FindPhpIniPath();
		
		SetWindowTextW(hwnd, L"PHP 配置管家 v2.0 - WINRX论坛版");
		
		// 创建Tab控件
		hTab = CreateWindowW(WC_TABCONTROLW, NULL, WS_CHILD | WS_VISIBLE | TCS_FIXEDWIDTH,
							 10, 10, 680, 420, hwnd, (HMENU)ID_TAB, hInst, NULL);
		TCITEMW tie = { TCIF_TEXT };
		WCHAR tab1[] = L"基础设置";
		WCHAR tab2[] = L"高级设置";
		WCHAR tab3[] = L"性能优化";
		tie.pszText = tab1; TabCtrl_InsertItem(hTab, 0, &tie);
		tie.pszText = tab2; TabCtrl_InsertItem(hTab, 1, &tie);
		tie.pszText = tab3; TabCtrl_InsertItem(hTab, 2, &tie);
		
		// ========== 面板1 基础设置 ==========
		hPanel1 = CreateWindowW(L"STATIC", NULL, WS_CHILD | WS_VISIBLE, 10, 45, 680, 370, hwnd, (HMENU)ID_TAB1_PANEL, hInst, NULL);
		
		// 左列
		CreateWindowW(L"STATIC", L"memory_limit", WS_CHILD | WS_VISIBLE, 20, 65, 100, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"内存限制", WS_CHILD | WS_VISIBLE, 20, 83, 100, 16, hPanel1, NULL, hInst, NULL);
		hMemoryEdit = CreateWindowW(L"EDIT", L"128M", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 68, 120, 24, hPanel1, (HMENU)ID_MEMORY_LIMIT, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"upload_max_filesize", WS_CHILD | WS_VISIBLE, 20, 110, 140, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"上传文件大小限制", WS_CHILD | WS_VISIBLE, 20, 128, 140, 16, hPanel1, NULL, hInst, NULL);
		hUploadEdit = CreateWindowW(L"EDIT", L"128M", WS_CHILD | WS_VISIBLE | WS_BORDER, 170, 113, 120, 24, hPanel1, (HMENU)ID_UPLOAD_SIZE, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"post_max_size", WS_CHILD | WS_VISIBLE, 20, 155, 100, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"POST数据大小限制", WS_CHILD | WS_VISIBLE, 20, 173, 120, 16, hPanel1, NULL, hInst, NULL);
		hPostEdit = CreateWindowW(L"EDIT", L"128M", WS_CHILD | WS_VISIBLE | WS_BORDER, 150, 158, 120, 24, hPanel1, (HMENU)ID_POST_SIZE, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"max_execution_time", WS_CHILD | WS_VISIBLE, 20, 200, 130, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"脚本最大执行时间", WS_CHILD | WS_VISIBLE, 20, 218, 120, 16, hPanel1, NULL, hInst, NULL);
		hMaxExecEdit = CreateWindowW(L"EDIT", L"30", WS_CHILD | WS_VISIBLE | WS_BORDER, 150, 203, 80, 24, hPanel1, (HMENU)ID_MAX_EXEC, hInst, NULL);
		CreateWindowW(L"STATIC", L"秒", WS_CHILD | WS_VISIBLE, 240, 205, 30, 20, hPanel1, NULL, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"max_input_time", WS_CHILD | WS_VISIBLE, 20, 245, 100, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"脚本解析输入时间", WS_CHILD | WS_VISIBLE, 20, 263, 120, 16, hPanel1, NULL, hInst, NULL);
		hMaxInputEdit = CreateWindowW(L"EDIT", L"60", WS_CHILD | WS_VISIBLE | WS_BORDER, 150, 248, 80, 24, hPanel1, (HMENU)ID_MAX_INPUT, hInst, NULL);
		CreateWindowW(L"STATIC", L"秒", WS_CHILD | WS_VISIBLE, 240, 250, 30, 20, hPanel1, NULL, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"date.timezone", WS_CHILD | WS_VISIBLE, 20, 290, 100, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"时区设置", WS_CHILD | WS_VISIBLE, 20, 308, 100, 16, hPanel1, NULL, hInst, NULL);
		hTimezoneEdit = CreateWindowW(L"EDIT", L"Asia/Shanghai", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 293, 150, 24, hPanel1, (HMENU)ID_TIMEZONE, hInst, NULL);
		
		// 右列
		CreateWindowW(L"STATIC", L"display_errors", WS_CHILD | WS_VISIBLE, 380, 65, 100, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"是否显示错误", WS_CHILD | WS_VISIBLE, 380, 83, 100, 16, hPanel1, NULL, hInst, NULL);
		hDisplayErrors = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 490, 68, 80, 100, hPanel1, (HMENU)ID_DISPLAY_ERRORS, hInst, NULL);
		SendMessageA(hDisplayErrors, CB_ADDSTRING, 0, (LPARAM)"开启");
		SendMessageA(hDisplayErrors, CB_ADDSTRING, 0, (LPARAM)"关闭");
		SendMessageA(hDisplayErrors, CB_SETCURSEL, 0, 0);
		
		CreateWindowW(L"STATIC", L"log_errors", WS_CHILD | WS_VISIBLE, 380, 110, 100, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"是否记录错误日志", WS_CHILD | WS_VISIBLE, 380, 128, 110, 16, hPanel1, NULL, hInst, NULL);
		hLogErrors = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 500, 113, 80, 100, hPanel1, (HMENU)ID_LOG_ERRORS, hInst, NULL);
		SendMessageA(hLogErrors, CB_ADDSTRING, 0, (LPARAM)"开启");
		SendMessageA(hLogErrors, CB_ADDSTRING, 0, (LPARAM)"关闭");
		SendMessageA(hLogErrors, CB_SETCURSEL, 0, 0);
		
		CreateWindowW(L"STATIC", L"error_reporting", WS_CHILD | WS_VISIBLE, 380, 155, 100, 20, hPanel1, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"错误报告级别", WS_CHILD | WS_VISIBLE, 380, 173, 100, 16, hPanel1, NULL, hInst, NULL);
		hErrorReporting = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 490, 158, 120, 100, hPanel1, (HMENU)ID_ERROR_REPORTING, hInst, NULL);
		SendMessageA(hErrorReporting, CB_ADDSTRING, 0, (LPARAM)"E_ALL");
		SendMessageA(hErrorReporting, CB_ADDSTRING, 0, (LPARAM)"E_WARNING");
		SendMessageA(hErrorReporting, CB_ADDSTRING, 0, (LPARAM)"E_ERROR");
		SendMessageA(hErrorReporting, CB_SETCURSEL, 0, 0);
		
		// ========== 面板2 高级设置 ==========
		hPanel2 = CreateWindowW(L"STATIC", NULL, WS_CHILD, 10, 45, 680, 370, hwnd, (HMENU)ID_TAB2_PANEL, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"max_file_uploads", WS_CHILD | WS_VISIBLE, 20, 65, 120, 20, hPanel2, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"单次最多上传文件数", WS_CHILD | WS_VISIBLE, 20, 83, 130, 16, hPanel2, NULL, hInst, NULL);
		hMaxFileEdit = CreateWindowW(L"EDIT", L"20", WS_CHILD | WS_VISIBLE | WS_BORDER, 160, 68, 80, 24, hPanel2, (HMENU)ID_MAX_FILE, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"allow_url_fopen", WS_CHILD | WS_VISIBLE, 20, 110, 120, 20, hPanel2, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"允许打开远程文件", WS_CHILD | WS_VISIBLE, 20, 128, 120, 16, hPanel2, NULL, hInst, NULL);
		hAllowUrlFopen = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 150, 113, 80, 100, hPanel2, (HMENU)ID_ALLOW_URL_FOPEN, hInst, NULL);
		SendMessageA(hAllowUrlFopen, CB_ADDSTRING, 0, (LPARAM)"开启");
		SendMessageA(hAllowUrlFopen, CB_ADDSTRING, 0, (LPARAM)"关闭");
		SendMessageA(hAllowUrlFopen, CB_SETCURSEL, 0, 0);
		
		CreateWindowW(L"STATIC", L"short_open_tag", WS_CHILD | WS_VISIBLE, 20, 155, 120, 20, hPanel2, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"短标签支持", WS_CHILD | WS_VISIBLE, 20, 173, 100, 16, hPanel2, NULL, hInst, NULL);
		hShortOpenTag = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 130, 158, 80, 100, hPanel2, (HMENU)ID_SHORT_OPEN_TAG, hInst, NULL);
		SendMessageA(hShortOpenTag, CB_ADDSTRING, 0, (LPARAM)"开启");
		SendMessageA(hShortOpenTag, CB_ADDSTRING, 0, (LPARAM)"关闭");
		SendMessageA(hShortOpenTag, CB_SETCURSEL, 0, 0);
		
		CreateWindowW(L"STATIC", L"zlib.output_compression", WS_CHILD | WS_VISIBLE, 20, 200, 150, 20, hPanel2, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"启用Gzip压缩输出", WS_CHILD | WS_VISIBLE, 20, 218, 120, 16, hPanel2, NULL, hInst, NULL);
		hZlibCompress = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 180, 203, 80, 100, hPanel2, (HMENU)ID_ZLIB_COMPRESS, hInst, NULL);
		SendMessageA(hZlibCompress, CB_ADDSTRING, 0, (LPARAM)"开启");
		SendMessageA(hZlibCompress, CB_ADDSTRING, 0, (LPARAM)"关闭");
		SendMessageA(hZlibCompress, CB_SETCURSEL, 1, 0);
		
		// 右列
		CreateWindowW(L"STATIC", L"session.auto_start", WS_CHILD | WS_VISIBLE, 380, 65, 120, 20, hPanel2, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"自动启动会话", WS_CHILD | WS_VISIBLE, 380, 83, 100, 16, hPanel2, NULL, hInst, NULL);
		hSessionAuto = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 500, 68, 80, 100, hPanel2, (HMENU)ID_SESSION_AUTO, hInst, NULL);
		SendMessageA(hSessionAuto, CB_ADDSTRING, 0, (LPARAM)"开启");
		SendMessageA(hSessionAuto, CB_ADDSTRING, 0, (LPARAM)"关闭");
		SendMessageA(hSessionAuto, CB_SETCURSEL, 1, 0);
		
		CreateWindowW(L"STATIC", L"session.name", WS_CHILD | WS_VISIBLE, 380, 110, 120, 20, hPanel2, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"会话名称", WS_CHILD | WS_VISIBLE, 380, 128, 100, 16, hPanel2, NULL, hInst, NULL);
		hSessionNameEdit = CreateWindowW(L"EDIT", L"PHPSESSID", WS_CHILD | WS_VISIBLE | WS_BORDER, 490, 113, 120, 24, hPanel2, (HMENU)ID_SESSION_NAME, hInst, NULL);
		
		// ========== 面板3 性能优化 ==========
		hPanel3 = CreateWindowW(L"STATIC", NULL, WS_CHILD, 10, 45, 680, 370, hwnd, (HMENU)ID_TAB3_PANEL, hInst, NULL);
		
		CreateWindowW(L"STATIC", L"opcache.enable", WS_CHILD | WS_VISIBLE, 20, 65, 120, 20, hPanel3, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"开启OPcache加速", WS_CHILD | WS_VISIBLE, 20, 83, 120, 16, hPanel3, NULL, hInst, NULL);
		hOpcacheEnable = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 150, 68, 80, 100, hPanel3, (HMENU)ID_OPCACHE_ENABLE, hInst, NULL);
		SendMessageA(hOpcacheEnable, CB_ADDSTRING, 0, (LPARAM)"开启");
		SendMessageA(hOpcacheEnable, CB_ADDSTRING, 0, (LPARAM)"关闭");
		SendMessageA(hOpcacheEnable, CB_SETCURSEL, 0, 0);
		
		CreateWindowW(L"STATIC", L"opcache.memory_consumption", WS_CHILD | WS_VISIBLE, 20, 110, 170, 20, hPanel3, NULL, hInst, NULL);
		CreateWindowW(L"STATIC", L"OPcache内存大小", WS_CHILD | WS_VISIBLE, 20, 128, 130, 16, hPanel3, NULL, hInst, NULL);
		hOpcacheMemEdit = CreateWindowW(L"EDIT", L"128", WS_CHILD | WS_VISIBLE | WS_BORDER, 200, 113, 80, 24, hPanel3, (HMENU)ID_OPCACHE_MEM, hInst, NULL);
		CreateWindowW(L"STATIC", L"MB", WS_CHILD | WS_VISIBLE, 290, 115, 40, 20, hPanel3, NULL, hInst, NULL);
		
		ShowTab(0);
		
		// 底部按钮
		CreateWindowW(L"BUTTON", L"保存配置", WS_CHILD | WS_VISIBLE, 20, 435, 100, 35, hwnd, (HMENU)ID_SAVE_BTN, hInst, NULL);
		CreateWindowW(L"BUTTON", L"帮助", WS_CHILD | WS_VISIBLE, 140, 435, 80, 35, hwnd, (HMENU)ID_HELP_BTN, hInst, NULL);
		CreateWindowW(L"BUTTON", L"重启 PHP (IIS)", WS_CHILD | WS_VISIBLE, 530, 435, 130, 35, hwnd, (HMENU)ID_RESTART_BTN, hInst, NULL);
		
		hProgress = CreateWindowW(PROGRESS_CLASSW, NULL, WS_CHILD | WS_VISIBLE, 20, 485, 640, 20, hwnd, (HMENU)ID_PROGRESS, hInst, NULL);
		SendMessageW(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		
		hStatus = CreateWindowW(L"STATIC", L"状态: 就绪", WS_CHILD | WS_VISIBLE, 20, 520, 400, 25, hwnd, (HMENU)ID_STATUS, hInst, NULL);
		
		LoadPhpSettings();
		break;
	}
		case WM_NOTIFY: {
			NMHDR* nmhdr = (NMHDR*)lParam;
			if (nmhdr->idFrom == ID_TAB && nmhdr->code == TCN_SELCHANGE) {
				ShowTab(TabCtrl_GetCurSel(hTab));
			}
			break;
		}
		case WM_COMMAND: {
			if (LOWORD(wParam) == ID_RESTART_BTN) {
				RestartPHP(hwnd);
			}
			else if (LOWORD(wParam) == ID_SAVE_BTN) {
				SavePhpIni();
			}
			else if (LOWORD(wParam) == ID_HELP_BTN) {
				ShowHelpWindow(hwnd);
			}
			break;
		}
	case WM_TIMER:
		if (wParam == 1) {
			KillTimer(hwnd, 1);
			SetWindowTextW(GetDlgItem(hwnd, ID_STATUS), L"状态: 就绪");
			SendMessageW(hProgress, PBM_SETPOS, 0, 0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	hInst = hInstance;
	WNDCLASSW wc = {0};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"PHPConfigTool";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	RegisterClassW(&wc);
	
	HWND hwnd = CreateWindowExW(0, L"PHPConfigTool", L"PHP 配置管家 v2.0 - WINRX论坛版",
								WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
								720, 600, NULL, NULL, hInstance, NULL);
	if (!hwnd) return 0;
	
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return msg.wParam;
}
