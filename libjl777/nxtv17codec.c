// created by chanc3r
// MIT License

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <stdint.h>

#include "nxtv17codec.h"


typedef unsigned char byte;

void printhex(const byte* bytes, size_t byte_size) {
    printf("hex");
    char str[4];
    for (int i=0; i<byte_size; i++) {
        int value=bytes[i];
        sprintf(str,"%02x",value);
        printf(":%s",str);
    }
    return;
}

//cant figure out how to remove '/' from default encoding scheme so this is a work around!
//all chars in base64scheme are url safe apart from '/'
//reference 
const char urlmap[][2]={
{'/','*'},
{'+','@'},
{0,0}
};

void url64encode(char* string) {
    for(int i=0; urlmap[i][0]; i++) 
	for(int j=0; string[j]; j++)
		if(string[j]==urlmap[i][0]) string[j]=urlmap[i][1];
}

void url64decode(char* string) {
    for(int i=0; urlmap[i][0]; i++) 
	for(int j=0; string[j]; j++)
		if(string[j]==urlmap[i][1]) string[j]=urlmap[i][0];
}
// base 64 codec start

int base64encode(const uint8_t* data_buf, size_t dataLength, char* result, size_t resultSize)
{
    const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *data = (const uint8_t *)data_buf;
    size_t resultIndex = 0;
    size_t x;
    uint32_t n = 0;
    int padCount = dataLength % 3;
    uint8_t n0, n1, n2, n3;
    
    V17DO printf("base64:enc:");
    V17DO printhex(data, dataLength);
    V17DO printf("\n");
    
    /* increment over the length of the string, three characters at a time */
    for (x = 0; x < dataLength; x += 3)
    {
        /* these three 8-bit (ASCII) characters become one 24-bit number */
        n = (uint32_t)data[x] << 16;
        
        if((x+1) < dataLength)
            n += (uint32_t)data[x+1] << 8;
        
        if((x+2) < dataLength)
            n += data[x+2];
        
        /* this 24-bit number gets separated into four 6-bit numbers */
        n0 = (uint8_t)(n >> 18) & 63;
        n1 = (uint8_t)(n >> 12) & 63;
        n2 = (uint8_t)(n >> 6) & 63;
        n3 = (uint8_t)n & 63;
        
        /*
         * if we have one byte available, then its encoding is spread
         * out over two characters
         */
        if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
        result[resultIndex++] = base64chars[n0];
        if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
        result[resultIndex++] = base64chars[n1];
        
        /*
         * if we have only two bytes available, then their encoding is
         * spread out over three chars
         */
        if((x+1) < dataLength)
        {
            if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
            result[resultIndex++] = base64chars[n2];
        }
        
        /*
         * if we have all three bytes available, then their encoding is spread
         * out over four characters
         */
        if((x+2) < dataLength)
        {
            if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
            result[resultIndex++] = base64chars[n3];
        }
    }
    
    /*
     * create and add padding that is required if we did not have a multiple of 3
     * number of characters available
     */
    if (padCount > 0)
    {
        for (; padCount < 3; padCount++)
        {
            if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
            result[resultIndex++] = '=';
        }
    }
    if(resultIndex >= resultSize) return 0;   /* indicate failure: buffer too small */
    result[resultIndex] = 0;
    url64encode(result);
    return 1;   /* indicate success */
}

#define WHITESPACE 64
#define EQUALS     65
#define INVALID    66

static const unsigned char d[] = {
    66,66,66,66,66,66,66,66,66,66,64,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,62,66,66,66,63,52,53,
    54,55,56,57,58,59,60,61,66,66,66,65,66,66,66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,66,66,66,66,66,66,26,27,28,
    29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66
};

