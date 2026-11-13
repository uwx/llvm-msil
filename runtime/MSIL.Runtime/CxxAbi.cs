using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

namespace MSIL.Runtime;

public static unsafe class CxxAbi
{
    [ThreadStatic]
    private static ExceptionHeader* caughtExceptions;

    [ThreadStatic]
    private static int uncaughtExceptions;

    [ThreadStatic]
    private static void* emptyGlobals;

    [ThreadStatic]
    private static List<ExitHandler>? threadExitHandlers;

    private static readonly object exitLock = new();
    private static readonly List<ExitHandler> exitHandlers = new();
    private static readonly List<ExitHandler> quickExitHandlers = new();
    private static readonly ConcurrentDictionary<uint, nint> keyDestructors = new();

    private readonly record struct ExitHandler(nint Function, nint Argument, bool TakesArgument);

    static CxxAbi()
    {
        AppDomain.CurrentDomain.ProcessExit += (_, _) => RunExitHandlers();
    }

    public static void* __cxa_allocate_exception(long thrownSize)
    {
        var totalSize = (nuint)(sizeof(ExceptionHeader) + thrownSize);
        var header = (ExceptionHeader*)NativeMemory.AllocZeroed(totalSize);
        if (header == null)
            _ZSt9terminatev();
        return ExceptionHeader.ObjectOf(header);
    }

    public static void __cxa_free_exception(void* thrownObject)
    {
        NativeMemory.Free(ExceptionHeader.FromObject(thrownObject));
    }

    public static void* __cxa_allocate_dependent_exception()
    {
        var header = (ExceptionHeader*)NativeMemory.AllocZeroed((nuint)sizeof(ExceptionHeader));
        header->IsDependent = 1;
        return header;
    }

    public static void __cxa_free_dependent_exception(void* dependent)
    {
        NativeMemory.Free(dependent);
    }

    public static void* __cxa_init_primary_exception(void* thrownObject, void* thrownType, void* destructor)
    {
        var header = ExceptionHeader.FromObject(thrownObject);
        header->ReferenceCount = 0;
        header->ThrownType = (TypeInfo*)thrownType;
        header->Destructor = destructor;
        return header;
    }

    public static void __cxa_throw(void* thrownObject, void* thrownType, void* destructor)
    {
        var header = (ExceptionHeader*)__cxa_init_primary_exception(thrownObject, thrownType, destructor);
        header->ReferenceCount = 1;
        uncaughtExceptions++;
        throw new CxxException(header);
    }

    public static void* __cxa_begin_catch(void* landed)
    {
        var header = (ExceptionHeader*)landed;
        var count = header->HandlerCount;
        header->HandlerCount = count < 0 ? -count + 1 : count + 1;
        if (header != caughtExceptions)
        {
            header->NextCaught = caughtExceptions;
            caughtExceptions = header;
        }
        uncaughtExceptions--;
        return header->AdjustedPointer;
    }

    public static void __cxa_end_catch()
    {
        var header = caughtExceptions;
        if (header == null)
            return;

        // rethrown: the handler is unwinding, the exception lives on
        if (header->HandlerCount < 0)
        {
            header->HandlerCount++;
            if (header->HandlerCount == 0)
                caughtExceptions = header->NextCaught;
            return;
        }

        header->HandlerCount--;
        if (header->HandlerCount != 0)
            return;
        caughtExceptions = header->NextCaught;
        ReleaseHandledException(header);
    }

    public static void __cxa_rethrow()
    {
        var header = caughtExceptions;
        if (header == null)
            _ZSt9terminatev();
        header->HandlerCount = -header->HandlerCount;
        uncaughtExceptions++;
        throw new CxxException(header);
    }

    public static void* __cxa_get_exception_ptr(void* landed)
    {
        var header = (ExceptionHeader*)landed;
        return header->AdjustedPointer;
    }

    public static void* __cxa_current_exception_type()
    {
        var header = caughtExceptions;
        if (header == null)
            return null;
        return header->ThrownType;
    }

    public static void __cxa_increment_exception_refcount(void* thrownObject)
    {
        if (thrownObject == null)
            return;
        var header = ExceptionHeader.FromObject(thrownObject);
        Interlocked.Increment(ref header->ReferenceCount);
    }

    public static void __cxa_decrement_exception_refcount(void* thrownObject)
    {
        if (thrownObject == null)
            return;
        var header = ExceptionHeader.FromObject(thrownObject);
        if (Interlocked.Decrement(ref header->ReferenceCount) == 0)
            DestroyException(header);
    }

    public static void* __cxa_current_primary_exception()
    {
        var header = caughtExceptions;
        if (header == null)
            return null;
        var thrownObject = ExceptionHeader.ThrownObjectOf(header);
        __cxa_increment_exception_refcount(thrownObject);
        return thrownObject;
    }

