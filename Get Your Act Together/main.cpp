#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <time.h>
#include <sddl.h>
#include "SiteBlocking.h"

using namespace std;

char workingSet[6];
bool exiting = false;

vector<char*> processes;
vector<char*> websites;

HINSTANCE hInst;
HWND hWnd;

//Checks that the input isn't broken
bool verifyInput()
{
	if (cin.fail())
	{
		cin.clear();
		cin.ignore(INT_MAX, '\n');

		cout << "That is not a valid input. Try again!" << endl;
		return false;
	}
	else
	{
		return true;
	}
}

//Kill application that should be allowed
DWORD WINAPI appKillThread(LPVOID text)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot;

	while (true)
	{
		snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				bool matches = false;

				for (unsigned int i = 0; i < processes.size(); i++)
				{
					if (_stricmp(entry.szExeFile, processes[i]) == 0)
					{
						matches = true;
					}
				}

				if (matches)
				{
					HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

					TerminateProcess(process, 0);

					CloseHandle(process);
				}
			}
		}

		CloseHandle(snapshot);
		Sleep(200);
	}
}

//Needed to protect from shutdown kill
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	case WM_QUIT:
	case WM_QUERYENDSESSION:
		return FALSE;
		break;
	case WM_CREATE:
		ShutdownBlockReasonCreate(hWnd, L"You can't close me that easily");
		break;
	case WM_DESTROY:
		ShutdownBlockReasonDestroy(hWnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

bool ProtectProcess(void)
{
	bool isSuccess = TRUE;

	//Protect process from task manager base kills
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;

	TCHAR * szSD = TEXT("D:PS:P");

	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &(sa.lpSecurityDescriptor), NULL))
		isSuccess = FALSE;

	if (!SetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, sa.lpSecurityDescriptor))
		isSuccess = FALSE;

	//Protect from shutdown kills

	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEX cls;
	ZeroMemory(&cls, sizeof(WNDCLASSEX));

	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = CS_NOCLOSE;
	cls.lpfnWndProc = WndProc;
	cls.hInstance = hInstance;
	cls.lpszClassName = "ShutdownProtect";

	RegisterClassEx(&cls);
	if (!CreateWindowEx(0, "ShutdownProtect", "Get Your Act Together", 0, 0, 0, 0, 0, 0, 0, hInstance, 0))
		isSuccess = FALSE;

	return isSuccess;
}

//Keyhook function. Checks if keyword is typed and closes if it is
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		if (wParam == WM_KEYDOWN)
		{
			KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)lParam;

			for (int i = 0; i < 5; i++)
			{
				workingSet[i] = workingSet[i + 1];
			}

			workingSet[5] = char(key->vkCode);

			if (!_strcmpi(workingSet, "qwebnm"))
			{
				exiting = true;
			}
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

//Thread to set keyhook and loop message thread
DWORD WINAPI hookThread(LPVOID lpParam)
{
	HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);

	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL) != 0 || !exiting)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hook);

	return 0;
}

//Define blocked websites
void assignVectors(int musicOpts)
{
	if (musicOpts == 2)
	{
		processes = vector<char*>({ "steam.exe", "minecraft.exe", "java.exe", "javaw.exe", "leagueoflegends.exe", "wow.exe" });

		websites = vector<char*>({ "facebook.com", "tumblr.com", "reddit.com", "xkcd.com", "myspace.com", "magic.wizards.com",
			"gatherer.wizards.com", "explosm.net", "smbc-comics.com", "youtube.com", "onemorelevel.com", "tappedout.net",
			"deckbox.org", "mtgstocks.com", "imgur.com" });
	}
	else
	{
		processes = vector<char*>({ "steam.exe", "minecraft.exe", "java.exe", "javaw.exe", "leagueoflegends.exe", "wow.exe", "spotify.exe" });

		websites = vector<char*>({ "facebook.com", "tumblr.com", "reddit.com", "xkcd.com", "myspace.com", "magic.wizards.com",
			"gatherer.wizards.com", "explosm.net", "smbc-comics.com", "youtube.com", "onemorelevel.com", "tappedout.net",
			"deckbox.org", "mtgstocks.com", "imgur.com", "pandora.com", "spotify.com" });
	}
}

int main(int argc, char** argv)
{
	ProtectProcess();

	cout << "Do you want to block music applications and sites?" << endl << "[1] Yes" << endl << "[2] No" << endl;

	int musicOpt;

	do
	{
		cin >> musicOpt;
	} while (!verifyInput());

	assignVectors(musicOpt);

	cout << endl << "How many minutes do you not want to be productive for?" << endl;

	int minutes;

	do
	{
		cin >> minutes;
	} while (!verifyInput());

	ShowWindow(GetConsoleWindow(), SW_HIDE);

	CreateThread(NULL, 0, appKillThread, NULL, 0, NULL);

	CreateThread(NULL, 0, hookThread, NULL, 0, NULL);

	time_t start = time(NULL);

	while (difftime(time(NULL), start) < minutes * 60 && !exiting)
	{
		blockSites(websites);
		Sleep(5000);
	}

	removeFilter();

	return 0;
}
