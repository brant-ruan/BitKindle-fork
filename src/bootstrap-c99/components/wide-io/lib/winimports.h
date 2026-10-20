#ifndef C1_OBJECT_WINIMPORTS_H
#define C1_OBJECT_WINIMPORTS_H
/* Addresses are cast to the precise existing API typedef at each use. */
extern void __c1_win_GetStdHandle(void);
extern void __c1_win_ReadFile(void);
extern void __c1_win_WriteFile(void);
extern void __c1_win_ExitProcess(void);
extern void __c1_win_CreateFileA(void);
extern void __c1_win_CloseHandle(void);
extern void __c1_win_GetEnvironmentVariableA(void);
extern void __c1_win_GetLocalTime(void);
extern void __c1_win_GetProcessHeap(void);
extern void __c1_win_HeapAlloc(void);
extern void __c1_win_HeapReAlloc(void);
extern void __c1_win_HeapFree(void);
extern void __c1_win_SetFilePointerEx(void);
extern void __c1_win_GetFileSizeEx(void);
extern void __c1_win_DeleteFileA(void);
extern void __c1_win_MoveFileExA(void);
extern void __c1_win_FlushFileBuffers(void);
extern void __c1_win_GetFileType(void);
extern void __c1_win_GetLastError(void);
extern void __c1_win_GetCommandLineA(void);
extern void __c1_win_GetSystemTimeAsFileTime(void);
extern void __c1_win_GetTimeZoneInformation(void);
extern void __c1_win_GetTickCount64(void);
extern void __c1_win_GetCurrentProcessId(void);
extern void __c1_win_GetTempPathA(void);
extern void __c1_win_GetTempFileNameA(void);
extern void __c1_win_GetConsoleMode(void);
extern void __c1_win_IsDBCSLeadByteEx(void);
extern void __c1_win_GetProcessTimes(void);
extern void __c1_win_GetCurrentProcess(void);
extern void __c1_win_GetDynamicTimeZoneInformation(void);
extern void __c1_win_GetTimeZoneInformationForYear(void);
extern void __c1_win_RemoveDirectoryA(void);
extern void __c1_win_GetFileAttributesA(void);
extern void __c1_win_GetSystemDirectoryA(void);
extern void __c1_win_CreateProcessA(void);
extern void __c1_win_WaitForSingleObject(void);
extern void __c1_win_GetExitCodeProcess(void);
#endif
