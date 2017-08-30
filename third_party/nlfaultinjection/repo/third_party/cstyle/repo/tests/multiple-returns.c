struct _Unused1 {
    int mUnused;
};

typedef struct _Unused2
{
    int mUnused;
} Unused2;

static int sMoof;

static inline int _mul(int bar) { return (bar * 3); }

int mul(int bar) { return _mul(bar); }

void baz(int bar)
{
    if (bar)
        sMoof += bar;
    else
        sMoof += mul(bar);

    return;
}

int foo(int bar) {
    if (bar)
        return (1);

    if (bar % 2)
        return (1 + bar);

    return 0;
}
