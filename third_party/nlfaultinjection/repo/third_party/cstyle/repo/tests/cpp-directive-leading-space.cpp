#define CPP_DIRECTIVE_LEADING_SPACE 1

// Negative tests: these should not generate a violation

#if CPP_DIRECTIVE_LEADING_SPACE

#endif

# if CPP_DIRECTIVE_LEADING_SPACE

#endif

#	if CPP_DIRECTIVE_LEADING_SPACE

#endif

// Positive tests: these should generate a violation

 #if CPP_DIRECTIVE_LEADING_SPACE

 #endif

	#if CPP_DIRECTIVE_LEADING_SPACE

	#endif

 	#if CPP_DIRECTIVE_LEADING_SPACE

 	#endif

	 #if CPP_DIRECTIVE_LEADING_SPACE

	 #endif

 	 #if CPP_DIRECTIVE_LEADING_SPACE

 	 #endif

	 	#if CPP_DIRECTIVE_LEADING_SPACE

	 	#endif

