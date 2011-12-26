/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "defs.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32 IPaddr2int(const char *addr, uint32 *num)
{
	uint8 tmp[4];
	uint32 i, byte;
	char *dot, *endp;

	for (i = 0; i < 4; i++)
	{
		if (i == 3)
		{
			dot = (char *)strchr(addr, '\0');
		}
		else
		{
			dot = (char *)strchr(addr, '.');
			if (dot == NULL)
				return -1;
		}

		byte = strtol(addr, &endp, 10);
		if (endp != dot)
			return -1;
		if (byte > 255)
			return -1;
		tmp[i] = byte;

		addr = dot + 1;
	}

	*num = (tmp[0] << 24) + (tmp[1] << 16) + (tmp[2] << 8) + tmp[3];

	return 0;

}

int32 str2int(const char *s, uint32 *num, uint8 base)
{
	uint32 n;
	char *endp;
	

	n = strtoul(s, &endp, base);
	if ((*endp != '\0') || (errno == ERANGE))
		return -1;

	*num = n;

	return 0;

}


string int2str(const uint32 num, uint8 base)
{
	char str[12];
	snprintf(str, 12, "%u", num);
	//ultoa(num, str, base);
	return string(str);
}