    public static void __cxa_rethrow_primary_exception(void* thrownObject)
    {
        if (thrownObject == null)
            return;
        var dependent = (ExceptionHeader*)__cxa_allocate_dependent_exception();
        dependent->PrimaryObject = thrownObject;
        dependent->ThrownType = ExceptionHeader.FromObject(thrownObject)->ThrownType;
        __cxa_increment_exception_refcount(thrownObject);
        uncaughtExceptions++;
        throw new CxxException(dependent);
    }

    public static bool __cxa_uncaught_exception()
    {
        return uncaughtExceptions != 0;
    }

    public static int __cxa_uncaught_exceptions()
    {
        return uncaughtExceptions;
    }

    // libc++abi's terminate handler reads these; an empty caught list keeps it
    // from interpreting our headers as its own
    public static void* __cxa_get_globals()
    {
        if (emptyGlobals == null)
            emptyGlobals = NativeMemory.AllocZeroed(16);
        return emptyGlobals;
    }

    public static void* __cxa_get_globals_fast()
    {
        return __cxa_get_globals();
    }

    public static long _ZN10__cxxabiv119__getExceptionClassEPK17_Unwind_Exception(void* unwindException)
    {
        return 0;
    }

    public static bool _ZN10__cxxabiv121__isOurExceptionClassEPK17_Unwind_Exception(void* unwindException)
    {
        return false;
    }

    public static int __cxa_atexit(void* function, void* argument, void* dsoHandle)
    {
        lock (exitLock)
            exitHandlers.Add(new ExitHandler((nint)function, (nint)argument, true));
        return 0;
    }

    public static int atexit(void* function)
    {
        lock (exitLock)
            exitHandlers.Add(new ExitHandler((nint)function, 0, false));
        return 0;
    }

    public static int at_quick_exit(void* function)
    {
        lock (exitLock)
            quickExitHandlers.Add(new ExitHandler((nint)function, 0, false));
        return 0;
    }

    // runs at_quick_exit handlers only, then leaves without static destructors
    public static void quick_exit(int status)
    {
        RunHandlers(quickExitHandlers);
        var exit = (delegate* unmanaged<int, void>)RuntimeSymbols.NativeFunction("_exit");
        exit(status);
    }

    // only a thread started through pthread_create below has an exit we see
    public static int __cxa_thread_atexit_impl(void* function, void* argument, void* dsoHandle)
    {
        if (threadExitHandlers == null)
            return __cxa_atexit(function, argument, dsoHandle);
        threadExitHandlers.Add(new ExitHandler((nint)function, (nint)argument, true));
        return 0;
    }

    private struct ThreadStart
    {
        public delegate*<void*, void*> Routine;
        public void* Argument;
    }

    // the start routine is a managed method, and glibc calling it straight
    // from a thread the runtime never saw breaks as soon as a GC wants that
    // thread suspended
    public static int pthread_create(void* thread, void* attributes, void* routine, void* argument)
    {
        var start = (ThreadStart*)NativeMemory.Alloc((nuint)sizeof(ThreadStart));
        start->Routine = (delegate*<void*, void*>)routine;
        start->Argument = argument;
        var create = (delegate* unmanaged<void*, void*, delegate* unmanaged<void*, void*>, void*, int>)
            RuntimeSymbols.NativeFunction("pthread_create");
        var result = create(thread, attributes, &RunThread, start);
        if (result != 0)
            NativeMemory.Free(start);
        return result;
    }

    [UnmanagedCallersOnly]
    private static void* RunThread(void* state)
    {
        var start = (ThreadStart*)state;
        var routine = start->Routine;
        var argument = start->Argument;
        NativeMemory.Free(start);
        threadExitHandlers = new List<ExitHandler>();
        var result = routine(argument);
        RunHandlers(threadExitHandlers);
        RunKeyDestructors();
        return result;
    }

    // glibc would run these from its own thread teardown, after the runtime
    // has already let go of the thread, so the native key gets no destructor
    public static int pthread_key_create(void* key, void* destructor)
    {
        var create = (delegate* unmanaged<void*, void*, int>)RuntimeSymbols.NativeFunction("pthread_key_create");
        var result = create(key, null);
        if (result == 0 && destructor != null)
            keyDestructors[*(uint*)key] = (nint)destructor;
        return result;
    }

    public static int pthread_key_delete(int key)
    {
        keyDestructors.TryRemove((uint)key, out _);
        var delete = (delegate* unmanaged<uint, int>)RuntimeSymbols.NativeFunction("pthread_key_delete");
        return delete((uint)key);
    }

    private const int DestructorIterations = 4;

