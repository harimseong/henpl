#include "HistManager.hpp"

// constructors & destructor
HistManager::HistManager()
{
}

HistManager::~HistManager()
{
}

HistManager::HistManager(const HistManager& histmanager)
{
	(void)histmanager;
}

// operators
HistManager&
HistManager::operator=(const HistManager& histmanager)
{
	(void)histmanager;
	return *this;
}
