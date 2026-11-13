using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace MSIL.Runtime;

internal static unsafe class RuntimeSymbols
{
    private const string VTablePrefix = "_ZTVN10__cxxabiv1";
    private const string TypeInfoPrefix = "_ZTI";
    private const string TypeNamePrefix = "_ZTS";
    private const string FundamentalVTable = "_ZTVN10__cxxabiv123__fundamental_type_infoE";
    private const string ClassVTable = "_ZTVN10__cxxabiv117__class_type_infoE";
    private const string PointerVTable = "_ZTVN10__cxxabiv119__pointer_type_infoE";
    private const string DsoHandle = "__dso_handle";

    // the vtable symbol's address plus two slots is what a type_info points at
    private const int VTableAddressPoint = 2 * 8;
    private const int VTableBlockSize = 4 * 8;

    private static readonly Dictionary<string, TypeInfoKind> vtableKinds = new()
    {
        [FundamentalVTable] = TypeInfoKind.Fundamental,
        [ClassVTable] = TypeInfoKind.Class,
        ["_ZTVN10__cxxabiv120__si_class_type_infoE"] = TypeInfoKind.SingleInheritanceClass,
        ["_ZTVN10__cxxabiv121__vmi_class_type_infoE"] = TypeInfoKind.VirtualMultipleInheritanceClass,
        [PointerVTable] = TypeInfoKind.Pointer,
        ["_ZTVN10__cxxabiv129__pointer_to_member_type_infoE"] = TypeInfoKind.PointerToMember,
        ["_ZTVN10__cxxabiv116__enum_type_infoE"] = TypeInfoKind.Enum,
        ["_ZTVN10__cxxabiv117__array_type_infoE"] = TypeInfoKind.Array,
        ["_ZTVN10__cxxabiv120__function_type_infoE"] = TypeInfoKind.Function,
    };

    private static readonly object creationLock = new();
    private static readonly Dictionary<string, nint> symbols = new();
    private static readonly ConcurrentDictionary<nint, TypeInfoKind> kindsByAddressPoint = new();
    private static readonly ConcurrentDictionary<nint, bool> abiAddressPoints = new();

    public static void* Resolve(string name)
    {
        lock (creationLock)
        {
            if (symbols.TryGetValue(name, out var existing))
                return (void*)existing;
            var created = Create(name);
            symbols[name] = (nint)created;
            return created;
        }
    }

    // a libc++abi vtable defined in the program, rather than one of our anchors
    public static void RegisterVTable(string name, void* vtable)
    {
        if (!vtableKinds.TryGetValue(name, out var kind))
            return;
        var addressPoint = (nint)((byte*)vtable + VTableAddressPoint);
        kindsByAddressPoint[addressPoint] = kind;
        abiAddressPoints[addressPoint] = true;
        lock (creationLock)
            symbols[name] = (nint)vtable;
    }

    public static bool IsAbiTypeInfo(TypeInfo* type)
    {
        return type != null && abiAddressPoints.ContainsKey((nint)type->VTable);
    }

    public static TypeInfoKind KindOf(TypeInfo* type)
    {
        if (type == null)
            return TypeInfoKind.Unknown;
        return kindsByAddressPoint.TryGetValue((nint)type->VTable, out var kind) ? kind : TypeInfoKind.Unknown;
    }

    private static void* Create(string name)
    {
        if (vtableKinds.TryGetValue(name, out var kind))
            return CreateVTableAnchor(kind);
        if (name.StartsWith(TypeInfoPrefix))
            return CreateTypeInfo(name.Substring(TypeInfoPrefix.Length));
        if (name.StartsWith(TypeNamePrefix))
            return AllocateString(name.Substring(TypeNamePrefix.Length));
        if (name.StartsWith(VTablePrefix))
            return CreateVTableAnchor(TypeInfoKind.Unknown);
        if (name == DsoHandle)
            return NativeMemory.AllocZeroed(8);
        return FindNativeData(name);
    }

    public static void* NativeFunction(string name)
    {
        var address = FindNativeData(name);
        if (address == null)
            throw new EntryPointNotFoundException(name);
        return address;
    }

    private static void* FindNativeData(string name)
    {
        if (NativeLibrary.TryGetExport(NativeLibrary.GetMainProgramHandle(), name, out var address))
            return (void*)address;
        foreach (var library in NativeDataLibraries())
        {
            if (!NativeLibrary.TryLoad(library, out var handle))
                continue;
            if (NativeLibrary.TryGetExport(handle, name, out address))
                return (void*)address;
        }
        return null;
    }

    private static string[] NativeDataLibraries()
    {
        if (OperatingSystem.IsWindows())
            return ["msvcrt.dll", "ucrtbase.dll"];
        return ["libc.so.6", "libm.so.6"];
    }

    private static void* CreateVTableAnchor(TypeInfoKind kind)
    {
        var block = (byte*)NativeMemory.AllocZeroed(VTableBlockSize);
        kindsByAddressPoint[(nint)(block + VTableAddressPoint)] = kind;
        return block;
    }

    // the runtime's own type_infos: fundamentals and pointers to them, or an
    // external class (std::exception and friends) we only know by name
    private static void* CreateTypeInfo(string mangledType)
    {
        if (mangledType.StartsWith('P'))
            return CreatePointerTypeInfo(mangledType);

        var vtableName = IsFundamental(mangledType) ? FundamentalVTable : ClassVTable;
        var type = (TypeInfo*)NativeMemory.AllocZeroed((nuint)sizeof(TypeInfo));
        type->VTable = AddressPoint(vtableName);
        type->Name = AllocateString(mangledType);
        return type;
    }

    private static void* CreatePointerTypeInfo(string mangledType)
    {
        var position = 1;
        var flags = 0u;
        while (position < mangledType.Length)
        {
            var qualifier = mangledType[position] switch
            {
                'K' => PointerTypeInfo.ConstMask,
                'V' => PointerTypeInfo.VolatileMask,
                'r' => PointerTypeInfo.RestrictMask,
                _ => 0u,
            };
            if (qualifier == 0)
                break;
            flags |= qualifier;
            position++;
        }

        var pointeeName = TypeInfoPrefix + mangledType.Substring(position);
        var type = (PointerTypeInfo*)NativeMemory.AllocZeroed((nuint)sizeof(PointerTypeInfo));
        type->Type.VTable = AddressPoint(PointerVTable);
        type->Type.Name = AllocateString(mangledType);
        type->Flags = flags;
        type->Pointee = (TypeInfo*)Resolve(pointeeName);
        return type;
    }

    private static void* AddressPoint(string vtableName)
    {
        var block = (byte*)Resolve(vtableName);
        return block + VTableAddressPoint;
    }

    private static bool IsFundamental(string mangledType)
    {
        if (mangledType.Length == 1)
            return "vwbcahstijlmxynofdegz".Contains(mangledType[0]);
        return mangledType.Length == 2 && mangledType[0] == 'D';
    }

    private static byte* AllocateString(string value)
    {
        var bytes = Encoding.ASCII.GetBytes(value);
        var buffer = (byte*)NativeMemory.AllocZeroed((nuint)bytes.Length + 1);
        Marshal.Copy(bytes, 0, (nint)buffer, bytes.Length);
        return buffer;
    }
}
