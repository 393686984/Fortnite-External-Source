#pragma once
#include <TlHelp32.h>

class _driver
{
private:
	typedef __int64(*NtUserFunction)(uintptr_t);
	NtUserFunction nt_user_function = 0;
	enum REQUEST_TYPE : INT
	{
		BASE,
		WRITE,
		READ
	};
	typedef struct _DRIVER_REQUEST
	{
		REQUEST_TYPE type;
		HANDLE pid;
		PVOID address;
		PVOID buffer;
		SIZE_T size;
		PVOID* base;
	} DRIVER_REQUEST, *PDRIVER_REQUEST;
	void send_request(PDRIVER_REQUEST out)
	{
		RtlSecureZeroMemory(out, 0);
		nt_user_function(reinterpret_cast<uintptr_t>(out));
	}
public:
	int process_id;
	uintptr_t base_address;
	DWORD get_process_id(LPCTSTR process_name)
	{
		PROCESSENTRY32 pt{};
		HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		pt.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hsnap, &pt))
		{
			do {
				if (!lstrcmpi(pt.szExeFile, process_name))
				{
					CloseHandle(hsnap);
					return pt.th32ProcessID;
				}
			} while (Process32Next(hsnap, &pt));
		}
		CloseHandle(hsnap);
		return 0;
	}
	bool setup()
	{
		const HMODULE user32 = LoadLibraryA("user32.dll");
		if (!user32) return false;
		nt_user_function = reinterpret_cast<NtUserFunction>(GetProcAddress(user32, "NtUserRegisterErrorReportingDialog"));
		if (!nt_user_function)
		{
			const HMODULE win32u = LoadLibraryA("win32u.dll");
			if (!win32u) return false;
			nt_user_function = reinterpret_cast<NtUserFunction>(GetProcAddress(win32u, "NtUserRegisterErrorReportingDialog"));
			if (!nt_user_function) return false;
		}
		return true;
	}
	uintptr_t get_base_address()
	{
		uintptr_t image_base = 0;
		DRIVER_REQUEST out{};
		out.type = BASE;
		out.pid = (HANDLE)process_id;
		out.base = (PVOID*)&image_base;
		send_request(&out);
		return image_base;
	}
	void writem(PVOID address, PVOID buffer, DWORD size)
	{
		DRIVER_REQUEST out{};
		out.type = WRITE;
		out.pid = (HANDLE)process_id;
		out.address = address;
		out.buffer = buffer;
		out.size = size;
		send_request(&out);
	}
	void readm(PVOID address, PVOID buffer, DWORD size)
	{
		DRIVER_REQUEST out{};
		out.type = READ;
		out.pid = (HANDLE)process_id;
		out.address = address;
		out.buffer = buffer;
		out.size = size;
		send_request(&out);
	}
	template<typename T> void write(uintptr_t address, T value)
	{
		writem((PVOID)address, &value, sizeof(T));
	}
	template<typename T> T read(uintptr_t address)
	{
		T buffer{};
		readm((PVOID)address, &buffer, sizeof(T));
		return buffer;
	}
};
_driver driver;