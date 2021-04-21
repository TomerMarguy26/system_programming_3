#pragma once
DWORD WINAPI check_resident(LPVOID lpParam);
static HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine,
	LPVOID p_thread_parameters,
	LPDWORD p_thread_id);