int base64decode (char *in, size_t inLen, unsigned char *out, size_t *outLen) {
    char *end = in + inLen;
    char iter = 0;
    size_t buf = 0, len = 0;
    
    url64decode(in);
    V17DO printf("base64:dec:");
    V17DO printhex((byte*)in, inLen);
    V17DO printf("\n");

    while (in < end) {
        unsigned char c = d[(int)*in++];
        
        switch (c) {
            case WHITESPACE: continue;   /* skip whitespace */
            case INVALID:    return 0;   /* invalid input, return error */
            case EQUALS:                 /* pad character, end of data */
                in = end;
                continue;
            default:
                buf = buf << 6 | c;
                iter++; // increment the number of iteration
                /* If the buffer is full, split it into bytes */
                if (iter == 4) {
                    if ((len += 3) > *outLen) return 0; /* buffer overflow */
                    *(out++) = (buf >> 16) & 255;
                    *(out++) = (buf >> 8) & 255;
                    *(out++) = buf & 255;
                    buf = 0; iter = 0;
                    
                }
        }
    }
    
    if (iter == 3) {
        if ((len += 2) > *outLen) return 0; /* buffer overflow */
        *(out++) = (buf >> 10) & 255;
        *(out++) = (buf >> 2) & 255;
    }
    else if (iter == 2) {
        if (++len > *outLen) return 0; /* buffer overflow */
        *(out++) = (buf >> 4) & 255;
    }
    
    *outLen = len; /* modify to reflect the actual output size */
    return 1;
}

// base 64 codec end

#include "cJSON.h"

//Encoding structure

//Bytes 00,01 - bit mask of the fields the array contains - bit set - field is present
//Bytes 03 - version character - set to 01 - will only change if we need to update the encding that breaks backward compatibility.
//Bytes 04-160 - encoded data
//  strings are copied
//  number is represented as 2,4,8 or 16 digit hex number (signed so 16bit is +/- 32767 in input/output
//  float is represented as 8 digit hex, double as 16 digit hex

//Encoding values - json strings

#define enc_NONE  0x00  // do nothing just include data as is - ONLY cJSON_String!!!

#define enc_INT8  0x01  // put number into 1 unsigned byte field
#define enc_INT16 0x02  // ditto 2 bytes 
#define enc_INT32 0x04  // ditto 4 bytes
#define enc_INT64 0x08  // bitto 8 bytes

#define enc_TOKEN 0x10  // use custom lookup token (256 to choose from)
#define enc_HEX64 0xF1  // re-encode HEX as ASCII 58

#define enc_FLT   0x40  // put into a float (4 bytes - signed)
#define enc_DBL   0x80  // put into a double (8 bytes - signed)
#define enc_CROP  0xFF  // REMOVE THE FIELD

#define ENC_BYTES_MAX 160



// this is hex
// first two byte is the content field
// this allows for 64 known fields to be encoded.
#define ENC_MASK "0000"

// This tells the encode/decoder how to treat a json field
// ** DO NOT CHANGE THE ORDER OF THIS ARRAY **
// ** WITHOUT VERSIONING THE MESSAGE **
///** ADDING NEW ITEMS TO THE END DOES NOT NEED A NEW VERSION **
struct {
    char* name;
    unsigned int mask;
    unsigned char enctype;
    unsigned char jsontype;
} json_map[] = {
    {"coin", 0x0001, enc_NONE, cJSON_String},
    {"coinaddr",0x0002, enc_NONE, cJSON_String},
    {"cointxid",0x0004, enc_HEX64, cJSON_String},
    {"amount",0x0008, enc_DBL, cJSON_String},
    {"coinv",0x0010,enc_INT16, cJSON_Number},

    {0,0,0} // end-of-list
};

// Custom token list of upto 256 tokens 
struct {
    char* name;
    unsigned char value;
} enc_token[] = {

    {0,0} // end-of-list
};


union value64_t {
    int64_t integer;
    double	real;
};

union value32_t {
    int32_t integer;
    float real;
};

int hexstr2bytes(const char *hex, byte* bytes, size_t byte_size) {
    int byte_count=0;
    size_t len=strlen(hex);
    V17DO printf("hexstr2bytes: ");
    if(len%2) return 0; // has to be an even number of bytes
    for (int i=0; i<len; i+=2, byte_count++) {
        unsigned int value;
        if(sscanf(hex+i,"%2x",&value) <= 0) return 0;
        bytes[byte_count]=value;
        //printf("%)
    }
    V17DO printhex(bytes, byte_count);
    V17DO printf("\n");
    return byte_count;
    
}

