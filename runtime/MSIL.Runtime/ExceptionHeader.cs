using System.Runtime.InteropServices;

namespace MSIL.Runtime;

[StructLayout(LayoutKind.Sequential, Size = 64)]
internal unsafe struct ExceptionHeader
{
    public TypeInfo* ThrownType;
    public void* Destructor;
    public void* AdjustedPointer;
    public ExceptionHeader* NextCaught;
    public void* PrimaryObject;
    public int HandlerCount;
    public int ReferenceCount;
    public int IsDependent;

    public static ExceptionHeader* FromObject(void* thrownObject)
    {
        return (ExceptionHeader*)thrownObject - 1;
    }

    public static void* ObjectOf(ExceptionHeader* header)
    {
        return header + 1;
    }

    public static void* ThrownObjectOf(ExceptionHeader* header)
    {
        if (header->IsDependent != 0)
            return header->PrimaryObject;
        return ObjectOf(header);
    }
}
