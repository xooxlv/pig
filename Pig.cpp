#include <Windows.h>
#include <stdio.h>

#pragma warning(disable: 4996)

#define ALL_GOOD				0
#define FATAL					GetLastError()
#define ERR						-1
#define WINAPI_ERR				ERROR
#define MAX_BUFF_SIZE			4096
#define MIN_BUFF_SIZE			1
#define COMMAND_SIZE			MAX_BUFF_SIZE  * 4

#define ID_MAINWINDOW			10
#define ID_BUTTON_INJECT		20
#define ID_BUTTON_BROWSE_XCODE	21
#define ID_BUTTON_BROWSE_OUT	22
#define ID_COMBOBOX				30
#define ID_LABEL_PID			40
#define ID_LABEL_XCODE_PATH		50
#define ID_LABEL_TYPE			51
#define ID_TEXTBOX_PID			60
#define	ID_LABEL_OUT_PATH		61
#define ID_TEXTBOX_XCODE_PATH	70

#define TEXT_REFL_DLL_INJECTION	L"Reflective Dll Injection"
#define TEXT_DLL_INJECTION		L"Dll Injection"
#define TEXT_CODE_INJECTION		L"Code Injection"

#define MAX_PID_TEXTBOX_SIZE	5
#define INJECTION_TIMEOUT		3000

typedef struct _COMBOBOX_ITEMS {
	LPCWSTR* items;
	UINT count;
} COMBOBOX_ITEMS, * PCOMBOBOX_ITEMS;

typedef struct _INJECTION_DATA {
	WCHAR pid[MAX_BUFF_SIZE];
	WCHAR path_xcode_file[MAX_BUFF_SIZE];
	WCHAR path_out_file[MAX_BUFF_SIZE];
	WCHAR injection_type[MAX_BUFF_SIZE];
}INJECTION_DATA, * PINJECTION_DATA;

HWND hMainWindow, hButtonInject, hButtonBrowseXCodeFile, hComboBox, hTextBoxOutFile, hLabelOutFile;
HWND hLabelPid, hLabelXCodeFile, hLabelType, hTextBoxPid, hTextBoxXCodeFile;
HINSTANCE hInstance;
COMBOBOX_ITEMS ComboBoxItems;
WCHAR ProgramDirectory[BUFSIZ];

LRESULT __stdcall WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

HWND CreateMainWindow(UINT Width, UINT Height, UINT Id, LPCWSTR Title);
HWND CreateButton(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id, LPCWSTR Text);
HWND CreateComboBox(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id, COMBOBOX_ITEMS Items);
HWND CreateLabel(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id, LPCWSTR Text);
HWND CreateTextBox(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id);

BOOL InitInstance(HINSTANCE hInst);
BOOL ComboBoxAddItem(PCOMBOBOX_ITEMS cbox_items, LPCWSTR item);

// выбор файла из ФС с помощью диалогового окна, результат в параметре по ссылке
BOOL ChooseFile(LPWSTR File);

// собирает данные с форм ввода в параметр from_user_data
BOOL GetInjectionParams(PINJECTION_DATA from_user_data);

// вызов соответствующего модуля на основе полученных от пользователя параметров
BOOL Inject(PINJECTION_DATA);

// открыть файл с отчетом инъекции
VOID ShowResult(LPWSTR);

// сбор и передача сообщений в оконную функцию
VOID MainLoop();


int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR cmd, int iShow)
{
	if (InitInstance(hInstance) == ERR)
		return FATAL;

	GetCurrentDirectory(MAX_PATH, ProgramDirectory);

	DWORD PosX = 152;
	hMainWindow = CreateMainWindow(630, 300, ID_MAINWINDOW, L"Pig");

	hButtonInject = CreateButton(hMainWindow, 100, 30, 250, 200, ID_BUTTON_INJECT, L"Внедрить");

	hLabelPid = CreateLabel(hMainWindow, 120, 30, 40, 20, ID_LABEL_PID, L"Идентификатор процесса: ");
	hTextBoxPid = CreateTextBox(hMainWindow, 100, 30, PosX, 20, ID_TEXTBOX_PID);
	SendMessage(hTextBoxPid, EM_SETLIMITTEXT, MAX_PID_TEXTBOX_SIZE, NULL);

	hLabelXCodeFile = CreateLabel(hMainWindow, 120, 30, 40, 60, ID_LABEL_XCODE_PATH, L"Полезная нагрузка: ");
	hTextBoxXCodeFile = CreateTextBox(hMainWindow, 400, 30, PosX, 60, ID_TEXTBOX_PID);
	hButtonBrowseXCodeFile = CreateButton(hMainWindow, 30, 30, 560, 60, ID_BUTTON_BROWSE_XCODE, L"...");

	hLabelOutFile = CreateLabel(hMainWindow, 120, 30, 40, 100, ID_LABEL_OUT_PATH, L"Результат внедрения: ");
	hTextBoxOutFile = CreateTextBox(hMainWindow, 400, 30, PosX, 100, ID_TEXTBOX_PID);
	hButtonBrowseXCodeFile = CreateButton(hMainWindow, 30, 30, 560, 100, ID_BUTTON_BROWSE_OUT, L"...");

	SendMessage(hTextBoxOutFile, EM_SETLIMITTEXT, MAX_PATH, NULL);
	SendMessage(hLabelOutFile, EM_SETLIMITTEXT, MAX_PATH, NULL);


	hLabelXCodeFile = CreateLabel(hMainWindow, 120, 30, 40, 140, ID_LABEL_TYPE, L"Техника внедрения: ");
	ComboBoxAddItem(&ComboBoxItems, TEXT_DLL_INJECTION);
	ComboBoxAddItem(&ComboBoxItems, TEXT_REFL_DLL_INJECTION);
	ComboBoxAddItem(&ComboBoxItems, TEXT_CODE_INJECTION);
	hComboBox = CreateComboBox(hMainWindow, 200, 400, PosX, 144, ID_COMBOBOX, ComboBoxItems);

	MainLoop();

	return ALL_GOOD;
}

LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//	printf_s("DBG:\thwnd: %d\tmsg: %d\twparam: %d\tlparam: %d\n",hwnd, msg, wparam, lparam);
	switch (msg)
	{
	case(WM_COMMAND):
		if (wparam == ID_BUTTON_BROWSE_XCODE)
		{
			WCHAR file_path[MAX_PATH] = {0};
			if (ChooseFile(file_path) == ALL_GOOD)
				SetWindowText(hTextBoxXCodeFile, file_path);
			else MessageBox(hMainWindow, L"Не выбран файл для внедрения", L"Предупреждение", MB_ICONWARNING);

		}
			
		if (wparam == ID_BUTTON_BROWSE_OUT)
		{
			WCHAR file_path[MAX_PATH] = { 0 };
			if (ChooseFile(file_path) == ALL_GOOD)
				SetWindowText(hTextBoxOutFile, file_path);
			else MessageBox(hMainWindow, L"Не выбран файл для предоставления отчета", L"Предупреждение", MB_ICONWARNING);
		}
			
			
		if (wparam == ID_BUTTON_INJECT) {
			
			INJECTION_DATA injection_params;
			memset(&injection_params, NULL, sizeof(injection_params));

			if (GetInjectionParams(&injection_params) == ALL_GOOD)
			{
				Inject(&injection_params);
				Sleep(INJECTION_TIMEOUT);
				ShowResult(injection_params.path_out_file);
			}
			else MessageBox(NULL, L"Получены неправильные параметры", L"Ошибка", MB_ICONERROR);

		}
	case(WM_SYSCOMMAND):
		if (wparam == SC_CLOSE)
			exit(ALL_GOOD);
	default:
		break;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

HWND CreateMainWindow(UINT Width, UINT Height, UINT Id, LPCWSTR Title)
{
	auto ret = CreateWindowEx(NULL,
		L"WINDOW", Title,
		WS_MAXIMIZEBOX & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX) | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT,
		Width, Height,
		HWND_DESKTOP,
		NULL,
		hInstance,
		NULL);
	ShowWindow(ret, SW_SHOWNORMAL);
	return ret;
}

HWND CreateButton(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id, LPCWSTR Text)
{
	auto res = CreateWindowEx(NULL,
		L"BUTTON", Text,
		WS_CHILD | WS_VISIBLE,
		x, y, Width, Height,
		Parent,
		(HMENU)Id,
		NULL, NULL);
	ShowWindow(res, SW_SHOWNORMAL);
	return res;
}

HWND CreateComboBox(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id, COMBOBOX_ITEMS Items)
{
	auto ret = CreateWindowEx(0, L"COMBOBOX",
		NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL | WS_VSCROLL,
		x, y, Width, Height, Parent, (HMENU)Id, NULL, NULL);

	for (UINT i = 0; i < Items.count; i++)
	{
		SendMessage(ret, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)Items.items[i]);
		SendMessage(ret, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);
	}

	ShowWindow(ret, SW_SHOWNORMAL);
	return ret;
}

HWND CreateLabel(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id, LPCWSTR Text)
{
	auto ret = CreateWindowEx(
		0, L"STATIC", Text, WS_CHILD,
		x, y, Width, Height, Parent,
		(HMENU)Id, NULL, NULL);
	ShowWindow(ret, SW_SHOWNORMAL);

	return ret;
}

HWND CreateTextBox(HWND Parent, UINT Width, UINT Height, UINT x, UINT y, UINT Id)
{
	auto ret = CreateWindowEx(0, L"edit",
		NULL, WS_BORDER | ES_MULTILINE | WS_CHILD | WS_VISIBLE,
		x, y, Width, Height, Parent, (HMENU)Id, NULL, NULL);

	ShowWindow(ret, SW_SHOWNORMAL);
	return ret;
}

