
int __declspec(dllimport) __stdcall GetCurrentProcessId(void);

int getpid(void)
{
    return GetCurrentProcessId();
}
