#include "jwt.h"

int getToken(char* username, char* token){
    char* jwt;
    size_t jwt_length;

    struct l8w8jwt_encoding_params params;
    l8w8jwt_encoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.sub = "cloudDisk";
    params.iss = "deng";
    params.aud = "client";

    params.iat = l8w8jwt_time(NULL);
    params.exp = l8w8jwt_time(NULL) + 600; /* Set to expire after 10 minutes (600 seconds). */

    params.secret_key = (unsigned char*)username;
    params.secret_key_length = strlen(params.secret_key);

    params.out = &jwt;
    params.out_length = &jwt_length;

    int r = l8w8jwt_encode(&params);

    // printf("\n l8w8jwt example HS512 token: %s \n", r == L8W8JWT_SUCCESS ? jwt : " (encoding failure) ");
    if(r == L8W8JWT_SUCCESS){
        memcpy(token, jwt, strlen(jwt));
    }else{
        return -1;
    }

    /* Always free the output jwt string! */
    l8w8jwt_free(jwt);

    return 0;
}

int valToken(char* username, char* token){
    struct l8w8jwt_decoding_params params;
    l8w8jwt_decoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.jwt = token;
    params.jwt_length = strlen(token);

    params.verification_key = (unsigned char*)username;
    params.verification_key_length = strlen(username);

    /* 
     * Not providing params.validate_iss_length makes it use strlen()
     * Only do this when using properly NUL-terminated C-strings! 
     */
    params.validate_iss = "deng"; 
    params.validate_sub = "cloudDisk";

    /* Expiration validation set to false here only because the above example token is already expired! */
    params.validate_exp = 1; 
    params.exp_tolerance_seconds = 0;

    params.validate_iat = 1;
    params.iat_tolerance_seconds = 60;

    enum l8w8jwt_validation_result validation_result;

    int decode_result = l8w8jwt_decode(&params, &validation_result, NULL, NULL);

    if (decode_result == L8W8JWT_SUCCESS && validation_result == L8W8JWT_VALID) 
    {
        printf("\n token validation successful! \n");
        return 0;
    }
    else
    {
        printf("\n token validation failed! \n");
        return -1;
    }
    
    /*
     * decode_result describes whether decoding/parsing the token succeeded or failed;
     * the output l8w8jwt_validation_result variable contains actual information about
     * JWT signature verification status and claims validation (e.g. expiration check).
     * 
     * If you need the claims, pass an (ideally stack pre-allocated) array of struct l8w8jwt_claim
     * instead of NULL,NULL into the corresponding l8w8jwt_decode() function parameters.
     * If that array is heap-allocated, remember to free it yourself!
     */

    return 0;
}