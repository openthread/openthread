#include <stdint.h>

// Positive tests: tests that should trigger a violation

// Void return type

// Spaces w/o extern
void free_function_test_1();
void free_function_test_2 ();
void  free_function_test_3();
void   free_function_test_4  ();
void free_function_test_5 () ;
void  free_function_test_6() ;
void   free_function_test_7  ()  ;

// Tabs w/o extern
void	free_function_test_8();
void	free_function_test_9	();
void		free_function_test_10();
void			free_function_test_11		();
void	free_function_test_12	()	;
void		free_function_test_13()	;
void			free_function_test_14		()		;

// Spaces w/ extern
extern void free_function_test_15();
extern void free_function_test_16 ();
extern void  free_function_test_17();
extern void   free_function_test_18  ();
extern void free_function_test_19 () ;
extern void  free_function_test_20() ;
extern void   free_function_test_21  ()  ;

// Tabs w/ extern
extern	void	free_function_test_22();
extern	void	free_function_test_23	();
extern	void		free_function_test_24();
extern	void			free_function_test_25		();
extern	void	free_function_test_26	()	;
extern	void		free_function_test_27()	;
extern	void			free_function_test_28		()		;

// Spaces w/ static
static void free_function_test_29();
static void free_function_test_30 ();
static void  free_function_test_31();
static void   free_function_test_32  ();
static void free_function_test_33 () ;
static void  free_function_test_34() ;
static void   free_function_test_35  ()  ;

// Tabs w/ static
static	void	free_function_test_36();
static	void	free_function_test_37	();
static	void		free_function_test_38();
static	void			free_function_test_39		();
static	void	free_function_test_40	()	;
static	void		free_function_test_41()	;
static	void			free_function_test_42		()		;

// Const char * return type

// Spaces w/o extern
const char * free_function_test_43();
const char * free_function_test_44 ();
const char *  free_function_test_45();
const char *   free_function_test_46  ();
const char * free_function_test_47 () ;
const char *  free_function_test_48() ;
const char *   free_function_test_49  ()  ;

// Tabs w/o extern
const char *	free_function_test_50();
const char *	free_function_test_51	();
const char *		free_function_test_52();
const char *			free_function_test_53		();
const char *	free_function_test_54	()	;
const char *		free_function_test_55()	;
const char *			free_function_test_56		()		;

// Spaces w/ extern
extern const char * free_function_test_57();
extern const char * free_function_test_58 ();
extern const char *  free_function_test_59();
extern const char *   free_function_test_60  ();
extern const char * free_function_test_61 () ;
extern const char *  free_function_test_62() ;
extern const char *   free_function_test_63  ()  ;

// Tabs w/ extern
extern	const char *	free_function_test_64();
extern	const char *	free_function_test_65	();
extern	const char *		free_function_test_66();
extern	const char *			free_function_test_67		();
extern	const char *	free_function_test_68	()	;
extern	const char *		free_function_test_69()	;
extern	const char *			free_function_test_70		()		;

// Spaces w/ static
static const char * free_function_test_71();
static const char * free_function_test_72 ();
static const char *  free_function_test_73();
static const char *   free_function_test_74  ();
static const char * free_function_test_75 () ;
static const char *  free_function_test_76() ;
static const char *   free_function_test_77  ()  ;

// Tabs w/ static
static	const char *	free_function_test_78();
static	const char *	free_function_test_79	();
static	const char *		free_function_test_80();
static	const char *			free_function_test_81		();
static	const char *	free_function_test_82	()	;
static	const char *		free_function_test_83()	;
static	const char *			free_function_test_84		()		;

// Const uint32_t & return type

// Spaces w/o extern
const uint32_t & free_function_test_85();
const uint32_t & free_function_test_86 ();
const uint32_t &  free_function_test_87();
const uint32_t &   free_function_test_88  ();
const uint32_t & free_function_test_89 () ;
const uint32_t &  free_function_test_90() ;
const uint32_t &   free_function_test_91  ()  ;

