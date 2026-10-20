/* Deliberately re-evaluate NDEBUG on every inclusion. */
#undef assert
#ifdef NDEBUG
#define assert(expression) ((void)0)
#else
#ifndef __C1_ASSERT_DECLARED
#define __C1_ASSERT_DECLARED
void __c1_assert_fail(const char *, const char *, int, const char *);
#endif
#define assert(expression) ((expression)?(void)0:__c1_assert_fail(#expression,__FILE__,__LINE__,__func__))
#endif
