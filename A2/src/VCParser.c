// VCParser.c
// Author: Kenny Adenuga, Student ID: 1304431
// Description: Implementation of vCard parsing functions with detailed error handling.

#include "VCParser.h"
#include "LinkedListAPI.h"
#include <ctype.h>

// ---------- Internal Helper Function Prototypes ----------
static char* readFileToString(const char* fileName);
static char* unfoldLines(const char* fileContent);
static char* trimWhitespace(char* str);
static Property* parseProperty(char* line, int lineNum, VCardErrorCode* err);

// ---------- Implementation of createCard ----------

VCardErrorCode createCard(char* fileName, Card** obj) {
    int lineNum = 0;
    VCardErrorCode retCode = OK;

    // Validate fileName argument.
    if (fileName == NULL || strlen(fileName) == 0) {
        *obj = NULL;
        return INV_FILE;
    }

    // Check file extension: must be .vcf or .vcard
    char* ext = strrchr(fileName, '.');
    if (ext == NULL || (strcasecmp(ext, ".vcf") != 0 && strcasecmp(ext, ".vcard") != 0)) {
        *obj = NULL;
        return INV_FILE;
    }

    // Open file for reading
    FILE* file = fopen(fileName, "r");
    if (!file) {
        *obj = NULL;
        return INV_FILE;
    }
    fclose(file);

    // Read entire file into a string.
    char* fileContent = readFileToString(fileName);
    if (fileContent == NULL) {
        *obj = NULL;
        return OTHER_ERROR;
    }
    
    // --- NEW: Check for proper CRLF line endings ---
    size_t fileLen = strlen(fileContent);
    for (size_t i = 0; i < fileLen; i++) {
        if (fileContent[i] == '\n') {
            // For a valid CRLF, the preceding character must be '\r'
            if (i == 0 || fileContent[i - 1] != '\r') {
                free(fileContent);
                *obj = NULL;
                return INV_CARD;
            }
        }
    }
    // --- End new CRLF check ---

    // Unfold lines (remove CRLF followed by space/tab)
    char* unfolded = unfoldLines(fileContent);
    free(fileContent);
    if (unfolded == NULL) {
        *obj = NULL;
        return OTHER_ERROR;
    }

    // Check for required begin and end tags.
    if (strstr(unfolded, "BEGIN:VCARD") == NULL || strstr(unfolded, "END:VCARD") == NULL) {
        free(unfolded);
        *obj = NULL;
        return INV_CARD;
    }

    // Create a new Card object.
    Card* card = malloc(sizeof(Card));
    if (card == NULL) {
        free(unfolded);
        *obj = NULL;
        return OTHER_ERROR;
    }
    card->fn = NULL;
    card->optionalProperties = initializeList(propertyToString, deleteProperty, compareProperties);
    card->birthday = NULL;
    card->anniversary = NULL;

    // (Rest of your parsing logic follows here...)

    // Process the unfolded content line by line, etc.
        // Process the unfolded content line by line.
    char* saveptr;
    char* line = strtok_r(unfolded, "\n", &saveptr);
    bool versionFound = false;
    while (line != NULL) {
        lineNum++;
        char* trimmed = trimWhitespace(line);
        // Skip empty lines and header/footer.
        if (strlen(trimmed) == 0 ||
            strcmp(trimmed, "BEGIN:VCARD") == 0 ||
            strcmp(trimmed, "END:VCARD") == 0) {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }
        
        // The first non-header line must be VERSION.
        if (!versionFound) {
            if (strncmp(trimmed, "VERSION:", 8) == 0) {
                char* ver = trimmed + 8;
                ver = trimWhitespace(ver);
                if (strcmp(ver, "4.0") != 0 || strlen(ver) == 0) {
                    free(unfolded);
                    deleteCard(card);
                    *obj = NULL;
                    return INV_CARD;
                }
                versionFound = true;
                line = strtok_r(NULL, "\n", &saveptr);
                continue;
            } else {
                free(unfolded);
                deleteCard(card);
                *obj = NULL;
                return INV_CARD;
            }
        }
        
        // Validate that the line has a colon.
        if (strchr(trimmed, ':') == NULL) {
            free(unfolded);
            deleteCard(card);
            *obj = NULL;
            return INV_PROP;
        }
        
        // Parse the property.
        Property* prop = parseProperty(trimmed, lineNum, &retCode);
        if (prop == NULL) {
            free(unfolded);
            deleteCard(card);
            *obj = NULL;
            return retCode;
        }
        
        // Process known properties specially.
        if (strcmp(prop->name, "FN") == 0) {
            if (card->fn == NULL) {
                card->fn = prop;
            } else {
                deleteProperty(prop);
                free(unfolded);
                deleteCard(card);
                *obj = NULL;
                return INV_PROP;
            }
        }
        else if (strcmp(prop->name, "ANNIVERSARY") == 0) {
    // Check for duplicate ANNIVERSARY.
    if (card->anniversary != NULL) {
        deleteProperty(prop);
        free(unfolded);
        deleteCard(card);
        *obj = NULL;
        return INV_PROP;
    }
    DateTime* dt = malloc(sizeof(DateTime));
    if (dt == NULL) {
        deleteProperty(prop);
        free(unfolded);
        deleteCard(card);
        *obj = NULL;
        return OTHER_ERROR;
    }
    
    // Check if the ANNIVERSARY property has a parameter VALUE=text.
    bool isText = false;
    ListIterator iter = createIterator(prop->parameters);
    Parameter* currParam;
    while ((currParam = nextElement(&iter)) != NULL) {
        if (strcasecmp(currParam->name, "VALUE") == 0 &&
            strcasecmp(currParam->value, "text") == 0) {
            isText = true;
            break;
        }
    }
    
    // Get the first value from the property’s values list.
    char* dtStr = getFromFront(prop->values);
    if (isText) {
        // If VALUE=text, mark the DateTime as text.
        dt->isText = true;
        dt->text = strdup(dtStr);
        dt->date = strdup("");
        dt->time = strdup("");
    } else {
        // Otherwise, assume a standard date-and-time format.
        dt->isText = false;
        dt->text = strdup("");
        char* tPos = strchr(dtStr, 'T');
        if (tPos != NULL) {
            size_t dateLen = tPos - dtStr;
            dt->date = malloc(dateLen + 1);
            if (dt->date == NULL) {
                free(dt);
                deleteProperty(prop);
                free(unfolded);
                deleteCard(card);
                *obj = NULL;
                return OTHER_ERROR;
            }
            strncpy(dt->date, dtStr, dateLen);
            dt->date[dateLen] = '\0';
            dt->time = strdup(tPos + 1);
        } else {
            dt->date = strdup(dtStr);
            dt->time = strdup("");
        }
    }
    dt->UTC = false;  // Set UTC appropriately if needed.
    card->anniversary = dt;
    // Once processed, free the property structure.
    deleteProperty(prop);
}

        else if (strcmp(prop->name, "BDAY") == 0) {
    // If a birthday was already set, that's an error.
    if (card->birthday != NULL) {
        deleteProperty(prop);
        free(unfolded);
        deleteCard(card);
        *obj = NULL;
        return INV_PROP;
    }
    DateTime* dt = malloc(sizeof(DateTime));
    if (dt == NULL) {
        deleteProperty(prop);
        free(unfolded);
        deleteCard(card);
        *obj = NULL;
        return OTHER_ERROR;
    }

    // Check if the BDAY property has a parameter VALUE=text.
    bool isText = false;
    ListIterator iter = createIterator(prop->parameters);
    Parameter* currParam;
    while ((currParam = nextElement(&iter)) != NULL) {
        if (strcasecmp(currParam->name, "VALUE") == 0 &&
            strcasecmp(currParam->value, "text") == 0) {
            isText = true;
            break;
        }
    }

    // Get the first (and should be only) value from the BDAY property.
    char* dtStr = getFromFront(prop->values);
    if (isText) {
        // For text values, set isText true and copy the text.
        dt->isText = true;
        dt->text = strdup(dtStr);
        dt->date = strdup("");
        dt->time = strdup("");
    } else {
        // Otherwise, assume the value is a standard date/time.
        dt->isText = false;
        dt->text = strdup("");
        char* tPos = strchr(dtStr, 'T');
        if (tPos != NULL) {
            size_t dateLen = tPos - dtStr;
            dt->date = malloc(dateLen + 1);
            if (dt->date == NULL) {
                free(dt);
                deleteProperty(prop);
                free(unfolded);
                deleteCard(card);
                *obj = NULL;
                return OTHER_ERROR;
            }
            strncpy(dt->date, dtStr, dateLen);
            dt->date[dateLen] = '\0';
            dt->time = strdup(tPos + 1);
        } else {
            dt->date = strdup(dtStr);
            dt->time = strdup("");
        }
    }
    dt->UTC = false;  // Set as appropriate (for now, false)
    card->birthday = dt;
    // Once the BDAY property is processed into a DateTime, free its property structure.
    deleteProperty(prop);
}

        else {
            // Other properties are added to the optional properties list.
            insertBack(card->optionalProperties, prop);
        }
        
        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(unfolded);
    
    // Final check: ensure that required properties have been set.
    if (!versionFound || card->fn == NULL) {
        deleteCard(card);
        *obj = NULL;
        return INV_CARD;
    }
    
    *obj = card;
    return OK;

}


// ---------- Helper function: readFileToString ----------
static char* readFileToString(const char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);

    char* string = malloc(fsize + 1);
    if (string == NULL) {
        fclose(file);
        return NULL;
    }

    size_t readSize = fread(string, 1, fsize, file);
    fclose(file);
    string[readSize] = '\0';
    return string;
}

