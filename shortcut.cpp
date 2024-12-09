#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string>
using std::string;

#define HOTKEY_ID_BASE		10		// 热键

//to do
/*
* read the outline of code
* find how to read the clipboard in this file
* find how to add custom action for special cmd in this file 
* add special action for "trim newline and paste" in this file, just as [exit] 
*	check if the clipboard content is a text format (messageBoxOut tmp info)
*	remove all the newline characters, then write back to the clipboard (messageBoxOut tmp info)
*	do the "Paste" action or simulate pressing Ctrl+V directly
*/

DWORD GetProcessIDFromName(LPCSTR szName);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SetAutoRun();
void ClearAutoRun();
int ReadSetting();
void HotKeyProc(HWND hWnd, UINT uModifiers, UINT uVirtKey);

// 配置
struct Setting
{
	UINT modifiers;
	UINT vkey1;
	UINT vkey2;
	char cmd[MAX_PATH];
	UINT param;
	char hotkey[16];
};
Setting* Settings;
int nSet = 0;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	// 防止重复打开与提权
	HANDLE hMutex = CreateMutex(NULL, FALSE, "ShortCut");
	if (__argc < 2 || strcmp(__argv[1], "uac") != 0)	
	{
		// 防止重复打开
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hMutex);
			MessageBox(NULL, "程序已经在运行！", "ShortCut", MB_OK | MB_ICONWARNING);
			return FALSE;
		}
		// 提权
		TCHAR szPath[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, szPath, MAX_PATH);
		TCHAR szParams[MAX_PATH] = { 0 };
		sprintf_s(szParams, MAX_PATH, "uac");
		for (int i = 1; i < __argc; i++)
		{
			sprintf_s(szParams, MAX_PATH, "%s %s", szParams, __argv[i]);
		}
		SHELLEXECUTEINFO si = { 0 };
		si.cbSize = sizeof(SHELLEXECUTEINFOW);
		si.lpFile = szPath;
		si.lpVerb = TEXT("runas");
		si.lpDirectory = NULL;
		si.lpParameters = szParams;
		ShellExecuteEx(&si);
		return FALSE;
	}
	// 是否开机启动
	if (__argc < 3 || strcmp(__argv[2], "AutoRun") != 0)
	{
		int nRet = MessageBox(NULL, "是否开机启动？", "ShortCut", MB_YESNO | MB_ICONQUESTION);
		if (IDYES == nRet)
		{
			SetAutoRun();
		}
		else
		{
			ClearAutoRun();
		}
	}
	// 注册窗口类
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = "ShortCut";
	if (!RegisterClassEx(&wcex))
	{
		return FALSE;
	}
	// 创建窗口(并不显示)
	HWND hWnd = CreateWindowA("ShortCut", "ShortCut", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{
		return FALSE;
	}
	// 读取配置
	nSet = ReadSetting();
	if (nSet < 0)
	{
		MessageBox(NULL, "未找到配置文件，点击确定退出！", "ShortCut", MB_OK | MB_ICONWARNING);
		return FALSE;
	}
	else if (nSet == 0)
	{
		MessageBox(NULL, "配置无效，点击确定退出！", "ShortCut", MB_OK | MB_ICONWARNING);
		return FALSE;
	}
	// 注册热键
	bool bOK = false;
	for (int i = 0; i < nSet; i++)
	{
		if (!RegisterHotKey(hWnd, HOTKEY_ID_BASE + i, Settings[i].modifiers, Settings[i].vkey1))
		{
			char tip[64] = { 0 };
			sprintf_s(tip, 64, "热键(%s)注册失败！", Settings[i].hotkey);
			MessageBox(NULL, tip, "ShortCut", MB_OK | MB_ICONWARNING);
		}
		else
		{
			bOK = true;
		}
	}
	if (!bOK)// 全部注册失败
	{
		return FALSE;
	}
	// 消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_HOTKEY:
		HotKeyProc(hWnd, UINT(LOWORD(lParam)), UINT(HIWORD(lParam)));
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 由进程Exe文件名获取进程ID，失败返回0
DWORD GetProcessIDFromName(LPCSTR szName)
{
	DWORD id = 0;       // 进程ID
	PROCESSENTRY32 pe;  // 进程信息
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // 获取系统进程列表
	if (Process32First(hSnapshot, &pe))      // 返回系统中第一个进程的信息
	{
		do
		{
			if (0 == _stricmp(pe.szExeFile, szName)) // 不区分大小写比较
			{
				id = pe.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe));      // 下一个进程
	}
	CloseHandle(hSnapshot);     // 删除快照
	return id;
}

void SetAutoRun()
{
	HKEY hKey;
	LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	CHAR sFileName[MAX_PATH];
	CHAR sKeyValue[MAX_PATH];
	GetModuleFileName(NULL, sFileName, MAX_PATH);
	sprintf_s(sKeyValue, MAX_PATH, "\"%s\" AutoRun", sFileName);
	// 打开启动项
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
		lpRun, 0, KEY_WRITE, &hKey))
	{
		RegSetValueEx(hKey, "ShortCut", 0, REG_SZ, (BYTE*)sKeyValue, strlen(sKeyValue));
		RegCloseKey(hKey);		// 关闭
	}
	else
	{
		MessageBox(NULL, "设置开机启动失败！", "ShortCut", MB_OK | MB_ICONWARNING);
	}
}

