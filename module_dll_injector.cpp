#include <Windows.h>
#include <stdio.h>
#include <locale.h>
#pragma warning (disable: 4996)

#define PID "--pid"
#define DLL "--dll"
#define OUTPUT "--output"
#define HELP "--help"

#define ARGC_MIN 5
#define ARGC_MAX 7

typedef struct injection_data
{
	char dll_file[BUFSIZ];
	char output_file[BUFSIZ];
	DWORD pid;
} INJECTION_DATA, * PINJECTION_DATA;

FILE* file; //поток вывода (консоль или файл)

void help_print(char* first_argv)
{
	fprintf(file, "   Usage: %s <options> <param> [, <param>, ...]\n\n", first_argv);
	fprintf(file, "   Options:\n");
	fprintf(file, "\t%s\t\t\t\t: Process identifator\n", PID);
	fprintf(file, "\t%s\t\t\t\t: File with the injection code\n", DLL);
	fprintf(file, "\t%s\t\t\t\t: File with result of work\n", OUTPUT);
	fprintf(file, "\t%s\t\t\t\t: Show this help\n", HELP);
	fprintf(file, "\n   Example:\n");
	fprintf(file, "\t%s --pid 12345 --dll C:\\uf.dll --output result.txt\n", first_argv);
}

INJECTION_DATA get_injection_daata(int argc, char* argv[])
{
	INJECTION_DATA to_return;
	memset(&to_return, NULL, sizeof(to_return));

	for (size_t i = 0; i < argc - 1; i++)
	{
		if (strcmp(PID, argv[i]) == 0)
			to_return.pid = atoi(argv[i + 1]);

		if (strcmp(DLL, argv[i]) == 0)
			strcpy(to_return.dll_file, argv[i + 1]);

		if (strcmp(OUTPUT, argv[i]) == 0)
			strcpy(to_return.output_file, argv[i + 1]);
	}
	return to_return;
}

void error_exit(const char* dbg_msg)
{
	fprintf(file, "Ошибка: %s\t| Причина: %d\n", dbg_msg, GetLastError());
	exit(GetLastError());
}


int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "rus");
	file = stdout;
	if (argc < ARGC_MIN || argc > ARGC_MAX)
	{
		help_print(argv[0]);
		exit(ERROR);
	}
	INJECTION_DATA  data = get_injection_daata(argc, argv);
	if (strlen(data.output_file) > 0)
		file = fopen(data.output_file, "w");

	fprintf(file, "[ Старт ] Получены параметры для внедрения:\n");
	fprintf(file, "		| ID процесса\t: %d\n", data.pid);
	fprintf(file, "		| DLL \t: %s\n", data.dll_file);
	fprintf(file, "		| Файл для отчета\t: %s\n\n", data.output_file);

	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES priv = { 0 };
	fprintf(file, "[ 0 ] Получение привилегий отладчика:\n");

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
			if (AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL))
				fprintf(file, "		| получены привилегии отладчика\n");
			else fprintf(file, "	| привилегии отладчкика не получены\n");
		CloseHandle(hToken);
	}

	int LenDllPath = 0;
	int LenOutfilePath = 0;
	if (data.dll_file != NULL)
		LenDllPath = strlen(data.dll_file) + 1;
	if (data.output_file != NULL)
		LenOutfilePath = strlen(data.output_file) + 1;
	HANDLE hProc, hThread;
	FARPROC WINAPI pLoadLib;


	fprintf(file, "[ 1 ] Попытка открыть процесс %d с PROCESS_ALL_ACCESS правами доступа\n", data.pid);
	hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, data.pid);
	if (!hProc)
		error_exit("не удалось открыть процесс");
	fprintf(file, "		| PID\t\t: %d\n", data.pid);
	fprintf(file, "		| Доступ\t: %d\n", PROCESS_ALL_ACCESS);
	fprintf(file, "		| Статус\t: OK\n\n");


	fprintf(file, "[ 2 ] Получение адреса функции LoadLibraryA:\n");
	pLoadLib = GetProcAddress(GetModuleHandleA("kernel32"), "LoadLibraryA");
	if (!pLoadLib)
		error_exit("не удалось получить адрес функции");
	fprintf(file, "		| Адрес\t\t: 0x%p\n", pLoadLib);
	fprintf(file, "		| Статус\t: OK\t\n\n");


	fprintf(file, "[ 3 ] Выделение памяти под параметр для функции LoadLibraryA в процессе %d:\n", data.pid);
	LPVOID pAllocated = VirtualAllocEx(hProc, NULL, LenDllPath, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pAllocated)
		error_exit("не удалось выделить память");
	MEMORY_BASIC_INFORMATION mbi;
	memset(&mbi, NULL, sizeof(mbi));
	VirtualQueryEx(hProc, pAllocated, &mbi, sizeof(mbi));
	fprintf(file, "		| Размер\t\t: %d\n", mbi.RegionSize);
	fprintf(file, "		| Бзовый адрес\t\t: 0x%p\n", pAllocated);
	fprintf(file, "		| Защита\t: %d\n", mbi.Protect);
	fprintf(file, "		| Статус\t: OK\n\n");


	fprintf(file, "[ 4 ] Запись параметра  \"%s\" по адресу 0x%p:\n", data.dll_file, pAllocated);
	SIZE_T Written = NULL;
	WriteProcessMemory(hProc,
		pAllocated,
		data.dll_file,
		LenDllPath,
		&Written);
	if (Written <= 0)
		error_exit("не удалось произвести запись");
	fprintf(file, "		| Записано\t: %d\n", Written);
	fprintf(file, "		| Параметр\t\t: %s\n", data.dll_file);
	fprintf(file, "		| Статус\t: OK\n\n");



	fprintf(file, "[ 5 ] Создание потока в удаленном процессе и выполнение внедрнного кода:\n");
	hThread = CreateRemoteThread(hProc,
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)pLoadLib,
		(LPVOID)pAllocated,
		NULL,
		NULL);
	if (!hThread)
		error_exit("не удалось запустить код");
	fprintf(file, "		| Внедрен\t: %s\n", data.dll_file);
	fprintf(file, "		| Статус\t: OK\n");


	CloseHandle(hProc);
	CloseHandle(pLoadLib);
	Sleep(2000);
	return 0;
}
