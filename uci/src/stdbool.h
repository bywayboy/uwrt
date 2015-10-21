#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined
 
	/* program is allowed to contain its own definitions, so ... */
	#undef bool
	#undef true
	#undef false
 
#define bool int
#define true 1
#define false 0
 
#endif /* !defined(__bool_true_false_are_defined) */