void ClearAutoRun()
{
	HKEY hKey;
	LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	// 打开启动项
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
		lpRun, 0, KEY_WRITE, &hKey))
	{
		//删除
		RegDeleteValue(hKey, "ShortCut");
		//关闭
		RegCloseKey(hKey);
	}
	else
	{
		MessageBox(NULL, "取消开机启动失败！", "ShortCut", MB_OK | MB_ICONWARNING);
	}
}

void HotKeyProc(HWND hWnd, UINT uModifiers, UINT uVirtKey)
{
	for (int i = 0; i < nSet; i++)
	{
		if (uModifiers == Settings[i].modifiers && uVirtKey == Settings[i].vkey1)
		{
			if (Settings[i].vkey2 > 0 && GetKeyState(Settings[i].vkey2) >= 0)
			{
				continue;
			}
			string clipboard = "";
			if (Settings[i].param == 1) // 退出
			{
				int nRet = MessageBox(NULL, "是否关闭ShortCut？", "ShortCut", MB_YESNO | MB_ICONQUESTION);
				if (nRet != IDYES)
				{
					break;
				}
				for (int k = 0; k < nSet; k++)
				{
					UnregisterHotKey(hWnd, HOTKEY_ID_BASE + k);
				}
				delete[] Settings;
				PostQuitMessage(0);
				break;
			}
			else if (Settings[i].param == 2)// 剪贴板
			{
				char clip[5120]{0};
				OpenClipboard(NULL);
				HANDLE hData = GetClipboardData(CF_TEXT);
				if (hData != NULL)
				{
					char* pData = (char*)GlobalLock(hData);
					strcpy_s(clip, 5120, pData);
					GlobalUnlock(hData);
					clipboard = clip;
				}
				CloseClipboard();
			}
			string cmd = Settings[i].cmd;
			//string exe = cmd.substr(0, cmd.find(".exe") + 4);
			//if (0 == GetProcessIDFromName(exe.c_str()))
			{
				// 启动程序
				string cmdline = "\"" + cmd + "\" " + clipboard;
				int len = cmd.length();
				if (len >= 4 &&
					(cmd[len - 1] == 't' || cmd[len - 1] == 'T') &&
					(cmd[len - 2] == 'a' || cmd[len - 2] == 'A') &&
					(cmd[len - 3] == 'b' || cmd[len - 3] == 'B') &&
					cmd[len - 4] == '.')	// .bat
				{
					system(cmdline.c_str());
				}
				else
				{
					WinExec(cmdline.c_str(), SW_SHOWNORMAL);
				}
			}
			break;
		}
	}
}

