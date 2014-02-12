// header file to disable certain warnings on Microsoft CL compiler.
// since it doesn't allow you to disable individual warnings from
// command line, but it can force inclusion of a .h from command line.

// 4355 is "'this' used in base member initializer list"; the stl_rope
//		class causes this every time.
// 4786 is 'long_ass_identifier_name' : identifier was truncated to 255
//		chars in the debug information" which lots of STL template classes
//		(esp. map classes) cause.

#pragma warning (disable: 4355 4786)
