#include "ustub.h"

int main(int argc, char **argv)
{
  init_getstring(0, NULL);
	if(argc < 3)
	{
		dnoise_usage(1);
	}
	return dnoise(argc, argv);
}
