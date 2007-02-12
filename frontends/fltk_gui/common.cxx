// COPY A STRING WITH 'new'
//    Value can be NULL
//
static char *strnew(const char *val) {
    if ( val == NULL ) return(NULL);
    char *s = new char[strlen(val)+1];
    strcpy(s, val);
    return(s);
}

// FREE STRING CREATED WITH strnew(), NULLS OUT STRING
//    Value can be NULL
//
static char *strfree(char *val) {
    if ( val ) delete [] val;
    return(NULL);
}

// 'DYNAMICALLY' APPEND ONE STRING TO ANOTHER
//    Returns newly allocated string, or NULL 
//    if s && val == NULL.
//    's' can be NULL; returns a strnew(val).
//    'val' can be NULL; s is returned unmodified.
//
//    Usage:
//	char *s = strnew("foo");	// s = "foo"
//      s = strapp(s, "bar");		// s = "foobar"
//
static char *strapp(char *s, const char *val) {
    if ( ! val ) {
        return(s);              // Nothing to append? return s
    }
    if ( ! s ) {
        return(strnew(val));    // New string? return copy of val
    }
    char *news = new char[strlen(s)+strlen(val)+1];
    strcpy(news, s);
    strcat(news, val);
    delete [] s;		// delete old string
    return(news);		// return new copy
}

// APPEND A CHARACTER TO A STRING
static void chrcat(char *s, char c) {
    char tmp[2] = { c, '\0' };;
    strcat(s, tmp);
}


