/* Copyright (C) 2018-2019 Bho Matthiesen
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

#define CALL(F, X) ((F)(shift(X), user_data))


template <size_t Dim>
typename PA<Dim>::vtype
PA<Dim>::projection(const typename PA<Dim>::vtype& x) const
{
	double beta, beta_min(0.0), beta_max(1.0);
	vtype ret;

	while (beta_max - beta_min > proj_delta)
	{
		beta = (beta_max + beta_min) / 2.0;

		for (size_t i = 0; i < Dim; i++)
			ret[i] = beta * x[i];

		if (CALL(inG, ret))
			beta_min = beta;
		else
			beta_max = beta;
	}

	return ret;
}


template <size_t Dim>
void
PA<Dim>::optimize(bool startPointPresent, typename PA<Dim>::vtype startPoint)
{
	VertexSet vertexSet;
	vtype b;

	std::chrono::time_point<clock> tic(clock::now());

	// shift
	if (doshift)
	{
		if (std::all_of(xshift.begin(), xshift.end(), [] (auto e) { return e == 0.0; }))
			xshift = ub;

		if (startPointPresent)
		{
			for (size_t i = 0; i < Dim; i++)
				startPoint[i] = startPoint[i] + xshift[i];
		}

		for (size_t i = 0; i < Dim; i++)
			b[i] = ub[i] + xshift[i];
	}
	else
		b = ub;

	// init xopt / optval
	if (startPointPresent)
	{
		if (CALL(inG, startPoint) && CALL(inH, startPoint))
		{
			xopt = startPoint;
			optval = CALL(obj, xopt);
		}
		else
			std::cerr << "Infeasible startPoint. Starting from scratch." << std::endl;

	}
	else
	{
		xopt = b;
		optval = -std::numeric_limits<double>::infinity();
	}

	// init other
	setStatus(UNSOLVED);
	iter = 0;
	lastUpdate = 0;

	vertexSet.emplace_back(b, CALL(obj, b));

	while (true)
	{
		// find new candidate
		typename VertexSet::const_iterator zk = std::max_element(vertexSet.begin(), vertexSet.end(),
				[](const Vertex& a, const Vertex& b) { return a.obj < b.obj; }
			);

		// output
		if (output && iter != 0 && iter % output_every == 0)
			std::printf("%8llu %8lu %11g %11g (D = %g) | Peak RSS: %zu\n", iter, vertexSet.size(), optval, zk->obj, zk->obj - optval, getPeakRSS());

		// increase counter & check MaxIter
		if (++iter > MaxIter) // this does not break if MaxIter = (unsigned long long) -1 
		{
			setStatus(MAXITER);
			break;
		}

		// we're done if candidate point is feasible
		if (CALL(inG, zk->v))
		{
			xopt = zk->v;
			optval = zk->obj;
			lastUpdate = iter;
			setStatus(OPTIMAL);
			break;
		}

		// projection
		Vertex newFeasiblePoint(std::move(projection(zk->v)));

		// is point feasible (i.e. also in H)
		if (CALL(inH, newFeasiblePoint.v))
		{
			newFeasiblePoint.obj = CALL(obj, newFeasiblePoint.v);

			if (newFeasiblePoint.obj >= optval)
			{
				xopt = newFeasiblePoint.v;
				optval = newFeasiblePoint.obj;
				lastUpdate = iter;
			}
		}

		// compute T*
		typename VertexSet::iterator Tsit = partition(vertexSet.begin(), vertexSet.end(),
				[&](const Vertex& v)->bool // return true if v not in Tstar
				{
					for (size_t i = 0; i < Dim; i++)
					{
						if (v.v[i] <= newFeasiblePoint.v[i])
						{
							return true;
						}
					}

					return false;
				}
			);

		VertexSet Tstar;
		Tstar.reserve(std::distance(Tsit, vertexSet.end()));

		Tstar.insert(
				Tstar.end(),
				std::move_iterator<typename VertexSet::iterator>(Tsit),
				std::move_iterator<typename VertexSet::iterator>(vertexSet.end())
			);

		vertexSet.erase(Tsit, vertexSet.end()); // TODO this should be done later

		// delete dominated
		vertexSet.erase(
				std::remove_if(
					vertexSet.begin(),
					vertexSet.end(),
					[this](const Vertex& v) { return v.obj <= optval; }
				),
				vertexSet.end()
			);

		// create new Vertices
		VertexSet newVertices;
		newVertices.reserve(Tstar.size() * Dim);
		for (typename VertexSet::const_iterator e = Tstar.begin(); e != Tstar.end(); e++)
		{
			for (size_t i = 0; i < Dim; i++)
			{
				Vertex v(e->v);
				v.v[i] = newFeasiblePoint.v[i];

				if (CALL(inH, v.v))
				{
					v.obj = CALL(obj, v.v);

					if (v.obj > optval + epsilon)
						newVertices.push_back(std::move(v));
				}
			}
		}

		// remove improper new vertices
		newVertices.erase(
				std::remove_if(
					newVertices.begin(),
					newVertices.end(),
					[newVertices,this](const Vertex& v) // TODO is there a smarter way instead of passing newVertices as value???
					{
						for(auto const &e : newVertices)
						{
							bool le(true), neq(false);

							for (size_t i = 0; i < Dim; i++)
							{
								if (v.v[i] > e.v[i])
								{
									le = false;
									break;
								}
							}

							for (size_t i = 0; i < Dim; i++)
							{	
								if (v.v[i] != e.v[i])
								{
									neq = true;
									break;
								}
							}

							if (le && neq)
								return true;
						}

						return false;
					}
				),
				newVertices.end()
			);

		// add newVertices to vertexSet
		if (newVertices.size() > 0)
		{
			vertexSet.reserve(vertexSet.size() + newVertices.size());
			vertexSet.insert(
					vertexSet.end(),
					std::move_iterator<typename VertexSet::iterator>(newVertices.begin()),
					std::move_iterator<typename VertexSet::iterator>(newVertices.end())
				);
			newVertices.clear();
		}

		if (vertexSet.size() == 0)
		{
			setStatus(OPTIMAL);
			break;
		}
	}

	// undo shift
	if (doshift)
		xopt = shift(xopt);

	runtime = std::chrono::duration_cast<std::chrono::nanoseconds> (clock::now() - tic).count() / 1e9;

	// final status output
	if (output)
	{
		std::printf("%8llu %8lu %11g %11g (D = %g) | Peak RSS: %zu\n", iter, vertexSet.size(), optval, optval, 0.0, getPeakRSS());
		std::cout << std::endl;
		printResult();
	}
}

template <size_t Dim>
void
PA<Dim>::printResult()
{
	std::cout << "Status: " << statusStr << std::endl;
	std::cout << "Optval: " << optval << std::endl;

	std::cout << "X*: [";
	std::for_each(xopt.begin(), xopt.end(), [] (const basetype &a) { std::cout << " " << a; });
	std::cout << " ]" << std::endl;

	std::cout << "Precision: epsilon = " << epsilon << std::endl;

	std::cout << "Iter: " << iter << std::endl;
	std::cout << "Solution found in iter: " << lastUpdate << std::endl;

	std::cout << "Runtime: " << runtime << " sec" << std::endl;
	std::cout << "Peak RSS: " << getPeakRSS() << std::endl;
}
