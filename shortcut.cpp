#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <psapi.h> 
#include <string>
#include <map>
#include <list>
#include <algorithm>
using std::string;
using std::wstring;
using std::map;
using std::list;

#define HOTKEY_ID_BASE		10		// �ȼ�
namespace winSwitch{
	map<wstring, list<HWND> > gWinLists;
	HWND ghWnd = NULL;
	bool inited = false;
	void SwitchWins();
	void InitIfNeed();
	void AddToWinLists(HWND hWnd);
	void RemoveFromWinLists(HWND hWnd);
	wstring GetExeNameFromHwnd(HWND hWnd);
	BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
	void WinChangeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool IsWindowTopShow(HWND hWnd);
}

DWORD GetProcessIDFromName(LPCSTR szName);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SetAutoRun();
void ClearAutoRun();
int ReadSetting();
void HotKeyProc(HWND hWnd, UINT uModifiers, UINT uVirtKey);

// ����
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

void SimulateCtrlV() {

	// Create an array to hold the input events
	INPUT inputs[4] = {};

	// Ctrl key down
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_CONTROL;
	inputs[0].ki.wScan = 0x1d;

	// 'V' key down
	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = 'V';
	inputs[1].ki.wScan = 0x2f;

	// 'V' key up
	inputs[2].type = INPUT_KEYBOARD;
	inputs[2].ki.wVk = 'V';
	inputs[2].ki.wScan = 0x2f;
	inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

	// Ctrl key up
	inputs[3].type = INPUT_KEYBOARD;
	inputs[3].ki.wVk = VK_CONTROL;
	inputs[3].ki.wScan = 0x1d;
	inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

	UINT eventsSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
	if (eventsSent != ARRAYSIZE(inputs)) {
		OutputDebugString("SendInput failed.");
	}
	else {
		OutputDebugString("Simulated Ctrl+V successfully!");
	}

}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	// ��ֹ�ظ�������Ȩ
	HANDLE hMutex = CreateMutex(NULL, FALSE, "ShortCut");
	if (__argc < 2 || strcmp(__argv[1], "uac") != 0)	
	{
		// ��ֹ�ظ���
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hMutex);
			MessageBox(NULL, "�����Ѿ������У�", "ShortCut", MB_OK | MB_ICONWARNING);
			return FALSE;
		}
		// ��Ȩ
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
	// �Ƿ񿪻�����
	if (__argc < 3 || strcmp(__argv[2], "AutoRun") != 0)
	{
		int nRet = MessageBox(NULL, "�Ƿ񿪻�������", "ShortCut", MB_YESNO | MB_ICONQUESTION);
		if (IDYES == nRet)
		{
			SetAutoRun();
		}
		else
		{
			ClearAutoRun();
		}
	}
	// ע�ᴰ����
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
	// ��������(������ʾ)
	HWND hWnd = CreateWindowA("ShortCut", "ShortCut", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{
		return FALSE;
	}
	winSwitch::ghWnd = hWnd;
	if ( !RegisterShellHookWindow(hWnd)) {
		MessageBox(NULL, "δ��ע�ṳ�ӣ�", "ShortCut", MB_OK | MB_ICONWARNING);
		return FALSE;
	}
	// ��ȡ����
	nSet = ReadSetting();
	if (nSet < 0)
	{
		MessageBox(NULL, "δ�ҵ������ļ������ȷ���˳���", "ShortCut", MB_OK | MB_ICONWARNING);
		return FALSE;
	}
	else if (nSet == 0)
	{
		MessageBox(NULL, "������Ч�����ȷ���˳���", "ShortCut", MB_OK | MB_ICONWARNING);
		return FALSE;
	}
	// ע���ȼ�
	bool bOK = false;
	for (int i = 0; i < nSet; i++)
	{
		if (!RegisterHotKey(hWnd, HOTKEY_ID_BASE + i, Settings[i].modifiers, Settings[i].vkey1))
		{
			char tip[64] = { 0 };
			sprintf_s(tip, 64, "�ȼ�(%s)ע��ʧ�ܣ�", Settings[i].hotkey);
			MessageBox(NULL, tip, "ShortCut", MB_OK | MB_ICONWARNING);
		}
		else
		{
			bOK = true;
		}
	}
	if (!bOK)// ȫ��ע��ʧ��
	{
		return FALSE;
	}
	// ��Ϣѭ��
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
		if (winSwitch::inited){
			static UINT sShellHookMsg = RegisterWindowMessage("SHELLHOOK");
			if (message == sShellHookMsg) {
				OutputDebugString("new shellhook!");
				winSwitch::WinChangeProc(hWnd, message, wParam, lParam);
				break;
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// �ɽ���Exe�ļ�����ȡ����ID��ʧ�ܷ���0
DWORD GetProcessIDFromName(LPCSTR szName)
{
	DWORD id = 0;       // ����ID
	PROCESSENTRY32 pe;  // ������Ϣ
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // ��ȡϵͳ�����б�
	if (Process32First(hSnapshot, &pe))      // ����ϵͳ�е�һ�����̵���Ϣ
	{
		do
		{
			if (0 == _stricmp(pe.szExeFile, szName)) // �����ִ�Сд�Ƚ�
			{
				id = pe.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe));      // ��һ������
	}
	CloseHandle(hSnapshot);     // ɾ������
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
	// ��������
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
		lpRun, 0, KEY_WRITE, &hKey))
	{
		RegSetValueEx(hKey, "ShortCut", 0, REG_SZ, (BYTE*)sKeyValue, strlen(sKeyValue));
		RegCloseKey(hKey);		// �ر�
	}
	else
	{
		MessageBox(NULL, "���ÿ�������ʧ�ܣ�", "ShortCut", MB_OK | MB_ICONWARNING);
	}
}

void ClearAutoRun()
{
	HKEY hKey;
	LPCTSTR lpRun = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	// ��������
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
		lpRun, 0, KEY_WRITE, &hKey))
	{
		//ɾ��
		RegDeleteValue(hKey, "ShortCut");
		//�ر�
		RegCloseKey(hKey);
	}
	else
	{
		MessageBox(NULL, "ȡ����������ʧ�ܣ�", "ShortCut", MB_OK | MB_ICONWARNING);
	}
}


