# libjavm

## Introduction

This VM library is an extended C++ port of [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter). The base of the library is a port of that project, improved with C++'s advantages, and later extended, implementing missing features like missing instructions, exceptions...

The main aim of this library is to provide a simple-to-use but complete API to be able to run Java on any place with C++17 being the only dependency, if any.

The main issue with this "independent" VM implementation is the lack of Java's standard library, so another aim of the library is to provide a minimal (and optional) implementation of that API.

Considering how a Java VM works, it should be definitely possible to write a VM in plain C++. In fact, the only dependency livjavm has is *andyzip* header-only ZIP libraries, included here as part of libjavm, which aren't completely necessary since they are only used for JAR loading.

## Documentation

### [Types](Types.md)

### [Native API](Native.md)