/*

 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at 
 * http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/CDDL.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END

 * Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
 * Use is subject to license terms.

file: arch.cc
created on: Fri Aug 13 11:07:33 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "arch.h"
#include "process.h"
#include "dbg_thread_db.h"
#include "target.h"

bool Architecture::isfpreg(int dwarfnum)
{
	return type_of_register(dwarfnum) == RT_FLOAT;
}
RegisterType Architecture::type_of_register(int dwarfnum)
{
	RegisterSetInfoList &sets = register_properties();

	for (RegisterSetInfoList::iterator i=sets.begin(), e=sets.end() ; i!=e ; i++)
	{
		int num = (*i)->register_number_for_dwarf_number(dwarfnum);
		if (num >= 0)
		{
			if ((*i)->is_integer(num))
			{
				if ((*i)->is_address(num))
				{
					return RT_ADDRESS;
				}
				return RT_INTEGRAL;
			}
			return RT_FLOAT;
		}
	}
	return RT_INVALID;
}
RegisterType Architecture::type_of_register(const std::string &name)
{
	RegisterSetInfoList &sets = register_properties();

	for (RegisterSetInfoList::iterator i=sets.begin(), e=sets.end() ; i!=e ; i++)
	{
		int num = (*i)->register_number_for_name(name);
		if (num >= 0)
		{
			if ((*i)->is_integer(num))
			{
				if ((*i)->is_address(num))
				{
					return RT_ADDRESS;
				}
				return RT_INTEGRAL;
			}
			return RT_FLOAT;
		}
	}
	return RT_INVALID;
}
int Architecture::size_of_register(const std::string &name)
{
	RegisterSetInfoList &sets = register_properties();

	for (RegisterSetInfoList::iterator i=sets.begin(), e=sets.end() ; i!=e ; i++)
	{
		int num = (*i)->register_number_for_name(name);
		if (num >= 0)
		{
			return (*i)->size_of_register();
		}
	}
	return -1;
}
