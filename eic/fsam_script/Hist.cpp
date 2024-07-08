#include "Hist.hpp"

// constructors & destructor
Hist::Hist()
{
}

Hist::~Hist()
{
}

Hist::Hist(const Hist& hist)
{
	(void)hist;
}

// operators
Hist&
Hist::operator=(const Hist& hist)
{
	(void)hist;
	return *this;
}