int bytes2hexstr(const byte* bytes, size_t byte_count, char *hex, size_t hex_size) {
    int hex_count=0;
    V17DO printf("bytes2hexstr: ");
    if(hex_size<2*byte_count) return 0; // hex array is 2 x size of byte array
    for(int i=0; i<byte_count; i++, hex_count+=2) {
        unsigned int value=bytes[i];
        sprintf(&hex[hex_count],"%02x",value);
    }
    V17DO printf("%s\n",hex);
    return hex_count;
}

//encode v17 msg into original json
cJSON* v17encode(cJSON* in_json) {
    cJSON* ret_json;

//  need to get number of elements in json array
    int json_cnt=cJSON_GetArraySize(in_json);
    unsigned int encMASK=0;
    char encString[2*ENC_BYTES_MAX];
    byte workBytes[2*ENC_BYTES_MAX];
    union value64_t value64;
    union value32_t value32;

    if(!json_cnt) return in_json; // got no json items to process
//  initialise the array
    strcpy(encString, ENC_MASK);
    size_t enc_len=strlen(encString);
    
    for(int i=0, t,n; json_map[i].name; i++) { // this loop must be same in encoder and decoder
	    cJSON *json_item=cJSON_GetObjectItem(in_json, json_map[i].name);
        if(json_item) {
            V17DO printf("=process[%s]\n", json_map[i].name);
            json_cnt-=1;
            encMASK |= json_map[i].mask; // register the json item.
            switch (json_map[i].enctype) {
                case enc_INT8:
                    if(json_map[i].jsontype==cJSON_Number)
                        sprintf((encString+enc_len),"%02x",(unsigned char)json_item->valueint);
                    else sprintf((encString+enc_len),"%02x",(unsigned char)strtol(json_item->valuestring,0,0));
                    V17DO printf("%s",encString+enc_len);
                    enc_len+=2;
                    break;
                case enc_INT16:
                    if(json_map[i].jsontype==cJSON_Number)
                        sprintf((encString+enc_len),"%04x",(uint16_t)json_item->valueint);
                    else sprintf((encString+enc_len),"%04x",(uint16_t)strtol(json_item->valuestring,0,0));
                    V17DO printf("%s",encString+enc_len);
                    enc_len+=4;
                    break;
                case enc_INT32:
                    if(json_map[i].jsontype==cJSON_Number)
                        sprintf((encString+enc_len),"%08x",(uint32_t)json_item->valueint);
                    else sprintf((encString+enc_len),"%08x",(uint32_t)strtol(json_item->valuestring,0,0));
                    V17DO printf("%s",encString+enc_len);
                    enc_len+=8;
                    break;
                case enc_INT64:
                    if (json_map[i].jsontype==cJSON_Number)
                        sprintf((encString+enc_len),"%016lx",(uint64_t)json_item->valueint);
                    else sprintf((encString+enc_len),"%016lx",(uint64_t)strtol(json_item->valuestring,0,0));
                    V17DO printf("%s",encString+enc_len);
                    enc_len+=16;
                    break;
                case enc_TOKEN:
                    for(t=0;enc_token[t].name;t++) {
                        if(!strcmp(enc_token[t].name, json_item->valuestring)) {
                            sprintf((encString+enc_len),"%02x",enc_token[t].value);
                            V17DO printf("%s",encString+enc_len);
                            enc_len+=2;
                            break;
                        }
                    }
                    if(enc_token[t].name==0) return in_json; //cannot encode token not understood
                    break;
                case enc_FLT:
                    if(json_map[i].jsontype==cJSON_Number)
                        value32.real=json_item->valuedouble;
                    else value32.real=atof(json_item->valuestring);
                    sprintf((encString+enc_len),"%08x",value32.integer);
                    V17DO printf("%s",encString+enc_len);
                    enc_len+=8;
                    break;
                case enc_DBL:
                    if(json_map[i].enctype==cJSON_Number)
                        value64.real=json_item->valuedouble;
                    else value64.real=atof(json_item->valuestring);
                    sprintf((encString+enc_len),"%016lx",value64.integer);
                    V17DO printf("%s",encString+enc_len);
                    enc_len+=16;
                    break;
                case enc_HEX64:
                    // +1 adjusts for inclusion of original string length in binary buffer - need this so can recover string.
                    n=hexstr2bytes(json_item->valuestring, workBytes+1, sizeof(workBytes)-1)+1;
                    workBytes[0]=strlen(json_item->valuestring); // store original length of string.
                    if (n>1) {
                        char base64str[sizeof(workBytes)];

                        if(base64encode(workBytes, n, base64str, sizeof(base64str))) {
                            int l=strlen(base64str);
                            V17DO printf("base64 encoded %d bytes ->string[%d]=%s \n", n,l, base64str);
                            sprintf((encString+enc_len),"%02x",l);
                            enc_len+=2;
                            strncpy((encString+enc_len),base64str,l);
                            V17DO printf("%s",encString+enc_len);
                            enc_len+=l;
                        } else {
                            return in_json; //
                        }
                    } else {
                        return in_json; // hex64 encoding failed
                    }                    break;
                case enc_NONE:
                    n = strlen(json_item->valuestring);
                    if(n>0xff) return in_json; // 255 char max
                    sprintf((encString+enc_len),"%02x",n);
                    enc_len+=2;
                    strcat(encString,json_item->valuestring);
                    V17DO printf("%s",encString+enc_len);
                    enc_len+=n;
                    break;
            }
            V17DO printf("\n");
        }
    }
    if(json_cnt) return in_json; // cannot encode - some json not processed.

    if(strlen(encString)>ENC_BYTES_MAX) return in_json; // encoding too long
    
//  Put the content mask into the string for the decoder.
    sprintf((char*)workBytes,"%04x",encMASK);
    strncpy(encString,(char*)workBytes,4);
    
//  if we are here then construct new JSON string containing compressed message
    ret_json=cJSON_CreateObject();
    cJSON_AddStringToObject(ret_json, ENC_JSON_NAME, encString);
    
    cJSON_Delete(in_json); // destroy original json
    return ret_json;
}

