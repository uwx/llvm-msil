extern "C" int printf(const char *, ...);

static int live = 0;

struct Tracked {
  int id;
  Tracked(int i) : id(i) { ++live; printf("ctor %d\n", id); }
  Tracked(const Tracked &o) : id(o.id + 100) { ++live; printf("copy %d\n", id); }
  ~Tracked() { --live; printf("dtor %d\n", id); }
};

struct A { int a = 1; virtual ~A() {} };
struct B { int b = 2; virtual ~B() {} };
struct C : A, B { int c = 3; };

struct V { int v = 7; virtual ~V() {} };
struct D1 : virtual V { int d1 = 0; };
struct D2 : virtual V { int d2 = 0; };
struct E : D1, D2 { int e = 0; };

struct X { int x = 5; };
struct Y1 : X {};
struct Y2 : X {};
struct Z : Y1, Y2 {};
struct P : private X { int p = 9; };

struct Base { int code; Base(int c) : code(c) {} virtual ~Base() {} };

struct ThrowsInDtorCatch {
  ~ThrowsInDtorCatch() {
    try {
      throw 1234;
    } catch (int v) {
      printf("inner dtor caught %d\n", v);
    }
  }
};

struct Member {
  int n;
  Member(int i) : n(i) { printf("member %d\n", n); }
  ~Member() { printf("~member %d\n", n); }
};

struct Ctor {
  Member first{1};
  Member second{2};
  Ctor() { throw 77; }
};

__attribute__((noinline)) void throw_c() { throw C(); }
__attribute__((noinline)) void throw_c_ptr(C *p) { throw p; }
__attribute__((noinline)) void throw_e() { throw E(); }
__attribute__((noinline)) void throw_z() { throw Z(); }
__attribute__((noinline)) void throw_p() { throw P(); }
__attribute__((noinline)) void throw_tracked() { throw Tracked(1); }
__attribute__((noinline)) void throw_char_ptr(char *s) { throw s; }
__attribute__((noinline)) void throw_const_char_ptr(const char *s) { throw s; }

void multiple_inheritance() {
  try {
    throw_c();
  } catch (B &b) {
    printf("MI ref b=%d\n", b.b);
  }
  C heap;
  try {
    throw_c_ptr(&heap);
  } catch (B *p) {
    printf("MI ptr b=%d adjusted=%d\n", p->b, (int)((char *)p - (char *)&heap));
  }
  try {
    throw_c_ptr(&heap);
  } catch (void *p) {
    printf("void* same=%d\n", p == &heap);
  }
}

void virtual_base() {
  try {
    throw_e();
  } catch (V &v) {
    printf("virtual base v=%d\n", v.v);
  }
}

void ambiguous_and_private() {
  try {
    throw_z();
  } catch (X &) {
    printf("BAD: ambiguous base matched\n");
  } catch (...) {
    printf("ambiguous base skipped\n");
  }
  try {
    throw_p();
  } catch (X &) {
    printf("BAD: private base matched\n");
  } catch (P &p) {
    printf("private base skipped p=%d\n", p.p);
  }
}

void qualification() {
  char buf[] = "hi";
  try {
    throw_char_ptr(buf);
  } catch (const char *s) {
    printf("char* as const char*: %s\n", s);
  }
  try {
    throw_const_char_ptr("const");
  } catch (char *) {
    printf("BAD: dropped const\n");
  } catch (const char *s) {
    printf("const char* kept: %s\n", s);
  }
}

void by_value_and_lifetime() {
  try {
    throw_tracked();
  } catch (Tracked t) {
    printf("by value id=%d live=%d\n", t.id, live);
  }
  printf("after by value live=%d\n", live);
  try {
    throw_tracked();
  } catch (const Tracked &t) {
    printf("by ref id=%d live=%d\n", t.id, live);
  }
  printf("after by ref live=%d\n", live);
}

__attribute__((noinline)) void bump_and_rethrow() {
  try {
    throw Base(10);
  } catch (Base &b) {
    b.code++;
    throw;
  }
}

void rethrow_mutated() {
  try {
    bump_and_rethrow();
  } catch (Base &b) {
    printf("rethrown mutated code=%d\n", b.code);
  }
}

__attribute__((noinline)) void unwind_through_throwing_dtor() {
  ThrowsInDtorCatch guard;
  throw Base(55);
}

void nested_during_unwind() {
  try {
    unwind_through_throwing_dtor();
  } catch (Base &b) {
    printf("outer still sees code=%d\n", b.code);
  }
}

__attribute__((noinline)) void throw_int(int v) { throw v; }

void catch_all_rethrow() {
  try {
    throw_int(99);
  } catch (...) {
    printf("caught ...\n");
    try {
      throw;
    } catch (int v) {
      printf("rethrown int %d\n", v);
    }
  }
}

void ctor_throws() {
  try {
    Ctor c;
    printf("BAD: constructed\n");
  } catch (int v) {
    printf("ctor threw %d\n", v);
  }
}

int main() {
  multiple_inheritance();
  virtual_base();
  ambiguous_and_private();
  qualification();
  by_value_and_lifetime();
  rethrow_mutated();
  nested_during_unwind();
  catch_all_rethrow();
  ctor_throws();
  printf("final live=%d\n", live);
  return live;
}
