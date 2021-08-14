# Nai
The Nai programming language, for fun and education!

# Design Philosophy
- We are not stuck in the way C does things.
- Performance and manual memory management is important.
- Compile speed is critical and it should be parallelised.
- The same code should be able to compile to machine code, be ran during compile time AND used as embedded scripting.
- The first priority is generating Bytecode and running it in a VM, this solves Compile Time Function Exection and embedded scripting.
- Compiling to machine code is a secondary objective.	
- Object orientation for the sake of object orientation is the root of all evil.
- Design decisions are taken with game development in mind.
- Integrated build system.

// LANGUAGE DESIGN
a : int; // Declares variable "a" of type int, completely uninitialized
a = 10; // Sets predeclared variable "a" to 10

// Declares function "myFunc" which takes "a" being a MyType pointer and "b" being a float, and returns an int
fn myFunc (a : MyType*, b : float) -> int
{
	return (int)b;
}

// Call the function, passing myObj and 25.0f
myFunc(myObj, 25.0f);