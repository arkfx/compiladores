#ifndef MINILANG_CODEGEN_C_H
#define MINILANG_CODEGEN_C_H

#include "ast.h"

int codegen_c_file(AstNode *program, const char *path);

#endif
