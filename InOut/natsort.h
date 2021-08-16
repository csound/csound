/**
 * natsort - Copyright (C) 2013 Olivier Brunel
 *
 * sort.h
 * Copyright (C) 2013 Olivier Brunel <i.am.jack.mail@gmail.com>
 *
 * This file is part of natsort.
 *
 * natsort is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * natsort is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * natsort. If not, see http://www.gnu.org/licenses/
 */

#ifndef __DONNA_SORT_H__
#define __DONNA_SORT_H__

typedef enum
{
    DONNA_SORT_NATURAL_ORDER    = (1 << 0),
    DONNA_SORT_CASE_INSENSITIVE = (1 << 1),
    DONNA_SORT_DOT_FIRST        = (1 << 2),
    DONNA_SORT_DOT_MIXED        = (1 << 3),
    DONNA_SORT_IGNORE_SPUNCT    = (1 << 4)
} DonnaSortOptions;

int
strcmp_ext (const char *s1, const char *s2, DonnaSortOptions options);

#endif /* __DONNA_SORT_H__ */
