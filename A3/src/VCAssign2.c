#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VCParser.h"      
#include "LinkedListAPI.h" 

VCardErrorCode writeCard(const char *fileName, const Card *obj) {
    if (fileName == NULL || obj == NULL){
        return WRITE_ERROR;}
    
    FILE *fp = fopen(fileName, "w");
    if (fp == NULL){
        return WRITE_ERROR;}
    
    // Write BEGIN:VCARD and VERSION:4.0 lines.
    if (fprintf(fp, "BEGIN:VCARD\r\n") < 0) {
        fclose(fp);
        return WRITE_ERROR;
    }
    if (fprintf(fp, "VERSION:4.0\r\n") < 0) {
        fclose(fp);
        return WRITE_ERROR;
    }
    
    // Write the mandatory FN property.
    Property *fnProp = obj->fn;
  

    if (fnProp->group && fnProp->group[0] != '\0') {
        if (fprintf(fp, "%s.", fnProp->group) < 0) {
            fclose(fp);
            return WRITE_ERROR;
        }
    }
    if (fprintf(fp, "%s", fnProp->name) < 0) {
        fclose(fp);
        return WRITE_ERROR;
    }
    
    
        ListIterator paramIter = createIterator(fnProp->parameters);
        Parameter *param;
        while ((param = (Parameter *) nextElement(&paramIter)) != NULL) {
            if (fprintf(fp, ";%s=%s", param->name, param->value) < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
        }
    
    
    if (fprintf(fp, ":") < 0) {
        fclose(fp);
        return WRITE_ERROR;
    }
    
    
        ListIterator valIter = createIterator(fnProp->values);
        char *val;
        int firstVal = 1;
        while ((val = (char *) nextElement(&valIter)) != NULL) {
            if (!firstVal) {
                if (fprintf(fp, ";") < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
            }
            firstVal = 0;
            if (fprintf(fp, "%s", val) < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
        }
    
    
    if (fprintf(fp, "\r\n") < 0) {
        fclose(fp);
        return WRITE_ERROR;
    }
    
    // First pass: Write the "N" property (if present) from the optionalProperties list.
    
        ListIterator iter1 = createIterator(obj->optionalProperties);
        Property *prop;
        while ((prop = (Property *) nextElement(&iter1)) != NULL) {
            if (strcmp(prop->name, "N") == 0) {
                if (prop->group && prop->group[0] != '\0') {
                    if (fprintf(fp, "%s.", prop->group) < 0) {
                        fclose(fp);
                        return WRITE_ERROR;
                    }
                }
                if (fprintf(fp, "%s", prop->name) < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
                {
                    ListIterator iter2 = createIterator(prop->parameters);
                    Parameter *p;
                    while ((p = (Parameter *) nextElement(&iter2)) != NULL) {
                        if (fprintf(fp, ";%s=%s", p->name, p->value) < 0) {
                            fclose(fp);
                            return WRITE_ERROR;
                        }
                    }
                }
                if (fprintf(fp, ":") < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
                
                    ListIterator valIter2 = createIterator(prop->values);
                    char *v;
                    int first2 = 1;
                    while ((v = (char *) nextElement(&valIter2)) != NULL) {
                        if (!first2) {
                            if (fprintf(fp, ";") < 0) {
                                fclose(fp);
                                return WRITE_ERROR;
                            }
                        }
                        first2 = 0;
                        if (fprintf(fp, "%s", v) < 0) {
                            fclose(fp);
                            return WRITE_ERROR;
                        }
                    }
                
                if (fprintf(fp, "\r\n") < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
            }
        }
    
    
    // Write birthday from dedicated field, if present.
    if (obj->birthday != NULL) {
        DateTime *dt = obj->birthday;
        if (fprintf(fp, "BDAY") < 0) {
            fclose(fp);
            return WRITE_ERROR;
        }
        if (dt->isText) {
            if (fprintf(fp, ";VALUE=text") < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
        }
        if (fprintf(fp, ":") < 0) {
            fclose(fp);
            return WRITE_ERROR;
        }
        if (dt->isText) {
            if (fprintf(fp, "%s", dt->text) < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
        } else {
            if (fprintf(fp, "%s", dt->date) < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
            if (dt->time && dt->time[0] != '\0') {
                if (fprintf(fp, "T%s", dt->time) < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
            }
        }
        if (fprintf(fp, "\r\n") < 0) {
            fclose(fp);
            return WRITE_ERROR;
        }
    }
    
    // Write anniversary from dedicated field, if present.
    if (obj->anniversary != NULL) {
        DateTime *dt = obj->anniversary;
        if (fprintf(fp, "ANNIVERSARY") < 0) {
            fclose(fp);
            return WRITE_ERROR;
        }
        if (dt->isText) {
            if (fprintf(fp, ";VALUE=text") < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
        }
        if (fprintf(fp, ":") < 0) {
            fclose(fp);
            return WRITE_ERROR;
        }
        if (dt->isText) {
            if (fprintf(fp, "%s", dt->text) < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
        } else {
            if (fprintf(fp, "%s", dt->date) < 0) {
                fclose(fp);
                return WRITE_ERROR;
            }
            if (dt->time && dt->time[0] != '\0') {
                if (fprintf(fp, "T%s", dt->time) < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
            }
        }
        if (fprintf(fp, "\r\n") < 0) {
            fclose(fp);
            return WRITE_ERROR;
        }
    }
    
   
        ListIterator iter3 = createIterator(obj->optionalProperties);
        Property *propRem;
        while ((propRem = (Property *) nextElement(&iter3)) != NULL) {
            if (strcmp(propRem->name, "N") != 0) {
                if (propRem->group && propRem->group[0] != '\0') {
                    if (fprintf(fp, "%s.", propRem->group) < 0) {
                        fclose(fp);
                        return WRITE_ERROR;
                    }
                }
                if (fprintf(fp, "%s", propRem->name) < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
                
                    ListIterator iter4 = createIterator(propRem->parameters);
                    Parameter *p3;
                    while ((p3 = (Parameter *) nextElement(&iter4)) != NULL) {
                        if (fprintf(fp, ";%s=%s", p3->name, p3->value) < 0) {
                            fclose(fp);
                            return WRITE_ERROR;
                        }
                    }
                
                if (fprintf(fp, ":") < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
                
                    ListIterator iterVal3 = createIterator(propRem->values);
                    char *v3;
                    int first3 = 1;
                    while ((v3 = (char *) nextElement(&iterVal3)) != NULL) {
                        if (!first3) {
                            if (fprintf(fp, ";") < 0) {
                                fclose(fp);
                                return WRITE_ERROR;
                            }
                        }
                        first3 = 0;
                        if (fprintf(fp, "%s", v3) < 0) {
                            fclose(fp);
                            return WRITE_ERROR;
                        }
                    }
                
                if (fprintf(fp, "\r\n") < 0) {
                    fclose(fp);
                    return WRITE_ERROR;
                }
            
        }
    }
    
    // Write END:VCARD.
    if (fprintf(fp, "END:VCARD\r\n") < 0) {
        fclose(fp);
        return WRITE_ERROR;
    }
    
    fclose(fp);
    return OK;
}



VCardErrorCode validateCard(const Card *obj) {
    // Allowed property names (Sections 6.1–6.9.3)
    const char *allowedProps[] = {
        "BEGIN", "END", "SOURCE", "KIND", "XML", "FN", "ORG", "N", "NICKNAME",
        "PHOTO", "BDAY", "ANNIVERSARY", "GENDER", "ADR", "TEL", "EMAIL", "IMPP",
        "LANG", "TZ", "GEO", "TITLE", "ROLE", "LOGO", "MEMBER", "RELATED",
        "CATEGORIES", "NOTE", "PRODID", "REV", "SOUND", "UID", "CLIENTPIDMAP",
        "URL", "VERSION", "KEY", "FBURL", "CALURI", "CALADRURI"
    };
    int numAllowed = sizeof(allowedProps) / sizeof(allowedProps[0]);
    
    int i, isAllowed;
    char *val;
    Parameter *param;

    if (obj == NULL || obj->fn == NULL){
        return INV_CARD;}
    
    Property *fn = obj->fn;
    if (fn->name == NULL || strlen(fn->name) == 0  || fn->group==NULL){
        return INV_PROP;}
    
    isAllowed = 0;
    for (i = 0; i < numAllowed; i++) {
        if (strcasecmp(fn->name, allowedProps[i]) == 0) { isAllowed = 1; break; }
    }
    if (!isAllowed){
        return INV_PROP;}
    
    if (fn->values == NULL || getLength(fn->values) == 0){
        return INV_PROP;}
    ListIterator iterVal = createIterator(fn->values);
    while ((val = (char *) nextElement(&iterVal)) != NULL) {
        if (val == NULL){
            return INV_PROP;}
    }
  
    if (fn->parameters == NULL){
        return INV_PROP;}
    ListIterator iterParam = createIterator(fn->parameters);
    while ((param = (Parameter *) nextElement(&iterParam)) != NULL) {
        if (param == NULL){
            return INV_PROP;}
        if (param->name == NULL || strlen(param->name) == 0){
            return INV_PROP;}
        if (param->value == NULL || strlen(param->value) == 0){
            return INV_PROP;}
    }
    
    if (obj->optionalProperties == NULL){
        return INV_CARD;}
    
    // Counters for properties that must be unique.
    int countN = 0;
    int countVersion = 0;
  
    ListIterator iterProp = createIterator(obj->optionalProperties);
    Property *prop;
    while ((prop = (Property *) nextElement(&iterProp)) != NULL) {
       
        if (prop->name == NULL || strlen(prop->name) == 0 || prop->group==NULL){
            return INV_PROP;}
  
        
        isAllowed = 0;
        for (i = 0; i < numAllowed; i++) {
            if (strcasecmp(prop->name, allowedProps[i]) == 0) { isAllowed = 1; break; }
        }
        if (!isAllowed){
            return INV_PROP;}
        // Validate that the values list exists and has at least one value.
        if (prop->values == NULL || getLength(prop->values) == 0){
            return INV_PROP;}
        ListIterator iterVal2 = createIterator(prop->values);
        while ((val = (char *) nextElement(&iterVal2)) != NULL) {
            if (val == NULL){
                return INV_PROP;}
        }
      
        if (prop->parameters == NULL){
            return INV_PROP;}
        ListIterator iterParam2 = createIterator(prop->parameters);
        while ((param = (Parameter *) nextElement(&iterParam2)) != NULL) {
            if (param == NULL){
                return INV_PROP;}
            if (param->name == NULL || strlen(param->name) == 0){
                return INV_PROP;}
            if (param->value == NULL || strlen(param->value) == 0){
                return INV_PROP;}
        }
      
        if (strcasecmp(prop->name, "VERSION") == 0){
            countVersion++;}
        if (strcasecmp(prop->name, "N") == 0) {
            countN++;
            // The N property must have exactly 5 values.
            if (getLength(prop->values) != 5){
                return INV_PROP;}
        }
     
        
        if (strcasecmp(prop->name, "BDAY") == 0 || strcasecmp(prop->name, "ANNIVERSARY") == 0){
            return INV_DT;}
    }
    if (countVersion > 0){
        return INV_CARD;} 
    if (countN > 1){
        return INV_PROP;} 

    
   
        ListIterator outer = createIterator(obj->optionalProperties);
        Property *outerProp;
        while ((outerProp = (Property *) nextElement(&outer)) != NULL) {
            ListIterator inner = createIterator(obj->optionalProperties);
            Property *innerProp;
            while ((innerProp = (Property *) nextElement(&inner)) != NULL) {
                if (outerProp == innerProp){
                    continue;}
                if (strcasecmp(outerProp->name, innerProp->name) == 0){
                    return INV_PROP;}
            }
        }
    
    
    if (obj->birthday != NULL) {
        DateTime *dt = obj->birthday;
        if (dt->date == NULL || dt->time == NULL || dt->text == NULL){
            return INV_DT;}
        if (dt->isText) {
            if (strlen(dt->date) != 0 || strlen(dt->time) != 0){
                return INV_DT;}
            if (dt->UTC != 0){
                return INV_DT;}
        } else {
            if (strlen(dt->date) == 0){
                return INV_DT;}
            if (strlen(dt->text) != 0){
                return INV_DT;}
        }
    }
    if (obj->anniversary != NULL) {
        DateTime *dt = obj->anniversary;
        if (dt->date == NULL || dt->time == NULL || dt->text == NULL){
            return INV_DT;}
        if (dt->isText) {
            if (strlen(dt->date) != 0 || strlen(dt->time) != 0){
                return INV_DT;}
            if (dt->UTC != 0){
                return INV_DT;}
        } else {
            if (strlen(dt->date) == 0){
                return INV_DT;}
            if (strlen(dt->text) != 0){
                return INV_DT;}
        }
    }
    
    return OK;
}
