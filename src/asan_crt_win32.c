/* substitute for libucrt.lib on Windows */

#include <Windows.h>

#include <stdio.h>

static void* get_function_addr(const char* const _FnName) {
  static HMODULE hCrt = NULL;
  if (hCrt == NULL) {
    hCrt = LoadLibraryA("ucrtbased.dll");
  }
  return GetProcAddress( hCrt, _FnName );
}

#define IMPLEMENT( _FnPtrTypedef, _FnName, ...)                         \
  do {                                                                  \
    static _FnPtrTypedef fn = NULL;                                     \
    if (fn == NULL) {                                                   \
      fn = (_FnPtrTypedef) get_function_addr(_FnName);                  \
    }                                                                   \
    return fn( __VA_ARGS__ );                                           \
  } while(0)

#pragma push_macro("_ACRTIMP")
#define _ACRTIMP



/*****************************************************************************/
/* <stdio.h>                                                                 */
/*****************************************************************************/
typedef int (*PFN__stdio_common_vsprintf)(unsigned __int64, char*, size_t, char const*,_locale_t, va_list);
_ACRTIMP int __cdecl __stdio_common_vsprintf(
  _In_                                    unsigned __int64 _Options,
  _Out_writes_opt_z_(_BufferCount)        char*            _Buffer,
  _In_                                    size_t           _BufferCount,
  _In_z_ _Printf_format_string_params_(2) char const*      _Format,
  _In_opt_                                _locale_t        _Locale,
                                          va_list          _ArgList)
{
  IMPLEMENT( PFN__stdio_common_vsprintf, "__stdio_common_vsprintf",
    _Options,
    _Buffer,
    _BufferCount,
    _Format,
    _Locale,
    _ArgList);
}

typedef int (*PFN__stdio_common_vfscanf)(unsigned __int64_, FILE*, char const*, _locale_t, va_list);
_ACRTIMP int __cdecl __stdio_common_vfscanf(
  _In_                                   unsigned __int64 _Options,
  _Inout_                                FILE*            _Stream,
  _In_z_ _Scanf_format_string_params_(2) char const*      _Format,
  _In_opt_                               _locale_t        _Locale,
                                         va_list          _Arglist)
{
  IMPLEMENT( PFN__stdio_common_vfscanf, "__stdio_common_vfscanf",
    _Options,
    _Stream,
    _Format,
    _Locale,
    _Arglist);
}

typedef char* (*PFN_fgets)(char*, int, FILE*);
_ACRTIMP char* __cdecl fgets(
  _Out_writes_z_(_MaxCount) char* _Buffer,
  _In_                      int   _MaxCount,
  _Inout_                   FILE* _Stream)
{
  IMPLEMENT( PFN_fgets, "fgets",
    _Buffer,
    _MaxCount,
    _Stream);
}

typedef int (*PFN_puts)(char const*);
_ACRTIMP int __cdecl puts(_In_z_ char const* _Buffer)
{
  IMPLEMENT( PFN_puts, "puts", _Buffer);
}

typedef FILE* (*PFN_fopen)(const char*, const char*);
_ACRTIMP FILE* __cdecl fopen(
  _In_z_ char const* _FileName,
  _In_z_ char const* _Mode)
{
  IMPLEMENT( PFN_fopen, "fopen",
    _FileName,
    _Mode);
}

typedef int (*PFN_fclose)(FILE*);
_ACRTIMP int __cdecl fclose(_Inout_ FILE* _Stream)
{
  IMPLEMENT( PFN_fclose, "fclose", _Stream );
}



/*****************************************************************************/
/* <string.h>                                                                */
/*****************************************************************************/
typedef char* (*PFN_strncpy)(char*, const char*, size_t);
_ACRTIMP char* strncpy(
  _Out_writes_(_Count)                char*        _Destination,
  _In_reads_or_z_(_Count)             char const*  _Source,
  _In_                                size_t       _Count)
{
  IMPLEMENT( PFN_strncpy, "strncpy",
    _Destination,
    _Source,
    _Count);
}



/*****************************************************************************/
/* <stdlib.h>                                                                */
/*****************************************************************************/
typedef long (*PFN_strtol)(char const*, char**, int);
_ACRTIMP long __cdecl strtol(
  _In_z_                   char const* _String,
  _Out_opt_ _Deref_post_z_ char**      _EndPtr,
  _In_                     int         _Radix)
{
  IMPLEMENT( PFN_strtol, "strtol",
    _String,
    _EndPtr,
    _Radix);
}



/*****************************************************************************/
/* <time.h>                                                                  */
/*****************************************************************************/
typedef __time64_t (*PFN__time64)(__time64_t*);
_ACRTIMP __time64_t __cdecl _time64(_Out_opt_ __time64_t* _Time )
{
  IMPLEMENT( PFN__time64, "__time64", _Time );
}

typedef struct tm* (*PFN__localtime64)(__time64_t const*);
_ACRTIMP struct tm* __cdecl _localtime64( _In_ __time64_t const* _Time)
{
  IMPLEMENT( PFN__time64, "_localtime64", _Time);
}



/*****************************************************************************/
/* <math.h>                                                                  */
/*****************************************************************************/
typedef double (*PFN_sqrt)(double);
_Check_return_ _CRT_JIT_INTRINSIC double __cdecl sqrt(_In_ double _X)
{
  IMPLEMENT( PFN_sqrt, "sqrt", _X );
}

typedef double (*PFN_log)(double);
_Check_return_ _CRT_JIT_INTRINSIC double __cdecl log(_In_ double _X)
{
  IMPLEMENT( PFN_log, "log", _X );
}

typedef double (*PFN_floor)(double);
_Check_return_ _CRT_JIT_INTRINSIC double __cdecl floor(_In_ double _X)
{
  IMPLEMENT( PFN_floor, "floor", _X );
}

typedef double (*PFN_ceil)(double);
_Check_return_ _CRT_JIT_INTRINSIC double __cdecl ceil(_In_ double _X)
{
  IMPLEMENT( PFN_ceil, "ceil", _X );
}

typedef double (*PFN_round)(double);
_Check_return_ _CRT_JIT_INTRINSIC double __cdecl round(_In_ double _X)
{
  IMPLEMENT( PFN_round, "round", _X );
}

