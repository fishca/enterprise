
#ifndef _COMPILE_ERROR_H__
#define _COMPILE_ERROR_H__

enum { //instruction types
	OPER_NOP = 0,
	OPER_ADD,
	OPER_DIV,
	OPER_MULT,
	OPER_SUB,
	OPER_NOT,
	OPER_AND,
	OPER_OR,
	OPER_RET,
	OPER_GOTO,
	OPER_FOR,
	OPER_FOREACH,
	OPER_IN,
	OPER_IF,
	OPER_LET,
	OPER_CONST,
	OPER_CONSTN,//integer constant
	OPER_NEXT,
	OPER_NEXT_ITER,
	OPER_MOD,
	OPER_INVERT,
	OPER_ITER,//?
	OPER_GT,//>
	OPER_EQ,//=
	OPER_LS,//<
	OPER_GE,//>=
	OPER_LE,//<=
	OPER_NE,//<>
	OPER_TRY,
	OPER_RAISE,
	OPER_RAISE_T,
	OPER_FUNC,//29 — function/procedure entry (also serves as FUNC_BEGIN tape marker)
	OPER_ENDFUNC,         // FUNC_END
	OPER_FUNC_PARAM,      // tape declarator: parameter slot in current function frame
	OPER_FUNC_LOCAL,      // tape declarator: named local in current function frame
	OPER_CTX_BEGIN,       // tape declarator: enter block scope ({ ... })
	OPER_CTX_END,         // tape declarator: exit block scope
	OPER_CALL,//function call
	OPER_SET, // setting the parameter as a variable
	OPER_SETREF,//setting the parameter as a variable by reference
	OPER_SETCONST, // setting the parameter as a constant
	OPER_ADDCONS,
	OPER_DIVCONS,
	OPER_MULTCONS,
	OPER_SUBCONS,
	OPER_GTCONS,//>
	OPER_EQCONS,//=
	OPER_LSCONS,//<
	OPER_GECONS,//>=
	OPER_LECONS,//<=
	OPER_NECONS,//<>
	OPER_MODCONS,
	OPER_SET_A,
	OPER_GET_A,
	OPER_ENTER_A,
	OPER_CALL_M,
	OPER_GET_ARRAY,
	OPER_SET_ARRAY,
	OPER_CHECK_ARRAY,
	OPER_SET_ARRAY_SIZE,
	OPER_ENDTRY,
	OPER_SET_TYPE,
	OPER_NEW,
	OPER_END,
};

#define TYPE_DELTA1 1 * (OPER_END + 1)  // for numeric operations
#define TYPE_DELTA2 2 * TYPE_DELTA1		// for string operations
#define TYPE_DELTA3 3 * TYPE_DELTA1		// for date operations
#define TYPE_DELTA4 4 * TYPE_DELTA1		// for operations with booleans

enum { //token types
	ERRORTYPE = 0,
	DELIMITER,	// single-character delimiters and operators
	IDENTIFIER, // unrecognized identifier (translation stage)
	CONSTANT,	// constant
	KEYWORD,	// contains the keyword number
	ENDPROGRAM, // end of the program module
};

enum { // numbers of keywords (in strict sequence as the values ​​themselves are specified)
	KEY_IF = 0,
	KEY_THEN,
	KEY_ELSE,
	KEY_ELSEIF,
	KEY_ENDIF,
	KEY_FOR,
	KEY_FOREACH,
	KEY_TO,
	KEY_IN,
	KEY_DO,
	KEY_ENDDO,
	KEY_WHILE,
	KEY_GOTO,
	KEY_NOT,
	KEY_AND,
	KEY_OR,
	KEY_PROCEDURE,
	KEY_ENDPROCEDURE,
	KEY_FUNCTION,
	KEY_ENDFUNCTION,
	KEY_EXPORT,
	KEY_VAL,
	KEY_RETURN,
	KEY_TRY,
	KEY_EXCEPT,
	KEY_ENDTRY,
	KEY_CONTINUE,
	KEY_BREAK,
	KEY_RAISE,
	KEY_VAR,
	KEY_NEW,
	KEY_UNDEFINED,
	KEY_NULL,
	KEY_TRUE,
	KEY_FALSE,
	KEY_DEFINE,
	KEY_UNDEF,
	KEY_IFDEF,
	KEY_IFNDEF,
	KEY_ELSEDEF,
	KEY_ENDIFDEF,
	KEY_REGION,
	KEY_ENDREGION,
	LastKeyWord
};

#endif