#ifndef _NTR_XML_H_
#define _NTR_XML_H_ 1
#include <stddef.h>
#include <stdbool.h>

/*
 * DEFINITIONS
 */

union XmlPrimElem {
	long lval;
	bool bval;
	const char *sval;
};

struct XmlElement {
	union {
		struct {
			size_t child_count;
			struct XmlElement *children;
		};
		union XmlPrimElem prim;
	};
	struct XmlElement *parent;
	char name[32];
	bool isValid;
};

/*
 * FUNCTIONS
 */

/** Open file, parse, then return the head element */
struct XmlElement XMLParse(const char *restrict fname);

#endif /* ifndef _NTR_XML_H_ */

