using System;

namespace MSIL.Runtime;

public sealed unsafe class CxxException : Exception
{
    internal CxxException(ExceptionHeader* header)
        : base("unhandled C++ exception")
    {
        Header = header;
    }

    internal ExceptionHeader* Header { get; }
}