//decode jssn msg into shorter v17 msg format
cJSON* v17decode(cJSON* in_json) {
    cJSON* ret_json;
    unsigned int encMask=0;
    int enc_len=0;
    char valuestring[ENC_BYTES_MAX*2];
    union value32_t value32;
    union value64_t value64;
    
    cJSON *mgw_json=cJSON_GetObjectItem(in_json,ENC_JSON_NAME);
    
    if(!mgw_json) {
        V17DO printf("ENCODER:[**NOT MGW JSON**]\n");
        return in_json; // its not our json
    }
    char *v7_json=mgw_json->valuestring;
    if(!v7_json) return in_json; // nothing to do
    sscanf(v7_json,"%04x",&encMask);
 
    enc_len+=4;
    ret_json=cJSON_CreateObject(); // need to delete on all error paths...


    for (int i=0 ; json_map[i].name; i++) { // this loop has to be the same in encoder/decoder
        if(json_map[i].mask&encMask) {
            byte workString[sizeof(valuestring)];
            size_t n;
            V17DO printf("=process[%s]\n", json_map[i].name);
            switch (json_map[i].enctype) {
                    int int8_value;
                    int int16_value;
                case enc_INT8:
                    sscanf((v7_json+enc_len),"%d",&int8_value);
                    if(json_map[i].jsontype==cJSON_Number)
                        cJSON_AddNumberToObject(ret_json, json_map[i].name, int8_value);
                    else {
                        sprintf(valuestring,"%d",int8_value);
                        cJSON_AddStringToObject(ret_json, json_map[i].name, valuestring);
                    }
                    V17DO printf("%2x",int8_value);
                    enc_len+=2;
                    break;
                case enc_INT16:
                    sscanf((v7_json+enc_len),"%04x",&int16_value);
                    if(json_map[i].jsontype==cJSON_Number)
                        cJSON_AddNumberToObject(ret_json, json_map[i].name, int16_value);
                    else {
                        sprintf(valuestring,"%d",int16_value);
                        cJSON_AddStringToObject(ret_json, json_map[i].name, valuestring);
                    }
                    V17DO printf("%d",int16_value);
                    enc_len+=4;
                    break;
                case enc_INT32:
                    sscanf((v7_json+enc_len),"%08x",&value32.integer);
                    if(json_map[i].jsontype==cJSON_Number)
                        cJSON_AddNumberToObject(ret_json, json_map[i].name, value32.integer);
                    else {
                        sprintf(valuestring,"%d",value32.integer);
                        cJSON_AddStringToObject(ret_json, json_map[i].name, valuestring);
                    }
                    V17DO printf("%d",value32.integer);
                    enc_len+=8;
                    break;
                case enc_INT64:
                    sscanf((v7_json+enc_len),"%016lx",&value64.integer);
                    if(json_map[i].jsontype==cJSON_Number)
                        cJSON_AddNumberToObject(ret_json, json_map[i].name, value64.integer);
                    else {
                        sprintf(valuestring,"%ld",value64.integer);
                        cJSON_AddStringToObject(ret_json, json_map[i].name, valuestring);
                    }
                    V17DO printf("%ld",value64.integer);
                    enc_len+=16;
                    break;
                case enc_TOKEN:
                    sscanf((v7_json+enc_len),"%02x",&value32.integer);
                    enc_len+=2;
                    for(int t=0;enc_token[t].name;t++) {
                        if(enc_token[t].value==value32.integer) {
                            V17DO printf("%s", enc_token[t].name);
                            cJSON_AddStringToObject(ret_json, json_map[i].name, enc_token[t].name);
                            break;
                        }
                    }
                    break;
                case enc_FLT:
                    sscanf((v7_json+enc_len),"%08x",&value32.integer);
                    V17DO printf("%.8f", value32.real);
                    if(json_map[i].jsontype==cJSON_Number)
                        cJSON_AddNumberToObject(ret_json, json_map[i].name, value32.real);
                    else {
                        sprintf(valuestring,"%.8f",value32.real);
                        cJSON_AddStringToObject(ret_json, json_map[i].name, valuestring);
                    }
                    enc_len+=8;
                    break;
                case enc_DBL:
                    sscanf((v7_json+enc_len),"%016lx",&value64.integer);
                    V17DO printf("%.8f", value64.real);
                    if(json_map[i].jsontype==cJSON_Number)
                        cJSON_AddNumberToObject(ret_json, json_map[i].name, value64.real);
                    else {
                        sprintf(valuestring,"%.8f",value64.real);
                        cJSON_AddStringToObject(ret_json, json_map[i].name, valuestring);
                    }
                    enc_len+=16;
                    break;
                case enc_HEX64:
                    sscanf((v7_json+enc_len),"%02x",&value32.integer);
                    enc_len+=2;
                    strncpy(valuestring,(v7_json+enc_len), value32.integer);
                    valuestring[value32.integer]=0;
                    n=sizeof(workString);
                    if(base64decode(valuestring, value32.integer, workString, &n)) {
                        enc_len+=value32.integer;
                        value32.integer=workString[0]; //recover original string length
                        if(bytes2hexstr(workString+1, n-1, valuestring, sizeof(valuestring))) {

                            valuestring[value32.integer]=0; // null terminate at original length
                            cJSON_AddStringToObject(ret_json,json_map[i].name,valuestring);
                        } else {
                            V17DO printf("FAILED: BIN->HEX: %s <- [%d]\"%s\"\n", json_map[i].name, value32.integer, valuestring);
                            cJSON_Delete(ret_json); return in_json; }  // hex85 decoding failed
                    } else {
                        V17DO printf("FAILED: B85->BIN: %s <- [%d]\"%s\"\n", json_map[i].name, value32.integer, valuestring);
                        cJSON_Delete(ret_json); return in_json; }// hex85 decoding failed
                    
                    break;
                case enc_NONE:
                    sscanf((v7_json+enc_len),"%02x",&value32.integer);
                    enc_len+=2;
                    strncpy(valuestring,(v7_json+enc_len),value32.integer);
                    enc_len+=value32.integer;
                    valuestring[value32.integer]=0;
                    V17DO printf("%s", valuestring);
                    cJSON_AddStringToObject(ret_json, json_map[i].name, valuestring);
                    break;

            }
            V17DO printf("\n");
        }
    }
    return ret_json;
}


