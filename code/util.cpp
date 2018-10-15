/* Copyright (C) 2018 Bho Matthiesen
 * 
 * This program is used in the article:
 * 
 * Bho Matthiesen and Eduard Jorswieck, "Efficient Global Optimal Resource
 * Allocation in Non-Orthogonal Interference Networks," submitted to IEEE
 * Transactions on Signal Processing.
 * 
 * 
 * License:
 * This program is licensed under the GPLv2 license. If you in any way use this
 * code for research that results in publications, please cite our original
 * article listed above.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. */

extern "C" {
	#include <stddef.h>
	#include <sys/time.h>
	#include <sys/resource.h>
}

size_t
getPeakRSS()
{
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage );
	return (size_t)rusage.ru_maxrss;
}