BOOL InitInstance(HINSTANCE hInst)
{
	WNDCLASSEX wcex = { 0 };
	memset(&wcex, 0, sizeof(WNDCLASS));

	hInst = GetModuleHandle(NULL);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
	wcex.hbrBackground = (HBRUSH)(31);
	wcex.lpszClassName = TEXT("WINDOW");
	if (!RegisterClassEx(&wcex))
		return ERR;
	return ALL_GOOD;
}

BOOL ComboBoxAddItem(PCOMBOBOX_ITEMS cbi, LPCWSTR item)
{
	auto len = lstrlen(item) * sizeof(WCHAR) + 2;						//bytes
	cbi->items = (LPCWSTR*)realloc((void*)cbi->items, cbi->count * sizeof(LPCWSTR) + sizeof(LPCWSTR));
	cbi->items[cbi->count] = (LPCWSTR)malloc(len);
	memcpy((void*)cbi->items[cbi->count], item, len);
	++(cbi->count);
	//если добавленная в список строка соответствует переданной то ок иначе не ок
	return ALL_GOOD;
}

BOOL ChooseFile(LPWSTR File)
{
	//wprintf(L"[dbg]\t|\tpath: %s\n", File);
	wchar_t CurrentDirectory[MAX_PATH];
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));

	GetCurrentDirectory(MAX_PATH, CurrentDirectory);

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hMainWindow;
	ofn.lpstrFile = File;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrInitialDir = CurrentDirectory;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn)==TRUE)
		return ALL_GOOD;

	return ERR;
}

BOOL Inject(PINJECTION_DATA injdata)
{
	//wprintf(L"RECV: %s\t%s\t%s\t%s\n", injdata->injection_type, injdata->path_out_file, injdata->path_xcode_file, injdata->pid);
	WCHAR command[COMMAND_SIZE] = {0};
	LPCSTR module_name;

	if (lstrcmp(injdata->injection_type, TEXT_CODE_INJECTION) == 0)
		lstrcat(command, L"module_code_injector.exe --pid ");
	else if (lstrcmp(injdata->injection_type, TEXT_DLL_INJECTION) == 0)
		lstrcat(command, L"module_dll_injector.exe --pid ");
	else if (lstrcmp(injdata->injection_type, TEXT_REFL_DLL_INJECTION) == 0)
		lstrcat(command, L"module_reflective_dll_injector.exe --pid ");
	else return ERR;

	lstrcat(command, injdata->pid);

	if (lstrcmp(injdata->injection_type, TEXT_CODE_INJECTION) == 0)
		lstrcat(command, L" --payload ");
	else lstrcat(command, L" --dll ");
	lstrcat(command, injdata->path_xcode_file);


	if (lstrlen(injdata->path_out_file) > 0)
	{
		lstrcat(command, L" --output ");
		lstrcat(command, injdata->path_out_file);
	}

	// проверьть файл на диске

	//wprintf(L"CMD: %s\n", command);
	SetCurrentDirectory(ProgramDirectory);
	_wsystem(command);

	return ALL_GOOD;
}

VOID ShowResult(LPWSTR file_path)
{
	_wsystem(file_path);
}

BOOL GetInjectionParams(PINJECTION_DATA inj_data)
{
	LPWSTR XCodePath = (LPWSTR)malloc(MAX_BUFF_SIZE);
	LPWSTR PID = (LPWSTR)malloc(MAX_BUFF_SIZE);
	LPWSTR InjectionType = (LPWSTR)malloc(MAX_BUFF_SIZE);
	LPWSTR OutputFilePath = (LPWSTR)malloc(MAX_BUFF_SIZE);

	if (XCodePath == NULL || PID == NULL || InjectionType == NULL || OutputFilePath == NULL)
		return ERR;

	GetWindowText(hTextBoxXCodeFile, XCodePath, MAX_BUFF_SIZE);
	GetWindowText(hTextBoxPid, PID, MAX_BUFF_SIZE);
	GetWindowText(hComboBox, InjectionType, MAX_BUFF_SIZE);
	GetWindowText(hTextBoxOutFile, OutputFilePath, MAX_BUFF_SIZE);

	lstrcpyn(inj_data->pid, PID, MAX_BUFF_SIZE);
	lstrcpyn(inj_data->path_out_file, OutputFilePath, MAX_BUFF_SIZE);
	lstrcpyn(inj_data->path_xcode_file, XCodePath, MAX_BUFF_SIZE);
	lstrcpyn(inj_data->injection_type, InjectionType, MAX_BUFF_SIZE);

	free(OutputFilePath);
	free(InjectionType);
	free(PID);
	free(XCodePath);

	if (lstrlen(inj_data->pid) < MIN_BUFF_SIZE)
		return ERR;
	if (lstrlen(inj_data->path_xcode_file) < MIN_BUFF_SIZE)
		return ERR;
	if (lstrlen(inj_data->injection_type) < MIN_BUFF_SIZE)
		return ERR;

	return ALL_GOOD;
}

VOID MainLoop()
{
	MSG msg;
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))	
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				break;
		}
	}
}

