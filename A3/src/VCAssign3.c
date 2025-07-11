#include "VCParser.h"
#include "LinkedListAPI.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * Convert the FN (formatted name) property of a Card to a string.
 */
char* fnToString(const Card* obj) {
    if (obj == NULL || obj->fn == NULL || obj->fn->values == NULL || obj->fn->values->head == NULL) {
        return strdup("null");
    }
    
    // Get the first value of the FN property.
    char* value = (char*)obj->fn->values->head->data;
    if (value == NULL) {
        return strdup("null");
    }

    // Return a duplicate of the name to avoid modifying original data.
    return strdup(value);
}


/**
 * Convert the birthday DateTime property to a string.
 * If no birthday is provided, returns "None".
 */
char* bdayToString(const Card* obj) {
    if (obj == NULL) {
        return strdup("null");
    }
    char* bdayStr = (obj->birthday != NULL) ? dateToString(obj->birthday) : strdup("None");
    return bdayStr;
}

/**
 * Convert the anniversary DateTime property to a string.
 * If no anniversary is provided, returns "None".
 */
char* annToString(const Card* obj) {
    if (obj == NULL) {
        return strdup("null");
    }
    char* annStr = (obj->anniversary != NULL) ? dateToString(obj->anniversary) : strdup("None");
    return annStr;
}

/**
 * Convert the number of optional properties into a string.
 */
char* numPropsToString(const Card* obj) {
    if (obj == NULL) {
        return strdup("null");
    }
    int length = getLength(obj->optionalProperties);
    char *result = malloc(32); // Allocate a buffer large enough for the string representation.
    if (result != NULL) {
        snprintf(result, 32, "%d", length);
    }
    return result;
}

/**
 * Create a minimal Card object.
 * This function allocates a Card, initializes the mandatory FN property,
 * and sets optionalProperties to an empty list.
 */
VCardErrorCode createMinimalCard(Card** card, char * fn) {
    if (card == NULL) {
        return INV_FILE;
    }
    // If no formatted name is provided, use a default.
    if (fn == NULL) {
        fn = "Default Name";
    }
    
    Card* newCard = malloc(sizeof(Card));
    if (!newCard)
        return OTHER_ERROR;
    
    // Create and initialize the mandatory FN property.
    newCard->fn = malloc(sizeof(Property));
    if (!newCard->fn) {
        free(newCard);
        return OTHER_ERROR;
    }
    newCard->fn->name = strdup("FN");
    newCard->fn->group = strdup("");
    if (newCard->fn->name == NULL || newCard->fn->group == NULL) {
        free(newCard->fn);
        free(newCard);
        return OTHER_ERROR;
    }
    
    // Create linked lists for parameters and values.
    newCard->fn->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    newCard->fn->values = initializeList(valueToString, deleteValue, compareValues);
    if (newCard->fn->parameters == NULL || newCard->fn->values == NULL) {
        free(newCard->fn->name);
        free(newCard->fn->group);
        free(newCard->fn);
        free(newCard);
        return OTHER_ERROR;
    }
    
    // Insert the FN value (formatted name) into the FN property's values list.
    char* fn_dup = strdup(fn);
    if (fn_dup == NULL) {
        // Cleanup omitted for brevity.
        return OTHER_ERROR;
    }
    insertBack(newCard->fn->values, fn_dup);
    
    // Initialize optionalProperties as an empty list.
    newCard->optionalProperties = initializeList(propertyToString, deleteProperty, compareProperties);
    
    // Minimal card does not have birthday or anniversary.
    newCard->birthday = NULL;
    newCard->anniversary = NULL;
    
    *card = newCard;
    return OK;
}

VCardErrorCode editMinimalCard(Card** card, char* fn) {
    if (card == NULL || *card == NULL) {
        return INV_FILE;  // Invalid pointer passed
    }

    // If no formatted name is provided, use a default.
    if (fn == NULL) {
        fn = "Default Name";
    }

    // Check if the FN property exists in the card.
    if ((*card)->fn == NULL) {
        // Allocate memory for FN property if it doesn't exist
        (*card)->fn = malloc(sizeof(Property));
        if ((*card)->fn == NULL) {
            return OTHER_ERROR;
        }

        // Initialize FN property
        (*card)->fn->name = strdup("FN");
        if ((*card)->fn->name == NULL) {
            free((*card)->fn);  
            (*card)->fn = NULL;
            return OTHER_ERROR;
        }
        
        // Create new list for FN values
        (*card)->fn->values = initializeList(&valueToString, &deleteValue, &compareValues);

    }

    // Clear previous FN values to avoid memory leaks.
    if ((*card)->fn->values != NULL) {
        clearList((*card)->fn->values);
    }

    // Insert the new FN value (formatted name) into the FN property's values list.
    char* fn_dup = strdup(fn);
    if (fn_dup == NULL) {
        return OTHER_ERROR;  // Memory allocation failed
    }

    insertBack((*card)->fn->values, fn_dup);
    return OK;
}

