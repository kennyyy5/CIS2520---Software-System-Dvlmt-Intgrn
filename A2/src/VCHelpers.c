// VCHelpers.c
// Author: Kenny Adenuga, Student ID: 1304431
// Description: Implementation of helper functions for the vCard parser.

#include "VCParser.h"
#include "LinkedListAPI.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ---------- deleteProperty ----------
void deleteProperty(void* toBeDeleted) {
    if (toBeDeleted == NULL) return;
    Property* prop = (Property*)toBeDeleted;
    if (prop->name) free(prop->name);
    if (prop->group) free(prop->group);
    if (prop->parameters) freeList(prop->parameters);
    if (prop->values) freeList(prop->values);
    free(prop);
}

// ---------- compareProperties ----------
// Compare based on property name.
int compareProperties(const void* first, const void* second) {
    Property* prop1 = *(Property**)first;
    Property* prop2 = *(Property**)second;
    return strcmp(prop1->name, prop2->name);
}

// ---------- propertyToString ----------
char* propertyToString(void* prop) {
    if (prop == NULL) return strdup("");
    Property* p = (Property*)prop;
    char* paramsStr = toString(p->parameters);
    char* valuesStr = toString(p->values);
    int size = snprintf(NULL, 0, "\n Group: %s, Name: %s, Parameters: %s, Values: %s", 
                        p->group, p->name, paramsStr ? paramsStr : "", valuesStr ? valuesStr : "") + 1;
    char* result = malloc(size);
    if (result != NULL) {
        snprintf(result, size, "\n Group: %s, Name: %s, Parameters: %s, Values: %s", 
                 p->group, p->name, paramsStr ? paramsStr : "", valuesStr ? valuesStr : "");
    }
    if (paramsStr) free(paramsStr);
    if (valuesStr) free(valuesStr);
    return result;
}

// ---------- deleteParameter ----------
void deleteParameter(void* toBeDeleted) {
    if (toBeDeleted == NULL) return;
    Parameter* param = (Parameter*)toBeDeleted;
    if (param->name) free(param->name);
    if (param->value) free(param->value);
    free(param);
}

// ---------- compareParameters ----------
int compareParameters(const void* first, const void* second) {
    Parameter* p1 = *(Parameter**)first;
    Parameter* p2 = *(Parameter**)second;
    int cmp = strcmp(p1->name, p2->name);
    if (cmp == 0) {
        cmp = strcmp(p1->value, p2->value);
    }
    return cmp;
}

// ---------- parameterToString ----------
char* parameterToString(void* param) {
    if (param == NULL) return strdup("");
    Parameter* p = (Parameter*)param;
    int size = snprintf(NULL, 0, "Name: %s, Value: %s", p->name, p->value) + 1;
    char* result = malloc(size);
    if (result != NULL) {
        snprintf(result, size, "Name: %s, Value: %s", p->name, p->value);
    }
    return result;
}

// ---------- deleteValue ----------
void deleteValue(void* toBeDeleted) {
    if (toBeDeleted == NULL) return;
    free(toBeDeleted);
}

// ---------- compareValues ----------
int compareValues(const void* first, const void* second) {
    char* str1 = *(char**)first;
    char* str2 = *(char**)second;
    return strcmp(str1, str2);
}

// ---------- valueToString ----------
char* valueToString(void* val) {
    if (val == NULL) return strdup("");
    return strdup((char*)val);
}

// ---------- deleteDate ----------
void deleteDate(void* toBeDeleted) {
    if (toBeDeleted == NULL) return;
    DateTime* dt = (DateTime*)toBeDeleted;
    if (dt->date) free(dt->date);
    if (dt->time) free(dt->time);
    if (dt->text) free(dt->text);
    free(dt);
}

// ---------- compareDates ----------
// For now, always return 0 (stub)
int compareDates(const void* first, const void* second) {
    return 0;
}

// ---------- dateToString ----------
char* dateToString(void* date) {
    if (date == NULL) return strdup("");
    DateTime* dt = (DateTime*)date;
    char buffer[256];
    if (dt->isText) {
        snprintf(buffer, sizeof(buffer), "Text: %s", dt->text);
    } else {
        snprintf(buffer, sizeof(buffer), "Date: %s, Time: %s, UTC: %s", 
                 dt->date, dt->time, dt->UTC ? "Yes" : "No");
    }
    return strdup(buffer);
}
