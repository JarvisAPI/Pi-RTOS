//
// util.cpp
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <util.h>


size_t strlen (const char *pString)
{
	size_t nResult = 0;

	while (*pString++)
	{
		nResult++;
	}

	return nResult;
}

int strcmp (const char *pString1, const char *pString2)
{
	while (   *pString1 != '\0'
	       && *pString2 != '\0')
	{
		if (*pString1 > *pString2)
		{
			return 1;
		}
		else if (*pString1 < *pString2)
		{
			return -1;
		}

		pString1++;
		pString2++;
	}

	if (*pString1 > *pString2)
	{
		return 1;
	}
	else if (*pString1 < *pString2)
	{
		return -1;
	}

	return 0;
}

char *strcpy (char *pDest, const char *pSrc)
{
	char *p = pDest;

	while (*pSrc)
	{
		*p++ = *pSrc++;
	}

	*p = '\0';

	return pDest;
}

char *strncpy (char *pDest, const char *pSrc, size_t nMaxLen)
{
	char *pResult = pDest;

	while (nMaxLen > 0)
	{
		if (*pSrc == '\0')
		{
			break;
		}

		*pDest++ = *pSrc++;
		nMaxLen--;
	}

	if (nMaxLen > 0)
	{
		*pDest = '\0';
	}

	return pResult;
}

char *strcat (char *pDest, const char *pSrc)
{
	char *p = pDest;

	while (*p)
	{
		p++;
	}

	while (*pSrc)
	{
		*p++ = *pSrc++;
	}

	*p = '\0';

	return pDest;
}

int char2int (char chValue)
{
	int nResult = chValue;

	if (nResult > 0x7F)
	{
		nResult |= -0x100;
	}

	return nResult;
}

uint16_t le2be16 (uint16_t usValue)
{
	return    ((usValue & 0x00FF) << 8)
		| ((usValue & 0xFF00) >> 8);
}

uint32_t le2be32 (uint32_t ulValue)
{
	return    ((ulValue & 0x000000FF) << 24)
		| ((ulValue & 0x0000FF00) << 8)
		| ((ulValue & 0x00FF0000) >> 8)
		| ((ulValue & 0xFF000000) >> 24);
}

int atoi( const char* str, size_t max_len )
{
    int ret = 0;

    // TODO: Check ASCCII
    for (size_t i = 0; str[i] != '\0' && i < max_len; i++ )
        ret = ret*10 + str[i] - '0';

    return ret;

}
