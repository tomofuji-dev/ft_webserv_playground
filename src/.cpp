#include "ClassName"

ClassName::ClassName(){
	std::cout << "ClassName()" << std::endl;
}

ClassName::ClassName(const ClassName &src){
	std::cout << "ClassName(src)" << std::endl;
}

ClassName::~ClassName(){
	std::cout << "~ClassName()" << std::endl;
}

ClassName&	ClassName::operator=(const ClassName &rhs){
}