// Tabs w/o extern
const uint32_t &	free_function_test_92();
const uint32_t &	free_function_test_93	();
const uint32_t &		free_function_test_94();
const uint32_t &			free_function_test_95		();
const uint32_t &	free_function_test_96	()	;
const uint32_t &		free_function_test_97()	;
const uint32_t &			free_function_test_98		()		;

// Spaces w/ extern
extern const uint32_t & free_function_test_99();
extern const uint32_t & free_function_test_100 ();
extern const uint32_t &  free_function_test_101();
extern const uint32_t &   free_function_test_102  ();
extern const uint32_t & free_function_test_103 () ;
extern const uint32_t &  free_function_test_104() ;
extern const uint32_t &   free_function_test_105  ()  ;

// Tabs w/ extern
extern	const uint32_t &	free_function_test_106();
extern	const uint32_t &	free_function_test_107	();
extern	const uint32_t &		free_function_test_108();
extern	const uint32_t &			free_function_test_109		();
extern	const uint32_t &	free_function_test_110	()	;
extern	const uint32_t &		free_function_test_111()	;
extern	const uint32_t &			free_function_test_112		()		;

// Spaces w/ static
static const uint32_t & free_function_test_113();
static const uint32_t & free_function_test_114 ();
static const uint32_t &  free_function_test_115();
static const uint32_t &   free_function_test_116  ();
static const uint32_t & free_function_test_117 () ;
static const uint32_t &  free_function_test_118() ;
static const uint32_t &   free_function_test_119  ()  ;

// Tabs w/ static
static	const uint32_t &	free_function_test_120();
static	const uint32_t &	free_function_test_121	();
static	const uint32_t &		free_function_test_122();
static	const uint32_t &			free_function_test_123		();
static	const uint32_t &	free_function_test_124	()	;
static	const uint32_t &		free_function_test_125()	;
static	const uint32_t &			free_function_test_126		()		;

// Void return type

class test
{
    // Spaces w/o qualifiers
    void method_test_1();
    void method_test_2 ();
    void  method_test_3();
    void   method_test_4  ();
    void method_test_5 () ;
    void  method_test_6() ;
    void   method_test_7  ()  ;

    // Tabs w/o qualifiers
    void	method_test_8();
    void	method_test_9	();
    void		method_test_10();
    void			method_test_11		();
    void	method_test_12	()	;
    void		method_test_13()	;
    void			method_test_14		()		;

    // Spaces w/ const qualifier
    void method_test_15() const;
    void method_test_16 () const;
    void  method_test_17() const;
    void   method_test_18  () const;
    void method_test_19 ()  const;
    void  method_test_20()  const;
    void   method_test_21  ()   const;

    // Tabs w/ const qualifier
    void	method_test_22()	const;
    void	method_test_23	()	const;
    void		method_test_24()	const;
    void			method_test_25		()	const;
    void	method_test_26	()	const	;
    void		method_test_27()	const	;
    void			method_test_28		()		const		;

    // Spaces w/ volatile qualifier
    void method_test_29() volatile;
    void method_test_30 () volatile;
    void  method_test_31() volatile;
    void   method_test_32  () volatile;
    void method_test_33 ()  volatile;
    void  method_test_34()  volatile;
    void   method_test_35  ()   volatile;

    // Tabs w/ volatile qualifier
    void	method_test_36()	volatile;
    void	method_test_37	()	volatile;
    void		method_test_38()	volatile;
    void			method_test_39		()	volatile;
    void	method_test_40	()	volatile	;
    void		method_test_41()	volatile	;
    void			method_test_42		()		volatile		;

    // Spaces w/ const and volatile qualifiers
    void method_test_43() const volatile;
    void method_test_44 () const volatile;
    void  method_test_45() const volatile;
    void   method_test_46  () const volatile;
    void method_test_47 ()  const volatile;
    void  method_test_48()  const volatile;
    void   method_test_49  ()   const volatile;

