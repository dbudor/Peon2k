#include <stdio.h>
#include <windows.h>

// Optional helper to convert code to a descriptive string.
const char* ExceptionCodeToString(DWORD code) {
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
      return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:
      return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:
      return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:
      return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:
      return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:
      return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:
      return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:
      return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:
      return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:
      return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:
      return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:
      return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:
      return "EXCEPTION_STACK_OVERFLOW";
    default:
      return "UNKNOWN_EXCEPTION";
  }
}

LONG WINAPI DumpExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo) {
  PCONTEXT ctx = pExceptionInfo->ContextRecord;  // CPU context at crash

  // Create a timestamped filename: crash_YYYYMMDD_HHMMSS.dmp
  SYSTEMTIME st;
  GetLocalTime(&st);

  char filename[256];
  sprintf(filename, "crash_%04d%02d%02d_%02d%02d%02d.dmp", st.wYear, st.wMonth,
          st.wDay, st.wHour, st.wMinute, st.wSecond);

  // Try to open the .dmp file
  FILE* fp = fopen(filename, "w");
  if (!fp) {
    // If we can't open the file, just fallback to EXCEPTION_EXECUTE_HANDLER
    return EXCEPTION_EXECUTE_HANDLER;
  }

  // 1) Dump general-purpose registers (x86):
  fprintf(fp, "\n=== Crash! Register dump (x86) ===\n");
  fprintf(fp, "EAX=0x%08lX EBX=0x%08lX ECX=0x%08lX EDX=0x%08lX\n", ctx->Eax,
          ctx->Ebx, ctx->Ecx, ctx->Edx);
  fprintf(fp, "ESI=0x%08lX EDI=0x%08lX EBP=0x%08lX ESP=0x%08lX\n", ctx->Esi,
          ctx->Edi, ctx->Ebp, ctx->Esp);
  fprintf(fp, "EIP=0x%08lX EFLAGS=0x%08lX\n", ctx->Eip, ctx->EFlags);

  EXCEPTION_RECORD* er = pExceptionInfo->ExceptionRecord;
  DWORD code = er->ExceptionCode;

  // Print the main exception code as a string:
  fprintf(fp, "\n=== Crash Type: %s (0x%08lX) ===\n", ExceptionCodeToString(code),
          code);

  // Special handling for Access Violation, if you want more detail:
  if (code == EXCEPTION_ACCESS_VIOLATION) {
    // er->ExceptionInformation[0] indicates the type of access:
    //   0 = read, 1 = write, 8 = DEP violation, etc.
    ULONG_PTR accessType = er->ExceptionInformation[0];
    ULONG_PTR faultAddr = er->ExceptionInformation[1];

    switch (accessType) {
      case 0:
        fprintf(fp, "Access Violation: tried to READ from address 0x%p\n",
                (void*)faultAddr);
        break;
      case 1:
        fprintf(fp, "Access Violation: tried to WRITE to address 0x%p\n",
                (void*)faultAddr);
        break;
      case 8:
        fprintf(fp, "Access Violation: DEP violation at address 0x%p\n",
                (void*)faultAddr);
        break;
      default:
        fprintf(fp,                "Access Violation: unknown type 0x%p, address=0x%p\n",
                (void *)accessType, (void*)faultAddr);
        break;
    }
  }

  // 2) Attempt to dump code bytes around EIP:
  //    We'll read a few bytes *before* EIP and a few bytes *after*.
  const int DUMP_BEFORE = 8;
  const int DUMP_AFTER = 16;
  unsigned char codeBuffer[DUMP_BEFORE + DUMP_AFTER] = {0};

  // The address we want to read "from"
  DWORD codeStart = ctx->Eip - DUMP_BEFORE;
  // Make sure we don’t underflow if EIP < 8, etc. (rare, but check if you want)

  memcpy(codeBuffer, (void*)codeStart, sizeof(codeBuffer));

  // Print a small hex dump
  fprintf(fp, "\n=== Code near EIP ===\n");
  // Mark where EIP lands in that dump
  for (int i = 0; i < (int)sizeof(codeBuffer); i++) {
    // If this is exactly EIP, show a marker
    if (i == DUMP_BEFORE)
      fprintf(fp, " [");
    else
      fprintf(fp, " ");

    fprintf(fp, "%02X", codeBuffer[i]);

    if (i == DUMP_BEFORE) fprintf(fp, "]");
  }
  fprintf(fp, "\n(Bracketed byte is the instruction at EIP)\n");

  fclose(fp);
  // Return handler code. Typically EXCEPTION_EXECUTE_HANDLER ends the process
  return EXCEPTION_EXECUTE_HANDLER;
}

void setup_dump() {
  // Install custom unhandled-exception filter
  SetUnhandledExceptionFilter(DumpExceptionFilter);
}