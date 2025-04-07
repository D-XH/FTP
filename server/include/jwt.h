#ifndef __JWT_H__
#define __JWT_H_

#include "heads.h"
#include <l8w8jwt/encode.h>
#include <l8w8jwt/decode.h>

int getToken(char* username, char* token);

int valToken(char* username, char* token);

#endif