    private static void RunKeyDestructors()
    {
        var getSpecific = (delegate* unmanaged<uint, void*>)RuntimeSymbols.NativeFunction("pthread_getspecific");
        var setSpecific = (delegate* unmanaged<uint, void*, int>)RuntimeSymbols.NativeFunction("pthread_setspecific");
        for (var round = 0; round < DestructorIterations; round++)
        {
            var ranAny = false;
            foreach (var (key, destructor) in keyDestructors)
            {
                var value = getSpecific(key);
                if (value == null)
                    continue;
                setSpecific(key, null);
                ((delegate*<void*, void>)destructor)(value);
                ranAny = true;
            }
            if (!ranAny)
                return;
        }
    }

    public static void _ZSt9terminatev()
    {
        Environment.FailFast("std::terminate called");
    }

    public static void __cxa_pure_virtual()
    {
        Environment.FailFast("pure virtual function called");
    }

    public static void* _Znwm(long size)
    {
        return AllocateOrTerminate(size);
    }

    public static void* _Znam(long size)
    {
        return AllocateOrTerminate(size);
    }

    public static void _ZdlPv(void* pointer)
    {
        NativeMemory.Free(pointer);
    }

    public static void _ZdaPv(void* pointer)
    {
        NativeMemory.Free(pointer);
    }

    public static void _ZdlPvm(void* pointer, long size)
    {
        NativeMemory.Free(pointer);
    }

    public static void _ZdaPvm(void* pointer, long size)
    {
        NativeMemory.Free(pointer);
    }

    public static void* Landing(CxxException exception)
    {
        return exception.Header;
    }

    public static bool Matches(void* landed, void* caughtType)
    {
        var header = (ExceptionHeader*)landed;
        var thrownObject = ExceptionHeader.ThrownObjectOf(header);
        if (caughtType == null)
        {
            header->AdjustedPointer = thrownObject;
            return true;
        }

        var caught = (TypeInfo*)caughtType;
        bool matched;
        void* adjusted;
        if (RuntimeSymbols.IsAbiTypeInfo(caught))
            matched = AbiCanCatch(caught, header->ThrownType, thrownObject, out adjusted);
        else
            matched = TypeMatcher.CanCatch(caught, header->ThrownType, thrownObject, out adjusted);
        if (!matched)
            return false;
        header->AdjustedPointer = adjusted;
        return true;
    }

    public static CxxException Resume(void* landed)
    {
        return new CxxException((ExceptionHeader*)landed);
    }

    public static void* Symbol(string name)
    {
        return RuntimeSymbols.Resolve(name);
    }

    public static void RegisterVTable(string name, void* vtable)
    {
        RuntimeSymbols.RegisterVTable(name, vtable);
    }

    // __shim_type_info::can_catch is slot 4 from the vtable's address point
    private static bool AbiCanCatch(TypeInfo* caught, TypeInfo* thrown, void* thrownObject, out void* adjusted)
    {
        const int CanCatchSlot = 4;
        var slots = (void**)caught->VTable;
        var canCatch = (delegate*<void*, void*, void*, bool>)slots[CanCatchSlot];
        void* adjustedPointer = thrownObject;
        var matched = canCatch(caught, thrown, &adjustedPointer);
        adjusted = adjustedPointer;
        return matched;
    }

    private static void ReleaseHandledException(ExceptionHeader* header)
    {
        if (header->IsDependent == 0)
        {
            __cxa_decrement_exception_refcount(ExceptionHeader.ObjectOf(header));
            return;
        }
        var primary = header->PrimaryObject;
        __cxa_free_dependent_exception(header);
        __cxa_decrement_exception_refcount(primary);
    }

    private static void DestroyException(ExceptionHeader* header)
    {
        var thrownObject = ExceptionHeader.ObjectOf(header);
        if (header->Destructor != null)
        {
            var destructor = (delegate*<void*, void>)header->Destructor;
            destructor(thrownObject);
        }
        NativeMemory.Free(header);
    }

    private static void RunExitHandlers()
    {
        RunHandlers(exitHandlers);
    }

    private static void RunHandlers(List<ExitHandler> registered)
    {
        List<ExitHandler> handlers;
        lock (exitLock)
        {
            handlers = new List<ExitHandler>(registered);
            registered.Clear();
        }
        for (var i = handlers.Count - 1; i >= 0; i--)
        {
            var handler = handlers[i];
            if (handler.TakesArgument)
                ((delegate*<void*, void>)handler.Function)((void*)handler.Argument);
            else
                ((delegate*<void>)handler.Function)();
        }
    }

    private static void* AllocateOrTerminate(long size)
    {
        var pointer = NativeMemory.Alloc((nuint)Math.Max(size, 1));
        if (pointer == null)
            _ZSt9terminatev();
        return pointer;
    }
}
