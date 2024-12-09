#include "../inc/ex02.hpp"

/*
* Convert all lowercase characters in the string to uppercase.
*/
string convertUp(string& str) {
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = toupper(str[i]);
        }
    }
    return str;
}

/*
* Convert all uppercase characters in the string to lowercase.
*/
string convertDown(string& str) {
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = tolower(str[i]);
        }
    }
    return str;
}
