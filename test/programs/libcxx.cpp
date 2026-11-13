/* Exercises the portable C++ runtime through llvm-msil.
 *
 * This is the half of a C++ implementation that is not in the headers: the
 * allocation operators, the local-static guards, and exit-time destructors.
 * The program uses no standard headers -- it is written against the same
 * freestanding subset cxxrt.cpp is -- so it isolates the runtime from the
 * header/library questions an STL port would add. libcxx.sh compares its
 * output against a native build of the same file.
 */
typedef __SIZE_TYPE__ size_t;

extern "C" int printf(const char *, ...);

namespace std {
struct nothrow_t {
  explicit nothrow_t() = default;
};
} // namespace std

/* The runtime under test, declared as the front end would emit them. */
void *operator new(size_t);
void *operator new[](size_t);
void operator delete(void *) noexcept;
void operator delete[](void *) noexcept;
void *operator new(size_t, const std::nothrow_t &) noexcept;
void operator delete(void *, const std::nothrow_t &) noexcept;

/* --- A polymorphic type: vtable slot, virtual dtor through operator delete. */
struct Shape {
  int sides;
  explicit Shape(int n) : sides(n) {}
  virtual int area() const { return 0; }
  virtual ~Shape() {}
};

struct Square : Shape {
  int side;
  explicit Square(int s) : Shape(4), side(s) {}
  int area() const override { return side * side; }
};

/* --- A destructor that reports, to prove when exit-time teardown runs. */
struct Tracer {
  const char *name;
  explicit Tracer(const char *n) : name(n) { printf("ctor %s\n", name); }
  ~Tracer() { printf("dtor %s\n", name); }
};

/* A global with a constructor and a destructor: .init_array and __cxa_atexit
 * (the MSVC protocol) or __cxa_atexit directly (Itanium). */
Tracer global_a("global_a");

/* A function-local static with a non-trivial destructor: the guard variables
 * and the exit-time registration both come from here. */
int local_static_use(int add) {
  static Tracer local("local_static");
  return add + 1;
}

/* A second static proves registration order is construction order, so
 * destruction runs in reverse. */
int second_static_use() {
  static Tracer second("second_static");
  return 2;
}

int main() {
  /* Plain and array new/delete. */
  int *p = new int[4];
  for (int i = 0; i < 4; i++)
    p[i] = i * i;
  printf("array %d %d\n", p[0], p[3]);
  delete[] p;

  int *one = new int(7);
  printf("scalar %d\n", *one);
  delete one;

  /* nothrow new: a normal allocation succeeds and returns non-null. */
  int *q = new (std::nothrow_t{}) int(9);
  printf("nothrow %d %d\n", q != nullptr, q ? *q : 0);
  delete q;

  /* A polymorphic object: the vtable is a data structure the front end emits,
   * and the virtual destructor's operator delete comes from the runtime. */
  Shape *s = new Square(5);
  printf("virtual %d %d\n", s->sides, s->area());
  delete s;

  /* Local statics: the initializer runs once, and the destructor is registered
   * for exit. */
  printf("local %d %d\n", local_static_use(10), local_static_use(20));
  printf("second %d\n", second_static_use());

  /* A local static object is constructed before exit teardown, so its dtor
   * must appear after this line and before any destructor of a later
   * registration. */
  printf("before exit\n");
  return 0;
}
