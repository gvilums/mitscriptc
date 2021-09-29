#include <exception>

class UninitializedVariableException : public exception{
	virtual const char* what() const throw() {
		return "UninitializedVariableException";
	}
};

class IllegalCastException : public exception{
	virtual const char* what() const throw() {
		return "IllegalCastException";
	}
};

class IllegalArithmeticException : public exception{
	virtual const char* what() const throw() {
		return "IllegalArithmeticException";
	}
};

class RuntimeException : public exception{
	virtual const char* what() const throw() {
		return "RuntimeException";
	}
};