bool TrimClipboardCtx()
{
	if (!OpenClipboard(nullptr)) {
		OutputDebugString("Failed to open clipboard.\n");
		return false;
	}

	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (hData == NULL) {
			CloseClipboard();
			return false;
		}
		bool trim = false;
		wchar_t* pData = (wchar_t*)GlobalLock(hData);
		wchar_t* pw = pData, * pr = pData;
		for (;*pr != L'\0';pr++) {
			if (*pr != L'\r' && *pr != L'\n') {
				*pw = *pr;
				pw++;
			}
			else {
				trim = true;
			}
		}
		*pw = L'\0';
		if (trim) {
			size_t sz = (pw - pData)*sizeof(*pw);
			HANDLE hnData = GlobalAlloc(GMEM_MOVEABLE, (sz+1)*sizeof(*pw));
			wchar_t* pnData = (wchar_t*)GlobalLock(hnData);
			wcscpy_s(pnData, sz + 1, pData);
			GlobalUnlock(hData);
			GlobalUnlock(hnData);
			EmptyClipboard();
			SetClipboardData(CF_UNICODETEXT, hnData); // Set the clipboard data					
		}
	}
	else if (IsClipboardFormatAvailable(CF_TEXT)) {
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData == NULL) {
			CloseClipboard();
			return false;
		}
		bool trim = false;
		char* pData = (char*)GlobalLock(hData);
		char* pw = pData, * pr = pData;
		for (;*pr != '\0';pr++) {
			if (*pr != '\r' && *pr != '\n') {
				*pw = *pr;
				pw++;
			}
			else {
				trim = true;
			}
		}
		*pw = '\0';
		if (trim) {
			size_t sz = pw - pData;
			HANDLE hnData = GlobalAlloc(GMEM_MOVEABLE, sz + 1);
			char* pnData = (char*)GlobalLock(hnData);
			strcpy_s(pnData, sz + 1, pData);
			GlobalUnlock(hData);
			GlobalUnlock(hnData);
			EmptyClipboard();
			SetClipboardData(CF_TEXT, hnData); // Set the clipboard data					
		}
	}
	CloseClipboard();				
	return true;
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
			if (Settings[i].param == 1) // �˳�
			{
				int nRet = MessageBox(NULL, "�Ƿ�ر�ShortCut��", "ShortCut", MB_YESNO | MB_ICONQUESTION);
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
			else if (Settings[i].param == 2)// ������
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
			else if (Settings[i].param == 3) // trimPaste
			{

				if (TrimClipboardCtx()) {
					SimulateCtrlV();
				};
				break;
			}
			else if (Settings[i].param == 4) // switch windows of the same exe
			{
				winSwitch::SwitchWins();
				break;
			}
			string cmd = Settings[i].cmd;
			//string exe = cmd.substr(0, cmd.find(".exe") + 4);
			//if (0 == GetProcessIDFromName(exe.c_str()))
			{
				// ��������
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

// ��ȡ����
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
				else if (s[3] == "[trimPaste]")
				{
					Settings[index].param = 3;
				}
				else if (s[3] == "[SWISA]")
				{
					Settings[index].param = 4;
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



void winSwitch::SwitchWins()
{
	InitIfNeed();
	HWND hwnd = GetForegroundWindow();
	auto exeName = GetExeNameFromHwnd(hwnd);
	OutputDebugStringW(exeName.c_str());
	auto itor = gWinLists.find(exeName);
	if (itor == gWinLists.end()){
		OutputDebugString("cant not find data in gMap!");
		return;
	}
	auto& wList = itor->second;
	char tmp[50];
	sprintf_s(tmp, "list size:%d", wList.size());
	OutputDebugString(tmp);
	if (wList.empty()){
		OutputDebugString("empty wList!");
		return;
	}
	auto it = std::find(wList.begin(), wList.end(), hwnd);
	if (it == wList.end()){
		OutputDebugString("cant not find data in wList!");
		return;
	}
	

	if (++it == wList.end()) {
		it = wList.begin();
	}
	ShowWindow(*it, SW_RESTORE);
	SetForegroundWindow(*it);
}

BOOL CALLBACK winSwitch::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	if ( !IsWindowTopShow(hwnd) || GetParent(hwnd) != NULL) {
		return TRUE;
	}
	AddToWinLists(hwnd);
	return TRUE;
}

void  winSwitch::InitIfNeed()
{
	if (inited) {
		return;
	}
	if (!gWinLists.empty()) {
		OutputDebugString("some dirty data in gWinLists!");
		return;
	}
	EnumWindows(EnumWindowsProc, 0);
	inited = true;
	return;
}


void  winSwitch::AddToWinLists(HWND hWnd)
{
	auto exeName = GetExeNameFromHwnd(hWnd);
	gWinLists[exeName].push_back(hWnd);
}


void  winSwitch::RemoveFromWinLists(HWND hWnd)
{
	auto exeName = GetExeNameFromHwnd(hWnd);
	auto& wList = gWinLists[exeName];
	for (auto it=wList.begin();it!=wList.end();)
	{
		if (hWnd == *it){
			it = wList.erase(it);
		}
		else {
			++it;
		}
	}
	if (wList.empty()){
		gWinLists.erase(exeName);
		OutputDebugString("remove list!");
		OutputDebugStringW(exeName.c_str());
	}
}

wstring  winSwitch::GetExeNameFromHwnd(HWND hWnd) {
	wstring re;
	DWORD processId;
	wchar_t exePath[MAX_PATH] = { 0 };

	GetWindowThreadProcessId(hWnd, &processId);

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (hProcess) {
		if (GetModuleFileNameExW(hProcess, NULL, exePath, MAX_PATH)) {
			re = exePath;
		}
		else {
			OutputDebugString("GetModuleFileNameExW failed!");
		}
		CloseHandle(hProcess); // �رվ��
	}
	else {
		OutputDebugString("Failed to open process!"); 
	}
	return re;
};

bool winSwitch::IsWindowTopShow(HWND hWnd) {
	if (!IsWindowVisible(hWnd)) {
		return false;
	}
	

	LONG exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

	if (exStyle & WS_EX_TOOLWINDOW) {
		return false;
	}
	return true;
	/*
	if (!(exStyle & WS_EX_APPWINDOW)) {
		return false;
	}
	return true;
	*/
}
void winSwitch::WinChangeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!inited){
		return;
	}
	if ( GetParent((HWND)lParam) != NULL){
		return;
	}
	if (wParam == HSHELL_WINDOWCREATED) {
		if (IsWindowTopShow((HWND)lParam)){
			AddToWinLists((HWND)lParam);
		}
		
	}
	else if (wParam == HSHELL_WINDOWDESTROYED) {
		LONG exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		if (!(exStyle & WS_EX_TOOLWINDOW)) {
			RemoveFromWinLists((HWND)lParam);
		}
		
	}
	
}