    // Tabs w/ const and volatile qualifiers
    void	method_test_50()	const volatile;
    void	method_test_51	()	const volatile;
    void		method_test_52()	const volatile;
    void			method_test_53		()	const volatile;
    void	method_test_54	()	const volatile	;
    void		method_test_55()	const volatile	;
    void			method_test_56		()		const volatile		;

    // Static

    // Spaces w/o qualifiers
    static void method_test_57();
    static void method_test_58 ();
    static void  method_test_59();
    static void   method_test_60  ();
    static void method_test_61 () ;
    static void  method_test_62() ;
    static void   method_test_63  ()  ;

    // Tabs w/o qualifiers
    static void	method_test_64();
    static void	method_test_65	();
    static void		method_test_66();
    static void			method_test_67		();
    static void	method_test_68	()	;
    static void		method_test_69()	;
    static void			method_test_70		()		;

    // Virtual

    // Spaces w/o qualifiers
    virtual void method_test_113();
    virtual void method_test_114 ();
    virtual void  method_test_115();
    virtual void   method_test_116  ();
    virtual void method_test_117 () ;
    virtual void  method_test_118() ;
    virtual void   method_test_119  ()  ;

    // Tabs w/o qualifiers
    virtual void	method_test_120();
    virtual void	method_test_121	();
    virtual void		method_test_122();
    virtual void			method_test_123		();
    virtual void	method_test_124	()	;
    virtual void		method_test_125()	;
    virtual void			method_test_126		()		;

    // Spaces w/ const qualifier
    virtual void method_test_127() const;
    virtual void method_test_128 () const;
    virtual void  method_test_129() const;
    virtual void   method_test_130  () const;
    virtual void method_test_131 ()  const;
    virtual void  method_test_132()  const;
    virtual void   method_test_133  ()   const;

    // Tabs w/ const qualifier
    virtual void	method_test_134()	const;
    virtual void	method_test_135	()	const;
    virtual void		method_test_136()	const;
    virtual void			method_test_137		()	const;
    virtual void	method_test_138	()	const	;
    virtual void		method_test_139()	const	;
    virtual void			method_test_140		()		const		;

    // Spaces w/ volatile qualifier
    virtual void method_test_141() volatile;
    virtual void method_test_142 () volatile;
    virtual void  method_test_143() volatile;
    virtual void   method_test_144  () volatile;
    virtual void method_test_145 ()  volatile;
    virtual void  method_test_146()  volatile;
    virtual void   method_test_147  ()   volatile;

    // Tabs w/ volatile qualifier
    virtual void	method_test_148()	volatile;
    virtual void	method_test_149	()	volatile;
    virtual void		method_test_150()	volatile;
    virtual void			method_test_151		()	volatile;
    virtual void	method_test_152	()	volatile	;
    virtual void		method_test_153()	volatile	;
    virtual void			method_test_154		()		volatile		;

    // Spaces w/ const and volatile qualifiers
    virtual void method_test_155() const volatile;
    virtual void method_test_156 () const volatile;
    virtual void  method_test_157() const volatile;
    virtual void   method_test_158  () const volatile;
    virtual void method_test_159 ()  const volatile;
    virtual void  method_test_160()  const volatile;
    virtual void   method_test_161  ()   const volatile;

    // Tabs w/ const and volatile qualifiers
    virtual void	method_test_162()	const volatile;
    virtual void	method_test_163	()	const volatile;
    virtual void		method_test_164()	const volatile;
    virtual void			method_test_165		()	const volatile;
    virtual void	method_test_166	()	const volatile	;
    virtual void		method_test_167()	const volatile	;
    virtual void			method_test_168		()		const volatile		;

    // Pure virtual

