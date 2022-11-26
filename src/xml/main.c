#include "pxml.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
	struct XmlElement head = XMLParse("bar.xml");
	if(!head.isValid) {
		return -1;
	}
	return 0;
}