// ---------- Helper function: unfoldLines ----------
// Removes CRLF followed by a space or tab.
static char* unfoldLines(const char* fileContent) {
    size_t len = strlen(fileContent);
    char* unfolded = malloc(len + 1);  // Allocate enough space (folding will only reduce length)
    if (unfolded == NULL) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        // Check for CRLF or LF followed by a space or tab (folded line)
        if ((i + 1 < len && fileContent[i] == '\r' && fileContent[i+1] == '\n') ||
            (fileContent[i] == '\n')) {
            
            size_t nextChar = i + ((fileContent[i] == '\r') ? 2 : 1); // Skip CRLF (2) or LF (1)
            
            // If the next character is a space or tab, remove the newline completely (folded line)
            if (nextChar < len && (fileContent[nextChar] == ' ' || fileContent[nextChar] == '\t')) {
                i = nextChar + 1; // Skip the newline and leading whitespace
                continue; // Don't insert a space, just merge the next word
            } 
        }
        
        // Copy character as normal
        unfolded[j++] = fileContent[i++];
    }
    unfolded[j] = '\0'; // Null-terminate the result
    return unfolded;
}


// ---------- Helper function: trimWhitespace ----------
static char* trimWhitespace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// ---------- Helper function: parseProperty ----------
// Parses a property line into a Property struct. Returns NULL if the property is invalid.
// If an error occurs, *err is set to an appropriate error code.
static Property* parseProperty(char* line, int lineNum, VCardErrorCode* err) {
    // Allocate a new Property structure.
    Property* prop = malloc(sizeof(Property));
    if (prop == NULL) {
        *err = OTHER_ERROR;
        return NULL;
    }
    prop->group = strdup("");
    prop->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    prop->values = initializeList(valueToString, deleteValue, compareValues);

    // Split the line at the first colon.
    char* colonPos = strchr(line, ':');
    if (colonPos == NULL) {
        free(prop);
        *err = INV_PROP;
        return NULL;
    }
    *colonPos = '\0';
    char* preamble = line;         // Contains property name and optional parameters.
    char* valuePart = colonPos + 1;  // Contains the property values.

    // Check for group (indicated by a dot).
    char* dotPos = strchr(preamble, '.');
    if (dotPos != NULL) {
        *dotPos = '\0';
        free(prop->group);
        prop->group = strdup(preamble);
        preamble = dotPos + 1;
    }

    // The property name is the first token in the preamble (delimited by ';').
    char* token = strtok(preamble, ";");
    if (token == NULL || strlen(token) == 0) {
        deleteProperty(prop);
        *err = INV_PROP;
        return NULL;
    }
    prop->name = strdup(token);

    // Process parameters (if any). Each parameter must be in the form name=value.
    token = strtok(NULL, ";");
    while (token != NULL) {
        char* equalPos = strchr(token, '=');
        if (equalPos == NULL || *(equalPos + 1) == '\0') {
            deleteProperty(prop);
            *err = INV_PROP;
            return NULL;
        }
        *equalPos = '\0';
        Parameter* param = malloc(sizeof(Parameter));
        if (param == NULL) {
            deleteProperty(prop);
            *err = OTHER_ERROR;
            return NULL;
        }
        param->name = strdup(token);
        param->value = strdup(equalPos + 1);
        insertBack(prop->parameters, param);
        token = strtok(NULL, ";");
    }

    // --- NEW VALUE SPLITTING LOGIC ---
    // Instead of using strtok (which skips empty tokens), we scan valuePart manually.
    {
        char* tokenStart = valuePart;
        char* semicolonPos = NULL;
        // Loop to find each semicolon and extract the token between delimiters.
        while ((semicolonPos = strchr(tokenStart, ';')) != NULL) {
            size_t tokenLength = semicolonPos - tokenStart;
            char* tokenVal = malloc(tokenLength + 1);
            if (tokenVal == NULL) {
                deleteProperty(prop);
                *err = OTHER_ERROR;
                return NULL;
            }
            strncpy(tokenVal, tokenStart, tokenLength);
            tokenVal[tokenLength] = '\0';
            insertBack(prop->values, tokenVal);
            tokenStart = semicolonPos + 1;
        }
        // Add the final token (which might be empty).
        char* tokenVal = strdup(tokenStart);
        if (tokenVal == NULL) {
            deleteProperty(prop);
            *err = OTHER_ERROR;
            return NULL;
        }
        insertBack(prop->values, tokenVal);
    }
    // --- END NEW VALUE SPLITTING LOGIC ---

    return prop;
}

