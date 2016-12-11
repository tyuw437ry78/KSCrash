//
//  KSCrashMonitor_User.c
//
//  Copyright (c) 2012 Karl Stenerud. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall remain in place
// in this source code.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "KSCrashMonitor_User.h"
#include "KSCrashMonitorContext.h"
#include "KSThread.h"

//#define KSLogger_LocalLevel TRACE
#include "KSLogger.h"

#include <execinfo.h>
#include <stdlib.h>


/** Context to fill with crash information. */
static KSCrash_MonitorContext* g_context;


bool kscrashmonitor_installUserExceptionHandler(KSCrash_MonitorContext* const context)
{
    KSLOG_DEBUG("Installing user exception handler.");
    g_context = context;
    return true;
}

void kscrashmonitor_uninstallUserExceptionHandler(void)
{
    KSLOG_DEBUG("Uninstalling user exception handler.");
    g_context = NULL;
}

void kscrashmonitor_reportUserException(const char* name,
                                       const char* reason,
                                       const char* language,
                                       const char* lineOfCode,
                                       const char* stackTrace,
                                       bool terminateProgram)
{
    if(g_context == NULL)
    {
        KSLOG_WARN("User-reported exception monitor is not installed. Exception has not been recorded.");
    }
    else
    {
        kscrashmonitor_beginHandlingCrash(g_context);

        KSLOG_DEBUG("Suspending all threads");
        ksmc_suspendEnvironment();

        KSLOG_DEBUG("Fetching call stack.");
        int callstackCount = 100;
        uintptr_t callstack[callstackCount];
        callstackCount = backtrace((void**)callstack, callstackCount);
        if(callstackCount <= 0)
        {
            KSLOG_ERROR("backtrace() returned call stack length of %d", callstackCount);
            callstackCount = 0;
        }

        KSLOG_DEBUG("Filling out context.");
        g_context->crashType = KSCrashMonitorTypeUserReported;
        KSMC_NEW_CONTEXT(machineContext);
        g_context->offendingMachineContext = machineContext;
        ksmc_getContextForThread(ksthread_self(), machineContext, true);
        g_context->registersAreValid = false;
        g_context->crashReason = reason;
        g_context->stackTrace = callstack;
        g_context->stackTraceLength = callstackCount;
        g_context->userException.name = name;
        g_context->userException.language = language;
        g_context->userException.lineOfCode = lineOfCode;
        g_context->userException.customStackTrace = stackTrace;

        KSLOG_DEBUG("Calling main crash handler.");
        g_context->onCrash();

        if(terminateProgram)
        {
            kscrashmonitor_uninstall(KSCrashMonitorTypeAll);
            ksmc_resumeEnvironment();
            abort();
        }
        else
        {
            kscrashmonitor_clearContext(g_context);
            ksmc_resumeEnvironment();
        }
    }
}