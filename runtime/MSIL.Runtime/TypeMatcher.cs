namespace MSIL.Runtime;

internal static unsafe class TypeMatcher
{
    public static bool CanCatch(TypeInfo* caught, TypeInfo* thrown, void* thrownObject, out void* adjusted)
    {
        var thrownKind = RuntimeSymbols.KindOf(thrown);
        var caughtKind = RuntimeSymbols.KindOf(caught);

        // a pointer catch binds to the pointer's value, everything else to the object
        var value = thrownKind == TypeInfoKind.Pointer ? *(void**)thrownObject : thrownObject;

        if (SameType(caught, thrown))
        {
            adjusted = value;
            return true;
        }

        if (IsClass(caughtKind) && IsClass(thrownKind))
            return TryUpcast(thrown, caught, value, out adjusted);

        if (caughtKind == TypeInfoKind.Pointer && thrownKind == TypeInfoKind.Pointer)
            return CanCatchPointer((PointerTypeInfo*)caught, (PointerTypeInfo*)thrown, value, out adjusted);

        adjusted = null;
        return false;
    }

    private static bool CanCatchPointer(PointerTypeInfo* caught, PointerTypeInfo* thrown, void* pointer, out void* adjusted)
    {
        adjusted = pointer;
        var droppedQualifiers = thrown->Flags & ~caught->Flags & PointerTypeInfo.QualifierMask;
        if (droppedQualifiers != 0)
            return false;

        if (SameType(caught->Pointee, thrown->Pointee))
            return true;

        var thrownPointeeKind = RuntimeSymbols.KindOf(thrown->Pointee);
        if (IsVoid(caught->Pointee))
            return thrownPointeeKind != TypeInfoKind.Function;

        var caughtPointeeKind = RuntimeSymbols.KindOf(caught->Pointee);
        if (IsClass(caughtPointeeKind) && IsClass(thrownPointeeKind))
            return TryUpcast(thrown->Pointee, caught->Pointee, pointer, out adjusted);

        return false;
    }

    private static bool TryUpcast(TypeInfo* derived, TypeInfo* target, void* derivedObject, out void* adjusted)
    {
        var search = new BaseSearch(target);
        search.Visit(derived, derivedObject);
        adjusted = search.Found;
        return search.Matches > 0 && !search.Ambiguous;
    }

    private struct BaseSearch(TypeInfo* target)
    {
        public void* Found;
        public int Matches;
        public bool Ambiguous;

        public void Visit(TypeInfo* type, void* objectPointer)
        {
            if (SameType(type, target))
            {
                Record(objectPointer);
                return;
            }

            switch (RuntimeSymbols.KindOf(type))
            {
                case TypeInfoKind.SingleInheritanceClass:
                    var single = (SingleInheritanceTypeInfo*)type;
                    Visit(single->BaseType, objectPointer);
                    break;
                case TypeInfoKind.VirtualMultipleInheritanceClass:
                    VisitBases((VirtualMultipleInheritanceTypeInfo*)type, objectPointer);
                    break;
            }
        }

        private void VisitBases(VirtualMultipleInheritanceTypeInfo* type, void* objectPointer)
        {
            var bases = VirtualMultipleInheritanceTypeInfo.BasesOf(type);
            for (var i = 0; i < type->BaseCount; i++)
            {
                var baseInfo = bases[i];
                if (!baseInfo.IsPublic)
                    continue;
                var basePointer = BasePointer(baseInfo, objectPointer);
                Visit(baseInfo.BaseType, basePointer);
            }
        }

        private void Record(void* objectPointer)
        {
            if (Matches > 0 && Found != objectPointer)
                Ambiguous = true;
            Found = objectPointer;
            Matches++;
        }
    }

    private static void* BasePointer(BaseClassInfo baseInfo, void* objectPointer)
    {
        if (objectPointer == null)
            return null;
        if (!baseInfo.IsVirtual)
            return (byte*)objectPointer + baseInfo.Offset;

        // for a virtual base the offset locates the base's offset in the vtable
        var vtable = *(byte**)objectPointer;
        var virtualBaseOffset = *(long*)(vtable + baseInfo.Offset);
        return (byte*)objectPointer + virtualBaseOffset;
    }

    private static bool IsClass(TypeInfoKind kind)
    {
        return kind is TypeInfoKind.Class
            or TypeInfoKind.SingleInheritanceClass
            or TypeInfoKind.VirtualMultipleInheritanceClass;
    }

    private static bool IsVoid(TypeInfo* type)
    {
        if (RuntimeSymbols.KindOf(type) != TypeInfoKind.Fundamental)
            return false;
        return type->Name[0] == (byte)'v' && type->Name[1] == 0;
    }

    private static bool SameType(TypeInfo* left, TypeInfo* right)
    {
        if (left == right)
            return true;
        if (left == null || right == null)
            return false;
        return NamesEqual(left->Name, right->Name);
    }

    private static bool NamesEqual(byte* left, byte* right)
    {
        if (left == null || right == null)
            return false;
        while (*left != 0 && *left == *right)
        {
            left++;
            right++;
        }
        return *left == *right;
    }
}