    // Spaces w/o qualifiers
    virtual void method_test_169() = 0;
    virtual void method_test_170 () = 0;
    virtual void  method_test_171() = 0;
    virtual void   method_test_172  () = 0;
    virtual void method_test_173 ()  = 0;
    virtual void  method_test_174()  = 0;
    virtual void   method_test_175  ()   = 0;

    // Tabs w/o qualifiers
    virtual void	method_test_176() = 0;
    virtual void	method_test_177	() = 0;
    virtual void		method_test_178() = 0;
    virtual void			method_test_179		() = 0;
    virtual void	method_test_180	()	 = 0;
    virtual void		method_test_181()	 = 0;
    virtual void			method_test_182		()		 = 0;

    // Spaces w/ const qualifier
    virtual void method_test_183() const = 0;
    virtual void method_test_184 () const = 0;
    virtual void  method_test_185() const = 0;
    virtual void   method_test_186  () const = 0;
    virtual void method_test_187 ()  const = 0;
    virtual void  method_test_188()  const = 0;
    virtual void   method_test_189  ()   const = 0;

    // Tabs w/ const qualifier
    virtual void	method_test_190()	const = 0;
    virtual void	method_test_191	()	const = 0;
    virtual void		method_test_192()	const = 0;
    virtual void			method_test_193		()	const = 0;
    virtual void	method_test_194	()	const	 = 0;
    virtual void		method_test_195()	const	 = 0;
    virtual void			method_test_196		()		const		 = 0;

    // Spaces w/ volatile qualifier
    virtual void method_test_197() volatile = 0;
    virtual void method_test_198 () volatile = 0;
    virtual void  method_test_199() volatile = 0;
    virtual void   method_test_200  () volatile = 0;
    virtual void method_test_201 ()  volatile = 0;
    virtual void  method_test_202()  volatile = 0;
    virtual void   method_test_203  ()   volatile = 0;

    // Tabs w/ volatile qualifier
    virtual void	method_test_204()	volatile = 0;
    virtual void	method_test_205	()	volatile = 0;
    virtual void		method_test_206()	volatile = 0;
    virtual void			method_test_207		()	volatile = 0;
    virtual void	method_test_208	()	volatile	 = 0;
    virtual void		method_test_209()	volatile	 = 0;
    virtual void			method_test_210		()		volatile		 = 0;

    // Spaces w/ const and volatile qualifiers
    virtual void method_test_211() const volatile = 0;
    virtual void method_test_212 () const volatile = 0;
    virtual void  method_test_213() const volatile = 0;
    virtual void   method_test_214  () const volatile = 0;
    virtual void method_test_215 ()  const volatile = 0;
    virtual void  method_test_216()  const volatile = 0;
    virtual void   method_test_217  ()   const volatile = 0;

    // Tabs w/ const and volatile qualifiers
    virtual void	method_test_218()	const volatile = 0;
    virtual void	method_test_219	()	const volatile = 0;
    virtual void		method_test_220()	const volatile = 0;
    virtual void			method_test_221		()	const volatile = 0;
    virtual void	method_test_222	()	const volatile	 = 0;
    virtual void		method_test_223()	const volatile	 = 0;
    virtual void			method_test_224		()		const volatile		 = 0;
};

// Negative tests: tests that should not trigger a violation

class negative_test_1
{
public:
    void void_method(void);
    static int int_method_1(void);
    int int_method_2(void);
};

class negative_test_2
{
public:
    inline void void_method(void)
    {
        m_negative_test_1_pointer->void_method();
        m_negative_test_1_reference.void_method();
    }

    inline int int_method_1(void)
    {
        return negative_test_1::int_method_1();
    }

    inline int int_method_2(void)
    {
        int retval;

        retval = m_negative_test_1_reference.int_method_2();

        return retval;
    }

private:
    negative_test_1 *m_negative_test_1_pointer;
    negative_test_1 &m_negative_test_1_reference;

};
