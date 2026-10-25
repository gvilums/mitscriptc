#pragma once

#include "AST.h"

#include <fstream>

AST::Program* parse(std::ifstream& file);