// ---------- Implementation of cardToString ----------

char* cardToString(const Card* obj) {
    if (obj == NULL) {
        return strdup("null");
    }

    // Convert the FN property to a string.
    char* fnStr = propertyToString(obj->fn);
    
    // Convert the list of optional properties to a string.
    // The toString() function provided by the LinkedListAPI will use the print function
    // (in this case, propertyToString) for each element.
    char* optPropsStr = toString(obj->optionalProperties);
    
    // Convert birthday and anniversary if they exist; otherwise, show "None".
    char* bdayStr = (obj->birthday != NULL) ? dateToString(obj->birthday) : strdup("None");
    char* annivStr = (obj->anniversary != NULL) ? dateToString(obj->anniversary) : strdup("None");

    // Allocate a buffer large enough for the final string.
    // Here we use a fixed-size buffer for simplicity.
    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
             "FN: %s\n"
             "Optional Properties:\n%s\n"
             "Birthday: %s\n"
             "Anniversary: %s",
             fnStr, optPropsStr, bdayStr, annivStr);

    // Free the temporary strings.
    free(fnStr);
    free(optPropsStr);
    free(bdayStr);
    free(annivStr);

    // Return a dynamically allocated copy of the result.
    return strdup(buffer);
}


// ---------- Implementation of deleteCard ----------

void deleteCard(Card* obj) {
    if (obj == NULL) return;
    if (obj->fn) {
        deleteProperty(obj->fn);
    }
    if (obj->optionalProperties) {
        freeList(obj->optionalProperties);
    }
    if (obj->birthday) {
        deleteDate(obj->birthday);
    }
    if (obj->anniversary) {
        deleteDate(obj->anniversary);
    }
    free(obj);
}

// ---------- Implementation of errorToString ----------

char* errorToString(VCardErrorCode err) {
    const char* errStr;
    switch(err) {
        case OK: errStr = "OK"; break;
        case INV_FILE: errStr = "Invalid File"; break;
        case INV_CARD: errStr = "Invalid Card"; break;
        case INV_PROP: errStr = "Invalid Property"; break;
        case INV_DT: errStr = "Invalid DateTime"; break;
        case WRITE_ERROR: errStr = "Write Error"; break;
        case OTHER_ERROR: errStr = "Other Error"; break;
        default: errStr = "Invalid error code"; break;
    }
    return strdup(errStr);
}