// 读取配置
int ReadSetting()
{
	char path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	strrchr(path, '\\')[0] = 0;
	sprintf_s(path, MAX_PATH, "%s\\setting.ini", path);
	FILE* pFile;
	if (0 == fopen_s(&pFile, path, "r"))
	{
		fseek(pFile, 0, SEEK_END);
		int len = ftell(pFile);
		char * txt = new char[len + 1];
		fseek(pFile, 0, SEEK_SET);
		fread(txt, 1, len, pFile);
		txt[len] = 0;
		string text = txt;
		delete[] txt;
		text = text.substr(0, text.find("\n#####") + 1);
		int lines = 0;
		for (UINT i = 0; i < text.length(); i++)
		{
			if (text.at(i) == '\n')
			{
				lines++;
			}
		}
		if (lines > 0)
		{
			Settings = new Setting[lines];
			int index = 0;
			int endline = -1;
			while ((endline = text.find('\n')) > 0)
			{
				string set = text.substr(0, endline);
				text = text.substr(endline + 1, text.length() - endline - 1);
				if (set.length() < 5 || set.at(0) != '#')
				{
					continue;
				}
				int second = set.find('#', 1);
				if (second > 0)
				{
					set = set.substr(1, second - 1);
				}
				else
				{
					set = set.substr(1, set.length() - 1);
				}
				string s[4]{"", "", "", ""};
				int pos = -1, si = 0;
				set = set + "|";
				while ((pos = set.find('|')) >= 0)
				{
					s[si++] = set.substr(0, pos);
					set = set.substr(pos + 1, set.length() - pos - 1);
				}
				if (s[0].length() == 1)
				{
					if (s[0].at(0) == 'C' || s[0].at(0) == 'c')
					{
						Settings[index].modifiers = MOD_CONTROL;
						sprintf_s(Settings[index].hotkey, 16, "%s", "Ctrl+");
					}
					else if (s[0].at(0) == 'A' || s[0].at(0) == 'a')
					{
						Settings[index].modifiers = MOD_ALT;
						sprintf_s(Settings[index].hotkey, 16, "%s", "Alt+");
					}
					else
					{
						continue;
					}
				}
				else if (s[0].length() == 2)
				{
					if (s[0] == "CA" || s[0] == "AC" || s[0] == "ca" || s[0] == "ac" || 
						s[0] == "Ca" || s[0] == "Ac" || s[0] == "cA" || s[0] == "aC")
					{
						Settings[index].modifiers = MOD_CONTROL | MOD_ALT;
						sprintf_s(Settings[index].hotkey, 16, "%s", "Ctrl+Alt+");
					}
					else
					{
						continue;
					}
				}
				else
				{
					continue;
				}
				if (s[1].length() == 1)
				{
					if (s[1].at(0) >= 'A' && s[1].at(0) <= 'Z')
					{
						Settings[index].vkey1 = UINT(s[1].at(0));
						Settings[index].vkey2 = 0;
						sprintf_s(Settings[index].hotkey, 16, "%s%c", Settings[index].hotkey, s[1].at(0));
					}
					else if (s[1].at(0) >= 'a' && s[1].at(0) <= 'z')
					{
						Settings[index].vkey1 = UINT(s[1].at(0) - 'a' + 'A');
						Settings[index].vkey2 = 0;
						sprintf_s(Settings[index].hotkey, 16, "%s%c", Settings[index].hotkey, s[1].at(0) - 'a' + 'A');
					}
					else
					{
						continue;
					}
				}
				else if (s[1].length() == 2)
				{
					if (s[1].at(0) >= 'A' && s[1].at(0) <= 'Z')
					{
						Settings[index].vkey2 = UINT(s[1].at(0));
					}
					else if (s[1].at(0) >= 'a' && s[1].at(0) <= 'z')
					{
						Settings[index].vkey2 = UINT(s[1].at(0) - 'a' + 'A');
					}
					else
					{
						continue;
					}
					if (s[1].at(1) >= 'A' && s[1].at(1) <= 'Z')
					{
						Settings[index].vkey1 = UINT(s[1].at(1));
					}
					else if (s[1].at(1) >= 'a' && s[1].at(1) <= 'z')
					{
						Settings[index].vkey1 = UINT(s[1].at(1) - 'a' + 'A');
					}
					else
					{
						continue;
					}
					sprintf_s(Settings[index].hotkey, 16, "%s%c+%c", Settings[index].hotkey,
						char(Settings[index].vkey2), char(Settings[index].vkey1));
				}
				else
				{
					continue;
				}
				memset(Settings[index].cmd, 0, MAX_PATH);
				sprintf_s(Settings[index].cmd, MAX_PATH, "%s", s[2].c_str());
				Settings[index].param = 0;
				if (s[3] == "[exit]")
				{
					Settings[index].param = 1;
				}
				else if (s[3] == "[clipboard]")
				{
					Settings[index].param = 2;
				}
				index++;
			}
			nSet = index;
		}
		fclose(pFile);
		return nSet;
	}
	return -1;
}


