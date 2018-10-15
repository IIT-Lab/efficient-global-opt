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

#ifndef _PA_H
#define _PA_H

#include <iostream>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <array>
#include <utility>
#include <iterator>
#include <cstdio>
#include <chrono>

#include "util.h"

/* base types */

namespace pa {
typedef double basetype;
enum PAstatus { OPTIMAL, UNSOLVED, MAXITER };

/* main class */
template <size_t Dim>
class PA
{
	public:
		using vtype = std::array<basetype, Dim>;

		PA()
			: output(true), output_every(100), MaxIter(-1), user_data(NULL), xopt {}, epsilon(1e-2), proj_delta(1e-10), ub {}, obj(NULL), inG(NULL), inH(NULL), doshift(false), xshift {}
		{ setStatus(UNSOLVED); }

		struct Vertex
		{
			vtype v;
			basetype obj;

			Vertex() {};

			Vertex(const vtype &vert, basetype b = -std::numeric_limits<double>::infinity())
				: v(vert), obj(b)
			{}

			Vertex(vtype &&vert, basetype b = -std::numeric_limits<double>::infinity())
				: v(std::move(vert)), obj(b)
			{ }
		};

		using VertexSet = std::vector<Vertex>;

		using PAfun = basetype (*)(const vtype &, void *);
		using PAboolfun = bool (*)(const vtype &, void *);

		// vector length
		constexpr size_t dim() { return Dim; };

		// parameter setter
		void setPrecision(basetype epsilon)
			{ this->epsilon = epsilon; setStatus(UNSOLVED); }

		void setObjective(PAfun fp)
			{ obj = fp; setStatus(UNSOLVED); }

		void setInG(PAboolfun fp)
			{ inG = fp; setStatus(UNSOLVED); }

		void setInH(PAboolfun fp)
			{ inH = fp; setStatus(UNSOLVED); }

		void setUB(vtype v)
			{ub = v; setStatus(UNSOLVED); }

		void setUB(basetype e)
		{
			for (auto &b : ub)
				b = e;

			setStatus(UNSOLVED);
		}

		void setUB(size_t idx, basetype e)
			{ub[idx] = e; setStatus(UNSOLVED); }


		void setUserData(void *ud)
			{ user_data = ud; setStatus(UNSOLVED); }

		void setShift(vtype v = {})
		{ xshift = v; doshift = true; }

		void unsetShift()
			{ doshift = false; }

		// parameter
		bool output;
		unsigned long long output_every;
		unsigned long long MaxIter;

		void *user_data;

		// result
		vtype xopt;
		basetype optval;
		unsigned long long iter;
		unsigned long long lastUpdate;
		PAstatus status;
		const char *statusStr;
		double runtime; // in seconds

		// run PA algorithm
		void optimize(vtype startPoint)
			{ optimize(true, std::forward(startPoint)); }

		void optimize()
			{ optimize(false, {}); }

		void printResult();

	private:
		// parameter
		basetype epsilon;
		const double proj_delta;

		vtype ub;
		PAfun obj;
		PAboolfun inG, inH;

		bool doshift;
		vtype xshift;

		typedef std::chrono::high_resolution_clock clock;

		// functions
		vtype projection(const vtype& x) const;
		void optimize(bool startPointPresent, typename PA<Dim>::vtype startPoint);

		vtype shift(const vtype &x) const
		{
			if (!doshift)
				return x;

			vtype res;
			std::transform(
				x.begin(),
				x.end(),
				xshift.begin(),
				res.begin(),
				[this] (const basetype &a, const basetype &b)
				{
					return a - b;
				}
			);

			return res;
		}

		void setStatus(const PAstatus s)
		{
			status = s;

			switch (s)
			{
				case OPTIMAL:
					statusStr = "Optimal";
					break;

				case UNSOLVED:
					statusStr = "Undefined";
					break;

				case MAXITER:
					statusStr = "Max Iterations";
			}
		}
};

#include "bits/PA.cpp"
}
#endif
