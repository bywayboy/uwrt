#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ul_reason_for_call,LPVOID lpReserved) 
{ 
	switch( ul_reason_for_call ) 
	{ 
	case DLL_PROCESS_ATTACH: 
		//....... 
		break;
	case DLL_THREAD_ATTACH: 
		//....... 
		break;
	case DLL_THREAD_DETACH: 
		//....... 
		break;
	case DLL_PROCESS_DETACH: 
		//....... 
		break;
	}
	return TRUE; 
} 
/*
int _v_asprintf(char **strp, const char *fmt, va_list ap)
{
	FILE *dev_null;
	int arg_len;
	
	dev_null = fopen("nul", "w");
	arg_len = vfprintf(dev_null, fmt, ap);
	if(arg_len != -1) {
		*strp = (char *)malloc((size_t)arg_len + 1);
		arg_len = vsprintf(*strp, fmt, ap);
	} else *strp = NULL;
	fclose(dev_null);
	return arg_len;
}

int _asprintf(char **strp, const char *fmt, ...)
{
	int result;
    
	va_list args;
	va_start(args, fmt);
	result = _v_asprintf(strp, fmt, args);
	va_end(args);
	return result;
}


char *_bprintf(const char *fmt, ...)
{
	char *strp = NULL;
	
	va_list args;
	va_start(args, fmt);
	_v_asprintf(&strp, fmt, args);
	va_end(args);
	
	return strp;
}
*/