# dladdr-test

A complicated toolset to demonstrate how `dladdr()` is currently slow in [GNU C Library][glibc] when used on shared object having lot of symbols.
The issue is reported as [bug #24561].

## Usage

```
$ make
$ perf stat ./dladdr-test-multiple 1000
$ perf stat ./dladdr-test-single 1000
```

## How it works

A bunch of symbols are emitted in multiple object files.
Each object file is linked as its own shared object, and all objects are also linked in a common shared object.
`dladdr-test-multiple` program is linked against all individual shared objects, while `dladdr-test-single` is linked only against the common shared object.

When run, `dladdr-test-multiple` and `dladdr-test-single` do the same things: iterate through the list of known symbols.
But you will notice `dladdr-test-single` is slower than `dladdr-test-multiple`.

[glibc]:      https://www.gnu.org/software/libc/ "GNU C library"
[bug #24561]: https://sourceware.org/bugzilla/show_bug.cgi?id=24561 "_dl_addr: linear lookup inefficient, making mtrace() feature very slow"
