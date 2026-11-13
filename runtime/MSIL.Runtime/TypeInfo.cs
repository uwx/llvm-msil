using System.Runtime.InteropServices;

namespace MSIL.Runtime;

internal enum TypeInfoKind
{
    Unknown,
    Fundamental,
    Class,
    SingleInheritanceClass,
    VirtualMultipleInheritanceClass,
    Pointer,
    PointerToMember,
    Enum,
    Array,
    Function,
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct TypeInfo
{
    public void* VTable;
    public byte* Name;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct SingleInheritanceTypeInfo
{
    public TypeInfo Type;
    public TypeInfo* BaseType;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct VirtualMultipleInheritanceTypeInfo
{
    public TypeInfo Type;
    public uint Flags;
    public uint BaseCount;

    public static BaseClassInfo* BasesOf(VirtualMultipleInheritanceTypeInfo* type)
    {
        return (BaseClassInfo*)(type + 1);
    }
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct BaseClassInfo
{
    public const long VirtualMask = 0x1;
    public const long PublicMask = 0x2;
    public const int OffsetShift = 8;

    public TypeInfo* BaseType;
    public long OffsetFlags;

    public bool IsVirtual => (OffsetFlags & VirtualMask) != 0;
    public bool IsPublic => (OffsetFlags & PublicMask) != 0;
    public long Offset => OffsetFlags >> OffsetShift;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct PointerTypeInfo
{
    public const uint ConstMask = 0x1;
    public const uint VolatileMask = 0x2;
    public const uint RestrictMask = 0x4;
    public const uint QualifierMask = ConstMask | VolatileMask | RestrictMask;

    public TypeInfo Type;
    public uint Flags;
    public TypeInfo* Pointee;
}
