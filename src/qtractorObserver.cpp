// qtractorObserver.cpp
//
/****************************************************************************
   Copyright (C) 2005-2022, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qtractorAbout.h"
#include "qtractorObserver.h"


//---------------------------------------------------------------------------
// qtractorSubjectQueue - Update/notify subject queue.

class qtractorSubjectQueue
{
public:

	struct QueueItem
	{
		qtractorSubject  *subject;
		qtractorObserver *sender;
		float             value;
	};

	qtractorSubjectQueue ( unsigned int iQueueSize = 1024 )
		: m_iQueueIndex(0), m_iQueueSize(0), m_pQueueItems(nullptr)
		{ resize(iQueueSize); }

	~qtractorSubjectQueue ()
		{ clear(); delete [] m_pQueueItems; }

	void clear()
		{ m_iQueueIndex = 0; }

	bool push ( qtractorSubject *pSubject, qtractorObserver *pSender, float fValue )
	{
		if (m_iQueueIndex >= m_iQueueSize)
			return false;
		pSubject->setQueued(true);
		QueueItem *pItem = &m_pQueueItems[m_iQueueIndex++];
		pItem->subject = pSubject;
		pItem->sender  = pSender;
		pItem->value   = fValue;
		return true;
	}

	bool pop (bool bUpdate)
	{
		if (m_iQueueIndex == 0)
			return false;
		QueueItem *pItem = &m_pQueueItems[--m_iQueueIndex];
		qtractorSubject *pSubject = pItem->subject;
		pSubject->notify(pItem->sender, pItem->value, bUpdate);
		pSubject->setQueued(false);
		return true;
	}

	bool flush (bool bUpdate)
		{ int i = 0; while (pop(bUpdate)) ++i; return (i > 0); }

	void reset ()
	{
		while (m_iQueueIndex > 0) {
			QueueItem *pItem = &m_pQueueItems[--m_iQueueIndex];
			(pItem->subject)->setQueued(false);
		}
		clear();
	}

	void resize ( unsigned int iNewSize )
	{
		QueueItem *pOldItems = m_pQueueItems;
		QueueItem *pNewItems = new QueueItem [iNewSize];
		if (pOldItems) {
			unsigned int iOldSize = m_iQueueIndex;
			if (iOldSize > iNewSize)
				iOldSize = iNewSize;
			if (iOldSize > 0)
				::memcpy(pNewItems, pOldItems, iOldSize * sizeof(QueueItem));
			m_iQueueSize  = iNewSize;
			m_pQueueItems = pNewItems;
			delete [] pOldItems;
		} else {
			m_iQueueSize  = iNewSize;
			m_pQueueItems = pNewItems;
		}
	}

	bool isEmpty() const
		{ return (m_iQueueIndex == 0); }

private:

	unsigned int m_iQueueIndex;
	unsigned int m_iQueueSize;
	QueueItem   *m_pQueueItems;
};


// The local subject queue singleton.
static qtractorSubjectQueue g_subjectQueue;


//---------------------------------------------------------------------------
// qtractorSubject - Scalar parameter value model.

// Constructor.
qtractorSubject::qtractorSubject ( float fValue, float fDefaultValue )
	: m_fValue(fValue), m_bQueued(false),
		m_fPrevValue(fValue), m_fLastValue(fValue),
		m_fMinValue(0.0f), m_fMaxValue(1.0f), m_fDefaultValue(fDefaultValue),
		m_bToggled(false), m_bInteger(false), m_pCurve(nullptr)
{
}

// Destructor.
qtractorSubject::~qtractorSubject (void)
{
	QListIterator<qtractorObserver *> iter(m_observers);
	while (iter.hasNext())
		iter.next()->setSubject(nullptr);

	m_observers.clear();
}


// Direct value accessors.
void qtractorSubject::setValue ( float fValue, qtractorObserver *pSender )
{
	if (fValue == m_fValue)
		return;

	if (!m_bQueued) {
		m_fPrevValue = m_fValue;
		g_subjectQueue.push(this, pSender, fValue);
	}

	m_fValue = safeValue(fValue);
}


// Observer/view updater.
void qtractorSubject::notify (
	qtractorObserver *pSender, float fValue, bool bUpdate )
{
	QListIterator<qtractorObserver *> iter(m_observers);
	while (iter.hasNext()) {
		qtractorObserver *pObserver = iter.next();
		if (pSender && pSender == pObserver)
			continue;
		m_fLastValue = fValue;
		pObserver->update(bUpdate);
	}
}


// Queue flush (singleton) -- notify all pending observers.
bool qtractorSubject::flushQueue ( bool bUpdate )
{
	return g_subjectQueue.flush(bUpdate);
}


// Queue reset (clear).
void qtractorSubject::resetQueue (void)
{
	g_subjectQueue.reset();
}

void qtractorSubject::clearQueue (void)
{
	g_subjectQueue.clear();
}


// end of qtractorObserver.cpp
