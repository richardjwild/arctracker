/* Copyright (c) Richard Wild 2004, 2005                                   *
 *                                                                         *
 * This file is part of Arctracker.                                        *
 *                                                                         *
 * Arctracker is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * Arctracker is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with Arctracker; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MS 02111-1307 USA */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include "arctracker.h"
#include "config.h"
#include "error.h"
#include "configuration.h"

size_t file_size(int fd)
{
    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1)
        system_error("Error reading file status");
    return (size_t) statbuf.st_size;
}

void load_file(void **addr, size_t *bytes_loaded)
{
	FILE *fp = fopen(configuration().mod_filename, READONLY);
	if (fp != NULL)
    {
        int fd = fileno(fp);
        *bytes_loaded = file_size(fd);
        *addr = mmap(NULL, *bytes_loaded, PROT_READ, MAP_SHARED, fd, 0);
    }
    else
        error("Cannot open file.");
}
