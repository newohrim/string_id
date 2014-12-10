string id
=========

Motivation
----------
It is often useful for logging purposes in real-time applications like games to give each entity a name. This makes tracking down errors easier because finding entities via unique names is easier than looking at unique numbers or other forms of identifiers. But strings are huge and copying and comparing them is slow, so they often can't be used in performance critical code.

One solution are hashed strings which are only integers and thus small, fast to copy and compare. But hashes don't allow retrieving the original string value which is exactly what is needed for logging and debugging purposes! In addition, there is a chance of collisions, so equal hash code doesn't necessarily mean equal strings. This chance is small, but a source for hard to find bugs.

Another solution is string interning. String interning uses a global look-up table where each string is stored only once and is referenced via an index or something similar. Copying and comparing is fast, too, but they are still not perfect: You can only access them at runtime. Getting the value at compile-time e.g. for a switch is not possible.

So on the one hand we want fast and lightweight identifiers but on the other hand also methods to get the name back. 


string_id
---------
This open source library provides a mix between the two solutions in the form of the class string_id. Each object stores a hashed string value and a pointer to the database in which the original string value is stored. This allows retrieving the string value when needed while also getting the performance benefits from hashed strings. In addition, the database can detect collisions which can be handled via a custom collision handler. There is a user-defined literal to create a compile-time hashed string value to use it as a constant expression.

See example/main.cpp for an example.

The database can be configured via the following CMake options:

* FOONATHAN_STRING_ID_DATABASE - if OFF, the database is disabled completely. This does not allow retrieving strings or collision checking but does not need that much memory. It is ON by default.

* FOONATHAN_STRING_ID_MULTITHREADED - if ON, database access will be synchronized via a mutex. It has no effect if database is disabled. Default value is ON.

Hashing
-------
It currently uses a FNV-1a 64bit hash. Collisions are really rare, I have tested 219,606 English words (in lowercase) mixed with a bunch of numbers and didn't encounter a single collision. Since this is the normal use case for identifiers, the hash function is pretty good. In addition, there is a good distribution of the hashed values and it is easy to calculate.